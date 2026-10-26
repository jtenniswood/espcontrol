#pragma once

#include <array>

#include "esphome/core/application.h"
#include "esphome/core/defines.h"
#ifdef USE_SENSOR
#include "esphome/components/sensor/sensor.h"
#endif
#ifdef USE_TEXT_SENSOR
#include "esphome/components/text_sensor/text_sensor.h"
#endif
#include "page_local_sensor_runtime.h"

namespace espcontrol::pages {
#ifdef ESPHOME_ENTITY_SENSOR_COUNT
constexpr size_t LOCAL_NUMERIC_PRODUCERS = ESPHOME_ENTITY_SENSOR_COUNT;
#else
constexpr size_t LOCAL_NUMERIC_PRODUCERS = 0;
#endif
#ifdef ESPHOME_ENTITY_TEXT_SENSOR_COUNT
constexpr size_t LOCAL_TEXT_PRODUCERS = ESPHOME_ENTITY_TEXT_SENSOR_COUNT;
#else
constexpr size_t LOCAL_TEXT_PRODUCERS = 0;
#endif
// The producer table is sized by ESPHome's generated entity counts, not pages
// or configuration keys. Callback captures contain only a stable table index.
// Those records outlive every view/configuration owner and use no heap strings.
template <size_t Numeric, size_t Text>
class PageESPHomeLocalSensorProvider final : public PageLocalSensorProvider {
 public:
  size_t capacity() const override {
    return Numeric + Text;
  }
  uint32_t resolve(std::string_view key, bool text) const override {
    char buffer[128];
#ifdef USE_SENSOR
    if (!text) {
      const auto& sources = esphome::App.get_sensors();
      for (size_t i = 0; i < sources.size() && i < Numeric; ++i) {
        const auto id = sources[i]->get_object_id_to(buffer);
        if (std::string_view(id.c_str(), id.size()) == key) return static_cast<uint32_t>(i);
      }
    }
#endif
#ifdef USE_TEXT_SENSOR
    if (text) {
      const auto& sources = esphome::App.get_text_sensors();
      for (size_t i = 0; i < sources.size() && i < Text; ++i) {
        const auto id = sources[i]->get_object_id_to(buffer);
        if (std::string_view(id.c_str(), id.size()) == key) return static_cast<uint32_t>(Numeric + i);
      }
    }
#endif
    (void)key;
    (void)text;
    (void)buffer;
    return NO_LOCAL_SENSOR;
  }
  bool watch(uint32_t index) override {
    if (index >= Numeric + Text) return false;
    auto& state = states()[index];
    if (state.attached) return true;
    (void)state;
#ifdef USE_SENSOR
    if (index < Numeric) {
      const auto& sources = esphome::App.get_sensors();
      if (index >= sources.size()) return false;
      sources[index]->add_on_state_callback([index](float) { reported(index); });
      state.attached = true;
      return true;
    }
#endif
#ifdef USE_TEXT_SENSOR
    if (index >= Numeric) {
      const auto& sources = esphome::App.get_text_sensors();
      if (index - Numeric >= sources.size()) return false;
      sources[index - Numeric]->add_on_state_callback([index](const std::string&) { reported(index); });
      state.attached = true;
      return true;
    }
#endif
    return false;
  }
  PageLocalSensorSample read(uint32_t index) const override {
    if (index >= Numeric + Text) return {};
#ifdef USE_SENSOR
    if (index < Numeric) {
      const auto& sources = esphome::App.get_sensors();
      if (index >= sources.size()) return {};
      return {states()[index].revision, sources[index]->state, {}};
    }
#endif
#ifdef USE_TEXT_SENSOR
    if (index >= Numeric) {
      const auto& sources = esphome::App.get_text_sensors();
      if (index - Numeric >= sources.size()) return {};
      const auto& text = sources[index - Numeric]->state;
      return {states()[index].revision, {}, std::string_view(text.data(), text.size())};
    }
#endif
    return {};
  }
  size_t fixed_bytes() const override {
    return sizeof(*this) + sizeof(states());
  }
  size_t callbacks() const override {
    size_t count = 0;
    for (const auto& state : states()) count += state.attached;
    return count;
  }

 private:
  struct Producer {
    uint64_t revision{1};
    bool attached{false};
  };
  static auto& states() {
    static std::array<Producer, Numeric + Text> records;
    return records;
  }
  static void reported(uint32_t index) {
    auto& revision = states()[index].revision;
    if (++revision == 0) ++revision;
  }
};
inline PageLocalSensorProvider& esphome_local_sensor_provider() {
  static PageESPHomeLocalSensorProvider<LOCAL_NUMERIC_PRODUCERS, LOCAL_TEXT_PRODUCERS> provider;
  return provider;
}
}  // namespace espcontrol::pages
