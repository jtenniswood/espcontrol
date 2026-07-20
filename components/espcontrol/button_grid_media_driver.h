#pragma once

// Shared lifecycle driver for every Media card mode. The specialised playback,
// artwork, playlist, progress, volume, Home Assistant, and modal helpers remain
// in button_grid_media.h; this driver owns the main-grid/subpage boundary.
// Contract coverage marker: "media".

namespace espcontrol::cards {

inline bool media_driver_matches(const Context &context) {
  if (context.legacy_dispatch) return false;
  using Driver = card_runtime::CardDriverId;
  switch (context.runtime.driver) {
    case Driver::MEDIA:
    case Driver::MEDIA_CONTROL:
    case Driver::MEDIA_PLAY_PAUSE:
    case Driver::MEDIA_TRANSPORT:
    case Driver::MEDIA_VOLUME:
    case Driver::MEDIA_POSITION:
    case Driver::MEDIA_NOW_PLAYING:
    case Driver::MEDIA_COVER_ART:
    case Driver::MEDIA_PLAYLIST:
      return true;
    default:
      return false;
  }
}

inline bool media_driver_setup_visual(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const CardPalette &palette, const DisplayProfile &display,
    int row_span = 1, int col_span = 1) {
  if (!media_driver_matches(context)) return false;
  const bool large_cover_art =
    context.runtime.driver == card_runtime::CardDriverId::MEDIA_COVER_ART &&
    media_cover_art_uses_screensaver_fonts(row_span, col_span);
  const bool compact_large_cover_art =
    context.runtime.driver == card_runtime::CardDriverId::MEDIA_COVER_ART &&
    media_cover_art_uses_compact_large_fonts(row_span, col_span);
  const bool compact_portrait_cover_art =
    compact_large_cover_art &&
    display.modal.layout_family == DisplayModalLayoutFamily::COMPACT_PORTRAIT;
  const lv_font_t *label_font = slot.text_lbl
    ? lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN)
    : nullptr;
  setup_media_card(
    slot, config,
    palette.has_on ? palette.on_val : DEFAULT_SLIDER_COLOR,
    palette.off_val, palette.sensor_val,
    display_sensor_font(display),
    compact_portrait_cover_art
      ? display_media_control_title_font(display)
      : compact_large_cover_art
      ? display_media_cover_art_artist_font(display, display_media_title_font(display))
      : large_cover_art
      ? display_media_cover_art_title_font(display)
      : display_media_title_font(display),
    compact_portrait_cover_art
      ? display_media_control_artist_font(display, label_font)
      : compact_large_cover_art
      ? label_font
      : large_cover_art
      ? display_media_cover_art_artist_font(display)
      : nullptr,
    display_main_width_percent(display), row_span, col_span);
  return true;
}

inline bool media_driver_attach_interaction(
    BtnSlot &, const ParsedCfg &, const Context &context) {
  return media_driver_matches(context);
}

inline bool media_driver_refresh_layout(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const GridConfig &grid_config, int row_span = 1, int col_span = 1) {
  if (!media_driver_matches(context)) return false;
  refresh_media_card_layout(slot, config, grid_config, row_span, col_span);
  return true;
}

inline bool media_driver_cleanup(
    BtnSlot &slot, const ParsedCfg &config, const Context &context) {
  (void) config;
  grid_prepare_media_runtime_for_visual_reset(slot.btn);
  if (!media_driver_matches(context)) return false;
  if (slot.btn) lv_obj_set_user_data(slot.btn, nullptr);
  return true;
}

struct MediaDriverEnvironment {
  const GridConfig *grid_config = nullptr;
  uint32_t accent_color = DEFAULT_SLIDER_COLOR;
  uint32_t secondary_color = SECONDARY_GREY;
  uint32_t tertiary_color = TERTIARY_GREY;
  const lv_font_t *sensor_font = nullptr;
  const lv_font_t *control_title_font = nullptr;
  const lv_font_t *control_artist_font = nullptr;
  const lv_font_t *volume_number_font = nullptr;
  const lv_font_t *volume_unit_font = nullptr;
  const lv_font_t *volume_label_font = nullptr;
  const lv_font_t *icon_font = nullptr;
  int volume_width_compensation_percent = 100;
  std::function<void(const std::string &)> add_parent_indicator;
};

inline MediaDriverEnvironment media_driver_environment(
    const CardPalette &palette, const DisplayProfile &display,
    const BtnSlot &slot, const GridConfig &grid_config) {
  MediaDriverEnvironment environment;
  environment.grid_config = &grid_config;
  environment.accent_color = palette.has_on
    ? palette.on_val : DEFAULT_SLIDER_COLOR;
  environment.secondary_color = palette.has_off
    ? palette.off_val : SECONDARY_GREY;
  environment.tertiary_color = palette.has_sensor_color
    ? palette.sensor_val : TERTIARY_GREY;
  environment.sensor_font = display_sensor_font(display);
  environment.control_title_font = display_media_control_title_font(display);
  environment.volume_number_font = display_volume_number_font(display);
  environment.volume_unit_font = display_volume_label_font(display)
    ? display_volume_label_font(display)
    : (slot.unit_lbl
        ? lv_obj_get_style_text_font(slot.unit_lbl, LV_PART_MAIN) : nullptr);
  environment.volume_label_font = display_volume_label_font(display)
    ? display_volume_label_font(display)
    : (slot.text_lbl
        ? lv_obj_get_style_text_font(slot.text_lbl, LV_PART_MAIN) : nullptr);
  environment.control_artist_font = display_media_control_artist_font(
    display, environment.volume_label_font);
  environment.icon_font = display_icon_font(display);
  environment.volume_width_compensation_percent =
    display_volume_width_percent(display);
  return environment;
}

inline MediaControlCtx *media_driver_track_control(
    const Context &context, lv_obj_t *owner, MediaControlCtx *control) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_media_control_with_owner(owner, control)
    : grid_track_media_control_runtime(owner, control);
}

inline MediaVolumeCtx *media_driver_track_volume(
    const Context &context, lv_obj_t *owner, MediaVolumeCtx *volume) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_media_volume_with_owner(owner, volume)
    : grid_track_media_volume_runtime(owner, volume);
}

inline MediaPlaylistCtx *media_driver_track_playlist(
    const Context &context, lv_obj_t *owner, MediaPlaylistCtx *playlist) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_media_playlist_with_owner(owner, playlist)
    : grid_track_media_playlist_runtime(owner, playlist);
}

inline MediaNowPlayingCtx *media_driver_track_now_playing(
    const Context &context, lv_obj_t *owner, MediaNowPlayingCtx *now_playing) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_media_now_playing_with_owner(owner, now_playing)
    : grid_track_media_now_playing_runtime(owner, now_playing);
}

inline SliderCtx *media_driver_track_slider(
    const Context &context, lv_obj_t *owner, SliderCtx *slider) {
  return context.surface == Surface::SUBPAGE
    ? grid_delete_media_slider_with_owner(owner, slider)
    : grid_track_media_slider_runtime(owner, slider);
}

inline MediaControlCtx *media_driver_create_control(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const MediaDriverEnvironment &environment) {
  return media_driver_track_control(
    context, slot.btn,
    create_media_control_context(
      slot, config, environment.accent_color,
      environment.secondary_color, environment.tertiary_color,
      environment.control_title_font, environment.control_artist_font,
      environment.volume_number_font, environment.icon_font,
      environment.volume_width_compensation_percent));
}

inline void media_driver_bind_cover_art_route(
    MediaNowPlayingCtx *now_playing, MediaControlCtx *control,
    const std::string &primary_entity,
    const std::string &secondary_entity) {
  if (!now_playing || primary_entity.empty()) return;
  now_playing->primary_entity = primary_entity;
  now_playing->secondary_entity = secondary_entity;
  now_playing->active_entity.clear();

  MediaPlaybackState *primary = media_playback_ensure_state(primary_entity);
  if (!primary) return;
  media_playback_attach_now_playing(primary, now_playing);
  media_playback_subscribe_playback_state(primary);
  media_playback_subscribe_metadata(primary);
  media_playback_subscribe_content(primary);
  media_playback_subscribe_progress(primary);

  MediaPlaybackState *secondary = nullptr;
  if (!secondary_entity.empty() && secondary_entity != primary_entity) {
    secondary = media_playback_ensure_state(secondary_entity);
    if (secondary) {
      media_playback_attach_now_playing(secondary, now_playing);
      media_playback_subscribe_playback_state(secondary);
      media_playback_subscribe_metadata(secondary);
      media_playback_subscribe_content(secondary);
      media_playback_subscribe_progress(secondary);
    }
  }

  subscribe_media_cover_art(now_playing, primary_entity);
  if (secondary) subscribe_media_cover_art(now_playing, secondary_entity);

  now_playing->refresh_entity_route = [now_playing, control]() {
    MediaPlaybackState *primary_state =
      media_playback_find_state(now_playing->primary_entity);
    MediaPlaybackState *secondary_state =
      media_playback_find_state(now_playing->secondary_entity);
    const bool secondary_configured =
      !now_playing->secondary_entity.empty() &&
      now_playing->secondary_entity != now_playing->primary_entity;
    const bool use_secondary = espcontrol::cover_art::use_secondary_media_entity(
      primary_state && primary_state->external_source,
      secondary_configured,
      secondary_state && secondary_state->available,
      media_playback_has_current_content(secondary_state));
    const bool external_source_fallback =
      primary_state && primary_state->external_source && !use_secondary;
    const std::string next_entity = use_secondary
      ? now_playing->secondary_entity : now_playing->primary_entity;
    const bool entity_changed = next_entity != now_playing->active_entity;
    const bool presentation_changed =
      external_source_fallback != now_playing->external_source_fallback;
    if (next_entity.empty() || (!entity_changed && !presentation_changed)) return;

    ESP_LOGI("media_card", "Cover art entity switched from %s to %s",
             now_playing->active_entity.empty() ? "<none>" : now_playing->active_entity.c_str(),
             next_entity.c_str());
    now_playing->active_entity = next_entity;
    now_playing->external_source_fallback = external_source_fallback;

    if (now_playing->cover_art) {
      ImageCardCtx *art = now_playing->cover_art;
      image_card_clear_media_artwork(art);
      art->access_token.clear();
      if (external_source_fallback) {
        art->entity_id.clear();
      } else {
        art->entity_id = next_entity;
        image_card_request_media_artwork(art);
      }
    }

    if (entity_changed && now_playing->progress_slider) {
      SliderCtx *slider = static_cast<SliderCtx *>(
        lv_obj_get_user_data(now_playing->progress_slider));
      if (slider) {
        media_playback_detach_slider(slider);
        slider->entity_id = next_entity;
        subscribe_media_slider_state(
          lv_obj_get_parent(now_playing->progress_slider),
          now_playing->progress_slider, next_entity);
      }
    }

    if (entity_changed && control) {
      media_playback_detach_control(control);
      control->entity_id = next_entity;
      subscribe_media_control_state(control);
    }
  };
  now_playing->refresh_entity_route();
  media_playback_apply_state_to_now_playing(primary, now_playing);
}

inline bool media_driver_bind_data(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const MediaDriverEnvironment &environment) {
  if (!media_driver_matches(context)) return false;
  if (config.entity.empty()) return true;

  const std::string mode = media_card_mode(config.sensor);
  if (mode == "playlist") {
    MediaPlaylistCtx *playlist = media_driver_track_playlist(
      context, slot.btn, create_media_playlist_context(slot.btn, config));
    subscribe_media_playlist_state(playlist);
  } else if (media_playback_button_mode(mode)) {
    if (mode == "play_pause") {
      subscribe_media_state(
        slot.btn,
        media_play_pause_show_state(config) ? slot.text_lbl : nullptr,
        config.entity);
    }
  } else if (mode == "control_modal") {
    MediaControlCtx *control = media_driver_create_control(
      slot, config, context, environment);
    subscribe_media_control_state(control);
  } else if (mode == "volume") {
    MediaVolumeCtx *volume = media_driver_track_volume(
      context, slot.btn,
      create_media_volume_context(
        slot.btn, slot.text_lbl, config,
        environment.accent_color, environment.secondary_color,
        environment.tertiary_color, environment.sensor_font,
        environment.volume_number_font, environment.volume_unit_font,
        environment.volume_label_font, environment.icon_font,
        environment.volume_width_compensation_percent,
        slot.sensor_lbl, slot.unit_lbl));
    subscribe_media_volume_state(volume);
    if (config.label.empty()) {
      subscribe_friendly_name(slot.text_lbl, config.entity);
    }
  } else if (mode == "now_playing" || mode == "cover_art") {
    MediaNowPlayingCtx *now_playing = slot.sensor_container
      ? static_cast<MediaNowPlayingCtx *>(
          lv_obj_get_user_data(slot.sensor_container))
      : nullptr;
    media_driver_track_now_playing(context, slot.btn, now_playing);
    if (now_playing && now_playing->progress_slider) {
      media_driver_track_slider(
        context, slot.btn,
        static_cast<SliderCtx *>(
          lv_obj_get_user_data(now_playing->progress_slider)));
    }
    if (environment.grid_config) {
      setup_media_cover_art(slot, config, *environment.grid_config);
    }
    if (mode == "now_playing") {
      subscribe_media_now_playing_state(now_playing, config.entity);
    }
    if (mode == "now_playing") {
      subscribe_media_cover_art(now_playing, config.entity);
    }
    MediaControlCtx *control = nullptr;
    if (mode == "cover_art" &&
        media_cover_art_press_action(config) == "control_modal") {
      control = media_driver_create_control(
        slot, config, context, environment);
      if (control) control->highlight_playing = false;
    }
    if (mode == "cover_art") {
      media_driver_bind_cover_art_route(
        now_playing, control, config.entity,
        media_cover_art_secondary_entity(config));
    }
  } else {
    lv_obj_t *slider_obj = slot.sensor_container
      ? static_cast<lv_obj_t *>(lv_obj_get_user_data(slot.sensor_container))
      : nullptr;
    SliderCtx *slider = slider_obj
      ? static_cast<SliderCtx *>(lv_obj_get_user_data(slider_obj))
      : nullptr;
    media_driver_track_slider(context, slot.btn, slider);
    if (slider_obj) {
      subscribe_media_slider_state(slot.btn, slider_obj, config.entity);
    }
    if (config.label.empty() && mode != "position") {
      subscribe_friendly_name(slot.text_lbl, config.entity);
    }
  }
  return true;
}

inline bool media_driver_bind_main(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const MediaDriverEnvironment &environment) {
  if (!media_driver_matches(context) ||
      context.surface != Surface::MAIN_GRID) return false;
  return media_driver_bind_data(slot, config, context, environment);
}

inline bool media_driver_handle_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  if (!media_driver_matches(context)) return false;
  const std::string mode = media_card_mode(config.sensor);
  if (mode == "control_modal") {
    MediaControlCtx *control = button
      ? static_cast<MediaControlCtx *>(lv_obj_get_user_data(button)) : nullptr;
    if (!control) control = grid_media_control_runtime_for_owner(button);
    if (control) media_control_open_modal(control);
  } else if (mode == "volume") {
    MediaVolumeCtx *volume = button
      ? static_cast<MediaVolumeCtx *>(lv_obj_get_user_data(button)) : nullptr;
    if (volume) media_volume_open_modal(volume);
  } else if (mode == "playlist") {
    send_media_playlist_action(config);
  } else if (mode == "now_playing" && config.precision == "play_pause") {
    send_media_playback_action(config.entity, "play_pause");
  } else if (mode == "cover_art") {
    if (media_cover_art_press_action(config) == "control_modal") {
      MediaControlCtx *control = button
        ? static_cast<MediaControlCtx *>(lv_obj_get_user_data(button)) : nullptr;
      if (!control) control = grid_media_control_runtime_for_owner(button);
      if (control) media_control_open_modal(control);
    } else {
      ImageCardCtx *art = button
        ? static_cast<ImageCardCtx *>(lv_obj_get_user_data(button)) : nullptr;
      send_media_playback_action(
        art && !art->entity_id.empty() ? art->entity_id : config.entity,
        "play_pause");
    }
  } else if (media_playback_button_mode(mode)) {
    send_media_playback_action(config.entity, mode);
  }
  return true;
}

inline bool media_driver_handle_main_click(
    const Context &context, const ParsedCfg &config, lv_obj_t *button) {
  return media_driver_handle_click(context, config, button);
}

inline bool media_driver_subpage_clickable(
    const ParsedCfg &config) {
  const std::string mode = media_card_mode(config.sensor);
  return mode == "control_modal" || mode == "volume" ||
         mode == "playlist" || mode == "cover_art" ||
         media_playback_button_mode(mode) ||
         (mode == "now_playing" && config.precision == "play_pause");
}

inline bool media_driver_bind_subpage(
    BtnSlot &slot, const ParsedCfg &config, const Context &context,
    const MediaDriverEnvironment &environment) {
  if (!media_driver_matches(context) ||
      context.surface != Surface::SUBPAGE) return false;
  media_driver_bind_data(slot, config, context, environment);
  if (config.entity.empty()) return true;

  if (environment.add_parent_indicator) {
    environment.add_parent_indicator(config.entity);
  }
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    media_playback_detach_button(static_cast<lv_obj_t *>(
      lv_event_get_target(event)));
  }, LV_EVENT_DELETE, nullptr);

  if (!media_driver_subpage_clickable(config)) return true;
  ParsedCfg *click = grid_delete_with_owner(
    slot.btn, new ParsedCfg(config));
  const std::string mode = media_card_mode(config.sensor);
  lv_obj_add_event_cb(slot.btn, [](lv_event_t *event) {
    ParsedCfg *saved = static_cast<ParsedCfg *>(
      lv_event_get_user_data(event));
    lv_obj_t *target = static_cast<lv_obj_t *>(lv_event_get_target(event));
    if (!saved) return;
    media_driver_handle_click(
      card_runtime_context(*saved, Surface::SUBPAGE), *saved, target);
  }, media_fast_press_mode(mode) ? LV_EVENT_PRESSED : LV_EVENT_CLICKED, click);
  return true;
}

}  // namespace espcontrol::cards
