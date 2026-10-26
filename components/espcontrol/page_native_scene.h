#pragma once

#include <cmath>

// Include after button_grid.h and the clock/date producer helpers. The selected
// firmware creates this application-lifetime owner after reserving its widgets.
#include "application_scene_runtime.h"
#include "page_alarm_presentation.h"
#include "page_artwork_cover_view.h"
#include "page_artwork_expanded_image_view.h"
#include "page_artwork_image_tile_view.h"
#include "page_card_control_router.h"
#include "page_grid_input.h"
#include "page_lvgl_qr_encoder.h"
#include "page_native_card_commands.h"
#include "page_native_modal_bank.h"
#include "page_native_scene_state.h"
#include "page_pooled_qr_tile_view.h"
#include "page_scene_selection.h"
#include "page_slider_command_policy.h"
#include "page_timer_command_policy.h"

namespace espcontrol::pages {
// The scene supplies the original owning page explicitly, including hidden
// authenticated openings. A device source supplies fonts/palette/layout only;
// it never searches the current visible bank by label/entity or retains views.
class PageNativeSceneStyles {
public:
  virtual ~PageNativeSceneStyles() = default;
  virtual const char *icon(std::string_view name,
                           std::string_view domain) const = 0;
#define PAGE_NATIVE_STYLE(Environment)                                         \
  virtual bool resolve(const PageStore &, PageId, PageModalToken,              \
                       const ParsedCfg &, Environment &) const = 0
  PAGE_NATIVE_STYLE(espcontrol::cards::ClimateControlDriverEnvironment);
  PAGE_NATIVE_STYLE(PageControlModalEnvironment);
  PAGE_NATIVE_STYLE(PageFanControlModalEnvironment);
  PAGE_NATIVE_STYLE(PageOptionSelectModalEnvironment);
  PAGE_NATIVE_STYLE(PageMediaVolumeModalEnvironment);
  PAGE_NATIVE_STYLE(PageMediaControlModalEnvironment);
  PAGE_NATIVE_STYLE(PageWeatherModalStyle);
  PAGE_NATIVE_STYLE(PageConfirmationModalEnvironment);
  PAGE_NATIVE_STYLE(PageAlarmModalEnvironment);
  PAGE_NATIVE_STYLE(PageExpandedImageModalEnvironment);
  PAGE_NATIVE_STYLE(PageWifiQrModalEnvironment);
#undef PAGE_NATIVE_STYLE
  virtual bool resolve(const PageStore &, PageLocalModalOrigin, PageModalToken,
                       PageNetworkStatusEnvironment &) const = 0;
};

class PageNativeScenePlatform {
public:
  virtual ~PageNativeScenePlatform() = default;
  // Read-only current display/wake policy. Critical alarm presentation may
  // update that policy synchronously, so the scene samples it after callbacks.
  virtual bool grid_active() const = 0;
  virtual bool input_allowed() const = 0;
  virtual std::string_view base_url() const = 0;
  virtual uint32_t endpoint_generation() const = 0;
  virtual bool request_allowed(uint16_t width, uint16_t height,
                               bool modal) const = 0;
  virtual uint16_t maximum_source_side(bool modal) const = 0;
  virtual bool retain_cache(uint16_t width, uint16_t height) const = 0;
  virtual uint32_t cache_ttl_ms() const = 0;
  virtual bool cache_skips_open_refresh() const = 0;
  virtual void critical(bool active) = 0;
  virtual void alarm_failure(PageAlarmFailure) = 0;
  // Called from safe maintenance only, after the current view/panel settles.
  virtual void chrome(const PageStore &, PageId, bool settings) = 0;
};

struct PageNativeSceneBudget {
  PageNativeStateBudget state;
  size_t document_arena{0};
};
struct PageNativeSceneMemory {
  PageRuntimeMemory configuration;
  PageRendererMemory renderer;
  PageBasicMemory view;
  PageCardCommandMemory commands;
  PageModalMemory modal;
  PageEntityMemory state;
  size_t image_leases{0}, qr_lease_bytes{0}, qr_frame_bytes{0};
};

template <size_t Slots, size_t ImageSlots>
class PageNativeScene final : public ApplicationSceneRuntime,
                              public ApplicationSceneNavigation,
                              public PageImageOrigin,
                              public PageQrOrigin {
  class ModalOrigin final : public PageModalImageOrigin {
  public:
    explicit ModalOrigin(PageNativeScene &scene) : scene_(scene) {}
    bool valid(PageModalToken t) const override {
      return !scene_.stopped_ && scene_.modals_.runtime().valid(t);
    }
    bool visible(PageModalToken t) const override {
      return valid(t) && scene_.platform_.grid_active() &&
             scene_.modals_.runtime().interactive(t);
    }
    bool connected() const override { return scene_.connected(); }
    uint32_t connection_generation() const override {
      return scene_.connection_generation();
    }

  private:
    PageNativeScene &scene_;
  };
  class TileEnvironment final : public PageImageTileEnvironment {
  public:
    explicit TileEnvironment(PageNativeScene &scene) : scene_(scene) {}
    std::string_view base_url() const override {
      return scene_.platform_.base_url();
    }
    uint32_t endpoint_generation() const override {
      return scene_.platform_.endpoint_generation();
    }
    bool request_allowed(uint16_t w, uint16_t h) const override {
      return !scene_.stopped_ && !scene_.covered_ &&
             scene_.platform_.grid_active() &&
             scene_.platform_.request_allowed(w, h, false);
    }
    bool visible(PageBindingToken t) const override {
      return scene_.visible(t);
    }
    bool expanded(PageBindingToken t) const override {
      const auto token = scene_.modals_.runtime().token();
      return token.kind == ControlModalKind::IMAGE_CARD &&
             token.configuration == t.configuration && token.card == t.card &&
             scene_.modals_.runtime().valid(token);
    }
    uint16_t maximum_source_side() const override {
      return scene_.platform_.maximum_source_side(false);
    }

  private:
    PageNativeScene &scene_;
  };
  class ExpandedEnvironment final : public PageExpandedImageEnvironment {
  public:
    explicit ExpandedEnvironment(PageNativeScene &scene) : scene_(scene) {}
    std::string_view base_url() const override {
      return scene_.platform_.base_url();
    }
    uint32_t endpoint_generation() const override {
      return scene_.platform_.endpoint_generation();
    }
    bool request_allowed(uint16_t w, uint16_t h) const override {
      return !scene_.stopped_ && scene_.platform_.grid_active() &&
             scene_.platform_.request_allowed(w, h, true);
    }
    bool visible(PageModalToken t) const override {
      return scene_.modal_origin_.visible(t);
    }
    uint16_t maximum_source_side() const override {
      return scene_.platform_.maximum_source_side(true);
    }
    bool retain_cache(uint16_t w, uint16_t h) const override {
      return scene_.platform_.retain_cache(w, h);
    }
    uint32_t cache_ttl_ms() const override {
      return scene_.platform_.cache_ttl_ms();
    }
    bool cache_skips_open_refresh() const override {
      return scene_.platform_.cache_skips_open_refresh();
    }

  private:
    PageNativeScene &scene_;
  };
  class AlarmPort final : public PageAlarmPresentationPort {
  public:
    explicit AlarmPort(PageNativeScene &scene) : scene_(scene) {}
    bool ready() const override {
      return (!scene_.stopped_ || scene_.destroying_) &&
             page_native_command_ready();
    }
    bool connected() const override { return scene_.connected(); }
    uint32_t connection_generation() const override {
      return scene_.connection_generation();
    }
    void critical(bool active) override { scene_.platform_.critical(active); }
    void failure(PageAlarmFailure failure) override {
      scene_.platform_.alarm_failure(failure);
    }

  private:
    PageNativeScene &scene_;
  };
  class Images final : public PageRuntimeResources {
  public:
    explicit Images(PageNativeScene &scene) : scene_(scene) {}
    bool prepare(const PageStore &) override { return true; }
    void discard_prepared() override {}
    void activate_prepared(const PageStore &) override { retire_active(); }
    void retire_active() override {
      scene_.expanded_.retire();
      scene_.cover_.retire();
      scene_.tiles_.retire();
      scene_.images_.retire();
      scene_.modal_images_.retire();
      scene_.qr_.retire();
    }

  private:
    PageNativeScene &scene_;
  };
  struct Flag {
    bool &value;
    explicit Flag(bool &v) : value(v) { value = true; }
    ~Flag() { value = false; }
  };

public:
  static_assert(Slots > 0 && Slots <= UINT8_MAX && ImageSlots > 0);
  PageNativeScene(
      const DeviceUiLimits &limits, PageArenaAllocator &allocator,
      PageNativeSceneBudget budget, std::array<PageBasicWidgets, Slots> widgets,
      PageViewGeometry geometry, const PageBasicTheme &theme,
      PageTimezoneResolver timezone_resolver, PageStatusCaptions captions,
      PageAlarmStateRuntime::Translate translate, PageNativeSceneStyles &styles,
      PageNativeScenePlatform &platform, PageNetworkStatusSource &network)
      : limits_(limits), platform_(platform), styles_(styles),
        state_(allocator, budget.state, timezone_resolver, captions, translate),
        modal_origin_(*this), tile_environment_(*this),
        expanded_environment_(*this), images_(requests_, *this),
        modal_images_(modal_requests_, modal_origin_),
        cover_(state_.graph, state_.media, images_, tile_environment_),
        tiles_(state_.graph, images_, tile_environment_,
               PageNativeSceneState::now),
        expanded_(state_.graph, tiles_, modal_images_, modal_origin_,
                  expanded_environment_, PageNativeSceneState::now),
        cover_view_(cover_, requests_), tile_view_(tiles_, requests_),
        expanded_view_(expanded_, modal_requests_, tile_view_),
        qr_frames_(allocator),
        qr_(allocator, qr_encoder_, qr_frames_, *this, Slots),
        qr_view_(qr_, qr_frames_),
        slider_policy_(state_.graph, state_.percent, state_.number,
                       state_.temperature),
        timer_policy_(state_.graph, state_.timers),
        media_policy_(state_.graph, state_.media),
        device_policy_(state_.graph, state_.fans, state_.access),
        controls_({{{PageCardCommandKind::PERCENT, &slider_policy_},
                    {PageCardCommandKind::NUMBER, &slider_policy_},
                    {PageCardCommandKind::TEMPERATURE, &slider_policy_},
                    {PageCardCommandKind::SLIDER_TOGGLE, &slider_policy_},
                    {PageCardCommandKind::TIMER_START, &timer_policy_},
                    {PageCardCommandKind::TIMER_CANCEL, &timer_policy_},
                    {PageCardCommandKind::MEDIA_ACTION, &media_policy_},
                    {PageCardCommandKind::MEDIA_SEEK, &media_policy_},
                    {PageCardCommandKind::DEVICE_ACTION, &device_policy_}}}),
        command_port_(this, grid_valid, &controls_, accepted),
        commands_(state_.graph, command_port_),
        modals_({allocator, state_.graph, state_.forecasts, state_.alarms,
                 state_.acknowledgements, qr_encoder_, expanded_view_, network,
                 PageNativeSceneState::now, Slots},
                modal_styles(), {this, grid_valid, local_valid, modal_input}),
        alarm_port_(*this), alarm_(state_.alarms, state_.acknowledgements,
                                   modals_.runtime(), alarm_port_),
        theme_(theme),
        view_(allocator, state_.graph, widgets, geometry, theme_, actions(),
              &state_.forecasts, &state_.media, &cover_view_, &state_.local,
              &state_.relays, &state_.fans, &state_.percent, &state_.number,
              &state_.temperature, &state_.timers, &state_.covers,
              &state_.access, &state_.calendar, &state_.timezones,
              &state_.alarms, &tile_view_, &qr_view_),
        renderer_(limits, allocator, view_),
        input_(view_, renderer_, commands_), image_resources_(*this),
        resources_({{&modals_, &image_resources_, &renderer_, &commands_,
                     &alarm_, &state_}}),
        store_(limits, allocator, budget.document_arena, &resources_) {
    configured_ =
        limits.slot_count == Slots && limits.image_slot_count == ImageSlots &&
        budget.document_arena && controls_.ready() &&
        state_.observers.add(cover_) && state_.observers.add(tiles_) &&
        state_.observers.add(expanded_);
  }
  ~PageNativeScene() override {
    if (application_scene_navigation() == this)
      application_scene_navigation() = nullptr;
    assert(!running_ && !changing_ && page_native_command_ready());
    destroying_ = true;
    stop();
    store_.discard_prepared();
    resources_.retire_active();
    modals_.loop();
    images_.loop();
    modal_images_.loop();
    qr_.loop();
    alarm_.loop();
    state_.observers.remove(expanded_);
    state_.observers.remove(tiles_);
    state_.observers.remove(cover_);
    assert(!modals_.runtime().memory().active_contexts &&
           !modals_.runtime().memory().retired_contexts);
  }
  PageNativeScene(const PageNativeScene &) = delete;
  PageNativeScene &operator=(const PageNativeScene &) = delete;

  bool initialize(esphome::artwork_image::ArtworkImage *const *images,
                  size_t image_count,
                  esphome::artwork_image::ArtworkImage *expanded,
                  bool legacy_renderer_retired) {
    if (ready())
      return true;
    if (!configured_ || stopped_ || !legacy_renderer_retired ||
        !page_native_command_ready())
      return false;
    if (!requests_.initialized() && !requests_.initialize(images, image_count))
      return false;
    if (!modal_requests_.initialized() &&
        !modal_requests_.initialize(&expanded, 1))
      return false;
    if (!qr_.initialize() || !view_.initialize() || !state_.initialize(true) ||
        !modals_.initialize(true))
      return false;
    initialized_ = true;
    return true;
  }
  bool ready() const override { return initialized_ && !stopped_; }
  bool maintenance_ready() const override {
    return ready() && !running_ && !changing_ && page_native_command_ready();
  }
  bool prepare(uint16_t version, const uint8_t *data, size_t size) override {
    if (!maintenance_ready())
      return false;
    Flag guard(changing_);
    return store_.prepare(version, data, size);
  }
  void discard_prepared() override { store_.discard_prepared(); }
  bool activate_prepared(uint32_t generation, uint16_t version,
                         const uint8_t *data, size_t size) override {
    if (!maintenance_ready())
      return false;
    Flag guard(changing_);
    const bool activated =
        store_.activate_prepared(generation, version, data, size);
    if (activated) {
      ui_request_ = {};
      close_requested_ = takeover_requested_ = false;
      origin_ = {};
      advance_chrome();
      chrome_dirty_ = layout_dirty_ = true;
      // First activation can occur while a loading/screensaver page owns the
      // display. Reconcile the newly bound renderer, not its previous empty
      // one.
      suspended_ = false;
      reconcile_display();
    }
    return activated;
  }
  bool apply(uint16_t, const uint8_t *, size_t) override { return false; }
  // Appearance/orientation changes prepare only the view. The immutable model,
  // graph, subscriptions and semantic state keep their ownership and values.
  // The supplied text/font views must have application lifetime, like boot's.
  bool reconfigure(PageViewGeometry geometry, const PageBasicTheme &theme) {
    if (!maintenance_ready() || store_.memory().staged_reserved_bytes ||
        !geometry.visible_slots || geometry.visible_slots > Slots ||
        !geometry.columns || geometry.columns > geometry.visible_slots)
      return false;
    Flag guard(changing_);
    const auto previous_geometry = view_.geometry(store_.active());
    const auto previous_theme = theme_;
    theme_ = theme;
    view_.geometry(geometry);
    // Restored appearance/orientation scripts may run before the delayed boot
    // document. No active resources exist yet; the first prepare uses these.
    if (!store_.active().page_count())
      return true;
    if (!renderer_.prepare(store_.active()) ||
        !modals_.prepare(store_.active())) {
      renderer_.discard_prepared();
      modals_.discard_prepared();
      theme_ = previous_theme;
      view_.geometry(previous_geometry);
      return false;
    }
    covered_ = true;
    input_.cover(true);
    commands_.cancel();
    modals_.activate_prepared(store_.active());
    image_resources_.retire_active();
    renderer_.activate_prepared(store_.active());
    if (ui_request_.kind == UiKind::ACTIVATE)
      ui_request_ = {};
    suspended_ = false;
    origin_ = {};
    advance_chrome();
    chrome_dirty_ = layout_dirty_ = true;
    return true;
  }
  void stop() override {
    stopped_ = true;
    ui_request_ = {};
    close_requested_ = takeover_requested_ = false;
    input_.cover(true);
    modals_.stop();
    state_.stop();
    origin_ = {};
    advance_chrome();
  }
  void loop() override {
    if (!maintenance_ready())
      return;
    Flag guard(running_);
    state_.loop();
    if (stopped_)
      return;
    reconcile_display();
    alarm_.loop();
    if (stopped_)
      return;
    reconcile_display();
    if (takeover_requested_) {
      takeover_requested_ = false;
      close_requested_ = false;
      modals_.runtime().display_takeover(true);
    } else if (close_requested_) {
      close_requested_ = false;
      modals_.runtime().dismiss(PageModalClose::USER);
    }
    const auto memory = modals_.runtime().memory();
    if (memory.pending_requests || memory.active_contexts ||
        memory.retired_contexts)
      commands_.cancel();
    else
      commands_.loop();
    if (stopped_)
      return;
    reconcile_display();
    modals_.loop();
    if (stopped_)
      return;
    reconcile_display();
    reconcile_cover();
    input_.settle();
    process_ui_request();
    if (layout_dirty_ && !suspended_) {
      layout_dirty_ = false;
      view_.settle_layout();
      renderer_.refresh();
    }
    images_.loop();
    modal_images_.loop();
    qr_.loop();
    if (stopped_)
      return;
    cover_.loop();
    tiles_.loop();
    expanded_.loop();
    if (stopped_)
      return;
    reconcile_display();
    reconcile_cover();
    renderer_.update_visible();
    if (chrome_dirty_) {
      chrome_dirty_ = false;
      platform_.chrome(store_.active(), renderer_.active_page(), settings_);
    }
  }

  // Full ownership stays valid under an ordinary panel, for accepted feedback
  // and retained artwork. Input admission has its separate, stricter guard.
  bool valid(PageBindingToken t) const override {
    return !stopped_ && renderer_.bound(t);
  }
  bool visible(PageBindingToken t) const override {
    return valid(t) && !covered_ && platform_.grid_active();
  }
  bool screen_active(PageBindingToken t) const override { return visible(t); }
  bool connected() const override {
    return !stopped_ && ha_api_state_connected();
  }
  uint32_t connection_generation() const override {
    return state_.graph.connection_generation();
  }
  const PageStore &active() const { return store_.active(); }
  PageNativeSceneMemory memory() const {
    PageNativeSceneMemory result{
        store_.memory(),    renderer_.memory(),         view_.memory(),
        commands_.memory(), modals_.runtime().memory(), state_.graph.memory()};
    for (size_t i = 0; i < ImageSlots; ++i)
      if (!images_.vacant(i))
        ++result.image_leases;
    result.qr_lease_bytes = qr_.reserved_bytes();
    result.qr_frame_bytes = qr_frames_.reserved_bytes();
    return result;
  }
  PageId active_page() const { return renderer_.active_page(); }
  PageBindingToken binding(size_t shell) const {
    return renderer_.binding(shell);
  }
  PageQrSnapshot qr_snapshot(PageBindingToken token) const {
    return qr_view_.snapshot(token);
  }
  PageLocalModalOrigin chrome_origin() const {
    return {store_.active().generation(), chrome_binding_,
            renderer_.active_page()};
  }
  bool request_settings(PageLocalModalOrigin origin) {
    const bool accepted =
        ready() && modals_.runtime().request_local(
                       origin, ControlModalKind::NETWORK_STATUS);
    if (accepted) {
      ui_request_ = {};
      close_requested_ = false;
    }
    return accepted;
  }
  bool request_card(ConfigurationHandle card,
                    ControlModalKind kind = ControlModalKind::NONE) {
    if (!ready() || !platform_.grid_active() ||
        espcontrol::screensaver_pin().locked())
      return false;
    origin_ = {};
    const bool accepted = modals_.runtime().request_card(card, kind);
    if (accepted) {
      close_requested_ = false;
      ui_request_ = {};
    }
    return accepted;
  }

  bool navigation_ready() const override {
    return ready() && !changing_ && active().page_count() &&
           active().page(0)->navigable;
  }
  bool grid_configured() const override {
    if (!navigation_ready())
      return false;
    for (size_t shell = 0; shell < Slots; ++shell)
      if (renderer_.cell(0, shell).visible)
        return true;
    return false;
  }
  int subpage_slot() const override {
    const auto *page = active().page(active_page());
    const auto *owner =
        page && active_page() != 0 ? active().card(page->owner) : nullptr;
    return owner ? owner->slot : 0;
  }
  std::string_view subpage_label() const override {
    const auto *page = active().page(active_page());
    const auto *owner =
        page && active_page() != 0 ? active().card(page->owner) : nullptr;
    return owner ? active().field(*owner, CardField::LABEL)
                 : std::string_view{};
  }
  bool critical_active() const override {
    return !stopped_ && alarm_.memory().critical;
  }
  bool can_open_modal(std::string_view entity) const override {
    return remote_allowed() && modal_target(entity).index != NO_CARD;
  }
  bool open_modal(std::string_view entity) override {
    const auto target =
        remote_allowed() ? modal_target(entity) : ConfigurationHandle{};
    return target.index != NO_CARD && request_card(target);
  }
  bool can_open_subpage(std::string_view label) const override {
    return remote_allowed() && folder_target(label) != NO_PAGE;
  }
  bool open_subpage(std::string_view label) override {
    if (!remote_allowed())
      return false;
    return request_page(folder_target(label));
  }
  bool close_subpage() override {
    if (!navigation_ready() || critical_active())
      return false;
    const auto *pending = ui_request_.kind == UiKind::ACTIVATE
                              ? active().card(ui_request_.card)
                              : nullptr;
    if (!subpage_slot() &&
        !(ui_request_.kind == UiKind::PAGE && ui_request_.page != 0) &&
        !(pending && pending->target != NO_PAGE))
      return false;
    return home();
  }
  bool navigate(std::string_view text) override {
    if (!remote_allowed())
      return false;
    const auto target = trim(text);
    if (equal(target, "home") || equal(target, "main"))
      return home();
    CardId card = NO_CARD;
    for (size_t shell = 0; shell < Slots; ++shell) {
      const auto cell = renderer_.cell(0, shell);
      const auto *definition =
          cell.visible && !cell.back ? active().card(cell.card) : nullptr;
      if (definition &&
          !active().field(*definition, CardField::LABEL).empty() &&
          equal(trim(active().field(*definition, CardField::LABEL)), target)) {
        card = cell.card;
        break;
      }
    }
    if (card == NO_CARD && target.size() > 5 &&
        equal(target.substr(0, 5), "slot:")) {
      const auto number = trim(target.substr(5));
      size_t slot = 0;
      if (number.empty())
        return false;
      for (char c : number) {
        if (c < '0' || c > '9' || slot > Slots / 10)
          return false;
        slot = slot * 10 + static_cast<unsigned>(c - '0');
        if (slot > Slots)
          return false;
      }
      for (size_t shell = 0; shell < Slots; ++shell) {
        const auto cell = renderer_.cell(0, shell);
        const auto *definition =
            cell.visible && !cell.back ? active().card(cell.card) : nullptr;
        if (definition && definition->slot == slot) {
          card = cell.card;
          break;
        }
      }
    }
    if (card == NO_CARD)
      return false;
    ui_request_ = {UiKind::ACTIVATE, active().generation(), 0, card,
                   connection_generation()};
    close_requested_ = true;
    cover_for_request();
    return true;
  }
  bool home() override {
    return navigation_ready() && !critical_active() && request_page(0);
  }
  void close_modal() override {
    if (!ready())
      return;
    ui_request_ = {};
    close_requested_ = true;
    cover_for_request();
  }
  bool open_settings() override {
    if (!remote_allowed() || !platform_.grid_active() ||
        !platform_.input_allowed())
      return false;
    const bool accepted = request_settings(chrome_origin());
    if (accepted) {
      ui_request_ = {};
      close_requested_ = false;
    }
    return accepted;
  }
  void close_settings() override {
    const auto &modal = modals_.runtime();
    if (modal.token().kind == ControlModalKind::NETWORK_STATUS ||
        modal.pending_kind() == ControlModalKind::NETWORK_STATUS)
      close_modal();
  }
  void display_takeover() override {
    if (!ready())
      return;
    takeover_requested_ = true;
    if (ui_request_.kind == UiKind::ACTIVATE)
      ui_request_ = {};
    cover_for_request();
  }
  void refresh_chrome() override { chrome_dirty_ = true; }

private:
  enum class UiKind : uint8_t { NONE, PAGE, ACTIVATE };
  struct UiRequest {
    UiKind kind{UiKind::NONE};
    uint32_t configuration{0};
    PageId page{NO_PAGE};
    CardId card{NO_CARD};
    uint32_t connection{0};
  } ui_request_;
  bool close_requested_{false}, takeover_requested_{false};
  static std::string_view trim(std::string_view text) {
    const auto space = [](char c) {
      return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' ||
             c == '\v';
    };
    while (!text.empty() && space(text.front()))
      text.remove_prefix(1);
    while (!text.empty() && space(text.back()))
      text.remove_suffix(1);
    return text;
  }
  static bool equal(std::string_view a, std::string_view b) {
    if (a.size() != b.size())
      return false;
    const auto lower = [](char c) {
      return c >= 'A' && c <= 'Z' ? static_cast<char>(c + 'a' - 'A') : c;
    };
    for (size_t i = 0; i < a.size(); ++i)
      if (lower(a[i]) != lower(b[i]))
        return false;
    return true;
  }
  bool remote_allowed() const {
    return navigation_ready() && !espcontrol::screensaver_pin().locked() &&
           !critical_active();
  }
  PageId folder_target(std::string_view label) const {
    label = trim(label);
    if (label.empty())
      return NO_PAGE;
    for (size_t shell = 0; shell < Slots; ++shell) {
      const auto cell = renderer_.cell(0, shell);
      const auto *card =
          cell.visible && !cell.back ? active().card(cell.card) : nullptr;
      const auto *page = card ? active().page(card->target) : nullptr;
      if (page && page->navigable &&
          equal(trim(active().field(*card, CardField::LABEL)), label))
        return card->target;
    }
    return NO_PAGE;
  }
  ConfigurationHandle modal_target(std::string_view entity) const {
    entity = trim(entity);
    if (entity.empty())
      return {};
    const auto match = [this,
                        entity](PageCellLayout cell) -> ConfigurationHandle {
      const auto *card =
          cell.visible && !cell.back ? active().card(cell.card) : nullptr;
      if (!card)
        return {};
      const auto type = active().field(*card, CardField::TYPE);
      const auto configured =
          type == "alarm" || type == "alarm_action"
              ? PageAlarmStateRuntime::effective_entity(active(), cell.card)
              : active().field(*card, CardField::ENTITY);
      const ConfigurationHandle handle{active().generation(), cell.card};
      if (configured != entity)
        return {};
      const auto kind = modals_.runtime().card_kind(handle);
      return kind != ControlModalKind::NONE &&
                     kind != ControlModalKind::SWITCH_CONFIRMATION
                 ? handle
                 : ConfigurationHandle{};
    };
    // Root controls win, then visible parent order and child display order.
    // Hidden/unreachable parent folders cannot contribute remote controls.
    for (size_t shell = 0; shell < Slots; ++shell) {
      const auto result = match(renderer_.cell(0, shell));
      if (result.index != NO_CARD)
        return result;
    }
    for (size_t parent = 0; parent < Slots; ++parent) {
      const auto root = renderer_.cell(0, parent);
      const auto *card =
          root.visible && !root.back ? active().card(root.card) : nullptr;
      const auto *page = card ? active().page(card->target) : nullptr;
      if (!page || !page->navigable)
        continue;
      for (size_t shell = 0; shell < Slots; ++shell) {
        const auto result = match(renderer_.cell(card->target, shell));
        if (result.index != NO_CARD)
          return result;
      }
    }
    return {};
  }
  void cover_for_request() {
    covered_ = true;
    input_.cover(true);
  }
  bool request_page(PageId page) {
    const auto *definition = active().page(page);
    if (!navigation_ready() || !definition || !definition->navigable)
      return false;
    ui_request_ = {UiKind::PAGE, active().generation(), page, NO_CARD, 0};
    close_requested_ = true;
    cover_for_request();
    return true;
  }
  void process_ui_request() {
    if (ui_request_.kind == UiKind::NONE)
      return;
    if (ui_request_.configuration != active().generation()) {
      ui_request_ = {};
      return;
    }
    const auto memory = modals_.runtime().memory();
    if (suspended_ || memory.pending_requests || memory.active_contexts ||
        memory.retired_contexts)
      return;
    const auto request = ui_request_;
    ui_request_ = {};
    if (critical_active() ||
        (request.kind == UiKind::ACTIVATE &&
         (request.connection != connection_generation() || !remote_allowed() ||
          !platform_.input_allowed())))
      return;
    if (request.page != 0) {
      bool reachable = false;
      for (size_t shell = 0; shell < Slots; ++shell) {
        const auto cell = renderer_.cell(0, shell);
        const auto *owner =
            cell.visible && !cell.back ? active().card(cell.card) : nullptr;
        reachable |= owner && owner->target == request.page;
      }
      if (!reachable)
        return;
    }
    if (!renderer_.open_page(request.page))
      return;
    advance_chrome();
    origin_ = {};
    chrome_dirty_ = layout_dirty_ = true;
    if (request.kind != UiKind::ACTIVATE)
      return;
    for (size_t shell = 0; shell < Slots; ++shell) {
      const auto token = renderer_.binding(shell);
      if (token.binding && !token.back && token.card == request.card) {
        view_.activate(token);
        return;
      }
    }
  }
  static PageNativeScene &self(void *owner) {
    return *static_cast<PageNativeScene *>(owner);
  }
  static bool modal_input(void *owner) {
    // This gates new panel input. Replies to already accepted media requests
    // validate ownership/connection independently of temporary input guards.
    const auto &scene = self(owner);
    return scene.ready() && scene.platform_.grid_active() &&
           scene.platform_.input_allowed() && !scene.close_requested_ &&
           !scene.takeover_requested_ &&
           !scene.modals_.runtime().pending_request() &&
           !espcontrol::screensaver_pin().locked() &&
           !ha_read_coordinator().dispatching() &&
           !modal_callback_dispatching();
  }
  static bool grid_valid(void *owner, PageBindingToken t) {
    auto &scene = self(owner);
    return scene.ready() && !scene.changing_ &&
           scene.platform_.input_allowed() && scene.platform_.grid_active() &&
           !espcontrol::screensaver_pin().locked() &&
           !ha_read_coordinator().dispatching() &&
           !modal_callback_dispatching() && scene.input_.valid(t);
  }
  static bool local_valid(void *owner, PageLocalModalOrigin origin) {
    const auto &scene = self(owner);
    return scene.ready() && scene.platform_.grid_active() &&
           scene.platform_.input_allowed() &&
           !espcontrol::screensaver_pin().locked() && origin.binding &&
           origin.binding == scene.chrome_binding_ &&
           origin.configuration == scene.active().generation() &&
           origin.page == scene.active_page();
  }
  void advance_chrome() {
    if (chrome_binding_)
      ++chrome_binding_;
  }
  void reconcile_display() {
    const bool takeover =
        !platform_.grid_active() || espcontrol::screensaver_pin().locked();
    if (takeover == suspended_)
      return;
    advance_chrome();
    commands_.cancel();
    if (takeover) {
      input_.cover(true);
      modals_.runtime().display_takeover(true);
      renderer_.suspend();
      // Full display ownership must release dashboard surfaces, including a
      // parked expanded-image cache, before screensaver pipeline replacements.
      image_resources_.retire_active();
    } else
      renderer_.resume();
    suspended_ = takeover;
    chrome_dirty_ = layout_dirty_ = true;
  }
  void reconcile_cover() {
    const auto memory = modals_.runtime().memory();
    const bool cover = suspended_ || !platform_.input_allowed() ||
                       memory.active_contexts || memory.retired_contexts;
    if (covered_ != cover) {
      covered_ = cover;
      input_.cover(cover);
      chrome_dirty_ = true;
    }
  }
  void navigate(PageBindingToken t, PageId page, bool back) {
    if (!grid_valid(this, t))
      return;
    commands_.cancel();
    modals_.runtime().dismiss(PageModalClose::SWITCH);
    const bool changed = back ? renderer_.back() : renderer_.open_page(page);
    if (changed) {
      origin_ = {};
      advance_chrome();
      chrome_dirty_ = layout_dirty_ = true;
    }
  }
  void open(PageBindingToken t,
            ControlModalKind kind = ControlModalKind::NONE) {
    if (grid_valid(this, t) && modals_.runtime().request(t, kind))
      origin_ = {modals_.runtime().pending_request(), t};
  }
  void confirm(PageBindingToken t, uint32_t connection, bool currently_on) {
    if (grid_valid(this, t) &&
        modals_.confirmations().request(t, connection, currently_on))
      origin_ = {modals_.runtime().pending_request(), t};
  }
  bool feedback(PageModalToken token) const {
    return !stopped_ && origin_.opening == token.opening &&
           origin_.binding.configuration == token.configuration &&
           origin_.binding.card == token.card &&
           renderer_.bound(origin_.binding) && modals_.runtime().valid(token);
  }
  static void accepted(void *owner, PageBindingToken t,
                       PageCardCommandKind kind,
                       const PageCardCommandValue &v) {
    auto &s = self(owner);
    if (kind == PageCardCommandKind::MEDIA_SEEK)
      s.view_.preview_seek(t, {s.connection_generation(), v.write_revision},
                           v.integer, v.number);
    if (kind == PageCardCommandKind::DEVICE_ACTION)
      s.view_.preview_device(t, s.connection_generation(), v.write_revision);
  }
  PageBasicActions actions() {
    PageBasicActions a;
    a.owner = this;
    a.icon = [](void *o, std::string_view name, std::string_view domain) {
      return self(o).styles_.icon(name, domain);
    };
    a.valid = grid_valid;
    a.bound = [](void *o, PageBindingToken t) { return self(o).valid(t); };
    a.toggle = [](void *o, PageBindingToken t, std::string_view) {
      auto &s = self(o);
      s.commands_.request(t, s.connection_generation());
    };
    a.action = [](void *o, PageBindingToken t) {
      auto &s = self(o);
      s.commands_.request(t, s.connection_generation());
    };
    a.open = [](void *o, PageBindingToken t, PageId p) {
      self(o).navigate(t, p, false);
    };
    a.back = [](void *o, PageBindingToken t) {
      self(o).navigate(t, NO_PAGE, true);
    };
    a.climate = [](void *o, PageBindingToken t) {
      self(o).open(t, ControlModalKind::CLIMATE);
    };
    a.control = [](void *o, PageBindingToken t) { self(o).open(t); };
    a.forecast = [](void *o, PageBindingToken t) {
      self(o).open(t, ControlModalKind::WEATHER_FORECAST);
    };
    a.media_control = [](void *o, PageBindingToken t) {
      self(o).open(t, ControlModalKind::MEDIA_CONTROL);
    };
    a.media_volume = [](void *o, PageBindingToken t) {
      self(o).open(t, ControlModalKind::MEDIA_VOLUME);
    };
    a.image = [](void *o, PageBindingToken t) {
      self(o).open(t, ControlModalKind::IMAGE_CARD);
    };
    a.wifi_qr = [](void *o, PageBindingToken t) {
      self(o).open(t, ControlModalKind::WIFI_QR);
    };
    a.media = [](void *o, PageBindingToken t, uint32_t c) {
      self(o).commands_.request_media(t, c);
    };
    a.seek = [](void *o, PageBindingToken t, PageMediaSeekProof p,
                uint8_t percent, float duration) {
      self(o).commands_.request_seek(t, p, percent, duration);
    };
    a.percent_slider = [](void *o, PageBindingToken t, int percent) {
      auto &s = self(o);
      s.commands_.request_percent(t, s.connection_generation(), percent);
    };
    a.number_slider = [](void *o, PageBindingToken t, double v,
                         const number_slider::Metadata &m) {
      auto &s = self(o);
      s.commands_.request_number(t, s.connection_generation(), v, m);
    };
    a.light_temperature = [](void *o, PageBindingToken t, int k) {
      auto &s = self(o);
      s.commands_.request_temperature(t, s.connection_generation(), k);
    };
    a.timer = [](void *o, PageBindingToken t, PageTimerAction action,
                 PageTimerCommandProof p) {
      self(o).commands_.request_timer(t, action, p);
    };
    a.device = [](void *o, PageBindingToken t, uint32_t c) {
      self(o).commands_.request_device(t, c);
    };
    a.slider_toggle = [](void *o, PageBindingToken t, uint32_t c) {
      self(o).commands_.request_slider_toggle(t, c);
    };
    a.confirmation = [](void *o, PageBindingToken t, uint32_t c, bool on) {
      self(o).confirm(t, c, on);
    };
    a.screen_locked = [](void *) {
      return espcontrol::screensaver_pin().locked();
    };
    a.screen_lock = [](void *o, PageBindingToken t, bool) {
      auto &s = self(o);
      s.commands_.request(t, s.connection_generation());
    };
    a.alarm = [](void *o, PageBindingToken t) {
      auto &s = self(o);
      if (!grid_valid(o, t))
        return;
      const auto *card = s.active().card({t.configuration, t.card});
      if (!card)
        return;
      const auto action =
          alarm::action(s.active().field(*card, CardField::SENSOR));
      if (PageAlarmStateRuntime::classify(s.active(), t.card) ==
              PageAlarmSourceKind::ACTION &&
          admission::option_equals_ascii(
              PageAlarmStateRuntime::effective_options(s.active(), t.card),
              action == alarm::Action::DISARM ? "pin_disarm" : "pin_arm", '0'))
        s.modals_.alarm_commands().queue({t, {}, s.connection_generation()},
                                         action);
      else
        s.open(t);
    };
    return a;
  }
  PageId source_page(PageModalToken token) const {
    const auto &store = active();
    if (token.configuration != store.generation() ||
        !store.card({token.configuration, token.card}))
      return NO_PAGE;
    for (PageId p = 0; p < store.page_count(); ++p) {
      const auto *page = store.page(p);
      if (page->navigable && token.card >= page->first_card &&
          token.card - page->first_card < page->card_count)
        return p;
    }
    return NO_PAGE;
  }
  template <class Environment>
  bool resolve(PageModalToken token, const ParsedCfg &config,
               Environment &environment) {
    const auto page = source_page(token);
    return page != NO_PAGE &&
           styles_.resolve(active(), page, token, config, environment);
  }
  PageNativeModalStyles modal_styles() {
    PageNativeModalStyles s;
#define PAGE_NATIVE_RESOLVER(Field, Environment)                               \
  s.Field.owner = this;                                                        \
  s.Field.resolve = [](void *o, PageModalToken t, const ParsedCfg &c,          \
                       Environment &e) { return self(o).resolve(t, c, e); }
    PAGE_NATIVE_RESOLVER(climate,
                         espcontrol::cards::ClimateControlDriverEnvironment);
    PAGE_NATIVE_RESOLVER(light, PageControlModalEnvironment);
    PAGE_NATIVE_RESOLVER(cover, PageControlModalEnvironment);
    PAGE_NATIVE_RESOLVER(fan, PageFanControlModalEnvironment);
    PAGE_NATIVE_RESOLVER(fan_preset, PageFanControlModalEnvironment);
    PAGE_NATIVE_RESOLVER(options, PageOptionSelectModalEnvironment);
    PAGE_NATIVE_RESOLVER(volume, PageMediaVolumeModalEnvironment);
    PAGE_NATIVE_RESOLVER(weather, PageWeatherModalStyle);
    PAGE_NATIVE_RESOLVER(confirmation, PageConfirmationModalEnvironment);
    PAGE_NATIVE_RESOLVER(alarm, PageAlarmModalEnvironment);
    PAGE_NATIVE_RESOLVER(image, PageExpandedImageModalEnvironment);
    PAGE_NATIVE_RESOLVER(wifi, PageWifiQrModalEnvironment);
#undef PAGE_NATIVE_RESOLVER
    s.media.owner = this;
    s.media.resolve = [](void *o, PageModalToken t, const ParsedCfg &c,
                         PageMediaControlModalEnvironment &e) {
      auto &scene = self(o);
      if (!scene.resolve(t, c, e))
        return false;
      e.media = &scene.state_.media;
      return true;
    };
    s.volume.numeric_action_sent = [](void *o, PageModalToken t, float value) {
      auto &scene = self(o);
      if (scene.feedback(t) && std::isfinite(value) && value >= 0 &&
          value <= 100)
        scene.view_.preview_volume(scene.origin_.binding,
                                   static_cast<uint8_t>(std::lround(value)));
    };
    s.media.numeric_action_sent = s.volume.numeric_action_sent;
    s.media.state_confirmed = [](void *o, PageModalToken t, bool value) {
      auto &scene = self(o);
      if (scene.feedback(t))
        scene.view_.preview_group(scene.origin_.binding, value);
    };
    s.image.covered = [](void *o, bool covered) {
      auto &scene = self(o);
      if (covered) {
        scene.covered_ = true;
        scene.input_.cover(true);
      }
    };
    s.network.owner = this;
    s.network.resolve = [](void *o, PageLocalModalOrigin origin,
                           PageModalToken t, PageNetworkStatusEnvironment &e) {
      auto &scene = self(o);
      const auto *page = scene.active().page(origin.page);
      return origin.configuration == scene.active().generation() &&
             t.configuration == origin.configuration && page &&
             page->navigable &&
             scene.styles_.resolve(scene.active(), origin, t, e);
    };
    s.network.settings = [](void *o, bool active) {
      auto &scene = self(o);
      scene.settings_ = active;
      scene.chrome_dirty_ = true;
    };
    return s;
  }

  const DeviceUiLimits &limits_;
  PageNativeScenePlatform &platform_;
  PageNativeSceneStyles &styles_;
  PageNativeSceneState state_;
  ModalOrigin modal_origin_;
  TileEnvironment tile_environment_;
  ExpandedEnvironment expanded_environment_;
  PageArtworkRequests<ImageSlots> requests_;
  PageArtworkRequests<1> modal_requests_;
  PageImageRuntime<ImageSlots> images_;
  PageModalImageRuntime modal_images_;
  PageCoverArtRuntime<ImageSlots> cover_;
  PageImageTileRuntime<ImageSlots> tiles_;
  PageExpandedImageSource<ImageSlots> expanded_;
  PageArtworkCoverView<ImageSlots> cover_view_;
  PageArtworkImageTileView<ImageSlots> tile_view_;
  PageArtworkExpandedImageView<ImageSlots> expanded_view_;
  PageLvglQrEncoder qr_encoder_;
  PageLvglQrFrames qr_frames_;
  PageQrRuntime<> qr_;
  PagePooledQrTileView qr_view_;
  PageSliderCommandPolicy slider_policy_;
  PageTimerCommandPolicy timer_policy_;
  PageMediaCommandPolicy media_policy_;
  PageDeviceCommandPolicy device_policy_;
  PageCardControlRouter<9> controls_;
  PageNativeCardCommandPort command_port_;
  PageCardCommands commands_;
  PageNativeModalBank modals_;
  AlarmPort alarm_port_;
  PageAlarmPresentation alarm_;
  PageBasicTheme theme_;
  PageBasicLvglPort<Slots> view_;
  PageRendererRuntime<Slots> renderer_;
  PageGridInput<Slots> input_;
  Images image_resources_;
  PageRuntimeResourcesGroup<6> resources_;
  PageStoreRuntime store_;
  struct {
    uint64_t opening{0};
    PageBindingToken binding;
  } origin_;
  uint64_t chrome_binding_{1};
  bool configured_{false}, initialized_{false}, running_{false},
      changing_{false}, stopped_{false}, destroying_{false}, suspended_{false},
      covered_{false}, settings_{false}, chrome_dirty_{true},
      layout_dirty_{true};
};
} // namespace espcontrol::pages
