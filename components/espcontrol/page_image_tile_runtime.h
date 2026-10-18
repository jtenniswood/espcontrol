#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

#include "page_cover_art_policy.h"
#include "page_entity_state_runtime.h"
#include "page_image_environment.h"
#include "page_image_refresh_state.h"
#include "page_image_runtime.h"
#include "page_image_tile_policy.h"
#include "page_modal_image_runtime.h"

namespace espcontrol::pages {
struct PageImageTileSource {
  // Borrowed prepared metadata: clear the lease before its prepared bank retires.
  std::string_view entity, trigger;
  camera::RefreshMode refresh{camera::RefreshMode::OFF};
  uint32_t interval_ms{10000};
  bool fit{false};
};
class PageImageTileEnvironment : public PageImageEnvironment {
 public:
  virtual bool visible(PageBindingToken) const = 0;
  virtual bool expanded(PageBindingToken) const {
    return false;
  }
  virtual uint16_t maximum_source_side() const = 0;
};
enum class PageImageTileStatus : uint8_t { LOADING, UNAVAILABLE, READY };
struct PageImageRefreshTransfer {
  PageModalToken modal;
  uint64_t transfer{0};
};
struct PageImageTileSnapshot {
  bool bound{false}, pending{false}, can_open{false};
  PageImageTileStatus status{PageImageTileStatus::UNAVAILABLE};
  uint64_t asset{0};
};
// A fixed policy record per leased downloader, sharing the pool with cover art.
// Bind/unbind/observer work never allocates or starts I/O. Pump image maintenance,
// this maintenance, then the renderer, outside native/coordinator/LVGL dispatch.
template <size_t Slots>
class PageImageTileRuntime final : public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  PageImageTileRuntime(PageEntityStateRuntime& state, PageImageRuntime<Slots>& images,
                       PageImageTileEnvironment& environment, Now now, uint64_t first_transfer = 1)
      : state_(state),
        images_(images),
        environment_(environment),
        now_(now),
        next_transfer_(first_transfer) {}
  ~PageImageTileRuntime() override {
    retire();
  }
  bool available() const {
    return now_ && images_.available();
  }
  size_t bind(PageBindingToken token, PageImageTileSource source) {
    if (!available() || page_image_entity_kind(source.entity) == PageImageEntityKind::UNSUPPORTED)
      return Slots;
    const auto slot = images_.acquire(token);
    if (slot == Slots) return Slots;
    auto& r = records_[slot];
    if (same(r.token, token)) return slot;
    reset(r);
    r.token = token;
    r.source = source;
    r.kind = page_image_entity_kind(source.entity);
    r.started = now_();
    r.endpoint = environment_.endpoint_generation();
    configure_policy(r);
    r.options.cover = !source.fit;
    r.options.retain_completed = true;
    r.options.priority = PageImagePriority::TILE;
    r.dirty = r.forced = true;
    return slot;
  }
  // Admission hint for the demanded source: a hidden card seeds independent
  // policy, while failure to transfer an existing tile must fail/retry safely.
  bool has_tile(ConfigurationHandle card) const {
    for (size_t slot = 0; slot < Slots; ++slot) {
      const auto token = records_[slot].token;
      if (token.binding && token.configuration == card.generation && token.card == card.index &&
          images_.owns(slot, token))
        return true;
    }
    return false;
  }
  // The demanded expanded observer must already own `state` before this call:
  // it may receive a new report during synchronous cancellation. The stable
  // modal origin outlives any transfer; no configuration bytes are borrowed.
  PageImageRefreshTransfer begin_expanded(PageModalToken modal, PageModalImageOrigin& origin,
                                          PageImageRefreshState& state) {
    if (!available() || !next_transfer_ || !PageModalImageIdentity::eligible(modal) || !origin.valid(modal))
      return {};
    abandon_expired();
    if (expanded_.id.transfer) return {};
    size_t slot = Slots;
    for (size_t i = 0; i < Slots; ++i)
      if (records_[i].token.binding && records_[i].token.configuration == modal.configuration &&
          records_[i].token.card == modal.card && images_.owns(i, records_[i].token)) {
        slot = i;
        break;
      }
    if (slot == Slots) return {};  // A direct hidden-card request has no tile policy.
    auto& r = records_[slot];
    synchronize(r);
    seed(r);
    const auto image = images_.snapshot(slot, r.token);
    const auto id = PageImageRefreshTransfer{modal, next_transfer_++};
    expanded_ = {id,
                 r.token,
                 &origin,
                 r.serial,
                 now_() + 1000,
                 (image.pending && !r.request_scheduled) || !image.ready || r.stale_unavailable || r.retry};
    state = r.policy;
    state.source_dirty = state.expanded_completed = state.expanded_success = false;
    state.schedule.in_flight = false;
    state.schedule.enter_expanded(now_(), state.activity.on(state.connection));
    r.request_scheduled = false;
    // Export and delegate before cancel, so a reentrant fresh report reaches
    // the demanded owner rather than being lost in the old tile record.
    images_.cancel_request(slot, r.token, true);
    if (!transfer_matches(id) || !origin.valid(modal)) {
      abandon_expired();
      return {};
    }
    return id;
  }
  bool end_expanded(PageImageRefreshTransfer id, const PageImageRefreshState& state) {
    if (!transfer_matches(id)) return false;
    const auto slot = expanded_slot();
    if (slot == Slots) return false;
    const auto transfer = expanded_;
    expanded_ = {};  // Modal retirement has already revoked the opening token.
    auto& r = records_[slot];
    const auto old_deadline = r.deadline;
    const bool admitted_wait = r.admission_failed && r.waiting;
    r.policy = state;
    r.policy.schedule.leave_expanded();
    const auto image = images_.snapshot(slot, r.token);
    r.forced |= transfer.retry_tile || !image.ready || r.stale_unavailable || r.policy.tile_invalidated;
    r.dirty |= r.policy.source_dirty || r.forced || r.policy.picture.tile_dirty();
    r.retry = r.policy.camera_errors > 0 || transfer.retry_tile || r.policy.tile_invalidated;
    r.failure_seen = r.cancelled_unavailable = false;
    if (r.retry) {
      r.deadline = r.policy.camera_errors ? r.policy.camera_retry_after
                   : r.policy.expanded_completed && r.policy.expanded_success
                       ? r.policy.schedule.last_completed + 1000
                   : r.policy.source_dirty ? r.policy.source_changed_at + 1000
                                           : transfer.retry_after;
    } else if (r.policy.source_dirty) {
      r.deadline = r.policy.source_changed_at + 1000;
      r.waiting = true;
    } else if (!r.admission_failed)
      r.waiting = false;
    if (admitted_wait && static_cast<int32_t>(old_deadline - r.deadline) > 0) r.deadline = old_deadline;
    r.stale_unavailable |= r.policy.unavailable || r.policy.camera_errors > 0 || r.policy.tile_invalidated;
    ++revision_;
    return true;
  }
  void unbind(PageBindingToken token) {
    const auto slot = find(token);
    if (slot == Slots) return;
    const bool owned = images_.bound_to(slot, token);
    reset(records_[slot]);
    if (owned) images_.unbind(slot);
  }
  void retire() {
    for (size_t slot = 0; slot < Slots; ++slot)
      if (records_[slot].token.binding) unbind(records_[slot].token);
  }
  size_t find(PageBindingToken token) const {
    for (size_t slot = 0; slot < Slots; ++slot)
      if (same(records_[slot].token, token)) return slot;
    return Slots;
  }
  void size(PageBindingToken token, uint16_t width, uint16_t height, uint32_t background = 0) {
    const auto slot = find(token);
    if (slot == Slots) return;
    auto& r = records_[slot];
    if (r.options.width == width && r.options.height == height &&
        (!r.source.fit || r.options.background == background))
      return;
    r.geometry_stale = r.options.width && r.options.height;
    r.options.width = width;
    r.options.height = height;
    r.options.background = background;
    r.dirty = r.forced = true;
    r.waiting = false;
    ++revision_;
  }
  PageImageTileSnapshot snapshot(PageBindingToken token) const {
    const auto slot = find(token);
    if (slot == Slots || !images_.owns(slot, token)) return {};
    const auto& r = records_[slot];
    const auto image = images_.snapshot(slot, token);
    PageImageTileSnapshot result{true, image.pending, image.ready || !r.selected.empty()};
    if (r.exhausted || r.policy.unavailable || r.stale_unavailable ||
        (image.failed && (!r.request_scheduled || !image.ready)))
      return result;
    if (r.endpoint == environment_.endpoint_generation() && !r.geometry_stale && image.ready) {
      result.status = PageImageTileStatus::READY;
      result.asset = image.asset;
      return result;
    }
    if (!r.admission_failed && (image.pending || !due(now_(), r.started + 45000)))
      result.status = PageImageTileStatus::LOADING;
    return result;
  }
  uint64_t revision() const {
    return revision_ + images_.revision();
  }
  void observed(uint32_t generation, std::string_view entity, std::string_view attribute,
                std::string_view value, bool copied) override {
    observed_update(generation, entity, attribute, value, copied, {});
  }
  void observed_update(uint32_t generation, std::string_view entity, std::string_view attribute,
                       std::string_view value, bool copied, std::string_view previous) override {
    if (copied) return;
    abandon_expired();
    for (auto& r : records_) {
      if (!r.token.binding || r.token.configuration != generation || delegated(r)) continue;
      synchronize(r);
      if (entity == r.source.entity) {
        if (attribute.empty()) {
          observe_primary(r, value);
          continue;
        }
        if (attribute == "entity_picture") {
          r.policy.picture_update(now_());
          r.dirty = true;
          if (!r.admission_failed) r.waiting = false;
        } else if (attribute == "access_token" && r.policy.token_update(value, previous, now_())) {
          r.dirty = true;
          if (!r.admission_failed) r.waiting = false;
        }
      } else if (attribute.empty() && !r.source.trigger.empty() && entity == r.source.trigger) {
        r.policy.trigger(value, r.source.trigger.substr(0, 14) == "binary_sensor.", now_(),
                         environment_.visible(r.token));
      }
    }
  }
  void loop() {
    if (!now_) return;
    abandon_expired();
    for (size_t slot = 0; slot < Slots; ++slot) {
      maintain(slot);
      auto& r = records_[slot];
      if (!r.token.binding) continue;
      const auto status = snapshot(r.token).status;
      if (status != r.status) {
        r.status = status;
        ++revision_;
      }
    }
  }

 private:
  struct Record {
    PageBindingToken token;
    PageImageTileSource source;
    PageImageEntityKind kind{PageImageEntityKind::UNSUPPORTED};
    PageImageOptions options;
    PageImageRefreshState policy;
    std::string selected;
    uint64_t serial{0}, handled_asset{0};
    uint32_t started{0}, deadline{0}, endpoint{0};
    PageImageTileStatus status{PageImageTileStatus::LOADING};
    bool dirty{false}, forced{false}, stale_unavailable{false}, cancelled_unavailable{false};
    bool request_scheduled{false}, failure_seen{false}, waiting{false}, retry{false}, geometry_stale{false},
        admission_failed{false}, exhausted{false};
  };
  struct Expanded {
    PageImageRefreshTransfer id;
    PageBindingToken tile;
    PageModalImageOrigin* origin{nullptr};
    uint64_t serial{0};
    uint32_t retry_after{0};
    bool retry_tile{false};
  };
  bool delegated(const Record& r) const {
    return expanded_.id.transfer && expanded_.serial == r.serial && same(expanded_.tile, r.token);
  }
  size_t expanded_slot() const {
    const auto slot = find(expanded_.tile);
    return slot < Slots && delegated(records_[slot]) && images_.owns(slot, expanded_.tile) ? slot : Slots;
  }
  bool transfer_matches(PageImageRefreshTransfer id) const {
    return id.transfer && id.transfer == expanded_.id.transfer &&
           PageModalImageIdentity::same(id.modal, expanded_.id.modal) && expanded_slot() != Slots;
  }
  void configure_policy(Record& r) {
    r.policy.connection = state_.connection_generation();
    r.policy.schedule.mode =
        r.kind == PageImageEntityKind::CAMERA ? r.source.refresh : camera::RefreshMode::OFF;
    if (r.policy.schedule.mode == camera::RefreshMode::ACTIVITY &&
        !page_image_trigger_valid(r.source.trigger))
      r.policy.schedule.mode = camera::RefreshMode::OFF;
    r.policy.schedule.interval_ms =
        r.source.interval_ms == 5000 || r.source.interval_ms == 30000 ? r.source.interval_ms : 10000;
  }
  void abandon_expired() {
    if (!expanded_.id.transfer) return;
    const auto slot = expanded_slot();
    if (slot != Slots && expanded_.origin && expanded_.origin->valid(expanded_.id.modal)) return;
    expanded_ = {};
    if (slot == Slots) return;
    auto& r = records_[slot];
    r.policy = {};
    configure_policy(r);
    r.policy.tile_invalidated = r.stale_unavailable = true;
    r.dirty = r.forced = true;
    r.policy.camera_errors = 0;
    r.retry = r.failure_seen = r.cancelled_unavailable = r.request_scheduled = false;
    if (!r.admission_failed) r.waiting = false;
    ++revision_;
  }
  static bool same(PageBindingToken a, PageBindingToken b) {
    return a.binding && a.configuration == b.configuration && a.binding == b.binding && a.page == b.page &&
           a.card == b.card && a.shell == b.shell && a.back == b.back;
  }
  static bool due(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
  }
  void reset(Record& r) {
    if (delegated(r)) expanded_ = {};
    auto selected = std::move(r.selected);
    selected.clear();
    r = {};
    r.selected = std::move(selected);
    r.serial = ++serial_;
    ++revision_;
  }
  void synchronize(Record& r) {
    if (!r.policy.synchronize(state_.connection_generation())) return;
    r.policy.camera_errors = 0;
    r.retry = r.waiting = r.failure_seen = false;
    r.policy.last_tile_completed = 0;
    r.forced = r.dirty = true;
    r.policy.schedule.in_flight = false;
  }
  void observe_primary(Record& r, std::string_view value) {
    const auto update = r.policy.primary(r.kind, value, now_());
    if (update == PageImagePrimaryUpdate::IGNORED) return;
    const bool unavailable = update == PageImagePrimaryUpdate::UNAVAILABLE;
    const bool recovered = update == PageImagePrimaryUpdate::RECOVERED;
    if (unavailable) {
      r.stale_unavailable = true;
      r.retry = r.waiting = false;
    }
    if (recovered) {
      r.policy.camera_errors = 0;
      r.policy.last_tile_completed = 0;
      r.retry = r.failure_seen = r.cancelled_unavailable = false;
      if (!r.admission_failed) r.waiting = false;
      r.forced = true;
    }
    r.policy.picture_event = false;
    r.dirty = true;
    if (!r.admission_failed) r.waiting = false;
    ++revision_;
  }
  void seed(Record& r) {
    const ConfigurationHandle card{r.token.configuration, r.token.card};
    if (!r.policy.primary_seeded) {
      const auto value = state_.read_attribute(card, r.source.entity, {});
      if (value.status == PageEntityStatus::AVAILABLE) observe_primary(r, value.value);
    }
    if (!r.policy.activity_seeded && !r.source.trigger.empty()) {
      const auto value = state_.read_attribute(card, r.source.trigger, {});
      if (value.status == PageEntityStatus::AVAILABLE) {
        r.policy.activity.observe(value.value, r.source.trigger.substr(0, 14) == "binary_sensor.",
                                  r.policy.connection);
        r.policy.activity_seeded = true;
      }
    }
  }
  bool clear_lease(size_t slot, PageBindingToken token, uint64_t serial) {
    images_.unbind(slot);
    auto& r = records_[slot];
    if (r.serial != serial || !same(r.token, token) || !images_.vacant(slot)) return false;
    return images_.bind(slot, token);
  }
  std::string source_url(const Record& r) {
    const ConfigurationHandle card{r.token.configuration, r.token.card};
    const auto picture = state_.read_attribute(card, r.source.entity, "entity_picture");
    const auto access = state_.read_attribute(card, r.source.entity, "access_token");
    const auto raw =
        picture.status == PageEntityStatus::AVAILABLE ? picture.value.substr(0, 4096) : std::string_view{};
    const auto token =
        access.status == PageEntityStatus::AVAILABLE ? access.value.substr(0, 512) : std::string_view{};
    const bool usable_token = !token.empty() && token != "unknown" && token != "unavailable";
    std::string url;
    if (r.policy.picture_event) url = PageArtworkUrl::join(environment_.base_url(), raw).copy();
    if (url.empty() && !environment_.base_url().empty() && usable_token) {
      url = environment_.base_url();
      url += r.kind == PageImageEntityKind::CAMERA ? "/api/camera_proxy/" : "/api/image_proxy/";
      url.append(r.source.entity.data(), r.source.entity.size());
    } else if (url.empty())
      url = PageArtworkUrl::join(environment_.base_url(), raw).copy();
    if (page_artwork_requires_token(url)) {
      if (!usable_token) return {};
      page_artwork_append_token(url, token);
    }
    return url;
  }
  static void dimension(std::string& url, std::string_view name, unsigned value) {
    if (page_artwork_query_has_param(url, name)) return;
    const auto fragment = url.find('#');
    const auto position = fragment == std::string::npos ? url.size() : fragment;
    char number[12];
    std::snprintf(number, sizeof(number), "%u", value);
    std::string suffix = url.find('?') < position ? "&" : "?";
    suffix.append(name.data(), name.size());
    suffix += '=';
    suffix += number;
    url.insert(position, suffix);
  }
  void maintain(size_t slot) {
    auto& r = records_[slot];
    const auto token = r.token;
    const auto serial = r.serial;
    if (!token.binding) {
      if (r.selected.capacity()) std::string().swap(r.selected);
      return;
    }
    if (!images_.owns(slot, token)) {
      const bool owned = images_.bound_to(slot, token);
      reset(r);
      if (owned) images_.unbind(slot);
      return;
    }
    if (delegated(r)) return;
    synchronize(r);
    seed(r);
    if (!images_.current_binding(slot, token)) return;
    const auto now = now_();
    if (r.endpoint != environment_.endpoint_generation() || r.geometry_stale) {
      if (!clear_lease(slot, token, serial)) return;
      r.endpoint = environment_.endpoint_generation();
      r.geometry_stale = false;
      r.handled_asset = 0;
      r.policy.last_tile_completed = 0;
      r.failure_seen = false;
      r.dirty = r.forced = true;
    }
    if (r.policy.unavailable) {
      if (!r.cancelled_unavailable) {
        r.cancelled_unavailable = true;
        r.policy.schedule.in_flight = false;
        r.request_scheduled = false;
        images_.cancel_request(slot, token, true);
      }
      return;
    }
    const auto image = images_.snapshot(slot, token);
    if (image.ready && !image.pending && !image.failed && image.asset != r.handled_asset) {
      r.handled_asset = image.asset;
      r.policy.picture.tile_applied = r.policy.picture.tile_requested;
      r.policy.tile_succeeded(now);
      r.stale_unavailable = r.admission_failed = r.failure_seen = r.retry = false;
      if (r.request_scheduled && r.policy.schedule.open) r.policy.schedule.finished(now, true);
      r.request_scheduled = false;
      ++revision_;
    }
    if (image.failed && !r.failure_seen) {
      r.failure_seen = true;
      if (r.request_scheduled && r.policy.schedule.open)
        r.policy.schedule.finished(now, false);
      else {
        r.policy.camera_failed(now);
        r.retry = true;
        r.deadline = r.policy.camera_retry_after;
        r.dirty = r.forced = r.stale_unavailable = true;
      }
      ++revision_;
    }
    const bool visible = environment_.visible(token), expanded = environment_.expanded(token);
    if (!visible) {
      r.policy.schedule.close();
      if (image.pending && r.request_scheduled) {
        r.request_scheduled = false;
        images_.cancel_request(slot, token, true);
      }
      return;
    }
    if (r.policy.schedule.mode != camera::RefreshMode::OFF && !r.policy.schedule.open) {
      r.policy.schedule.begin(now, false);
      if (image.ready && r.policy.last_tile_completed)
        r.policy.schedule.finished(r.policy.last_tile_completed, true);
    }
    if (expanded) return;
    if (r.retry && !due(now, r.deadline)) return;
    if (r.retry) {
      r.retry = false;
      r.dirty = r.forced = true;
    }
    if (r.waiting && !due(now, r.deadline)) return;
    r.waiting = false;
    const bool scheduled = !r.dirty && !r.forced && r.policy.schedule.due(now, true);
    const bool revision_dirty = r.kind == PageImageEntityKind::IMAGE && r.policy.picture.tile_dirty();
    if (!r.dirty && !r.forced && !scheduled && !revision_dirty) return;
    if (!r.options.width || !r.options.height) return;
    auto url = source_url(r);
    if (url.empty()) {
      r.waiting = true;
      r.deadline = now + 2000;
      return;
    }
    const bool changed = url != r.selected;
    if (r.kind == PageImageEntityKind::IMAGE && image.ready && !revision_dirty && !r.forced) {
      r.selected = std::move(url);
      r.dirty = false;
      return;
    }
    if (image.pending && (r.kind == PageImageEntityKind::IMAGE || !changed)) return;
    if (!scheduled && !r.forced && !revision_dirty && !changed && image.ready &&
        r.policy.camera_errors == 0 && r.policy.last_tile_completed &&
        static_cast<uint32_t>(now - r.policy.last_tile_completed) < 30000) {
      r.dirty = false;
      return;
    }
    if (!environment_.request_allowed(r.options.width, r.options.height)) {
      r.admission_failed = true;
      r.waiting = true;
      r.deadline = now + 2000;
      return;
    }
    if (!images_.requestable()) {
      r.exhausted = true;
      return;
    }
    r.selected = url;
    if (page_artwork_requires_token(url) || url.find("/api/camera_proxy/") != std::string::npos ||
        url.find("/api/image_proxy/") != std::string::npos) {
      const auto side = std::max<uint16_t>(1, environment_.maximum_source_side());
      const auto longest = std::max(r.options.width, r.options.height);
      const auto width = longest > side
                             ? std::max<unsigned>(1, static_cast<uint32_t>(r.options.width) * side / longest)
                             : r.options.width;
      const auto height =
          longest > side ? std::max<unsigned>(1, static_cast<uint32_t>(r.options.height) * side / longest)
                         : r.options.height;
      dimension(url, "width", width);
      dimension(url, "height", height);
      r.options.max_source_dimension = std::max(width, height);
    } else
      r.options.max_source_dimension = std::max(r.options.width, r.options.height);
    r.policy.picture.tile_requested = r.policy.picture.latest;
    r.failure_seen = r.dirty = r.forced = r.admission_failed = false;
    r.request_scheduled = scheduled;
    if (scheduled) r.policy.schedule.started();
    images_.request(slot, token, url, r.options);
    // External start/cancel can replace the scene. Never use borrowed config,
    // packed graph bytes or this record's old role after the request call.
  }
  PageEntityStateRuntime& state_;
  PageImageRuntime<Slots>& images_;
  PageImageTileEnvironment& environment_;
  Now now_;
  std::array<Record, Slots> records_{};
  uint64_t revision_{0}, serial_{0}, next_transfer_{1};
  Expanded expanded_;
};
}  // namespace espcontrol::pages
