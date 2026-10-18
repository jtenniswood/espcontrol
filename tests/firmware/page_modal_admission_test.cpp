#include <array>
#include <cassert>
#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#include "esphome/core/string_ref.h"
struct lv_obj_t {};
inline void lv_label_set_text(lv_obj_t*, const char*) {}
#include "i18n_generated.h"

#include "button_grid_config_parser.h"
#include "page_modal_admission.h"

static size_t allocations = 0;
void* operator new(size_t bytes) {
  ++allocations;
  if (auto* memory = std::malloc(bytes ? bytes : 1)) return memory;
  std::abort();
}
void* operator new[](size_t bytes) {
  return ::operator new(bytes);
}
void operator delete(void* memory) noexcept {
  std::free(memory);
}
void operator delete[](void* memory) noexcept {
  std::free(memory);
}
void operator delete(void* memory, std::size_t) noexcept {
  std::free(memory);
}
void operator delete[](void* memory, std::size_t) noexcept {
  std::free(memory);
}

using namespace espcontrol::pages;

int main() {
  // Compare the input-time borrowed results with the existing saved-card helpers,
  // including strings that cannot fit the standard library's small-string buffer.
  for (const auto* mode : CARD_CONTRACT_MEDIA_MODES) {
    const std::string owned(mode);
    const auto expected = card_runtime_media_mode(owned);
    const auto before = allocations;
    assert(admission::media_mode(owned) == expected);
    assert(allocations == before);
  }
  for (const auto* mode : {"", "controls", "VOLUME", "invalid_media_mode_with_a_long_suffix"}) {
    const std::string owned(mode);
    const auto expected = card_runtime_media_mode(owned);
    const auto before = allocations;
    assert(admission::media_mode(owned) == expected);
    assert(allocations == before);
  }
  for (const auto* action : {"input_select.select_option", "select.select_option", "select.select_next",
                             "input_select.select_option_with_suffix", "", "INPUT_SELECT.SELECT_OPTION"}) {
    const std::string owned(action);
    const auto expected = action_card_option_select_action(owned);
    const auto before = allocations;
    assert(admission::option_select_action(owned) == expected);
    assert(allocations == before);
  }
  for (const auto* precision : {"daily", "today", "tomorrow", "", "daily_with_a_long_invalid_suffix"}) {
    const std::string owned(precision);
    const auto expected = card_contract_weather_forecast_precision(owned);
    const auto before = allocations;
    assert(admission::forecast_precision(owned) == expected);
    assert(allocations == before);
  }
  for (const auto* options :
       {"", ",cover_art,", "cover_art=1", "cover_art_extra", " cover_art", "cover_art,cover_art",
        "power_entity=switch.a_long_entity_name,cover_art,volume_max=80"}) {
    const std::string owned(options);
    const auto expected = cfg_option_token_present(owned, "cover_art");
    const auto before = allocations;
    assert(admission::option_token(owned, "cover_art") == expected);
    assert(!admission::option_token(owned, ""));
    assert(allocations == before);
  }
  const std::string padding = "another_option_with_a_long_name=some_long_saved_value,";
  for (const auto* value : {"0",   "%30", "%30%00", "00",     " 0",     "0 ",   "+", "%2B", "%", "%3",
                            "%GG", "%FF", "%C0%80", "%C2%A0", "%30%FF", "%300", "",  "%00", "1", "%31"}) {
    for (const auto* name : {"pin_arm", "pin_disarm"}) {
      for (const auto& suffix : {std::string{}, std::string(",") + name + "=0"}) {
        const std::string options = padding + name + "=" + value + suffix;
        const auto expected = cfg_option_value(options, name) == "0";
        const auto before = allocations;
        assert(admission::option_equals_ascii(options, name, '0') == expected);
        assert(allocations == before);
      }
    }
  }
  // All byte escapes, malformed escapes, and literal values retain decoder parity.
  for (int high = 0; high < 128; ++high) {
    for (int low = 0; low < 128; ++low) {
      const std::string options = padding + "pin_arm=%" + char(high) + char(low);
      const auto expected = cfg_option_value(options, "pin_arm") == "0";
      const auto before = allocations;
      assert(admission::option_equals_ascii(options, "pin_arm", '0') == expected);
      assert(allocations == before);
    }
  }
  for (const auto* options : {"pin_arm_extra=0", "pin_arm", "pin_arm=,pin_arm=0", "pin_arm=1,pin_arm=0",
                              "pin_arm=0,pin_arm=1", ",,pin_arm=%30,,", "pin_arm_extra=0,pin_arm=%30"}) {
    const std::string owned(options);
    const auto expected = cfg_option_value(owned, "pin_arm") == "0";
    const auto before = allocations;
    assert(admission::option_equals_ascii(owned, "pin_arm", '0') == expected);
    assert(!admission::option_equals_ascii(owned, "", '0'));
    assert(allocations == before);
  }
}
