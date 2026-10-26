#include "page_card_dependencies.h"

#include <initializer_list>
#include <string>

#include "button_grid_subpage_parser.h"
#include "guest_wifi_state.h"
#include "media_group_discovery.h"
#include "wifi_qr_codec.h"

namespace espcontrol::pages {
namespace {
using Driver = card_runtime::CardDriverId;
using Type = card_runtime::CardTypeId;
ParsedCfg configuration(const PageStore &store, const CardDefinition &card) {
  ParsedCfg result;
  result.entity = store.field(card, CardField::ENTITY);
  result.label = store.field(card, CardField::LABEL);
  result.icon = store.field(card, CardField::ICON);
  result.icon_on = store.field(card, CardField::ICON_ON);
  result.sensor = store.field(card, CardField::SENSOR);
  result.unit = store.field(card, CardField::UNIT);
  result.type = store.field(card, CardField::TYPE);
  result.precision = store.field(card, CardField::PRECISION);
  result.options = store.field(card, CardField::OPTIONS);
  return result;
}
bool starts(const std::string &value, const char *prefix) { return value.rfind(prefix, 0) == 0; }
class CardEmitter {
 public:
  CardEmitter(PageDependencySink &sink, CardId card, bool low_heap_media)
      : sink_(sink), card_(card), low_heap_media_(low_heap_media) {}
  bool pair(const std::string &entity, const char *attribute = "") {
    return entity.empty() || sink_.add(card_, entity, attribute);
  }
  bool attributes(const std::string &entity, std::initializer_list<const char *> names) {
    for (const char *name : names) if (!pair(entity, name)) return false;
    return true;
  }
  bool friendly(const ParsedCfg &config, const std::string &entity) {
    return !config.label.empty() || pair(entity, "friendly_name");
  }
  bool progress(const std::string &entity) {
    return attributes(entity, {"media_duration", "media_position", "media_position_updated_at", "media_content_id"});
  }
  bool content(const std::string &entity) {
    return attributes(entity, {"media_content_type", "media_content_id"});
  }
  bool metadata(const std::string &entity) {
    return attributes(entity, {"media_title", "media_artist"});
  }
  bool artwork(const std::string &entity) {
    return attributes(entity, {"entity_picture", "entity_picture_local", "access_token"});
  }
  bool media_control(const ParsedCfg &config, const std::string &entity) {
    if (entity.empty()) return true;
    if (!pair(entity) || !content(entity) || !metadata(entity) ||
        !attributes(entity, {"volume_level", "supported_features", "shuffle", "repeat", "group_members"})) return false;
    const auto decoded = media::decode_config_v1(config);
    if (!pair(decoded.power_entity)) return false;
    const std::string discovery = media_group_discovery_entity(media_speaker_group_entity(config));
    if (!pair(discovery, media_group_discovery_attribute(discovery))) return false;
    return low_heap_media_ || (progress(entity) && pair(entity, "friendly_name"));
  }
  bool cover_art(const ParsedCfg &config) {
    if (!media_control(config,config.entity) || !content(config.entity) || !metadata(config.entity) ||
        !progress(config.entity) || !pair(config.entity,"source") || !artwork(config.entity)) return false;
    const std::string secondary=media_cover_art_secondary_entity(config);
    return secondary.empty() || secondary==config.entity ||
        (media_control(config,secondary) && content(secondary) && metadata(secondary) && progress(secondary) &&
         pair(secondary,"source") && artwork(secondary));
  }
  bool slider(const ParsedCfg &config, bool tilt) {
    if (!pair(config.entity) || !friendly(config, config.entity)) return false;
    if (starts(config.entity,"number.") || starts(config.entity,"input_number."))
      return attributes(config.entity,{"min","max","step","unit_of_measurement"});
    if (starts(config.entity,"cover.")) return pair(config.entity,tilt ? "current_tilt_position" : "current_position");
    if (starts(config.entity,"fan.")) return pair(config.entity,"percentage");
    if (starts(config.entity,"light.")) return pair(config.entity,"brightness");
    return true;
  }
  bool fan(const ParsedCfg &config) {
    if (config.entity.empty()) return true;
    if (!pair(config.entity) || !attributes(config.entity, {"friendly_name", "supported_features", "percentage",
          "percentage_step", "oscillating", "direction", "preset_mode", "preset_modes"})) return false;
    return pair(cfg_option_value(config.options, FAN_LIGHT_ENTITY_OPTION));
  }
  bool emit(const ParsedCfg &config, bool in_subpage, bool aggregate_parent) {
    const auto context = cards::context_for(config.type, config.sensor,
        in_subpage ? cards::Surface::SUBPAGE : cards::Surface::MAIN_GRID);
    if (!context.known || (in_subpage && !context.allow_in_subpage)) return false;
    if (context.family == cards::Family::MEDIA && config.entity.empty()) return true;
    if (action_card_option_select(config) || context.runtime.driver == Driver::OPTION_SELECT)
      return pair(config.entity) && pair(config.entity,"options") && friendly(config,config.entity);
    switch (context.runtime.driver) {
      case Driver::TOGGLE:
        return config.entity.empty() || (pair(config.entity) && friendly(config,config.entity) && pair(config.sensor));
      case Driver::ACTION:
        return pair(action_card_state_entity(config));
      case Driver::ALARM:
        return pair(config.entity) && attributes(config.entity, {"arm_mode", "delay"}) &&
               (normalize_alarm_label_display(cfg_option_value(config.options, "label_display")) ==
                    "status" ||
                friendly(config, config.entity));
      case Driver::ALARM_ACTION:
        return pair(config.entity) && attributes(config.entity,{"arm_mode","delay"});
      case Driver::DATE_TIME:
        return context.runtime.type != Type::CALENDAR || pair(config.entity.empty() ? "sensor.date" : config.entity);
      case Driver::CLIMATE:
        // Reserve every optional field which capability changes can expose.
        // This prevents a later tab/menu selection from allocating channels.
        return pair(config.entity) && attributes(config.entity, {"friendly_name", "hvac_action", "supported_features",
            "current_temperature", "temperature", "target_temp_low", "target_temp_high", "min_temp", "max_temp",
            "target_temp_step", "hvac_modes", "fan_modes", "swing_modes", "swing_horizontal_modes", "preset_modes",
            "preset_mode", "fan_mode", "swing_mode", "swing_horizontal_mode"});
      case Driver::COVER_MODAL:
        return pair(config.entity) && attributes(config.entity,
            {"current_position","current_tilt_position","supported_features"}) && friendly(config,config.entity);
      case Driver::COVER_POSITION:
      case Driver::COVER_TILT:
      case Driver::COVER:
        return slider(config,context.runtime.driver == Driver::COVER_TILT);
      case Driver::COVER_TOGGLE:
        return pair(config.entity) && friendly(config,config.entity);
      case Driver::COVER_COMMAND:
        return pair(config.entity,"supported_features") && friendly(config,config.entity);
      case Driver::STATUS_ENTITY:
        return pair(config.sensor) && friendly(config,config.sensor);
      case Driver::FAN:
        return context.runtime.type == Type::FAN_SPEED ? slider(config, false) : fan(config);
      case Driver::FAN_CONTROL:
        return fan(config);
      case Driver::ACCESS: {
        if (context.runtime.type == Type::GARAGE) {
          const bool status = normalize_garage_label_display(cfg_option_value(config.options,"label_display")) == "status";
          if (card_runtime_garage_command_mode(config.sensor) && !status) return true;
          return pair(config.entity) && (status || friendly(config,config.entity));
        }
        if (context.runtime.type == Type::GATE) {
          const bool status = normalize_gate_label_display(cfg_option_value(config.options,"label_display")) == "status";
          const bool command = card_runtime_gate_command_mode(config.sensor);
          return pair(config.entity) && (status || command || friendly(config,config.entity));
        }
        return card_runtime_lock_command_mode(config.sensor) || (pair(config.entity) && friendly(config,config.entity));
      }
      case Driver::NUMERIC:
        return slider(config,false);
      case Driver::LIGHT_TEMPERATURE:
        return pair(config.entity) && pair(config.entity,"color_temp_kelvin") && friendly(config,config.entity);
      case Driver::LIGHT_CONTROL:
        return pair(config.entity) && attributes(config.entity,
            {"brightness","color_temp_kelvin","supported_color_modes","friendly_name"});
      case Driver::SENSOR:
        if (sensor_card_local_sensor(config) || context.runtime.type == Type::LOCAL_SENSOR) return true;
        if (config.sensor.empty()) return true;
        if (!pair(config.sensor)) return false;
        if (config.precision == "text") return true;
        if (config.precision == "time" && cfg_option_value(config.options,SENSOR_TIME_UNIT_OPTION).empty() &&
            !pair(config.sensor,"unit_of_measurement")) return false;
        return friendly(config,config.sensor);
      case Driver::VACUUM:
        return (!card_runtime_vacuum_state_mode(config.sensor) && !(in_subpage && aggregate_parent)) || pair(config.entity);
      case Driver::LAWN_MOWER:
        return (!card_runtime_lawn_mower_state_mode(config.sensor) && !(in_subpage && aggregate_parent)) || pair(config.entity);
      case Driver::WEATHER:
        return card_runtime_weather_forecast_precision(config.precision) || pair(config.entity);
      case Driver::IMAGE: {
        if (!starts(config.entity,"camera.") && !starts(config.entity,"image.")) return true;
        if (!pair(config.entity) || !attributes(config.entity,{"entity_picture","access_token"}) ||
            !friendly(config,config.entity)) return false;
        const std::string trigger=cfg_option_value(config.options,"image_modal_refresh_trigger");
        return !starts(config.entity,"camera.") ||
            camera::refresh_mode(cfg_option_value(config.options,"image_modal_refresh_mode")) != camera::RefreshMode::ACTIVITY ||
            !camera::valid_trigger(trigger) || pair(trigger);
      }
      case Driver::WIFI_QR: {
        const auto tabs=wifi_qr_tabs(cfg_option_value(config.options,"wifi_tabs"));
        return !guest_wifi_valid_entity(config.entity) || std::find(tabs.begin(),tabs.end(),"guest")==tabs.end() || pair(config.entity);
      }
      case Driver::TIMER:
        return !starts(config.entity,"timer.") || (pair(config.entity) &&
            attributes(config.entity,{"duration","remaining","finishes_at"}) && friendly(config,config.entity));
      case Driver::SUBPAGE:
        if (!config.sensor.empty() && config.sensor != "indicator")
          return pair(config.sensor) && (config.precision == "text" || friendly(config,config.sensor));
        if (config.sensor == "indicator" && !config.entity.empty())
          return friendly(config,config.entity) && pair(config.entity) &&
              (normalize_subpage_kind(cfg_option_value(config.options,"subpage_kind")) != "climate" ||
               pair(config.entity,"hvac_action"));
        return true;
      case Driver::MEDIA_PLAY_PAUSE:
        return pair(config.entity);
      case Driver::MEDIA_TRANSPORT:
        return !(in_subpage && aggregate_parent) || pair(config.entity);
      case Driver::MEDIA_VOLUME:
        return pair(config.entity) && attributes(config.entity,{"volume_level","supported_features"}) && friendly(config,config.entity);
      case Driver::MEDIA_POSITION:
        return pair(config.entity) && progress(config.entity);
      case Driver::MEDIA_PLAYLIST:
        return pair(config.entity) && content(config.entity);
      case Driver::MEDIA_NOW_PLAYING:
        if (cfg_option_token_present(config.options, MEDIA_COVER_ART_OPTION)) return cover_art(config);
        return content(config.entity) && pair(config.entity,"source") && metadata(config.entity) &&
            (config.precision != "progress" || (pair(config.entity) && progress(config.entity))) &&
            ((config.precision != "play_pause" && !(in_subpage && aggregate_parent)) || pair(config.entity));
      case Driver::MEDIA_COVER_ART:
        return cover_art(config);
      case Driver::MEDIA_CONTROL:
      case Driver::MEDIA_GROUP:
      case Driver::MEDIA:
        return media_control(config,config.entity);
      case Driver::INTERNAL:
      case Driver::PUSH:
      case Driver::SCREEN_LOCK:
      case Driver::SCREENSAVER:
      case Driver::WEBHOOK:
        return true;
      case Driver::OPTION_SELECT:
      case Driver::UNKNOWN:
        return false;
    }
    return false;
  }
 private:
  PageDependencySink &sink_;
  CardId card_;
  bool low_heap_media_;
};
}  // namespace

bool PageCardDependencies::enumerate(const PageStore &store, PageDependencySink &sink) const {
  for (PageId page_id=0;page_id<store.page_count();++page_id) {
    const auto *page=store.page(page_id);
    if (page==nullptr || !page->navigable) continue;
    ParsedCfg parent;
    if (page_id!=0) {
      const auto *owner=store.card(page->owner);
      if (owner==nullptr) return false;
      parent=configuration(store,*owner);
    }
    const bool aggregate=parent.sensor=="indicator" && parent.entity.empty();
    for (size_t i=0;i<page->card_count;++i) {
      const auto card_id=static_cast<CardId>(page->first_card+i);
      const auto *card=store.card(card_id);
      if (card==nullptr) return false;
      auto config=configuration(store,*card);
      // Alarm children can inherit their target/options from the parent.
      if (page_id!=0 && (config.type=="alarm" || config.type=="alarm_action")) {
        if (config.entity.empty()) config.entity=parent.entity;
        if (config.options.empty()) config.options=parent.options;
      }
      if (!CardEmitter(sink,card_id,policy_.low_heap_media_control).emit(config,page_id!=0,aggregate)) return false;
    }
  }
  return true;
}
}  // namespace espcontrol::pages
