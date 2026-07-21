#pragma once

// Internal card-background runtime. GridConfig and image-card helpers must be
// defined before this header is included by button_grid_grid.h.

#include "card_background_controller.h"
#include <cassert>

constexpr int CARD_BACKGROUND_IMAGE_MAX_CONTEXTS = MAX_GRID_SLOTS;
constexpr int CARD_BACKGROUND_IMAGE_MAX_BINDINGS = MAX_GRID_SLOTS;
constexpr uint8_t CARD_BACKGROUND_IMAGE_MAX_RETRIES = 3;
constexpr uint32_t CARD_BACKGROUND_IMAGE_RETRY_DELAY_MS = 750;

struct CardBackgroundImageCtx {
  esphome::artwork_image::ArtworkImage *image = nullptr;
  std::string url;
  bool callbacks_bound = false;
  struct Binding {
    lv_obj_t *btn = nullptr;
    lv_obj_t *widget = nullptr;
    bool active = false;
  } bindings[CARD_BACKGROUND_IMAGE_MAX_BINDINGS];
};

struct CardBackgroundWidgetRef {
  lv_obj_t *btn = nullptr;
  lv_obj_t *widget = nullptr;
  std::string id;
};

using CardBackgroundController =
  espcontrol::card_background::Controller<CARD_BACKGROUND_IMAGE_MAX_CONTEXTS>;

struct CardBackgroundRuntimeState {
  CardBackgroundImageCtx contexts[CARD_BACKGROUND_IMAGE_MAX_CONTEXTS];
  CardBackgroundController controller;
  std::vector<CardBackgroundWidgetRef> widget_refs;
  lv_obj_t *active_page = nullptr;
  lv_timer_t *final_refresh_timer = nullptr;
  lv_timer_t *cache_write_timer = nullptr;
  lv_timer_t *next_download_timer = nullptr;
};

inline void card_background_runtime_shutdown(CardBackgroundRuntimeState &runtime) {
  if (runtime.final_refresh_timer) lv_timer_del(runtime.final_refresh_timer);
  if (runtime.cache_write_timer) lv_timer_del(runtime.cache_write_timer);
  if (runtime.next_download_timer) lv_timer_del(runtime.next_download_timer);
  runtime.final_refresh_timer = nullptr;
  runtime.cache_write_timer = nullptr;
  runtime.next_download_timer = nullptr;
  for (auto &context : runtime.contexts) {
    if (context.image) context.image->release();
    context.image = nullptr;
  }
}

inline CardBackgroundRuntimeState &card_background_runtime() {
  auto *assets = espcontrol::card_asset_service();
  assert(assets != nullptr);
  return assets->ensure_card_background_runtime<CardBackgroundRuntimeState>(
      card_background_runtime_shutdown);
}

inline CardBackgroundImageCtx *card_background_image_contexts() {
  return card_background_runtime().contexts;
}

inline CardBackgroundController &card_background_controller() {
  return card_background_runtime().controller;
}

inline int card_background_context_index(const CardBackgroundImageCtx *ctx) {
  if (!ctx) return CardBackgroundController::NO_RESOURCE;
  return static_cast<int>(ctx - card_background_image_contexts());
}

inline espcontrol::card_background::ResourceState &card_background_state(
    CardBackgroundImageCtx *ctx) {
  return card_background_controller().resource(
    static_cast<size_t>(card_background_context_index(ctx)));
}

inline const espcontrol::card_background::ResourceState &card_background_state(
    const CardBackgroundImageCtx *ctx) {
  return card_background_controller().resource(
    static_cast<size_t>(card_background_context_index(ctx)));
}

inline CardBackgroundImageCtx *card_background_context_at(int index) {
  if (index < 0 || index >= CARD_BACKGROUND_IMAGE_MAX_CONTEXTS) return nullptr;
  return &card_background_image_contexts()[index];
}

inline std::vector<CardBackgroundWidgetRef> &card_background_widget_refs() {
  return card_background_runtime().widget_refs;
}

inline lv_obj_t *&card_background_active_page() {
  return card_background_runtime().active_page;
}

inline lv_timer_t *&card_background_final_refresh_timer() {
  return card_background_runtime().final_refresh_timer;
}

inline lv_timer_t *&card_background_cache_write_timer() {
  return card_background_runtime().cache_write_timer;
}

inline lv_timer_t *&card_background_next_download_timer() {
  return card_background_runtime().next_download_timer;
}

inline bool card_background_widget_on_page(lv_obj_t *btn, lv_obj_t *page);

inline void card_background_sync_label_shadow_text(lv_obj_t *label,
                                                   const std::string &text) {
  if (!label) return;
  for (lv_obj_t *parent = lv_obj_get_parent(label); parent; parent = lv_obj_get_parent(parent)) {
    lv_obj_t *shadow = image_card_label_shadow(label, parent);
    if (!shadow) continue;
    lv_label_set_long_mode(shadow, LV_LABEL_LONG_WRAP);
    lv_label_set_text(shadow, text.c_str());
    return;
  }
}

inline void card_background_move_content_foreground(const BtnSlot &s) {
  button_label_text_sync_hook() = card_background_sync_label_shadow_text;
  auto sync_shadow = [](lv_obj_t *target, lv_obj_t *btn, lv_coord_t x_offset, lv_coord_t y_offset) {
    if (!target || !btn || lv_obj_has_flag(target, LV_OBJ_FLAG_HIDDEN)) return;
    lv_obj_update_layout(btn);
    lv_obj_update_layout(target);
    lv_obj_t *shadow = image_card_label_shadow(target, btn);
    if (!shadow) {
      shadow = lv_label_create(btn);
      lv_obj_set_user_data(shadow, target);
    }
    lv_obj_add_flag(shadow, CARD_TEXT_COLOR_PROTECTED_FLAG);
    const lv_font_t *font = lv_obj_get_style_text_font(target, LV_PART_MAIN);
    if (font) lv_obj_set_style_text_font(shadow, font, LV_PART_MAIN);
    lv_obj_set_style_text_color(shadow, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_text_opa(shadow, LV_OPA_80, LV_PART_MAIN);
    lv_obj_set_style_text_align(shadow, lv_obj_get_style_text_align(target, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(shadow, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_radius(shadow, 0, LV_PART_MAIN);
    lv_label_set_long_mode(shadow, lv_label_get_long_mode(target));
    const char *text = lv_label_get_text(target);
    lv_label_set_text(shadow, text ? text : "");
    lv_obj_set_size(shadow, lv_obj_get_width(target), lv_obj_get_height(target));
    lv_obj_set_style_pad_left(shadow, lv_obj_get_style_pad_left(target, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_right(shadow, lv_obj_get_style_pad_right(target, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_top(shadow, lv_obj_get_style_pad_top(target, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(shadow, lv_obj_get_style_pad_bottom(target, LV_PART_MAIN), LV_PART_MAIN);
    lv_obj_align_to(shadow, target, LV_ALIGN_TOP_LEFT, x_offset, y_offset);
    lv_obj_clear_flag(shadow, LV_OBJ_FLAG_HIDDEN);
    lv_obj_move_foreground(shadow);
  };

  // State-driven cards replace their icon glyph directly, bypassing the label
  // text hook. Avoid a second icon label that could retain the old glyph.
  image_card_delete_label_shadow(s.icon_lbl, s.btn);
  sync_shadow(s.text_lbl, s.btn, 1, 1);
  sync_shadow(s.subpage_lbl, s.btn, 1, 1);
  if (s.sensor_container) lv_obj_move_foreground(s.sensor_container);
  if (s.icon_lbl) lv_obj_move_foreground(s.icon_lbl);
  if (s.text_lbl) lv_obj_move_foreground(s.text_lbl);
  if (s.subpage_lbl) lv_obj_move_foreground(s.subpage_lbl);
}

inline bool card_background_configured_for_card(const ParsedCfg &p) {
  std::string id = cfg_option_value(p.options, CARD_BACKGROUND_IMAGE_OPTION);
  return card_background_supported_type(p.type) && card_background_image_id_valid(id);
}

inline void card_background_position_widget(lv_obj_t *btn, lv_obj_t *widget) {
  lv_coord_t width = 0;
  lv_coord_t height = 0;
  if (!image_card_position_widget(btn, widget, &width, &height)) return;
  (void) width;
  (void) height;
  image_card_apply_tile_image_align(widget);
}

inline CardBackgroundImageCtx *card_background_active_download_context() {
  return card_background_context_at(card_background_controller().active_download());
}

inline void card_background_set_widget_source_hidden(
    lv_obj_t *widget, esphome::artwork_image::ArtworkImage *image) {
  image_card_set_widget_source(widget, image);
  // The shared image helper reveals its widget. Keep it hidden until its
  // decoded image is ready and the page containing it is active.
  if (widget) lv_obj_add_flag(widget, LV_OBJ_FLAG_HIDDEN);
}

inline void card_background_release_download_slot(CardBackgroundImageCtx *ctx);
inline bool card_background_reveal_ready_widgets();
inline void card_background_handle_download_error(CardBackgroundImageCtx *ctx);
inline void card_background_schedule_cache_writes();
inline void card_background_schedule_next_download(CardBackgroundImageCtx *finished_ctx);

inline void card_background_request_download(CardBackgroundImageCtx *ctx) {
  if (!ctx || !ctx->image || ctx->url.empty()) return;
  auto &state = card_background_state(ctx);
  auto action = card_background_controller().request(
    static_cast<size_t>(card_background_context_index(ctx)));
  if (action != espcontrol::card_background::RequestAction::START) return;
  int width = ctx->image->get_fixed_width();
  int height = ctx->image->get_fixed_height();
  if (width <= 0 || height <= 0) {
    card_background_release_download_slot(ctx);
    ESP_LOGD("card_background", "Waiting for card size before downloading background image: %s", state.id.c_str());
    return;
  }
  ESP_LOGI("card_background", "Decoding card background image: %s (%dx%d)", state.id.c_str(), width, height);
  auto *assets = espcontrol::card_asset_service();
  if (assets == nullptr) {
    card_background_handle_download_error(ctx);
    return;
  }
  esphome::card_image_store::CardImageInfo info;
  if (!assets->find(state.id, info)) {
    card_background_handle_download_error(ctx);
    return;
  }
  state.source_crc32 = info.crc32;
  size_t cached_size = static_cast<size_t>(width) * height * 2;
  bool cache_started = ctx->image->request_update_rgb565_frame(
      ctx->url, width, height, [assets, ctx, width, height](uint8_t *buffer, size_t size) {
        const auto &current = card_background_state(ctx);
        return assets->read_rgb565_cache(current.id, current.source_crc32,
                                         static_cast<uint16_t>(width), static_cast<uint16_t>(height),
                                         buffer, size);
      });
  if (cache_started) {
    ESP_LOGI("card_background", "Loaded device-sized card background cache: %s (%zu bytes)",
             state.id.c_str(), cached_size);
    return;
  }
  auto reader = store.open(state.id);
  if (!reader || !ctx->image->request_update_container(reader, ctx->url)) {
    card_background_handle_download_error(ctx);
    return;
  }
}

inline bool card_background_start_next_queued_download(CardBackgroundImageCtx *finished_ctx) {
  int next = card_background_controller().next_queued(
    card_background_context_index(finished_ctx));
  if (next == CardBackgroundController::NO_RESOURCE) return false;
  card_background_request_download(card_background_context_at(next));
  return true;
}

inline void card_background_schedule_next_download(CardBackgroundImageCtx *finished_ctx) {
  lv_timer_t *&timer = card_background_next_download_timer();
  if (timer) return;
  timer = lv_timer_create([](lv_timer_t *next_timer) {
    auto *finished = static_cast<CardBackgroundImageCtx *>(lv_timer_get_user_data(next_timer));
    lv_timer_del(next_timer);
    card_background_next_download_timer() = nullptr;
    if (card_background_active_download_context() != nullptr) return;
    if (!card_background_start_next_queued_download(finished)) card_background_schedule_cache_writes();
  }, 40, finished_ctx);
}

inline void card_background_release_download_slot(CardBackgroundImageCtx *ctx) {
  if (!ctx) return;
  bool was_active = card_background_active_download_context() == ctx;
  card_background_controller().release_download(
    static_cast<size_t>(card_background_context_index(ctx)));
  if (was_active) card_background_schedule_next_download(ctx);
}

inline void card_background_apply_downloaded(CardBackgroundImageCtx *ctx, bool cached) {
  if (!ctx || !ctx->image) return;
  auto &state = card_background_state(ctx);
  if (!state.active) return;
  if (ctx->image->get_url() != ctx->url) {
    card_background_release_download_slot(ctx);
    return;
  }
  card_background_controller().mark_ready(
    static_cast<size_t>(card_background_context_index(ctx)), cached);
  ESP_LOGI("card_background", "Applied card background image: %s", state.id.c_str());
  for (auto &binding : ctx->bindings) {
    if (!binding.active || !binding.widget) continue;
    card_background_position_widget(binding.btn, binding.widget);
    card_background_set_widget_source_hidden(binding.widget, ctx->image);
    lv_obj_move_background(binding.widget);
    if (binding.btn) lv_obj_invalidate(binding.btn);
  }
  card_background_reveal_ready_widgets();
  notify_dashboard_content_changed();
  // Release the decoder through a short asynchronous handoff. This gives LVGL
  // a frame to paint the completed card without blocking on a forced refresh.
  card_background_release_download_slot(ctx);
}

inline void card_background_schedule_cache_writes() {
  lv_timer_t *&timer = card_background_cache_write_timer();
  if (timer) return;
  timer = lv_timer_create([](lv_timer_t *cache_timer) {
    CardBackgroundImageCtx *contexts = card_background_image_contexts();
    for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
      CardBackgroundImageCtx *ctx = &contexts[i];
      auto &state = card_background_state(ctx);
      if (!state.active || !state.cache_write_pending || !ctx->image ||
          state.width <= 0 || state.height <= 0) {
        continue;
      }
      card_background_controller().mark_cache_written(static_cast<size_t>(i));
      const uint8_t *data = ctx->image->get_buffer_data();
      size_t size = ctx->image->get_active_buffer_size();
      if (data != nullptr && size == static_cast<size_t>(state.width) * state.height * 2) {
        auto *assets = espcontrol::card_asset_service();
        if (assets == nullptr) return;
        auto err = assets->write_rgb565_cache(
            state.id, state.source_crc32, static_cast<uint16_t>(state.width),
            static_cast<uint16_t>(state.height), data, size);
        if (err != ESP_OK) {
          ESP_LOGW("card_background", "Could not cache card background %s: %s",
                   state.id.c_str(), esp_err_to_name(err));
        }
      }
      return;
    }
    lv_timer_del(cache_timer);
    card_background_cache_write_timer() = nullptr;
  }, 75, nullptr);
}

inline bool card_background_reveal_ready_widgets() {
  lv_obj_t *page = card_background_active_page();
  if (!page) return false;
  if (lv_scr_act() != page) {
    lv_timer_t *&refresh_timer = card_background_final_refresh_timer();
    if (!refresh_timer) {
      refresh_timer = lv_timer_create([](lv_timer_t *timer) {
        lv_timer_del(timer);
        card_background_final_refresh_timer() = nullptr;
        card_background_reveal_ready_widgets();
      }, 50, nullptr);
    }
    return false;
  }

  lv_timer_t *&refresh_timer = card_background_final_refresh_timer();
  if (refresh_timer) {
    lv_timer_del(refresh_timer);
    refresh_timer = nullptr;
  }

  bool revealed = false;
  for (const auto &ref : card_background_widget_refs()) {
    if (!card_background_widget_on_page(ref.btn, page) || !ref.widget) continue;
    bool ready = false;
    for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS && !ready; i++) {
      CardBackgroundImageCtx *candidate = &card_background_image_contexts()[i];
      const auto &state = card_background_state(candidate);
      if (!state.active || !state.ready) continue;
      for (const auto &binding : candidate->bindings) {
        if (binding.active && binding.widget == ref.widget) {
          ready = true;
          break;
        }
      }
    }
    if (ready && lv_obj_has_flag(ref.widget, LV_OBJ_FLAG_HIDDEN)) {
      lv_obj_clear_flag(ref.widget, LV_OBJ_FLAG_HIDDEN);
      if (ref.btn) lv_obj_invalidate(ref.btn);
      revealed = true;
    }
  }
  return revealed;
}

inline void card_background_handle_download_error(CardBackgroundImageCtx *ctx) {
  if (!ctx) return;
  auto &state = card_background_state(ctx);
  if (!state.active) return;
  card_background_release_download_slot(ctx);
  bool retry = card_background_controller().mark_failed(
    static_cast<size_t>(card_background_context_index(ctx)), esphome::millis(),
    CARD_BACKGROUND_IMAGE_MAX_RETRIES, CARD_BACKGROUND_IMAGE_RETRY_DELAY_MS);
  if (retry) {
    ESP_LOGW("card_background", "Card background image download failed: %s; retry %u/%u scheduled",
             state.id.c_str(), state.retry_count, CARD_BACKGROUND_IMAGE_MAX_RETRIES);
  } else {
    ESP_LOGW("card_background", "Card background image download failed after %u retries: %s",
             CARD_BACKGROUND_IMAGE_MAX_RETRIES, state.id.c_str());
  }
  for (auto &binding : ctx->bindings) {
    if (!binding.active) continue;
    if (binding.widget) lv_obj_add_flag(binding.widget, LV_OBJ_FLAG_HIDDEN);
  }
  card_background_reveal_ready_widgets();
}

inline void card_background_bind_callbacks(CardBackgroundImageCtx *ctx) {
  if (!ctx || !ctx->image || ctx->callbacks_bound) return;
  ctx->callbacks_bound = true;
  ctx->image->add_on_finished_callback([ctx](bool cached) {
    card_background_apply_downloaded(ctx, cached);
  });
  ctx->image->add_on_error_callback([ctx]() {
    card_background_handle_download_error(ctx);
  });
}

inline std::string card_background_image_url(const std::string &id);

inline void card_background_configure_target_size(CardBackgroundImageCtx *ctx,
                                                  int width, int height) {
  if (!ctx || !ctx->image) return;
  auto &state = card_background_state(ctx);
  state.width = width;
  state.height = height;
  ctx->image->set_target_size(width, height);
  ctx->image->set_resize_mode(esphome::artwork_image::ImageResizeMode::COVER);
}

inline bool card_background_context_has_active_bindings(CardBackgroundImageCtx *ctx) {
  return ctx && card_background_controller().has_bindings(
    static_cast<size_t>(card_background_context_index(ctx)));
}

inline void card_background_release_contexts(const GridConfig &cfg) {
  lv_timer_t *&refresh_timer = card_background_final_refresh_timer();
  if (refresh_timer) {
    lv_timer_del(refresh_timer);
    refresh_timer = nullptr;
  }
  lv_timer_t *&cache_timer = card_background_cache_write_timer();
  if (cache_timer) {
    lv_timer_del(cache_timer);
    cache_timer = nullptr;
  }
  lv_timer_t *&next_timer = card_background_next_download_timer();
  if (next_timer) {
    lv_timer_del(next_timer);
    next_timer = nullptr;
  }
  CardBackgroundImageCtx *contexts = card_background_image_contexts();
  int count = cfg.card_background_image_count;
  if (count > CARD_BACKGROUND_IMAGE_MAX_CONTEXTS) count = CARD_BACKGROUND_IMAGE_MAX_CONTEXTS;
  for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
    for (auto &binding : contexts[i].bindings) {
      image_card_clear_widget_source(binding.widget);
      binding.active = false;
      binding.btn = nullptr;
      binding.widget = nullptr;
      card_background_controller().remove_binding(static_cast<size_t>(i));
    }
    contexts[i].url.clear();
    contexts[i].image = i < count && cfg.card_background_images ? cfg.card_background_images[i] : nullptr;
    if (contexts[i].image) contexts[i].image->release();
  }
  card_background_controller().reset(static_cast<size_t>(count));
}

inline void reset_card_background_image_pool(const GridConfig &cfg) {
  card_background_release_contexts(cfg);
  card_background_active_page() = nullptr;
  card_background_widget_refs().clear();
}

inline CardBackgroundImageCtx *acquire_card_background_image_context(const GridConfig &cfg,
                                                                     const std::string &id,
                                                                     int target_width,
                                                                     int target_height);

inline CardBackgroundImageCtx::Binding *card_background_add_binding(CardBackgroundImageCtx *ctx,
                                                                    lv_obj_t *btn,
                                                                    lv_obj_t *widget);

inline void card_background_sync_binding_image(CardBackgroundImageCtx *ctx,
                                               CardBackgroundImageCtx::Binding *binding) {
  if (!ctx || !ctx->image || !binding || !binding->widget) return;
  const auto &state = card_background_state(ctx);
  if (state.width <= 0 || state.height <= 0) return;
  if (state.ready && ctx->image->get_url() == ctx->url) {
    card_background_set_widget_source_hidden(binding->widget, ctx->image);
    card_background_reveal_ready_widgets();
  } else {
    card_background_request_download(ctx);
  }
}

inline void card_background_deactivate_if_unused(CardBackgroundImageCtx *ctx) {
  if (!ctx || card_background_context_has_active_bindings(ctx)) return;
  card_background_release_download_slot(ctx);
  ctx->url.clear();
  card_background_controller().deactivate(
    static_cast<size_t>(card_background_context_index(ctx)));
  if (ctx->image) ctx->image->release();
}

inline void card_background_refresh_due() {
  CardBackgroundImageCtx *contexts = card_background_image_contexts();
  uint32_t now = esphome::millis();
  for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
    CardBackgroundImageCtx *ctx = &contexts[i];
    if (!card_background_controller().retry_due(static_cast<size_t>(i), now)) continue;
    card_background_controller().consume_retry(static_cast<size_t>(i));
    card_background_request_download(ctx);
  }
}

inline bool card_background_move_binding_to_size(CardBackgroundImageCtx *ctx,
                                                 CardBackgroundImageCtx::Binding &binding,
                                                 const GridConfig &cfg,
                                                 int width, int height) {
  if (!ctx || !binding.active || !binding.btn || !binding.widget) return false;
  const auto &state = card_background_state(ctx);
  CardBackgroundImageCtx *target = acquire_card_background_image_context(
    cfg, state.id, width, height);
  if (!target || target == ctx) return false;

  CardBackgroundImageCtx::Binding *new_binding = card_background_add_binding(
    target, binding.btn, binding.widget);
  if (!new_binding) return false;

  binding.active = false;
  binding.btn = nullptr;
  binding.widget = nullptr;
  card_background_controller().remove_binding(
    static_cast<size_t>(card_background_context_index(ctx)));
  card_background_deactivate_if_unused(ctx);
  card_background_sync_binding_image(target, new_binding);
  return true;
}

inline void refresh_card_background_image(lv_obj_t *btn, const GridConfig &cfg) {
  if (!btn) return;
  CardBackgroundImageCtx *contexts = card_background_image_contexts();
  for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
    CardBackgroundImageCtx *ctx = &contexts[i];
    auto &state = card_background_state(ctx);
    if (!state.active || !ctx->image) continue;
    for (auto &binding : ctx->bindings) {
      if (!binding.active || binding.btn != btn || !binding.widget) continue;
      int previous_width = ctx->image->get_fixed_width();
      int previous_height = ctx->image->get_fixed_height();
      card_background_position_widget(binding.btn, binding.widget);
      int width = lv_obj_get_width(btn);
      int height = lv_obj_get_height(btn);
      if (width <= 0 || height <= 0) return;
      if (state.width != width || state.height != height) {
        if (card_background_move_binding_to_size(ctx, binding, cfg, width, height)) {
          refresh_card_background_image(btn, cfg);
          return;
        }
        return;
      }
      ctx->image->set_target_size(width, height);
      if ((previous_width != width || previous_height != height) && !ctx->url.empty()) {
        ctx->image->release();
        state.ready = false;
        ctx->image->set_target_size(width, height);
        card_background_request_download(ctx);
      }
      lv_obj_move_background(binding.widget);
    }
  }
}

inline CardBackgroundImageCtx *acquire_card_background_image_context(const GridConfig &cfg,
                                                                     const std::string &id,
                                                                     int target_width,
                                                                     int target_height) {
  CardBackgroundImageCtx *contexts = card_background_image_contexts();
  int count = cfg.card_background_image_count;
  if (count > CARD_BACKGROUND_IMAGE_MAX_CONTEXTS) count = CARD_BACKGROUND_IMAGE_MAX_CONTEXTS;
  if (card_background_controller().capacity() != static_cast<size_t>(count)) {
    card_background_controller().reset(static_cast<size_t>(count));
  }
  int index = card_background_controller().acquire(id, target_width, target_height);
  if (index == CardBackgroundController::NO_RESOURCE || !contexts[index].image) return nullptr;
  CardBackgroundImageCtx *ctx = &contexts[index];
  ctx->url = card_background_image_url(id);
  card_background_configure_target_size(ctx, target_width, target_height);
  card_background_bind_callbacks(ctx);
  return ctx;
}

inline CardBackgroundImageCtx::Binding *card_background_add_binding(CardBackgroundImageCtx *ctx,
                                                                    lv_obj_t *btn,
                                                                    lv_obj_t *widget) {
  if (!ctx) return nullptr;
  for (auto &binding : ctx->bindings) {
    if (binding.active && binding.btn == btn && binding.widget == widget) {
      return &binding;
    }
  }
  for (auto &binding : ctx->bindings) {
    if (!binding.active) {
      binding.active = true;
      binding.btn = btn;
      binding.widget = widget;
      card_background_controller().add_binding(
        static_cast<size_t>(card_background_context_index(ctx)));
      return &binding;
    }
  }
  return nullptr;
}

inline std::string card_background_image_url(const std::string &id) {
  return "card-image:" + id;
}

inline void card_background_register_widget(lv_obj_t *btn, lv_obj_t *widget,
                                            const std::string &id) {
  if (!btn || !widget || id.empty()) return;
  for (auto &ref : card_background_widget_refs()) {
    if (ref.widget == widget) {
      ref.btn = btn;
      ref.id = id;
      return;
    }
  }
  card_background_widget_refs().push_back({btn, widget, id});
}

inline void card_background_unregister_widget(lv_obj_t *widget) {
  if (!widget) return;
  auto &refs = card_background_widget_refs();
  refs.erase(
    std::remove_if(refs.begin(), refs.end(),
                   [widget](const CardBackgroundWidgetRef &ref) { return ref.widget == widget; }),
    refs.end());
}

inline bool card_background_widget_on_page(lv_obj_t *btn, lv_obj_t *page) {
  if (!btn || !page) return false;
  lv_obj_t *obj = btn;
  while (obj) {
    if (obj == page) return true;
    obj = lv_obj_get_parent(obj);
  }
  return false;
}

inline void card_background_unregister_page(lv_obj_t *page) {
  if (!page) return;
  if (card_background_active_page() == page) {
    card_background_active_page() = nullptr;
    lv_timer_t *&refresh_timer = card_background_final_refresh_timer();
    if (refresh_timer) {
      lv_timer_del(refresh_timer);
      refresh_timer = nullptr;
    }
  }
  CardBackgroundImageCtx *contexts = card_background_image_contexts();
  for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
    CardBackgroundImageCtx *ctx = &contexts[i];
    for (auto &binding : ctx->bindings) {
      if (!binding.active || !card_background_widget_on_page(binding.btn, page)) continue;
      image_card_clear_widget_source(binding.widget);
      binding.active = false;
      binding.btn = nullptr;
      binding.widget = nullptr;
      card_background_controller().remove_binding(static_cast<size_t>(i));
    }
    card_background_deactivate_if_unused(ctx);
  }
  auto &refs = card_background_widget_refs();
  refs.erase(
    std::remove_if(
      refs.begin(), refs.end(),
      [page](const CardBackgroundWidgetRef &ref) {
        return card_background_widget_on_page(ref.btn, page);
      }),
    refs.end());
}

inline void card_background_activate_page(const GridConfig &cfg, lv_obj_t *page) {
  if (!page) return;
  int total_refs = 0;
  int matched_refs = 0;
  int activated_refs = 0;
  for (const auto &ref : card_background_widget_refs()) {
    if (card_background_widget_on_page(ref.btn, page)) matched_refs++;
  }
  if (matched_refs == 0) {
    if (card_background_active_page() != page) {
      card_background_release_contexts(cfg);
      card_background_active_page() = page;
    }
    ESP_LOGD("card_background", "Tracking page=%p with no background image refs yet", page);
    return;
  }

  const bool same_page = card_background_active_page() == page;
  if (!same_page) {
    card_background_release_contexts(cfg);
    card_background_active_page() = page;
  }
  matched_refs = 0;
  for (auto &ref : card_background_widget_refs()) {
    total_refs++;
    if (!same_page) image_card_clear_widget_source(ref.widget);
    if (!card_background_widget_on_page(ref.btn, page)) continue;
    matched_refs++;
    lv_obj_update_layout(ref.btn);
    int target_width = lv_obj_get_width(ref.btn);
    int target_height = lv_obj_get_height(ref.btn);
    ESP_LOGD("card_background", "Activating ref page=%p btn=%p id=%s size=%dx%d",
             page, ref.btn, ref.id.c_str(), target_width, target_height);
    if (target_width <= 0 || target_height <= 0) {
      ESP_LOGD("card_background", "Waiting for card layout before activating background image: %s", ref.id.c_str());
      continue;
    }
    CardBackgroundImageCtx *ctx = acquire_card_background_image_context(
      cfg, ref.id, target_width, target_height);
    if (!ctx) {
      ESP_LOGW("card_background", "No downloader available for visible card background image %s", ref.id.c_str());
      continue;
    }
    CardBackgroundImageCtx::Binding *binding = card_background_add_binding(ctx, ref.btn, ref.widget);
    if (!binding) {
      ESP_LOGW("card_background", "No card binding available for visible card background image %s", ref.id.c_str());
      continue;
    }
    card_background_position_widget(binding->btn, binding->widget);
    card_background_configure_target_size(ctx, target_width, target_height);
    card_background_sync_binding_image(ctx, binding);
    activated_refs++;
  }
  ESP_LOGD("card_background", "Activated page=%p refs=%d matched=%d active=%d",
           page, total_refs, matched_refs, activated_refs);
  card_background_reveal_ready_widgets();
}

inline void apply_card_background_image(BtnSlot &s, const ParsedCfg &p,
                                        const GridConfig &cfg) {
  if (!s.btn || !card_background_supported_type(p.type)) return;
  std::string id = cfg_option_value(p.options, CARD_BACKGROUND_IMAGE_OPTION);
  if (!card_background_image_id_valid(id)) return;

  int target_width = lv_obj_get_width(s.btn);
  int target_height = lv_obj_get_height(s.btn);

#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 4, 0)
  lv_obj_t *img = lv_image_create(s.btn);
#else
  lv_obj_t *img = lv_img_create(s.btn);
#endif
  lv_obj_clear_flag(img, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_clear_flag(img, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_pad_all(img, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(img, 0, LV_PART_MAIN);
  lv_obj_set_style_bg_opa(img, LV_OPA_TRANSP, LV_PART_MAIN);
  lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
  image_card_apply_tile_image_align(img);
  card_background_register_widget(s.btn, img, id);

  lv_obj_t *active_page = card_background_active_page();
  if (!active_page || !card_background_widget_on_page(s.btn, active_page)) {
    card_background_position_widget(s.btn, img);
    lv_obj_move_background(img);
    card_background_move_content_foreground(s);
    return;
  }

  if (target_width <= 0 || target_height <= 0) {
    lv_obj_move_background(img);
    card_background_move_content_foreground(s);
    return;
  }

  CardBackgroundImageCtx *ctx = acquire_card_background_image_context(cfg, id, target_width, target_height);
  if (!ctx) {
    ESP_LOGW("card_background", "No downloader available for card background image %s", id.c_str());
    card_background_position_widget(s.btn, img);
    lv_obj_move_background(img);
    card_background_move_content_foreground(s);
    return;
  }

  CardBackgroundImageCtx::Binding *binding = card_background_add_binding(ctx, s.btn, img);
  if (!binding) {
    ESP_LOGW("card_background", "No card binding available for card background image %s", id.c_str());
    card_background_unregister_widget(img);
    lv_obj_del(img);
    return;
  }

  card_background_position_widget(binding->btn, binding->widget);
  card_background_configure_target_size(ctx, target_width, target_height);
  card_background_sync_binding_image(ctx, binding);
  lv_obj_move_background(img);
  card_background_move_content_foreground(s);
}

inline void sync_card_background_image(BtnSlot &s, const ParsedCfg &p,
                                       const GridConfig &cfg) {
  if (!s.btn) return;
  std::string desired_id = card_background_supported_type(p.type)
    ? cfg_option_value(p.options, CARD_BACKGROUND_IMAGE_OPTION) : "";
  if (!card_background_image_id_valid(desired_id)) desired_id.clear();

  lv_obj_t *existing_widget = nullptr;
  std::string existing_id;
  for (const auto &ref : card_background_widget_refs()) {
    if (ref.btn == s.btn) {
      existing_widget = ref.widget;
      existing_id = ref.id;
      break;
    }
  }
  if (existing_widget && existing_id == desired_id) {
    refresh_card_background_image(s.btn, cfg);
    if (!desired_id.empty()) card_background_move_content_foreground(s);
    return;
  }

  if (existing_widget) {
    CardBackgroundImageCtx *contexts = card_background_image_contexts();
    for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
      for (auto &binding : contexts[i].bindings) {
        if (!binding.active || binding.widget != existing_widget) continue;
        binding.active = false;
        binding.btn = nullptr;
        binding.widget = nullptr;
        card_background_controller().remove_binding(static_cast<size_t>(i));
      }
      card_background_deactivate_if_unused(&contexts[i]);
    }
    card_background_unregister_widget(existing_widget);
    image_card_clear_widget_source(existing_widget);
    lv_obj_del(existing_widget);
    image_card_delete_label_shadow(s.icon_lbl, s.btn);
    image_card_delete_label_shadow(s.text_lbl, s.btn);
    image_card_delete_label_shadow(s.subpage_lbl, s.btn);
  }

  if (!desired_id.empty()) apply_card_background_image(s, p, cfg);
}

inline void clear_card_background_image(BtnSlot &s) {
  if (!s.btn) return;
  lv_obj_t *existing_widget = nullptr;
  for (const auto &ref : card_background_widget_refs()) {
    if (ref.btn == s.btn) {
      existing_widget = ref.widget;
      break;
    }
  }
  if (!existing_widget) return;

  CardBackgroundImageCtx *contexts = card_background_image_contexts();
  for (int i = 0; i < CARD_BACKGROUND_IMAGE_MAX_CONTEXTS; i++) {
    for (auto &binding : contexts[i].bindings) {
      if (!binding.active || binding.widget != existing_widget) continue;
      binding.active = false;
      binding.btn = nullptr;
      binding.widget = nullptr;
      card_background_controller().remove_binding(static_cast<size_t>(i));
    }
    card_background_deactivate_if_unused(&contexts[i]);
  }
  card_background_unregister_widget(existing_widget);
  image_card_clear_widget_source(existing_widget);
  lv_obj_del(existing_widget);
  image_card_delete_label_shadow(s.icon_lbl, s.btn);
  image_card_delete_label_shadow(s.text_lbl, s.btn);
  image_card_delete_label_shadow(s.subpage_lbl, s.btn);
}
