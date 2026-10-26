#pragma once

#include <cmath>

#include "page_card_commands.h"
#include "page_light_temperature_runtime.h"
#include "page_number_slider_runtime.h"
#include "page_percent_slider_runtime.h"

namespace espcontrol::pages {
// Application-owned state validation and failed/cancelled preview rollback.
// Uses existing histories: no per-card records or new subscriptions.
class PageSliderCommandPolicy final : public PageCardControlPolicy {
 public:
  PageSliderCommandPolicy(PageEntityStateRuntime& graph, PagePercentSliderRuntime& percent,
                          PageNumberSliderRuntime& number, PageLightTemperatureRuntime& temperature)
      : graph_(graph), percent_(percent), number_(number), temperature_(temperature) {}
  bool admit(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             PageCardCommandValue& value) override {
    const ConfigurationHandle handle{origin.configuration, origin.card};
    if (kind == PageCardCommandKind::PERCENT) value.write_revision = percent_.read(handle).write_revision;
    if (kind == PageCardCommandKind::TEMPERATURE)
      value.write_revision = temperature_.read(handle).write_revision;
    return valid(store, origin, kind, value);
  }
  bool valid(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             const PageCardCommandValue& value) override {
    const ConfigurationHandle handle{origin.configuration, origin.card};
    const auto* card = store.card(handle);
    if (!card) return false;
    const auto entity = store.field(*card, CardField::ENTITY);
    if (entity.empty()) return false;
    const auto available = [&](std::string_view attribute) {
      return graph_.read_attribute(handle, entity, attribute).status != PageEntityStatus::EXHAUSTED;
    };
    if (kind == PageCardCommandKind::PERCENT || kind == PageCardCommandKind::SLIDER_TOGGLE) {
      const auto type = PagePercentSliderRuntime::classify(store, origin.card);
      if (type != PagePercentSliderKind::NONE) {
        percent_.loop();
        const auto snapshot = percent_.read(handle);
        return snapshot.kind == type && snapshot.available && available({}) &&
               available(PagePercentSliderRuntime::attribute(type)) &&
               (kind == PageCardCommandKind::SLIDER_TOGGLE || (value.integer >= 0 && value.integer <= 100));
      }
      if (kind != PageCardCommandKind::SLIDER_TOGGLE) return false;
    }
    if (kind == PageCardCommandKind::TEMPERATURE || kind == PageCardCommandKind::SLIDER_TOGGLE) {
      if (!PageLightTemperatureRuntime::supported(store, origin.card)) return false;
      temperature_.loop();
      const auto snapshot = temperature_.read(handle);
      return snapshot.available && available({}) && available("color_temp_kelvin") &&
             (kind == PageCardCommandKind::SLIDER_TOGGLE ||
              (value.integer >= snapshot.minimum && value.integer <= snapshot.maximum));
    }
    if (kind != PageCardCommandKind::NUMBER || !PageNumberSliderRuntime::supported(store, origin.card))
      return false;
    number_.loop();
    const auto snapshot = number_.read(handle);
    const auto& m = snapshot.metadata;
    return snapshot.available && snapshot.ready && available({}) && available("min") && available("max") &&
           available("step") && number_slider::metadata_valid(value.metadata) &&
           std::isfinite(value.number) && m.minimum == value.metadata.minimum &&
           m.maximum == value.metadata.maximum && m.step == value.metadata.step &&
           value.number >= m.minimum && value.number <= m.maximum &&
           number_slider::snap_value(m, value.number) == value.number;
  }
  void discard(PageBindingToken origin, PageCardCommandKind kind,
               const PageCardCommandValue& value) override {
    const ConfigurationHandle handle{origin.configuration, origin.card};
    if (kind == PageCardCommandKind::PERCENT && value.integer >= 0 && value.integer <= 100)
      percent_.discard_preview(handle, static_cast<uint8_t>(value.integer), value.write_revision);
    if (kind == PageCardCommandKind::NUMBER) number_.discard_finished(handle, value.number, value.metadata);
    if (kind == PageCardCommandKind::TEMPERATURE)
      temperature_.discard_finished(handle, value.integer, value.write_revision, graph_);
  }

 private:
  PageEntityStateRuntime& graph_;
  PagePercentSliderRuntime& percent_;
  PageNumberSliderRuntime& number_;
  PageLightTemperatureRuntime& temperature_;
};
}  // namespace espcontrol::pages
