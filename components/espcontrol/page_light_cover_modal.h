#pragma once

// Include after the legacy light/cover UI helpers, as part of button_grid.h.
#include "page_legacy_modal_port.h"

namespace espcontrol::pages {
struct PageControlModalEnvironment {
  uint32_t accent_color{DEFAULT_ACCENT_COLOR},secondary_color{0x212121};
  const lv_font_t *number_font{nullptr},*label_font{nullptr};
  const lv_font_t *option_title_font{nullptr},*option_value_font{nullptr},*option_menu_font{nullptr};
  const lv_font_t *card_icon_font{nullptr},*icon_font{nullptr};
  int width_compensation_percent{100};
};
struct PageLightModalFamily {
  using Context=LightControlCtx;
  using Environment=PageControlModalEnvironment;
  static constexpr ControlModalKind kind=ControlModalKind::LIGHT_CONTROL;
  static constexpr const char *tag="light panel setup";
  static LightModalLifetime &lifetime(){return light_modal_lifetime();}
  static bool matches(const PageStore &store,const CardDefinition &card) {
    return espcontrol::cards::context_for(std::string(store.field(card,CardField::TYPE)),
      std::string(store.field(card,CardField::SENSOR))).runtime.driver==espcontrol::card_runtime::CardDriverId::LIGHT_CONTROL;
  }
  static bool initialize_context(Context &ctx,const ParsedCfg &config,const Environment &environment) {
    BtnSlot empty{};
    return create_light_control_context(empty,config,environment.accent_color,environment.number_font,
      environment.label_font,environment.icon_font,environment.width_compensation_percent,&ctx)==&ctx;
  }
  static void subscribe(Context *ctx){subscribe_light_control_state(ctx);}
  static void open(Context *ctx){light_control_open_modal(ctx);}
  static void hide(){light_control_hide_modal();}
  static Context *active(){return light_control_modal_ui().active;}
  static lv_obj_t *overlay(){return light_control_modal_ui().overlay;}
  static std::string title(Context *ctx){return light_control_title(ctx);}
  static void cancel(Context *ctx) {
    ctx->dragging_slider=ctx->dragging_temp_slider=false;
    ctx->updating_slider=ctx->updating_temp_slider=false;
  }
};
struct PageCoverModalFamily {
  using Context=CoverControlCtx;
  using Environment=PageControlModalEnvironment;
  static constexpr ControlModalKind kind=ControlModalKind::COVER_CONTROL;
  static constexpr const char *tag="cover panel setup";
  static CoverModalLifetime &lifetime(){return cover_modal_lifetime();}
  static bool matches(const PageStore &store,const CardDefinition &card) {
    return espcontrol::cards::context_for(std::string(store.field(card,CardField::TYPE)),
      std::string(store.field(card,CardField::SENSOR))).runtime.driver==espcontrol::card_runtime::CardDriverId::COVER_MODAL;
  }
  static bool initialize_context(Context &ctx,const ParsedCfg &config,const Environment &environment) {
    BtnSlot empty{};
    return create_cover_control_context(empty,config,environment.accent_color,environment.secondary_color,
      environment.option_title_font,environment.option_value_font,environment.option_menu_font,
      environment.card_icon_font,environment.icon_font,environment.width_compensation_percent,&ctx)==&ctx;
  }
  static void subscribe(Context *ctx){subscribe_cover_control_state(ctx);}
  static void open(Context *ctx){cover_control_open_modal(ctx);}
  static void hide(){cover_control_hide_modal();}
  static Context *active(){return cover_control_modal_ui().active;}
  static lv_obj_t *overlay(){return cover_control_modal_ui().overlay;}
  static std::string title(Context *ctx){return cover_control_title(ctx);}
  static void cancel(Context *ctx) {
    ctx->dragging_position=ctx->dragging_tilt=false;
    ctx->updating_position=ctx->updating_tilt=false;
    if(active()==ctx)cover_control_modal_ui().pending_preset=-1;
  }
};
using PageLightModalPort=PageLegacyModalPort<PageLightModalFamily>;
using PageCoverModalPort=PageLegacyModalPort<PageCoverModalFamily>;
using PageLightModalStyle=PageLegacyModalStyle<PageLightModalFamily>;
using PageCoverModalStyle=PageLegacyModalStyle<PageCoverModalFamily>;
}  // namespace espcontrol::pages
