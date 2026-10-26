#include "page_esphome_local_sensor_provider.h"

#include <cassert>
#include <cmath>
using namespace espcontrol::pages;
int main() {
#if defined(USE_SENSOR) && defined(USE_TEXT_SENSOR)
  esphome::sensor::Sensor temperature("temperature"), other("other");
  esphome::text_sensor::TextSensor text("text"), same_key("temperature");
  esphome::App.sensors = {&temperature, &other};
  esphome::App.texts = {&text, &same_key};
  auto& provider = esphome_local_sensor_provider();
  assert(provider.capacity() == 4 && provider.callbacks() == 0);
  const auto number = provider.resolve("temperature", false), string = provider.resolve("temperature", true),
             status = provider.resolve("text", true);
  assert(number != NO_LOCAL_SENSOR && string != NO_LOCAL_SENSOR && number != string &&
         status != NO_LOCAL_SENSOR);
  assert(provider.resolve("missing", false) == NO_LOCAL_SENSOR &&
         provider.resolve("text", false) == NO_LOCAL_SENSOR);
  for (int i = 0; i < 1000; ++i) {
    assert(provider.watch(number) && provider.watch(string) && provider.watch(status));
  }
  assert(provider.callbacks() == 3 && temperature.callbacks.size() == 1 && same_key.callbacks.size() == 1 &&
         text.callbacks.size() == 1);
  assert(std::isnan(provider.read(number).number));
  const auto before = provider.read(number).revision;
  temperature.publish_state(-4.25f);
  assert(provider.read(number).number == -4.25f && provider.read(number).revision == before + 1);
  const auto empty = provider.read(status).revision;
  assert(provider.read(status).text.empty());
  text.publish_state("");
  assert(provider.read(status).revision == empty + 1 && provider.read(status).text.empty());
  text.publish_state("Raw ÉTÉ");
  assert(provider.read(status).text == "Raw ÉTÉ");
  const auto repeated = provider.read(status).revision;
  text.publish_state("Raw ÉTÉ");
  assert(provider.read(status).revision == repeated + 1);
  PageESPHomeLocalSensorProvider<2, 2> second;
  assert(second.watch(status) && provider.callbacks() == 3 && text.callbacks.size() == 1);
  assert(second.read(status).revision == provider.read(status).revision);
  assert(!provider.watch(NO_LOCAL_SENSOR) && provider.read(NO_LOCAL_SENSOR).revision == 0);
#elif defined(USE_SENSOR)
  esphome::sensor::Sensor source("source");
  esphome::App.sensors = {&source};
  auto& provider = esphome_local_sensor_provider();
  assert(provider.capacity() == 2);
  const auto id = provider.resolve("source", false);
  assert(id != NO_LOCAL_SENSOR && provider.watch(id));
  source.publish_state(5);
  assert(provider.read(id).number == 5);
  assert(provider.resolve("source", true) == NO_LOCAL_SENSOR);
#elif defined(USE_TEXT_SENSOR)
  esphome::text_sensor::TextSensor source("source");
  esphome::App.texts = {&source};
  auto& provider = esphome_local_sensor_provider();
  assert(provider.capacity() == 2);
  const auto id = provider.resolve("source", true);
  assert(id != NO_LOCAL_SENSOR && provider.watch(id));
  source.publish_state("raw");
  assert(provider.read(id).text == "raw");
  assert(provider.resolve("source", false) == NO_LOCAL_SENSOR);
#else
  auto& provider = esphome_local_sensor_provider();
  assert(provider.capacity() == 0 && provider.callbacks() == 0);
  assert(provider.resolve("missing", true) == NO_LOCAL_SENSOR && !provider.watch(0) &&
         provider.read(0).revision == 0);
#endif
}
