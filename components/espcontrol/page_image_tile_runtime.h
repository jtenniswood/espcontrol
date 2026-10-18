#pragma once

#include <algorithm>
#include <array>
#include <cstdio>
#include <string>

#include "page_cover_art_policy.h"
#include "page_entity_state_runtime.h"
#include "page_image_environment.h"
#include "page_image_runtime.h"
#include "page_image_tile_policy.h"

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
                       PageImageTileEnvironment& environment, Now now)
      : state_(state), images_(images), environment_(environment), now_(now) {}
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
    r.connection = state_.connection_generation();
    r.options.cover = !source.fit;
    r.options.retain_completed = true;
    r.options.priority = PageImagePriority::TILE;
    r.schedule.mode = r.kind == PageImageEntityKind::CAMERA ? source.refresh : camera::RefreshMode::OFF;
    if (r.schedule.mode == camera::RefreshMode::ACTIVITY && !page_image_trigger_valid(source.trigger))
      r.schedule.mode = camera::RefreshMode::OFF;
    r.schedule.interval_ms =
        source.interval_ms == 5000 || source.interval_ms == 30000 ? source.interval_ms : 10000;
    r.dirty = r.forced = true;
    return slot;
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
    if (r.exhausted || r.unavailable || r.stale_unavailable ||
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
    for (auto& r : records_) {
      if (!r.token.binding || r.token.configuration != generation) continue;
      synchronize(r);
      if (entity == r.source.entity) {
        if (attribute.empty()) {
          observe_primary(r, value);
          continue;
        }
        if (attribute == "entity_picture") {
          r.picture_event = true;
          r.dirty = true;
          if (!r.admission_failed) r.waiting = false;
        } else if (attribute == "access_token" && value.substr(0, 512) != previous.substr(0, 512)) {
          r.picture_event = false;
          r.dirty = true;
          if (!r.admission_failed) r.waiting = false;
        }
      } else if (attribute.empty() && !r.source.trigger.empty() && entity == r.source.trigger) {
        r.activity_seeded = true;
        const bool edge =
            r.activity.observe(value, r.source.trigger.substr(0, 14) == "binary_sensor.", r.connection);
        if (edge && environment_.visible(r.token)) {
          if (!r.schedule.open) r.schedule.begin(now_(), false);
          r.schedule.activate(now_());
        }
      }
    }
  }
  void loop() {
    if (!now_) return;
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
    camera::RefreshSchedule schedule;
    PageImageActivity activity;
    PageImageRevision picture;
    std::string selected;
    uint64_t serial{0}, handled_asset{0};
    uint32_t started{0}, deadline{0}, last_completed{0}, endpoint{0}, connection{0};
    uint8_t errors{0};
    PageImageTileStatus status{PageImageTileStatus::LOADING};
    bool dirty{false}, forced{false}, picture_event{false}, primary_seeded{false}, activity_seeded{false},
        unavailable{false}, stale_unavailable{false}, cancelled_unavailable{false};
    bool request_scheduled{false}, failure_seen{false}, waiting{false}, retry{false}, geometry_stale{false},
        admission_failed{false}, exhausted{false};
  };
  static bool same(PageBindingToken a, PageBindingToken b) {
    return a.binding && a.configuration == b.configuration && a.binding == b.binding && a.page == b.page &&
           a.card == b.card && a.shell == b.shell && a.back == b.back;
  }
  static bool due(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
  }
  void reset(Record& r) {
    auto selected = std::move(r.selected);
    selected.clear();
    r = {};
    r.selected = std::move(selected);
    r.serial = ++serial_;
    ++revision_;
  }
  void synchronize(Record& r) {
    const auto connection = state_.connection_generation();
    if (r.connection == connection) return;
    r.connection = connection;
    r.primary_seeded = r.activity_seeded = false;
    r.activity = {};
    r.picture.invalidate();
    r.errors = 0;
    r.retry = r.waiting = r.failure_seen = false;
    r.last_completed = 0;
    r.forced = r.dirty = true;
    r.schedule.in_flight = false;
  }
  void observe_primary(Record& r, std::string_view value) {
    r.primary_seeded = true;
    const bool unavailable = page_image_unavailable(value);
    if (r.kind == PageImageEntityKind::IMAGE) {
      if (unavailable)
        r.picture.invalidate();
      else if (!r.picture.observe(value))
        return;
    }
    const bool recovered = r.unavailable && !unavailable;
    r.unavailable = unavailable;
    if (unavailable) {
      r.stale_unavailable = true;
      r.retry = r.waiting = false;
    }
    if (recovered) {
      r.errors = 0;
      r.last_completed = 0;
      r.retry = r.failure_seen = r.cancelled_unavailable = false;
      if (!r.admission_failed) r.waiting = false;
      r.forced = true;
    }
    r.picture_event = false;
    r.dirty = true;
    if (!r.admission_failed) r.waiting = false;
    ++revision_;
  }
  void seed(Record& r) {
    const ConfigurationHandle card{r.token.configuration, r.token.card};
    if (!r.primary_seeded) {
      const auto value = state_.read_attribute(card, r.source.entity, {});
      if (value.status == PageEntityStatus::AVAILABLE) observe_primary(r, value.value);
    }
    if (!r.activity_seeded && !r.source.trigger.empty()) {
      const auto value = state_.read_attribute(card, r.source.trigger, {});
      if (value.status == PageEntityStatus::AVAILABLE) {
        r.activity.observe(value.value, r.source.trigger.substr(0, 14) == "binary_sensor.", r.connection);
        r.activity_seeded = true;
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
    if (r.picture_event) url = PageArtworkUrl::join(environment_.base_url(), raw).copy();
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
    synchronize(r);
    seed(r);
    if (!images_.current_binding(slot, token)) return;
    const auto now = now_();
    if (r.endpoint != environment_.endpoint_generation() || r.geometry_stale) {
      if (!clear_lease(slot, token, serial)) return;
      r.endpoint = environment_.endpoint_generation();
      r.geometry_stale = false;
      r.handled_asset = 0;
      r.last_completed = 0;
      r.failure_seen = false;
      r.dirty = r.forced = true;
    }
    if (r.unavailable) {
      if (!r.cancelled_unavailable) {
        r.cancelled_unavailable = true;
        r.schedule.in_flight = false;
        r.request_scheduled = false;
        images_.cancel_request(slot, token, true);
      }
      return;
    }
    const auto image = images_.snapshot(slot, token);
    if (image.ready && !image.pending && !image.failed && image.asset != r.handled_asset) {
      r.handled_asset = image.asset;
      r.picture.tile_applied = r.picture.tile_requested;
      r.last_completed = now;
      r.errors = 0;
      r.stale_unavailable = r.admission_failed = r.failure_seen = r.retry = false;
      if (r.request_scheduled && r.schedule.open) r.schedule.finished(now, true);
      r.request_scheduled = false;
      ++revision_;
    }
    if (image.failed && !r.failure_seen) {
      r.failure_seen = true;
      if (r.request_scheduled && r.schedule.open)
        r.schedule.finished(now, false);
      else {
        r.errors = std::min<unsigned>(5, r.errors + 1);
        r.retry = true;
        r.deadline = now + std::min<uint32_t>(30000, 2000u << (r.errors - 1));
        r.dirty = r.forced = r.stale_unavailable = true;
      }
      ++revision_;
    }
    const bool visible = environment_.visible(token), expanded = environment_.expanded(token);
    if (!visible) {
      r.schedule.close();
      if (image.pending && r.request_scheduled) {
        r.request_scheduled = false;
        images_.cancel_request(slot, token, true);
      }
      return;
    }
    if (r.schedule.mode != camera::RefreshMode::OFF && !r.schedule.open) {
      r.schedule.begin(now, false);
      if (image.ready && r.last_completed) r.schedule.finished(r.last_completed, true);
    }
    if (expanded) return;
    if (r.retry && !due(now, r.deadline)) return;
    if (r.retry) {
      r.retry = false;
      r.dirty = r.forced = true;
    }
    if (r.waiting && !due(now, r.deadline)) return;
    r.waiting = false;
    const bool scheduled = !r.dirty && !r.forced && r.schedule.due(now, true);
    const bool revision_dirty = r.kind == PageImageEntityKind::IMAGE && r.picture.tile_dirty();
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
    if (!scheduled && !r.forced && !revision_dirty && !changed && image.ready && r.errors == 0 &&
        r.last_completed && static_cast<uint32_t>(now - r.last_completed) < 30000) {
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
    r.picture.tile_requested = r.picture.latest;
    r.failure_seen = r.dirty = r.forced = r.admission_failed = false;
    r.request_scheduled = scheduled;
    if (scheduled) r.schedule.started();
    images_.request(slot, token, url, r.options);
    // External start/cancel can replace the scene. Never use borrowed config,
    // packed graph bytes or this record's old role after the request call.
  }
  PageEntityStateRuntime& state_;
  PageImageRuntime<Slots>& images_;
  PageImageTileEnvironment& environment_;
  Now now_;
  std::array<Record, Slots> records_{};
  uint64_t revision_{0}, serial_{0};
};
}  // namespace espcontrol::pages
