#include <cstdlib>
#include <string>
#include <vector>

#include "legacy_card_config.h"

using espcontrol::card_config_references_asset;
using espcontrol::clear_card_background_reference;
using espcontrol::clear_subpage_background_references;
using espcontrol::split_subpage_config;
using espcontrol::subpage_config_references_asset;

int main() {
  const std::string id = "asset-123";

  std::string legacy =
      "light.kitchen;Kitchen;lightbulb;;;%;light;;confirm_on,bg_image=asset-123,label_display=off";
  if (!card_config_references_asset(legacy, id) ||
      !clear_card_background_reference(legacy, id) ||
      legacy != "light.kitchen;Kitchen;lightbulb;;;%;light;;confirm_on,label_display=off" ||
      card_config_references_asset(legacy, id)) {
    return EXIT_FAILURE;
  }

  std::string compact =
      "~light.kitchen,Kitchen,lightbulb,,,%25,light,,confirm_on%2Cbg_image=asset-123%2Clabel_display=off";
  if (!clear_card_background_reference(compact, id) ||
      compact != "~light.kitchen,Kitchen,lightbulb,,,%25,light,,confirm_on%2Clabel_display=off") {
    return EXIT_FAILURE;
  }

  std::string shared =
      "1,2|light.a;A;;;;;light;;bg_image=asset-123|switch.b;B;;;;;switch;;bg_image=other|"
      "sensor.c;C;;;;;sensor;;large_numbers,bg_image=asset-123";
  if (!subpage_config_references_asset(shared, id) ||
      !clear_subpage_background_references(shared, id) ||
      shared != "1,2|light.a;A;;;;;light;;|switch.b;B;;;;;switch;;bg_image=other|"
                "sensor.c;C;;;;;sensor;;large_numbers") {
    return EXIT_FAILURE;
  }

  std::string compact_subpage =
      "~1,2|L,light.a,A,,,,,,bg_image=asset-123%2Cconfirm_on|S,sensor.b,B,,,,,,bg_image=other";
  if (!clear_subpage_background_references(compact_subpage, id) ||
      compact_subpage != "~1,2|L,light.a,A,,,,,,confirm_on|S,sensor.b,B,,,,,,bg_image=other") {
    return EXIT_FAILURE;
  }

  std::string prefix_collision = "x;;;;;;switch;;bg_image=asset-1234";
  if (clear_card_background_reference(prefix_collision, id)) return EXIT_FAILURE;

  std::vector<std::string> chunks;
  const std::string utf8 = std::string(254, 'a') + "\xC2\xA3" + "tail";
  if (!split_subpage_config(utf8, 2, 255, chunks) || chunks.size() != 2 ||
      chunks[0].size() != 254 || chunks[0] + chunks[1] != utf8) {
    return EXIT_FAILURE;
  }
  if (split_subpage_config(std::string(511, 'x'), 2, 255, chunks)) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
