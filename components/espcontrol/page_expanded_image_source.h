#pragma once

#include "page_image_tile_runtime.h"

namespace espcontrol::pages {
class PageExpandedImageEnvironment : public PageImageEnvironment {
 public:
  virtual bool visible(PageModalToken) const = 0;
  virtual uint16_t maximum_source_side() const = 0;
  // Concrete scene delegates to the existing PSRAM retention policy. Disabled
  // by default until that device environment is wired. Zero TTL is persistent.
  virtual bool retain_cache(uint16_t, uint16_t) const {
    return false;
  }
  virtual uint32_t cache_ttl_ms() const {
    return 0;
  }
  virtual bool cache_skips_open_refresh() const {
    return false;
  }
};
struct PageExpandedImageSnapshot {
  bool bound{false}, pending{false};
  PageImageTileStatus status{PageImageTileStatus::UNAVAILABLE};
  uint64_t asset{0};
  PageBindingToken preview;
};
// Borrowed cache-key candidate. Copy only during safe maintenance and before
// the next source delivery/loop/unbind; this does not park or retain any pixels.
struct PageExpandedImageContent {
  PageModalToken token;
  uint64_t asset{0};
  std::string_view entity, url;
  uint16_t width{0}, height{0};
  uint32_t background{0}, endpoint{0}, connection{0};
  bool fit{false};
};
// One fixed demanded-source record, not one per configured image. Entity and
// trigger views belong to the modal context; unbind before that context retires.
// Register this observer once. Pump raw/source maintenance before modal painting,
// outside native/HA/LVGL dispatch, after all configuration owners activate.
template <size_t TileSlots>
class PageExpandedImageSource final : public PageEntityObserver {
 public:
  using Now = uint32_t (*)();
  PageExpandedImageSource(PageEntityStateRuntime& graph, PageImageTileRuntime<TileSlots>& tiles,
                          PageModalImageRuntime& images, PageModalImageOrigin& origin,
                          PageExpandedImageEnvironment& environment, Now now)
      : graph_(graph),
        tiles_(tiles),
        images_(images),
        origin_(origin),
        environment_(environment),
        now_(now) {}
  ~PageExpandedImageSource() override {
    retire();
  }
  bool available() const {
    return now_ && images_.available();
  }
  bool bind(PageModalToken token, PageImageTileSource source) {
    if (retiring_ || !available() || !PageModalImageIdentity::eligible(token) || !origin_.valid(token) ||
        page_image_entity_kind(source.entity) == PageImageEntityKind::UNSUPPORTED)
      return false;
    if (same(record_.token, token)) return true;
    if (record_.token.opening) unbind(record_.token);
    if (record_.token.opening) return false;
    if (images_.acquire(token) != 0) return false;
    if (!origin_.valid(token)) {
      if (images_.bound_to(0, token)) images_.unbind(0);
      return false;
    }
    reset();
    auto& r = record_;
    r.token = token;
    r.source = source;
    r.kind = page_image_entity_kind(source.entity);
    r.started = now_();
    r.endpoint = environment_.endpoint_generation();
    r.options.cover = !source.fit;
    r.options.retain_completed = true;
    r.options.priority = PageImagePriority::MODAL;
    r.policy.connection = graph_.connection_generation();
    r.policy.schedule.mode =
        r.kind == PageImageEntityKind::CAMERA ? source.refresh : camera::RefreshMode::OFF;
    if (r.policy.schedule.mode == camera::RefreshMode::ACTIVITY && !page_image_trigger_valid(source.trigger))
      r.policy.schedule.mode = camera::RefreshMode::OFF;
    r.policy.schedule.interval_ms =
        source.interval_ms == 5000 || source.interval_ms == 30000 ? source.interval_ms : 10000;
    const auto serial = r.serial;
    r.dirty = r.forced = true;
    if (tiles_.has_tile({token.configuration, token.card})) {
      const auto transfer = tiles_.begin_expanded(token, origin_, r.policy);
      if (!current(serial, token)) return false;
      r.transfer = transfer;
      if (!transfer.transfer) {
        unbind(token);
        return false;
      }
    } else {
      seed();
      r.policy.schedule.enter_expanded(now_(), r.policy.activity.on(r.policy.connection));
    }
    if (!tiles_.defer_for_expanded(token, origin_) || !current(serial, token)) {
      if (current_record(serial, token)) unbind(token);
      return false;
    }
    return true;
  }
  void unbind(PageModalToken token) {
    if (!same(record_.token, token)) return;
    const auto transfer = record_.transfer;
    const auto policy = record_.policy;
    const bool owned = images_.bound_to(0, token);
    const bool retain = owned && same(cache_.candidate, token) && cache_.asset == record_.handled_asset &&
                        cache_.asset && !record_.dirty && !record_.forced && !record_.queued &&
                        !record_.retry && !record_.geometry_stale && !record_.policy.unavailable &&
                        !record_.stale_unavailable && !record_.policy.camera_errors &&
                        cache_.endpoint == environment_.endpoint_generation() &&
                        cache_.connection == graph_.connection_generation() &&
                        environment_.retain_cache(cache_.width, cache_.height);
    reset();  // Forget borrowed metadata before cancellation can replace context.
    tiles_.end_expanded(transfer, policy);
    if (retain && images_.bound_to(0, token)) {
      cache_.closed_at = now_();
      images_.park(0, token, cache_.lease);
    }
    // Parking/cancellation may synchronously admit a different opening.
    if (owned && images_.bound_to(0, token)) images_.unbind(0);
  }
  void retire() {
    if (retiring_) return;
    retiring_ = true;
    if (record_.token.opening) unbind(record_.token);
    purge_cache();
    retiring_ = false;
  }
  void purge_cache() {
    const auto id = cache_.lease;
    cache_.lease = cache_.asset = 0;
    cache_.candidate = {};
    if (id) images_.discard(0, id);
  }
  void size(PageModalToken token, uint16_t width, uint16_t height, uint32_t background) {
    if (!same(record_.token, token)) return;
    const auto target = page_image_limit_size(width, height, environment_.maximum_source_side());
    auto& r = record_;
    if (r.options.width == target.width && r.options.height == target.height &&
        (!r.source.fit || r.options.background == background))
      return;
    cache_.candidate = {};
    r.geometry_stale = r.options.width && r.options.height;
    r.options.width = target.width;
    r.options.height = target.height;
    r.options.background = background;
    r.dirty = r.forced = true;
    ++revision_;
  }
  PageExpandedImageSnapshot snapshot(PageModalToken token) const {
    if (!same(record_.token, token) || !images_.owns(0, token)) return {};
    const auto& r = record_;
    const auto image = images_.snapshot(0, token);
    PageExpandedImageSnapshot result;
    result.bound = true;
    result.pending = image.pending || r.queued;
    if (r.exhausted || r.policy.unavailable || r.geometry_stale ||
        r.endpoint != environment_.endpoint_generation())
      return result;
    const bool healthy = !r.stale_unavailable && !r.policy.camera_errors && !image.failed;
    if (healthy && image.ready && image.asset) {
      result.status = PageImageTileStatus::READY;
      result.asset = image.asset;
      return result;
    }
    if (healthy && !r.policy.tile_invalidated) {
      const auto preview = tiles_.expanded_tile(r.transfer);
      if (preview.binding && tiles_.snapshot(preview).status == PageImageTileStatus::READY) {
        result.status = PageImageTileStatus::READY;
        result.preview = preview;
        return result;
      }
    }
    if (result.pending ||
        (!r.stale_unavailable && !r.policy.camera_errors && !image.failed && !due(now_(), r.started + 45000)))
      result.status = PageImageTileStatus::LOADING;
    return result;
  }
  PageExpandedImageContent content(PageModalToken token) const {
    const auto frame = snapshot(token);
    const auto& r = record_;
    if (!frame.asset || frame.pending || r.dirty || r.forced || r.retry ||
        r.applied_source != r.source_version ||
        (r.kind == PageImageEntityKind::IMAGE && r.policy.picture.modal_dirty()))
      return {};
    return {token,
            frame.asset,
            r.source.entity,
            r.selected,
            r.options.width,
            r.options.height,
            r.options.background,
            r.endpoint,
            r.policy.connection,
            r.source.fit};
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
    // A hidden cache still observes its already-registered entity. Invalidation
    // only revokes value ownership here; pixel cleanup stays in maintenance.
    if (cache_.lease && entity == cache_.entity &&
        ((attribute.empty() &&
          (page_image_unavailable(value) || (page_image_entity_kind(entity) == PageImageEntityKind::IMAGE &&
                                             value.substr(0, 80) != cache_.picture.timestamp()))) ||
         (attribute == "access_token" && value.substr(0, 512) != previous.substr(0, 512)) ||
         (attribute == "entity_picture" && value != previous)))
      purge_cache();
    auto& r = record_;
    if (!r.token.opening || r.token.configuration != generation || !images_.owns(0, r.token)) return;
    if (r.policy.synchronize(graph_.connection_generation())) {
      r.dirty = r.forced = true;
      r.retry = r.waiting = false;
    }
    if (entity == r.source.entity) {
      if (attribute.empty()) {
        const auto update = r.policy.primary(r.kind, value, now_());
        if (update == PageImagePrimaryUpdate::IGNORED) return;
        if (update == PageImagePrimaryUpdate::UNAVAILABLE) {
          r.stale_unavailable = true;
          r.queued = false;
        }
        if (update == PageImagePrimaryUpdate::RECOVERED) {
          r.forced = true;
          r.retry = r.waiting = r.failure_seen = r.cancelled_unavailable = false;
        }
      } else if (attribute == "entity_picture")
        r.policy.picture_update(now_());
      else if (attribute == "access_token") {
        if (!r.policy.token_update(value, previous, now_())) return;
      } else
        return;
      cache_.candidate = {};
      r.dirty = true;
      r.waiting = false;
      ++revision_;
    } else if (attribute.empty() && !r.source.trigger.empty() && entity == r.source.trigger) {
      // An admitted opening owns fresh edges during initialization too; I/O
      // still waits for a visible, completed shell and its delayed request.
      r.policy.trigger(value, r.source.trigger.substr(0, 14) == "binary_sensor.", now_(), true);
    }
  }
  void loop() {
    maintain_cache();
    auto& r = record_;
    if (!now_ || !r.token.opening) return;
    const auto token = r.token;
    const auto serial = r.serial;
    if (!images_.owns(0, token)) {
      unbind(token);
      return;
    }
    seed();
    if (!images_.current_binding(0, token)) return;
    const auto now = now_();
    if (r.endpoint != environment_.endpoint_generation() || r.geometry_stale) {
      r.queued = false;
      r.policy.schedule.in_flight = false;
      images_.unbind(0);
      if (!current_record(serial, token) || !origin_.valid(token) || !images_.vacant(0)) return;
      if (!images_.bind(0, token)) return;
      r.endpoint = environment_.endpoint_generation();
      r.geometry_stale = false;
      r.handled_asset = 0;
      r.failure_seen = false;
      r.dirty = r.forced = true;
    }
    if (r.policy.unavailable) {
      r.queued = false;
      r.retry = false;
      r.policy.schedule.in_flight = false;
      if (!r.cancelled_unavailable) {
        r.cancelled_unavailable = true;
        images_.cancel_request(0, token, true);
      }
      return;
    }
    if (cache_.lease && r.options.width && r.options.height) try_cache();
    const auto image = images_.snapshot(0, token);
    if (image.ready && !image.pending && !image.failed && image.asset != r.handled_asset) {
      r.handled_asset = image.asset;
      r.policy.picture.modal_applied = r.policy.picture.modal_requested;
      r.policy.expanded_finished(now, true);
      r.applied_source = r.requested_source;
      r.stale_unavailable = r.failure_seen = r.retry = false;
      ++revision_;
    }
    if (image.failed && !r.failure_seen) {
      r.failure_seen = true;
      failed(now);
    }
    remember_cache();
    if (!environment_.visible(token) || image.pending || !r.options.width || !r.options.height || r.exhausted)
      return;
    if (r.retry && !due(now, r.policy.camera_retry_after)) return;
    if (r.waiting && !due(now, r.wake)) return;
    if (r.queued && !due(now, r.wake)) return;
    const bool image_dirty = r.kind == PageImageEntityKind::IMAGE && r.policy.picture.modal_dirty();
    const bool scheduled = r.policy.schedule.due(now, true);
    if (!r.queued && !r.dirty && !r.forced && !r.retry && !image_dirty && !scheduled) return;
    auto url = page_image_select_source(graph_, {token.configuration, token.card}, r.source.entity, r.kind,
                                        r.policy.picture_event, environment_.base_url());
    if (url.empty()) {
      r.queued = false;
      r.waiting = true;
      r.wake = now + 2000;
      return;
    }
    const bool changed = url != r.selected;
    if (changed) {
      if (r.source_version == UINT64_MAX) {
        r.exhausted = true;
        r.queued = false;
        ++revision_;
        return;
      }
      r.selected = url;
      ++r.source_version;
    }
    r.waiting = false;
    if (!r.queued) {
      if (!r.forced && !r.retry) {
        if (r.kind == PageImageEntityKind::IMAGE && !image_dirty) {
          r.dirty = false;
          return;
        }
        if (r.kind == PageImageEntityKind::CAMERA && r.policy.schedule.mode != camera::RefreshMode::OFF &&
            !scheduled) {
          r.dirty = false;
          return;
        }
        if (!scheduled && !image_dirty && !changed && !r.policy.camera_errors &&
            r.policy.last_tile_completed &&
            static_cast<uint32_t>(now - r.policy.last_tile_completed) < 30000) {
          r.dirty = false;
          return;
        }
      }
      r.queued = true;
      r.wake = now + 100;
      ++revision_;
      return;
    }
    if (!environment_.request_allowed(r.options.width, r.options.height)) {
      r.queued = false;
      failed(now);
      return;
    }
    if (!images_.requestable()) {
      r.queued = false;
      r.exhausted = true;
      ++revision_;
      return;
    }
    cache_.candidate = {};
    r.options.max_source_dimension =
        page_image_size_source(url, r.options.width, r.options.height, environment_.maximum_source_side());
    r.queued = r.retry = r.dirty = r.forced = r.failure_seen = false;
    r.policy.picture.modal_requested = r.policy.picture.latest;
    r.requested_source = r.source_version;
    r.policy.schedule.started();
    images_.request(0, token, url, r.options);
    // Transport callbacks can replace the scene; no borrowed state is used now.
  }

 private:
  struct Cache {
    std::string entity, url;
    PageImageRevision picture;
    PageModalToken candidate;
    uint64_t lease{0}, asset{0};
    uint16_t width{0}, height{0};
    uint32_t background{0}, endpoint{0}, connection{0}, closed_at{0};
    bool fit{false};
  };
  void maintain_cache() {
    if (!cache_.lease) return;
    const auto ttl = environment_.cache_ttl_ms();
    if (images_.parked(0) != cache_.lease || cache_.connection != graph_.connection_generation() ||
        cache_.endpoint != environment_.endpoint_generation() ||
        (ttl && static_cast<uint32_t>(now_() - cache_.closed_at) >= ttl) ||
        !environment_.retain_cache(cache_.width, cache_.height))
      purge_cache();
  }
  void remember_cache() {
    const auto frame = content(record_.token);
    if (!frame.asset || !environment_.retain_cache(frame.width, frame.height)) return;
    cache_.candidate = frame.token;
    cache_.asset = frame.asset;
    cache_.entity.assign(frame.entity.data(), frame.entity.size());
    cache_.url.assign(frame.url.data(), frame.url.size());
    cache_.width = frame.width;
    cache_.height = frame.height;
    cache_.background = frame.background;
    cache_.fit = frame.fit;
    cache_.endpoint = frame.endpoint;
    cache_.connection = frame.connection;
    cache_.picture = record_.policy.picture;
  }
  void try_cache() {
    auto& r = record_;
    const bool key =
        cache_.entity == r.source.entity && cache_.width == r.options.width &&
        cache_.height == r.options.height && cache_.fit == r.source.fit &&
        (!r.source.fit || cache_.background == r.options.background) && !r.policy.unavailable &&
        !r.stale_unavailable && !r.policy.camera_errors && r.policy.primary_seeded &&
        (r.kind != PageImageEntityKind::IMAGE || cache_.picture.timestamp() == r.policy.picture.timestamp());
    if (!key) {
      purge_cache();
      return;
    }
    auto url = page_image_select_source(graph_, {r.token.configuration, r.token.card}, r.source.entity,
                                        r.kind, r.policy.picture_event, environment_.base_url());
    if (url != cache_.url || !images_.adopt(0, r.token, cache_.lease)) {
      purge_cache();
      return;
    }
    cache_.lease = 0;
    r.selected = std::move(url);
    r.source_version = r.requested_source = r.applied_source = 1;
    r.handled_asset = images_.snapshot(0, r.token).asset;
    r.policy.picture.modal_requested = r.policy.picture.modal_applied = r.policy.picture.latest;
    if (environment_.cache_skips_open_refresh()) r.dirty = r.forced = false;
    // A cached result is not a new network completion and cannot reset cadence.
    ++revision_;
  }
  struct Record {
    PageModalToken token;
    PageImageTileSource source;
    PageImageEntityKind kind{PageImageEntityKind::UNSUPPORTED};
    PageImageRefreshState policy;
    PageImageRefreshTransfer transfer;
    PageImageOptions options;
    std::string selected;
    uint64_t serial{0}, handled_asset{0}, source_version{0}, requested_source{0}, applied_source{0};
    uint32_t started{0}, wake{0}, endpoint{0};
    bool dirty{false}, forced{false}, queued{false}, waiting{false}, retry{false}, failure_seen{false},
        geometry_stale{false}, stale_unavailable{false}, cancelled_unavailable{false}, exhausted{false};
  };
  static bool same(PageModalToken a, PageModalToken b) {
    return PageModalImageIdentity::same(a, b);
  }
  static bool due(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
  }
  bool current_record(uint64_t serial, PageModalToken token) const {
    return record_.serial == serial && same(record_.token, token);
  }
  bool current(uint64_t serial, PageModalToken token) const {
    return current_record(serial, token) && images_.owns(0, token);
  }
  void reset() {
    auto selected = std::move(record_.selected);
    selected.clear();
    record_ = {};
    record_.selected = std::move(selected);
    record_.serial = ++serial_;
    ++revision_;
  }
  void failed(uint32_t now) {
    auto& r = record_;
    r.policy.expanded_finished(now, false);
    r.stale_unavailable = true;
    r.retry = true;
    r.queued = r.waiting = false;
    ++revision_;
  }
  void seed() {
    auto& r = record_;
    const ConfigurationHandle card{r.token.configuration, r.token.card};
    if (!r.policy.primary_seeded) {
      const auto value = graph_.read_attribute(card, r.source.entity, {});
      if (value.status == PageEntityStatus::AVAILABLE) r.policy.primary(r.kind, value.value, now_());
    }
    if (!r.policy.activity_seeded && !r.source.trigger.empty()) {
      const auto value = graph_.read_attribute(card, r.source.trigger, {});
      if (value.status == PageEntityStatus::AVAILABLE) {
        r.policy.activity.observe(value.value, r.source.trigger.substr(0, 14) == "binary_sensor.",
                                  r.policy.connection);
        r.policy.activity_seeded = true;
      }
    }
  }
  PageEntityStateRuntime& graph_;
  PageImageTileRuntime<TileSlots>& tiles_;
  PageModalImageRuntime& images_;
  PageModalImageOrigin& origin_;
  PageExpandedImageEnvironment& environment_;
  Now now_;
  Record record_;
  Cache cache_;
  bool retiring_{false};
  uint64_t serial_{0}, revision_{0};
};
}  // namespace espcontrol::pages
