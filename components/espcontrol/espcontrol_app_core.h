#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cstdlib>
#include <new>
#include <optional>
#include <type_traits>

#include "button_grid_card_runtime.h"
#include "configuration_service.h"
#include "display_lifecycle_service.h"
#include "home_assistant_binding_service.h"

namespace espcontrol {

// UI-owned service types can carry LVGL handles, so the core cannot name them
// directly without taking on framework dependencies. This fixed-capacity slot
// gives one such service an explicit application-owned lifetime instead.
template<size_t Capacity>
class FixedRuntimeServiceSlot {
 public:
  static constexpr size_t CAPACITY = Capacity;

  FixedRuntimeServiceSlot() = default;
  ~FixedRuntimeServiceSlot() { reset(); }
  FixedRuntimeServiceSlot(const FixedRuntimeServiceSlot &) = delete;
  FixedRuntimeServiceSlot &operator=(const FixedRuntimeServiceSlot &) = delete;

  template<typename Service>
  Service &get_or_create() {
    static_assert(sizeof(Service) <= CAPACITY,
                  "runtime service exceeds the fixed core slot capacity");
    static_assert(alignof(Service) <= alignof(std::max_align_t),
                  "runtime service alignment exceeds the fixed core slot");
    const char *type_name = service_type<Service>();
    if (type_name_ == nullptr) {
      new (storage_.data()) Service();
      type_name_ = type_name;
      destroy_ = [](void *storage) { static_cast<Service *>(storage)->~Service(); };
    } else if (std::strcmp(type_name_, type_name) != 0) {
      std::abort();
    }
    return *static_cast<Service *>(static_cast<void *>(storage_.data()));
  }

  // Some ESP-IDF builds emit separate type-name constants for a UI type that
  // crosses the ESPHome component boundary.  Modal state has one concrete
  // production type, so reuse that fixed storage without a cross-unit token
  // comparison rather than turning an implementation-detail mismatch into a
  // firmware-wide abort.
  template<typename Service>
  Service &get_or_create_ui_service() {
    static_assert(sizeof(Service) <= CAPACITY,
                  "runtime service exceeds the fixed core slot capacity");
    static_assert(alignof(Service) <= alignof(std::max_align_t),
                  "runtime service alignment exceeds the fixed core slot");
    if (destroy_ == nullptr) {
      new (storage_.data()) Service();
      type_name_ = service_type<Service>();
      destroy_ = [](void *storage) { static_cast<Service *>(storage)->~Service(); };
    }
    return *static_cast<Service *>(static_cast<void *>(storage_.data()));
  }

  void reset() {
    if (destroy_ != nullptr) destroy_(storage_.data());
    type_name_ = nullptr;
    destroy_ = nullptr;
  }

 private:
  template<typename Service>
  static const char *service_type() {
    // Function-local marker addresses can differ between ESP-IDF translation
    // units even for the same template specialisation.  The signature text is
    // stable for the concrete service and preserves the fixed-slot invariant.
    return __PRETTY_FUNCTION__;
  }

  alignas(std::max_align_t) std::array<uint8_t, CAPACITY> storage_{};
  const char *type_name_{nullptr};
  void (*destroy_)(void *){nullptr};
};

enum class AppLifecycleState : uint8_t {
  CONSTRUCTED,
  RUNNING,
  STOPPED,
};

// Framework-independent owner for EspControl's long-lived firmware services.
// Keeping this core free of ESPHome APIs makes lifecycle and ownership
// executable in host tests.
class EspControlAppCore {
 public:
  ~EspControlAppCore();

  bool start();
  bool run_once();
  bool stop();

  AppLifecycleState lifecycle_state() const { return lifecycle_state_; }
  uint32_t loop_count() const { return loop_count_; }

  DisplayLifecycleService &display_lifecycle() { return display_lifecycle_; }
  const DisplayLifecycleService &display_lifecycle() const { return display_lifecycle_; }

  cards::CardRuntimeRegistryService &card_runtime_registry() {
    return card_runtime_registry_;
  }
  const cards::CardRuntimeRegistryService &card_runtime_registry() const {
    return card_runtime_registry_;
  }

  // Storage and legacy text adapters are device wiring concerns, while the
  // configuration service itself has one application-owned lifetime.
  bool configure_configuration_service(
      configuration::ConfigurationStore &store,
      configuration::LegacyConfigurationAdapter &legacy,
      const configuration::ConfigurationDocumentValidator *validator = nullptr,
      configuration::LegacyConfigurationMode legacy_mode =
          configuration::LegacyConfigurationMode::DUAL_WRITE);
  bool has_configuration_service() const {
    return configuration_service_.has_value();
  }
  configuration::ConfigurationService *configuration_service() {
    return configuration_service_ ? &*configuration_service_ : nullptr;
  }
  const configuration::ConfigurationService *configuration_service() const {
    return configuration_service_ ? &*configuration_service_ : nullptr;
  }

  HomeAssistantCallbackOwnerService &home_assistant_callback_owner() {
    return home_assistant_callback_owner_;
  }
  const HomeAssistantCallbackOwnerService &home_assistant_callback_owner() const {
    return home_assistant_callback_owner_;
  }

  // The ESPHome transport type stays in the UI/wiring layer, but its binding
  // and callback state receive one core-owned lifetime.
  template<typename BindingService>
  BindingService &home_assistant_binding_service() {
    return home_assistant_binding_service_.get_or_create_ui_service<BindingService>();
  }

  template<typename NavigationService>
  NavigationService &grid_navigation_service() {
    return grid_navigation_service_.get_or_create_ui_service<NavigationService>();
  }

  // Modal widgets stay in the LVGL-facing UI layer, while their lifecycle
  // state receives the same application-owned lifetime as navigation.
  template<typename ModalService>
  ModalService &modal_state_service() {
    return modal_state_service_.get_or_create_ui_service<ModalService>();
  }

  // Compatibility facade for ESPHome YAML while display ownership migrates to
  // the explicit lifecycle service.
  DisplayModeController &display() { return display_lifecycle_.controller(); }
  const DisplayModeController &display() const {
    return display_lifecycle_.controller();
  }

 private:
  AppLifecycleState lifecycle_state_{AppLifecycleState::CONSTRUCTED};
  uint32_t loop_count_{0};
  DisplayLifecycleService display_lifecycle_{};
  cards::CardRuntimeRegistryService card_runtime_registry_{};
  std::optional<configuration::ConfigurationService> configuration_service_;
  HomeAssistantCallbackOwnerService home_assistant_callback_owner_{};
  // The binding service is 200 bytes: coordinator metadata and vector handles
  // are fixed here, while pointed-to request data remains demand-allocated.
  FixedRuntimeServiceSlot<224> home_assistant_binding_service_{};
  // The concrete UI services assert their own sizes when they bind to these
  // slots. Keeping each bound small avoids reserving a generic 128-byte buffer
  // for every service in every firmware image.
  FixedRuntimeServiceSlot<64> grid_navigation_service_{};
  FixedRuntimeServiceSlot<64> modal_state_service_{};
};

inline EspControlAppCore *&active_espcontrol_app_core() {
  static EspControlAppCore *core = nullptr;
  return core;
}

}  // namespace espcontrol
