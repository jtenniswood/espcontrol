#include <cmath>
#include <cstdio>
#include <cstdlib>

#include "backlight_fade.h"
#include "display_mode_controller.h"

#define CHECK(condition) do { if (!(condition)) { \
  std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
  return EXIT_FAILURE; } } while (false)
#define id(value) value

struct Fixture {
  struct App {
    espcontrol::DisplayModeController controller;
    auto &display() { return controller; }
  } espcontrol_app;
  struct Light {
    struct Values {
      bool is_on() const { return true; }
    } current_values;
    float stored_level{0.8f};
    float physical_level{0.8f};
    unsigned writes{0};
    void current_values_as_brightness(float *out) { *out = stored_level; }
    struct Call {
      Light &light;
      float target{0};
      void set_brightness(float value) { target = value; }
      void set_transition_length(unsigned) {}
      void perform() {
        ++light.writes;
        // Model the output after ESPHome finishes the requested transition.
        light.stored_level = light.physical_level = target;
      }
    };
    Call turn_on() { return Call{*this}; }
  } display_backlight;
  bool backlight_force_next_write{false};
  float backlight_expected_internal_level{0};
  bool backlight_expected_internal_level_valid{false};

  void apply_brightness(float pct) {
    // Extracted from backlight_apply_brightness, not a copy of its policy.
#include "backlight_brightness_adapter.inc"
  }
};

int main() {
  using namespace espcontrol;
  for (const auto destination : {DisplayMode::ACTIVE, DisplayMode::COVER_ART}) {
    // Interrupt during clock fade-out, clock reveal, and automatic screen-off.
    for (int phase = 0; phase < 3; ++phase) {
      Fixture fixture;
      auto &controller = fixture.espcontrol_app.display();
      auto &light = fixture.display_backlight;
      controller.request(DisplayRequestSource::IDLE_TIMER,
                         phase == 2 ? DisplayMode::DISPLAY_OFF : DisplayMode::CLOCK);
      const auto interrupted = controller.resolve();
      CHECK(controller.start_transition(interrupted, 1000));
      BacklightFade fade;
      if (phase == 1) {
        fade.start(CLOCK_HANDOFF_LEVEL, 0.35f, 1000, CLOCK_FADE_IN_MS);
      } else {
        fade.start(light.stored_level, phase == 2 ? 0.0f : CLOCK_HANDOFF_LEVEL,
                   1000, phase == 2 ? DISPLAY_OFF_FADE_OUT_MS : CLOCK_FADE_OUT_MS);
      }
      light.physical_level = fade.level(1200);
      CHECK(light.physical_level < light.stored_level);

      if (destination == DisplayMode::ACTIVE) {
        CHECK(controller.begin_takeover(DisplayTakeoverKind::CRITICAL));
      } else {
        CHECK(controller.request(DisplayRequestSource::MEDIA_PLAYBACK, destination));
      }
      CHECK(controller.cancel_transition());
      const auto recovery = controller.resolve();
      CHECK(recovery.target_mode == destination);
      CHECK(controller.start_transition(recovery, 1200));
      fixture.apply_brightness(80.0f);
      CHECK(light.writes == 1);
      CHECK(light.physical_level == 0.8f);
      CHECK(fixture.backlight_expected_internal_level_valid);
      CHECK(!controller.complete_transition(interrupted, 1300));
      CHECK(controller.complete_transition(recovery, 1350));

      // Once settled, retain duplicate-write suppression and explicit Wake.
      fixture.apply_brightness(80.0f);
      CHECK(light.writes == 1);
      fixture.backlight_force_next_write = true;
      fixture.apply_brightness(80.0f);
      CHECK(light.writes == 2);
      CHECK(!fixture.backlight_force_next_write);
      fixture.apply_brightness(60.0f);
      CHECK(light.writes == 3);
      CHECK(light.physical_level == 0.6f);
    }
  }
  return EXIT_SUCCESS;
}
