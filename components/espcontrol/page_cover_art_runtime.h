#pragma once

#include <array>
#include <string>

#include "artwork_controller.h"
#include "page_cover_art_policy.h"
#include "page_image_runtime.h"
#include "page_media_state_runtime.h"

namespace espcontrol::pages {
class PageCoverArtEnvironment {
 public:
  virtual ~PageCoverArtEnvironment() = default;
  // Borrowed settings remain stable for this main-loop call. Change the
  // generation whenever the base URL, HTTP credentials or endpoint changes.
  virtual std::string_view base_url() const = 0;
  virtual uint32_t endpoint_generation() const = 0;
  // Includes modal visibility and the device's existing image memory policy.
  // A refusal defers the request and keeps completed artwork visible.
  virtual bool request_allowed(uint16_t width, uint16_t height) const = 0;
};

// Fixed visible-image leases. The graph owns all HA attribute bytes; this owner
// keeps just a bounded signature and one selected URL per physical image slot.
// Register after PageMediaStateRuntime in the graph's observer set, and remove
// before destruction. No observer/bind/unbind starts work or allocates storage.
// Main loop: image maintenance, this maintenance, then renderer update. The
// renderer must validate snapshot.asset before drawing any borrowed descriptor.
template <size_t Slots>
class PageCoverArtRuntime final : public PageEntityObserver {
 public:
  PageCoverArtRuntime(PageEntityStateRuntime& state, PageMediaStateRuntime& media,
                      PageImageRuntime<Slots>& images, PageCoverArtEnvironment& environment)
      : state_(state), media_(media), images_(images), environment_(environment) {}
  ~PageCoverArtRuntime() override {
    retire();
  }
  PageCoverArtRuntime(const PageCoverArtRuntime&) = delete;
  PageCoverArtRuntime& operator=(const PageCoverArtRuntime&) = delete;

  bool available() const {
    return images_.available();
  }

  size_t bind(PageBindingToken token, PageImageOptions options = {}) {
    const auto slot = images_.acquire(token);
    if (slot == Slots) return Slots;
    auto& record = records_[slot];
    if (!same(record.token, token)) {
      reset(record);
      record.token = token;
      record.started = media_.now_ms();
      record.endpoint = environment_.endpoint_generation();
      record.connection = media_.connection_generation();
      record.dirty = true;
    }
    size(token, options.width, options.height, options.background);
    return slot;
  }
  void size(PageBindingToken token, uint16_t width, uint16_t height, uint32_t background = 0) {
    const auto slot = find(token);
    if (slot == Slots) return;
    auto& record = records_[slot];
    if (record.options.width == width && record.options.height == height &&
        record.options.background == background)
      return;
    record.options = {width, height, background, true, true};
    record.dirty = record.forced = true;
    record.waiting = false;
  }
  void unbind(PageBindingToken token) {
    const auto slot = find(token);
    if (slot == Slots) return;
    const bool owned = images_.bound_to(slot, token);
    reset(records_[slot]);
    if (owned) images_.unbind(slot);
  }
  void retire() {
    for (size_t slot = 0; slot < Slots; ++slot) {
      if (!records_[slot].token.binding) continue;
      const bool owned = images_.bound_to(slot, records_[slot].token);
      reset(records_[slot]);
      if (owned) images_.unbind(slot);
    }
  }
  size_t find(PageBindingToken token) const {
    for (size_t slot = 0; slot < Slots; ++slot)
      if (same(records_[slot].token, token)) return slot;
    return Slots;
  }
  PageImageSnapshot snapshot(PageBindingToken token) const {
    const auto slot = find(token);
    if (slot == Slots || records_[slot].endpoint != environment_.endpoint_generation()) return {};
    PageMediaRouteSnapshot route;
    if (!media_.cover_route({token.configuration, token.card}, route) || route.clear_artwork ||
        route.suppress_artwork || !records_[slot].routed ||
        route.secondary_selected != records_[slot].secondary)
      return {};
    const auto remote =
        state_.read_attribute({token.configuration, token.card}, route.entity, "entity_picture");
    if (remote.status == PageEntityStatus::AVAILABLE &&
        PageArtworkUrl::join(environment_.base_url(), remote.value.substr(0, 4096)).empty())
      return {};
    return images_.snapshot(slot, token);
  }
  uint64_t revision() const {
    return revision_ + images_.revision();
  }

  void observed(uint32_t configuration, std::string_view entity, std::string_view attribute,
                std::string_view value, bool copied) override {
    observed_update(configuration, entity, attribute, value, copied, {});
  }
  void observed_update(uint32_t configuration, std::string_view entity, std::string_view attribute,
                       std::string_view value, bool copied, std::string_view previous) override {
    if (copied) return;
    if (!attribute.empty() && attribute != "source" && attribute != "media_content_id" &&
        attribute != "media_content_type" && attribute != "media_title" && attribute != "media_artist" &&
        attribute != "entity_picture" && attribute != "entity_picture_local" && attribute != "access_token")
      return;
    for (auto& record : records_) {
      if (record.token.configuration != configuration || !record.token.binding) continue;
      PageMediaRouteSnapshot route;
      if (!media_.cover_route({configuration, record.token.card}, route)) continue;
      if (entity != route.primary && entity != route.secondary) continue;
      if (attribute == "entity_picture") {
        const auto base = environment_.base_url();
        record.remote_changed[entity == route.secondary] |=
            PageArtworkUrl::join(base, previous.substr(0, 4096)) !=
            PageArtworkUrl::join(base, value.substr(0, 4096));
      }
      // Pull the final pair and metadata only after the graph has published the
      // whole delivery. A later notification does not lose an earlier force.
      record.dirty = true;
      record.forced |= attribute == "access_token" && entity == route.entity &&
                       value.substr(0, 512) != previous.substr(0, 512);
      record.retry_selected = false;
      record.waiting = false;
    }
  }
  void loop() {
    for (size_t slot = 0; slot < Slots; ++slot) maintain(slot);
  }

 private:
  struct Record {
    PageBindingToken token;
    PageImageOptions options;
    PageCoverArtSignature signature;
    std::string selected;
    uint32_t started{0}, deadline{0}, retry_deadline{0}, endpoint{0}, connection{0};
    uint8_t errors{0};
    bool remote_changed[2]{false, false};
    bool routed{false}, secondary{false}, dirty{false}, forced{false};
    bool waiting{false}, retry{false}, retry_selected{false}, failure_seen{false};
  };
  static bool same(PageBindingToken a, PageBindingToken b) {
    return a.binding && a.configuration == b.configuration && a.binding == b.binding && a.page == b.page &&
           a.card == b.card && a.shell == b.shell && a.back == b.back;
  }
  static bool due(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
  }
  void reset(Record& record) {
    // Keep capacity through allocation-free navigation; release unused URL
    // storage from ordinary maintenance, after all page owners activate.
    auto selected = std::move(record.selected);
    selected.clear();
    record = {};
    record.selected = std::move(selected);
    ++revision_;
  }
  std::string_view attribute(ConfigurationHandle card, std::string_view entity, std::string_view name,
                             bool metadata = false) {
    const auto value = state_.read_attribute(card, entity, name);
    if (value.status != PageEntityStatus::AVAILABLE || (metadata && !media_.current(card, entity, name)))
      return {};
    return value.value;
  }
  void clear(size_t slot, bool clear_route = true) {
    auto& record = records_[slot];
    const auto token = record.token;
    // Invalidate ready/asset before cancellation can reenter the renderer.
    if (clear_route) record.routed = false;
    record.selected.clear();
    record.failure_seen = record.retry = false;
    ++revision_;
    images_.unbind(slot);
    // Rebind only this same still-live token; a cancellation may have replaced it.
    if (same(record.token, token) && images_.vacant(slot)) images_.bind(slot, token);
  }
  void maintain(size_t slot) {
    auto& record = records_[slot];
    const auto token = record.token;
    if (!token.binding) {
      if (record.selected.capacity()) std::string().swap(record.selected);
      return;
    }
    if (!images_.owns(slot, token)) {
      const bool owned = images_.bound_to(slot, token);
      reset(record);
      if (owned) images_.unbind(slot);
      return;
    }
    if (!images_.current_binding(slot, token)) {
      record.routed = false;
      record.dirty = record.forced = true;
      record.waiting = false;
      return;
    }
    if (record.endpoint != environment_.endpoint_generation() ||
        record.connection != media_.connection_generation()) {
      clear(slot);
      if (!same(record.token, token) || !images_.owns(slot, token)) return;
      record.endpoint = environment_.endpoint_generation();
      record.connection = media_.connection_generation();
      record.dirty = record.forced = true;
      record.waiting = false;
    }
    PageMediaRouteSnapshot route;
    const ConfigurationHandle card{token.configuration, token.card};
    if (!media_.cover_route(card, route) || route.clear_artwork) {
      if (record.routed || !record.selected.empty()) clear(slot);
      return;
    }
    if (!record.routed || record.secondary != route.secondary_selected) {
      if (record.routed) clear(slot);
      if (!same(record.token, token) || !images_.owns(slot, token)) return;
      record.routed = true;
      record.secondary = route.secondary_selected;
      record.dirty = record.forced = true;
      record.waiting = false;
      record.signature = {};
      record.errors = 0;
      record.started = media_.now_ms();
      ++revision_;
    }
    if (route.suppress_artwork) return;
    const auto now = media_.now_ms();
    const auto image = images_.snapshot(slot, token);
    if (image.ready && !image.pending && !image.failed) record.errors = 0;
    if (image.failed && !record.failure_seen) {
      record.failure_seen = true;
      const bool startup = !due(now, record.started + 45000) && record.errors < 10;
      if (image.ready || startup) {
        if (!image.ready) ++record.errors;
        record.retry = true;
        record.retry_deadline = now + 2000;
      }
    }
    if (record.retry && due(now, record.retry_deadline)) {
      record.retry = false;
      record.retry_selected = !record.dirty;
      record.dirty = record.forced = record.waiting = true;
      record.deadline = now;
    }
    if (!record.dirty) return;
    const auto signature =
        PageCoverArtSignature::make(attribute(card, route.entity, "media_content_id", true),
                                    attribute(card, route.entity, "media_title", true),
                                    attribute(card, route.entity, "media_artist", true),
                                    route.state.state_known, attribute(card, route.entity, ""));
    if (record.signature != signature) {
      record.signature = signature;
      record.forced = true;
    }
    if (!record.waiting) {
      record.waiting = true;
      record.deadline = now + 75;
      return;
    }
    if (!due(now, record.deadline)) return;
    const auto remote_raw = state_.read_attribute(card, route.entity, "entity_picture");
    if (remote_raw.status != PageEntityStatus::AVAILABLE) return;
    const auto base = environment_.base_url();
    const auto remote = PageArtworkUrl::join(base, remote_raw.value.substr(0, 4096));
    if (remote.empty()) {
      clear(slot, false);
      record.dirty = record.waiting = record.forced = false;
      return;
    }
    const auto local_raw = state_.read_attribute(card, route.entity, "entity_picture_local");
    if (!record.retry_selected && local_raw.status != PageEntityStatus::AVAILABLE &&
        !due(now, record.deadline + 300))
      return;
    const auto local = PageArtworkUrl::join(base, local_raw.status == PageEntityStatus::AVAILABLE
                                                      ? local_raw.value.substr(0, 4096)
                                                      : std::string_view{});
    const auto candidates = artwork::select_source_candidates(
        remote, local, record.remote_changed[record.secondary], remote.equals(record.selected),
        record.remote_changed[record.secondary]);
    const auto selected = record.retry_selected && !record.selected.empty()
                              ? PageArtworkUrl{{}, record.selected}
                              : candidates.primary;
    if (!record.forced && selected.equals(record.selected)) {
      record.dirty = record.waiting = false;
      record.remote_changed[record.secondary] = false;
      return;
    }
    if (!record.options.width || !record.options.height ||
        !environment_.request_allowed(record.options.width, record.options.height))
      return;
    // Copies only the selected job URL during maintenance. Observer callbacks
    // compare segmented views and never allocate candidate or entity strings.
    auto url = selected.copy();
    if (page_artwork_requires_token(url)) {
      const auto token = attribute(card, route.entity, "access_token").substr(0, 512);
      if (token.empty() || token == "unknown" || token == "unavailable") {
        record.deadline = now + 2000;
        return;
      }
      page_artwork_append_token(url, token);
    }
    if (!selected.equals(record.selected)) record.errors = 0;
    record.selected = selected.copy();
    record.dirty = record.waiting = record.forced = record.retry = record.retry_selected =
        record.failure_seen = false;
    record.remote_changed[record.secondary] = false;
    images_.request(slot, token, url, record.options);
    // start/cancel may deliver synchronously or replace the entire scene. Do
    // not touch the borrowed route, graph values or record after this call.
  }
  PageEntityStateRuntime& state_;
  PageMediaStateRuntime& media_;
  PageImageRuntime<Slots>& images_;
  PageCoverArtEnvironment& environment_;
  std::array<Record, Slots> records_{};
  uint64_t revision_{0};
};
}  // namespace espcontrol::pages
