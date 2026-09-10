#include <array>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "espcontrol_app_core.h"
#include "control_modal_service.h"
#include "grid_navigation_service.h"

using espcontrol::AppLifecycleState;
using espcontrol::DisplayLifecycleState;
using espcontrol::DisplayMode;
using espcontrol::DisplayRequestSource;
using espcontrol::EspControlAppCore;

namespace {

class MemoryBackend final : public espcontrol::configuration::StorageBackend {
 public:
  MemoryBackend() : slots_{std::vector<uint8_t>(128, 0xFF),
                           std::vector<uint8_t>(128, 0xFF)} {}

  size_t slot_capacity() const override { return slots_[0].size(); }
  bool read(uint8_t slot, size_t offset, uint8_t *output, size_t size) override {
    if (slot >= slots_.size() || offset > slots_[slot].size() ||
        size > slots_[slot].size() - offset) return false;
    if (size > 0) std::memcpy(output, slots_[slot].data() + offset, size);
    return true;
  }
  bool write(uint8_t slot, size_t offset, const uint8_t *input,
             size_t size) override {
    if (slot >= slots_.size() || offset > slots_[slot].size() ||
        size > slots_[slot].size() - offset) return false;
    if (size > 0) std::memcpy(slots_[slot].data() + offset, input, size);
    return true;
  }
  bool sync() override { return true; }

 private:
  std::array<std::vector<uint8_t>, 2> slots_;
};

class EmptyLegacy final
    : public espcontrol::configuration::LegacyConfigurationAdapter {
 public:
  espcontrol::configuration::LegacyLoadResult load(uint8_t *, size_t) override {
    return {espcontrol::configuration::LegacyStatus::EMPTY, 1, 0};
  }
  bool mirror(uint16_t, const uint8_t *, size_t) override { return true; }
};

struct HomeTarget {
  int slot = 0;
};

struct Subpage {
  int slot = 0;
};

using NavigationService = GridNavigationService<HomeTarget, Subpage>;

struct ModalOverlay {
  int slot = 0;
};

using ModalService = ControlModalStateService<ModalOverlay>;

}  // namespace

int main() {
  EspControlAppCore app;
  if (app.lifecycle_state() != AppLifecycleState::CONSTRUCTED) return EXIT_FAILURE;
  if (app.has_configuration_service() || app.configuration_service() != nullptr) {
    return EXIT_FAILURE;
  }
  MemoryBackend backend;
  espcontrol::configuration::ConfigurationStore store(backend);
  EmptyLegacy legacy;
  if (!app.configure_configuration_service(store, legacy) ||
      !app.has_configuration_service() ||
      app.configure_configuration_service(store, legacy)) {
    return EXIT_FAILURE;
  }
  const char configuration[] = "core-owned-config";
  if (!app.configuration_service()->save_current(
          reinterpret_cast<const uint8_t *>(configuration),
          sizeof(configuration) - 1).ok()) {
    return EXIT_FAILURE;
  }
  if (app.run_once() || app.stop()) return EXIT_FAILURE;
  if (!app.start() || app.start()) return EXIT_FAILURE;
  if (app.lifecycle_state() != AppLifecycleState::RUNNING) return EXIT_FAILURE;
  if (app.display_lifecycle().state() != DisplayLifecycleState::RUNNING) {
    return EXIT_FAILURE;
  }

  auto &display = app.display();
  if (!display.request(DisplayRequestSource::MANUAL_SLEEP,
                       DisplayMode::DISPLAY_OFF)) {
    return EXIT_FAILURE;
  }
  if (!app.display().target_mode_is(DisplayMode::DISPLAY_OFF)) {
    return EXIT_FAILURE;
  }

  const auto media = app.card_runtime_registry().context_for("media", "");
  if (!media.known || media.family != espcontrol::cards::Family::MEDIA) {
    return EXIT_FAILURE;
  }

  int callback_owner = 0;
  {
    auto scope = app.home_assistant_callback_owner().scope(&callback_owner);
    if (app.home_assistant_callback_owner().callback_owner() !=
        &callback_owner) {
      return EXIT_FAILURE;
    }
  }
  if (app.home_assistant_callback_owner().callback_owner() != nullptr) {
    return EXIT_FAILURE;
  }

  NavigationService &navigation = app.grid_navigation_service<NavigationService>();
  navigation.home_targets().push_back({1});
  navigation.subpages().push_back({2});
  if (navigation.home_target_count() != 1 || navigation.subpage_count() != 1) {
    return EXIT_FAILURE;
  }

  ModalOverlay overlay;
  ModalService &modal = app.modal_state_service<ModalService>();
  modal.set_active(ControlModalKind::IMAGE_CARD, &overlay, nullptr,
                   ControlModalDismissPolicy::DISMISS);
  if (modal.active().overlay != &overlay) return EXIT_FAILURE;

  if (!app.run_once() || !app.run_once() || app.loop_count() != 2 ||
      app.display_lifecycle().loop_count() != 2) {
    return EXIT_FAILURE;
  }
  if (!app.stop() || app.stop() || app.run_once() ||
      app.display_lifecycle().state() != DisplayLifecycleState::STOPPED) {
    return EXIT_FAILURE;
  }
  if (home_assistant_callback_owner_service_binding() != nullptr) {
    return EXIT_FAILURE;
  }
  if (app.lifecycle_state() != AppLifecycleState::STOPPED) return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
