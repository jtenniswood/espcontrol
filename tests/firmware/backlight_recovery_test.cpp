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
      bool on{true};
      bool is_on() const { return on; }
    } current_values;
    struct RemoteValues {
      float brightness{0.8f};
      float get_brightness() const { return brightness; }
    } remote_values;
    float stored_level{0.8f};
    float physical_level{0.8f};
    unsigned writes{0};
    unsigned publications{0};
    unsigned saves{0};
    unsigned transition_length{0};
    void current_values_as_brightness(float *out) {
      *out = current_values.is_on() ? stored_level : 0.0f;
    }
    void set_level(float value) { physical_level = value; }
    struct Call {
      Light &light;
      float target{0};
      bool on{true};
      bool publish{true};
      bool save{true};
      void set_brightness(float value) { target = value; }
      void set_state(bool value) { on = value; }
      void set_publish(bool value) { publish = value; }
      void set_save(bool value) { save = value; }
      void set_transition_length(unsigned value) { light.transition_length = value; }
      void perform() {
        ++light.writes;
        light.current_values.on = on;
        if (publish) {
          ++light.publications;
          light.remote_values.brightness = target;
        }
        if (save) ++light.saves;
        // Model the output after ESPHome finishes the requested transition.
        light.stored_level = target;
        light.physical_level = on ? target : 0.0f;
      }
    };
    Call make_call() { return Call{*this}; }
    Call turn_on() { return Call{*this}; }
  } display_backlight;
  bool backlight_force_next_write{false};
  float backlight_expected_internal_level{0};
  bool backlight_expected_internal_level_valid{false};
  espcontrol::BacklightFade screensaver_fade_out;
  uint32_t now_ms{1000};
  uint32_t millis() const { return now_ms; }

  void start_fade() {
#include "backlight_fade_start.inc"
  }

  void apply_brightness(float pct) {
    // Extracted from backlight_apply_brightness, not a copy of its policy.
#include "backlight_brightness_adapter.inc"
  }
};

int main() {
  using namespace espcontrol;
  // A replacement display-off request must continue from the last output
  // sample, including interruptions near and at full black.
  for (float initial : {0.8f, 0.01f, 0.0f}) {
    for (uint32_t interrupted_ms : {0u, 100u, 200u, 400u}) {
      Fixture fixture;
      auto &light = fixture.display_backlight;
      auto &controller = fixture.espcontrol_app.display();
      controller.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::DISPLAY_OFF);
      const auto first = controller.resolve();
      CHECK(controller.start_transition(first, fixture.now_ms));
      apply_backlight_fade_level(&light, &light, initial);
      BacklightFade fade;
      fade.start(initial, 0.0f, fixture.now_ms, DISPLAY_OFF_FADE_OUT_MS);
      for (uint32_t elapsed : {0u, 100u, 200u, 400u}) {
        if (elapsed > interrupted_ms) break;
        const float sample = fade.level(fixture.now_ms + elapsed);
        apply_backlight_fade_level(&light, &light, sample);
      }
      const float before = light.physical_level;
      CHECK(controller.request(DisplayRequestSource::PRESENCE_SENSOR, DisplayMode::DISPLAY_OFF));
      CHECK(controller.cancel_transition());
      fixture.now_ms += 450;
      CHECK(controller.start_transition(controller.resolve(), fixture.now_ms));
      fixture.start_fade();
      const float first_replacement_sample = fixture.screensaver_fade_out.level(fixture.now_ms);
      CHECK(std::fabs(first_replacement_sample - before) < 0.00001f);
      apply_backlight_fade_level(&light, &light, first_replacement_sample);
      CHECK(std::fabs(light.physical_level - before) < 0.00001f);
      CHECK(!controller.complete_transition(first, fixture.now_ms));
      CHECK(light.remote_values.get_brightness() == 0.8f);
      CHECK(light.publications == 0);
      CHECK(light.saves == 0);
      CHECK(light.transition_length == 0);
    }
  }
  for (const auto destination : {DisplayMode::ACTIVE, DisplayMode::COVER_ART}) {
    // Interrupt an automatic screen-off fade.
    {
      Fixture fixture;
      auto &controller = fixture.espcontrol_app.display();
      auto &light = fixture.display_backlight;
      controller.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::DISPLAY_OFF);
      const auto interrupted = controller.resolve();
      CHECK(controller.start_transition(interrupted, 1000));
      BacklightFade fade;
      fade.start(light.stored_level, 0.0f, 1000, DISPLAY_OFF_FADE_OUT_MS);
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
