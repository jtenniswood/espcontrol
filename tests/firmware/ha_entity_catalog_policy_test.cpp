#include <cassert>
#include <string>

#include "ha_entity_catalog_policy.h"

using espcontrol::ha_entity_catalog_capabilities_valid;
using espcontrol::ha_entity_catalog_filter_valid;

int main() {
  // A generated per-field limit can be either smaller or larger than the
  // bridge budget. Changing the contract must not bypass either ceiling.
  assert(ha_entity_catalog_filter_valid(std::string(80, 'a'), 80));
  assert(!ha_entity_catalog_filter_valid(std::string(81, 'a'), 80));
  assert(ha_entity_catalog_filter_valid(std::string(120, 'a'), 200));
  assert(!ha_entity_catalog_filter_valid(std::string(121, 'a'), 200));
  assert(ha_entity_catalog_filter_valid("", 0));
  assert(!ha_entity_catalog_filter_valid("a", 0));
  assert(ha_entity_catalog_capabilities_valid(""));
  assert(ha_entity_catalog_capabilities_valid(" , \t,\n"));
  assert(ha_entity_catalog_capabilities_valid("brightness,color_temp"));
  assert(ha_entity_catalog_capabilities_valid(std::string(80, 'a')));
  assert(ha_entity_catalog_capabilities_valid(" \t" + std::string(80, 'a') + "\r\n"));
  assert(!ha_entity_catalog_capabilities_valid(std::string(81, 'a')));
  assert(!ha_entity_catalog_capabilities_valid(std::string(120, 'a')));
  assert(!ha_entity_catalog_capabilities_valid("brightness, " + std::string(81, 'a')));
  assert(!ha_entity_catalog_capabilities_valid(std::string(81, 'a') + ",brightness"));
  // A valid CSV can exceed 80 characters, but must fit the 120-byte bridge budget.
  assert(ha_entity_catalog_capabilities_valid(std::string(80, 'a') + "," + std::string(39, 'b')));
  assert(!ha_entity_catalog_capabilities_valid(std::string(80, 'a') + "," + std::string(40, 'b')));
  return 0;
}
