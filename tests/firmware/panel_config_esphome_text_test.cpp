#include "panel_config_esphome_text.h"
#include <cassert>
#include <string>
#include <tuple>

using espcontrol::configuration::EspHomePanelConfigTextValue;
int main() {
  esphome::Preferences prefs;
  esphome::global_preferences = &prefs;
  esphome::text::Text text;
  EspHomePanelConfigTextValue value;
  value.bind(&text, true);
  // Golden preference identities from the pinned SDK's TemplateText setup:
  // base hash0x12345678, empty pattern, min/max 0/255, 0/120 and6/6.
  for (const auto &[minimum, maximum, key] :
       {std::tuple<uint16_t, uint16_t, uint32_t>{0, 255, 0x595bd7a8},
        {0, 120, 0x595bcf38},
        {6, 6, 0x595bc830}}) {
    text.traits.minimum = minimum;
    text.traits.maximum = maximum;
    std::vector<uint8_t> deployed(maximum + 1, 0xA5);
    deployed[0] = 3;
    std::memcpy(deployed.data() + 1, "old", 3);
    prefs.durable[key] = deployed;
    text.state = "old";
    const auto saves = prefs.saves;
    // Reuse the already durable value, irrespective of historical tail padding.
    assert(value.set_value("old", 3) && prefs.saves == saves);
    text.state = "new"; // Native publication precedes compatibility mirroring.
    const auto published = text.publications;
    assert(value.set_value("new", 3));
    assert(text.controls == 0 && text.publications == published);
    assert(prefs.durable[key].size() == maximum + 1u &&
           prefs.durable[key][0] == 3);
    assert(std::memcmp(prefs.durable[key].data() + 1, "new", 3) == 0);
    prefs.fail_sync = prefs.syncs + 2;
    text.state = "later";
    assert(!value.set_value("later", 5));
    assert(std::memcmp(prefs.durable[key].data() + 1, "new", 3) == 0);
    assert(value.set_value("later", 5));
    assert(std::memcmp(prefs.durable[key].data() + 1, "later", 5) == 0);
  }
  text.traits.minimum = 0;
  text.traits.maximum = 255;
  for (unsigned i = 0; i < 182; ++i) {
    const std::string body(250, 'a' + i % 26);
    assert(value.set_value(body.data(), body.size()));
    assert(prefs.pending.empty() && prefs.peak_queued_bytes <= 256);
  }
  const auto saves = prefs.saves;
  assert(!value.set_value(nullptr, 1));
  const std::string oversized(256, 'x');
  assert(!value.set_value(oversized.data(), oversized.size()));
  assert(prefs.saves == saves);
  // A custom Text platform keeps its original control behavior, including when
  // no global ESP32 preference manager is present.
  esphome::global_preferences = nullptr;
  assert(!value.set_value("x", 1));
  value.bind(&text);
  assert(value.set_value("custom", 6) && text.controls == 1 &&
         text.state == "custom");
}
