#include "esphome_configuration_registry.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <span>
#include <string>

#include "esphome/core/application.h"
#include "esphome/core/entity_base.h"

#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif
#ifdef USE_TEXT
#include "esphome/components/text/text.h"
#endif

namespace espcontrol::configuration {
namespace {

bool is_configuration_entity(const esphome::EntityBase *entity) {
  return entity != nullptr &&
         entity->get_entity_category() == esphome::ENTITY_CATEGORY_CONFIG;
}

bool same_object_id(const esphome::EntityBase *entity,
                    const ConfigurationEntityView &candidate) {
  if (entity == nullptr || candidate.object_id == nullptr ||
      candidate.object_id_size == 0 ||
      candidate.object_id_size >= esphome::OBJECT_ID_MAX_LEN) {
    return false;
  }
  char buffer[esphome::OBJECT_ID_MAX_LEN];
  const esphome::StringRef object_id = entity->get_object_id_to(buffer);
  return object_id.size() == candidate.object_id_size &&
         std::memcmp(object_id.c_str(), candidate.object_id,
                     candidate.object_id_size) == 0;
}

template<typename Entity>
Entity *find_entity(const std::vector<Entity *> &entities,
                    const ConfigurationEntityView &candidate) {
  for (Entity *entity : entities) {
    if (is_configuration_entity(entity) &&
        same_object_id(entity, candidate)) {
      return entity;
    }
  }
  return nullptr;
}

bool copy_number(const ConfigurationEntityView &entity, char *buffer,
                 size_t capacity, float *output) {
  if (buffer == nullptr || output == nullptr || entity.value == nullptr ||
      entity.value_size == 0 || entity.value_size >= capacity) {
    return false;
  }
  std::memcpy(buffer, entity.value, entity.value_size);
  buffer[entity.value_size] = '\0';
  char *end = nullptr;
  const float value = std::strtof(buffer, &end);
  if (end != buffer + entity.value_size || !std::isfinite(value)) return false;
  *output = value;
  return true;
}

bool parse_switch(const ConfigurationEntityView &entity, bool *output) {
  if (output == nullptr || entity.value == nullptr) return false;
  if ((entity.value_size == 1 && entity.value[0] == '1') ||
      (entity.value_size == 4 &&
       std::memcmp(entity.value, "true", 4) == 0)) {
    *output = true;
    return true;
  }
  if ((entity.value_size == 1 && entity.value[0] == '0') ||
      (entity.value_size == 5 &&
       std::memcmp(entity.value, "false", 5) == 0)) {
    *output = false;
    return true;
  }
  return false;
}

}  // namespace

size_t EspHomeConfigurationRegistry::size() const {
  size_t count = 0;
#ifdef USE_TEXT
  for (auto *entity : esphome::App.get_texts()) {
    if (is_configuration_entity(entity)) ++count;
  }
#endif
#ifdef USE_SELECT
  for (auto *entity : esphome::App.get_selects()) {
    if (is_configuration_entity(entity)) ++count;
  }
#endif
#ifdef USE_NUMBER
  for (auto *entity : esphome::App.get_numbers()) {
    if (is_configuration_entity(entity)) ++count;
  }
#endif
#ifdef USE_SWITCH
  for (auto *entity : esphome::App.get_switches()) {
    if (is_configuration_entity(entity)) ++count;
  }
#endif
  return count;
}

bool EspHomeConfigurationRegistry::read(
    size_t index, ConfigurationEntityView *output) const {
  if (output == nullptr) return false;
  size_t current = 0;
#ifdef USE_TEXT
  for (auto *entity : esphome::App.get_texts()) {
    if (!is_configuration_entity(entity)) continue;
    if (current++ != index) continue;
    const esphome::StringRef object_id =
        entity->get_object_id_to(object_id_buffer_);
    *output = {ConfigurationEntityDomain::TEXT, object_id.c_str(),
               object_id.size(), entity->state.data(), entity->state.size()};
    return true;
  }
#endif
#ifdef USE_SELECT
  for (auto *entity : esphome::App.get_selects()) {
    if (!is_configuration_entity(entity)) continue;
    if (current++ != index) continue;
    const esphome::StringRef object_id =
        entity->get_object_id_to(object_id_buffer_);
    const esphome::StringRef value = entity->current_option();
    *output = {ConfigurationEntityDomain::SELECT, object_id.c_str(),
               object_id.size(), value.c_str(), value.size()};
    return true;
  }
#endif
#ifdef USE_NUMBER
  for (auto *entity : esphome::App.get_numbers()) {
    if (!is_configuration_entity(entity)) continue;
    if (current++ != index) continue;
    const esphome::StringRef object_id =
        entity->get_object_id_to(object_id_buffer_);
    const int written = std::snprintf(value_buffer_, sizeof(value_buffer_),
                                      "%.9g", entity->state);
    if (written <= 0 || static_cast<size_t>(written) >= sizeof(value_buffer_)) {
      return false;
    }
    *output = {ConfigurationEntityDomain::NUMBER, object_id.c_str(),
               object_id.size(), value_buffer_, static_cast<size_t>(written)};
    return true;
  }
#endif
#ifdef USE_SWITCH
  for (auto *entity : esphome::App.get_switches()) {
    if (!is_configuration_entity(entity)) continue;
    if (current++ != index) continue;
    const esphome::StringRef object_id =
        entity->get_object_id_to(object_id_buffer_);
    const char *value = entity->state ? "1" : "0";
    *output = {ConfigurationEntityDomain::SWITCH, object_id.c_str(),
               object_id.size(), value, 1};
    return true;
  }
#endif
  return false;
}

bool EspHomeConfigurationRegistry::can_apply(
    const ConfigurationEntityView &entity) const {
  switch (entity.domain) {
    case ConfigurationEntityDomain::TEXT: {
#ifdef USE_TEXT
      auto *target = find_entity(esphome::App.get_texts(), entity);
      return target != nullptr &&
             entity.value_size >=
                 static_cast<size_t>(target->traits.get_min_length()) &&
             entity.value_size <=
                 static_cast<size_t>(target->traits.get_max_length());
#else
      return false;
#endif
    }
    case ConfigurationEntityDomain::SELECT: {
#ifdef USE_SELECT
      auto *target = find_entity(esphome::App.get_selects(), entity);
      return target != nullptr && entity.value != nullptr &&
             target->index_of(entity.value, entity.value_size).has_value();
#else
      return false;
#endif
    }
    case ConfigurationEntityDomain::NUMBER: {
#ifdef USE_NUMBER
      auto *target = find_entity(esphome::App.get_numbers(), entity);
      float value = NAN;
      return target != nullptr &&
             copy_number(entity, value_buffer_, sizeof(value_buffer_), &value) &&
             value >= target->traits.get_min_value() &&
             value <= target->traits.get_max_value();
#else
      return false;
#endif
    }
    case ConfigurationEntityDomain::SWITCH: {
#ifdef USE_SWITCH
      bool value = false;
      return find_entity(esphome::App.get_switches(), entity) != nullptr &&
             parse_switch(entity, &value);
#else
      return false;
#endif
    }
  }
  return false;
}

bool EspHomeConfigurationRegistry::apply(
    const ConfigurationEntityView &entity) {
  if (!can_apply(entity)) return false;
  switch (entity.domain) {
    case ConfigurationEntityDomain::TEXT:
#ifdef USE_TEXT
      find_entity(esphome::App.get_texts(), entity)
          ->make_call()
          .set_value(entity.value, entity.value_size)
          .perform();
      return true;
#else
      return false;
#endif
    case ConfigurationEntityDomain::SELECT:
#ifdef USE_SELECT
      find_entity(esphome::App.get_selects(), entity)
          ->make_call()
          .set_option(entity.value, entity.value_size)
          .perform();
      return true;
#else
      return false;
#endif
    case ConfigurationEntityDomain::NUMBER:
#ifdef USE_NUMBER
      {
        float value = NAN;
        if (!copy_number(entity, value_buffer_, sizeof(value_buffer_), &value)) {
          return false;
        }
        find_entity(esphome::App.get_numbers(), entity)
            ->make_call()
            .set_value(value)
            .perform();
        return true;
      }
#else
      return false;
#endif
    case ConfigurationEntityDomain::SWITCH:
#ifdef USE_SWITCH
      {
        bool value = false;
        if (!parse_switch(entity, &value)) return false;
        find_entity(esphome::App.get_switches(), entity)->control(value);
        return true;
      }
#else
      return false;
#endif
  }
  return false;
}

}  // namespace espcontrol::configuration
