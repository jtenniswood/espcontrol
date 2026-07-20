#include <cstdlib>

#include "display_mode_controller.h"

using namespace espcontrol;

#define CHECK(condition) do { if (!(condition)) return EXIT_FAILURE; } while (false)

static bool decision_is(const DisplayModeController &controller, DisplayMode mode,
                        std::optional<DisplayRequestSource> source = std::nullopt,
                        std::optional<DisplayTakeoverKind> takeover = std::nullopt) {
  const auto decision = controller.resolve();
  return decision.target_mode == mode && decision.winning_source == source &&
         decision.winning_takeover == takeover;
}

static void activate_priority(DisplayModeController &controller, int priority) {
  switch (priority) {
    case 1: controller.begin_takeover(DisplayTakeoverKind::CRITICAL); break;
    case 2: controller.request(DisplayRequestSource::MANUAL_SLEEP, DisplayMode::DISPLAY_OFF); break;
    case 3: controller.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE); break;
    case 4: controller.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::CLOCK); break;
    case 5: controller.begin_takeover(DisplayTakeoverKind::INTERACTIVE); break;
    case 6: controller.request(DisplayRequestSource::MEDIA_PLAYBACK, DisplayMode::COVER_ART); break;
    case 7: controller.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::DIMMED); break;
    case 8: controller.request(DisplayRequestSource::SETUP_TIMEOUT, DisplayMode::SETUP_DIMMED); break;
    default: break;
  }
}

static DisplayMode expected_mode_for_priority(int priority) {
  const DisplayMode modes[] = {
      DisplayMode::ACTIVE, DisplayMode::ACTIVE, DisplayMode::DISPLAY_OFF,
      DisplayMode::ACTIVE, DisplayMode::CLOCK, DisplayMode::ACTIVE,
      DisplayMode::COVER_ART, DisplayMode::DIMMED, DisplayMode::SETUP_DIMMED};
  return modes[priority];
}

int main() {
  DisplayModeController controller;
  CHECK(decision_is(controller, DisplayMode::ACTIVE));
  CHECK(controller.target_mode_is(DisplayMode::ACTIVE));
  CHECK(controller.current_mode_is(DisplayMode::ACTIVE));
  CHECK(!controller.transition_required(controller.resolve()));
  CHECK(!presence_can_wake_display(controller.resolve()));

  for (DisplayMode mode : {DisplayMode::DISPLAY_OFF, DisplayMode::DIMMED,
                           DisplayMode::CLOCK, DisplayMode::PHOTOS}) {
    DisplayModeController presence_wake;
    CHECK(presence_wake.request(DisplayRequestSource::PRESENCE_SENSOR, mode));
    CHECK(presence_can_wake_display(presence_wake.resolve()));
  }
  DisplayModeController idle_wake;
  CHECK(idle_wake.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::DIMMED));
  CHECK(presence_can_wake_display(idle_wake.resolve()));
  DisplayModeController cover_art_presence;
  CHECK(cover_art_presence.request(DisplayRequestSource::MEDIA_PLAYBACK,
                                   DisplayMode::COVER_ART));
  CHECK(!presence_can_wake_display(cover_art_presence.resolve()));
  DisplayModeController scheduled_presence;
  CHECK(scheduled_presence.request(DisplayRequestSource::SCREEN_SCHEDULE,
                                   DisplayMode::CLOCK));
  CHECK(!presence_can_wake_display(scheduled_presence.resolve()));

  // Every higher-priority policy beats every lower-priority policy.
  for (int higher = 1; higher <= 8; ++higher) {
    for (int lower = higher + 1; lower <= 9; ++lower) {
      DisplayModeController pair;
      activate_priority(pair, lower);
      activate_priority(pair, higher);
      CHECK(pair.resolve().target_mode == expected_mode_for_priority(higher));
    }
  }

  CHECK(controller.request(DisplayRequestSource::SETUP_TIMEOUT, DisplayMode::SETUP_DIMMED));
  CHECK(controller.target_source_is(DisplayRequestSource::SETUP_TIMEOUT));
  CHECK(!controller.current_source_is(DisplayRequestSource::SETUP_TIMEOUT));
  CHECK(controller.transition_required(controller.resolve()));
  CHECK(decision_is(controller, DisplayMode::SETUP_DIMMED,
                    DisplayRequestSource::SETUP_TIMEOUT));
  CHECK(controller.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::CLOCK));
  CHECK(decision_is(controller, DisplayMode::CLOCK, DisplayRequestSource::IDLE_TIMER));
  CHECK(controller.request(DisplayRequestSource::PRESENCE_SENSOR, DisplayMode::DIMMED));
  CHECK(decision_is(controller, DisplayMode::DIMMED,
                    DisplayRequestSource::PRESENCE_SENSOR));
  CHECK(controller.request(DisplayRequestSource::MEDIA_PLAYBACK, DisplayMode::COVER_ART));
  CHECK(decision_is(controller, DisplayMode::COVER_ART,
                    DisplayRequestSource::MEDIA_PLAYBACK));

  CHECK(controller.begin_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(decision_is(controller, DisplayMode::ACTIVE, std::nullopt,
                    DisplayTakeoverKind::INTERACTIVE));
  CHECK(controller.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::CLOCK));
  CHECK(decision_is(controller, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(controller.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE));
  CHECK(decision_is(controller, DisplayMode::ACTIVE, DisplayRequestSource::USER_WAKE));
  CHECK(controller.request(DisplayRequestSource::MANUAL_SLEEP, DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(controller, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::MANUAL_SLEEP));

  CHECK(controller.begin_takeover(DisplayTakeoverKind::CRITICAL));
  CHECK(decision_is(controller, DisplayMode::ACTIVE, std::nullopt,
                    DisplayTakeoverKind::CRITICAL));
  CHECK(controller.end_takeover(DisplayTakeoverKind::CRITICAL));
  CHECK(decision_is(controller, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::MANUAL_SLEEP));
  CHECK(controller.clear(DisplayRequestSource::MANUAL_SLEEP));
  CHECK(decision_is(controller, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(controller.clear(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(decision_is(controller, DisplayMode::ACTIVE, std::nullopt,
                    DisplayTakeoverKind::INTERACTIVE));
  CHECK(controller.end_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(decision_is(controller, DisplayMode::COVER_ART,
                    DisplayRequestSource::MEDIA_PLAYBACK));

  // Boot guard is the schedule-level fail-dark request and rejects invalid time.
  CHECK(controller.request(DisplayRequestSource::BOOT_GUARD, DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(controller, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::BOOT_GUARD));
  CHECK(!controller.request(DisplayRequestSource::BOOT_GUARD, DisplayMode::CLOCK));
  CHECK(controller.clear(DisplayRequestSource::BOOT_GUARD));

  // Manual sleep removes a temporary wake regardless of request order, so
  // clearing manual sleep always re-resolves the live schedule.
  DisplayModeController manual_sleep;
  CHECK(manual_sleep.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::CLOCK));
  CHECK(manual_sleep.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE));
  CHECK(manual_sleep.request(DisplayRequestSource::MANUAL_SLEEP, DisplayMode::DISPLAY_OFF));
  CHECK(manual_sleep.clear(DisplayRequestSource::MANUAL_SLEEP));
  CHECK(decision_is(manual_sleep, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(manual_sleep.request(DisplayRequestSource::MANUAL_SLEEP, DisplayMode::DISPLAY_OFF));
  CHECK(!manual_sleep.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE));
  CHECK(manual_sleep.clear(DisplayRequestSource::MANUAL_SLEEP));
  CHECK(decision_is(manual_sleep, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));

  // Home Assistant backlight OFF is manual sleep even while an absent room is
  // showing the presence-owned clock. Clearing it restores the live sensor mode.
  DisplayModeController presence_manual_sleep;
  CHECK(presence_manual_sleep.request(DisplayRequestSource::PRESENCE_SENSOR,
                                      DisplayMode::CLOCK));
  CHECK(presence_manual_sleep.request(DisplayRequestSource::MANUAL_SLEEP,
                                      DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(presence_manual_sleep, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::MANUAL_SLEEP));
  CHECK(presence_manual_sleep.clear(DisplayRequestSource::MANUAL_SLEEP));
  CHECK(decision_is(presence_manual_sleep, DisplayMode::CLOCK,
                    DisplayRequestSource::PRESENCE_SENSOR));

  // Every request source accepts its documented mode and returns to default
  // after its clear path when tested independently.
  struct SourceMode { DisplayRequestSource source; DisplayMode mode; };
  const SourceMode clear_paths[] = {
      {DisplayRequestSource::BOOT_GUARD, DisplayMode::DISPLAY_OFF},
      {DisplayRequestSource::IDLE_TIMER, DisplayMode::DIMMED},
      {DisplayRequestSource::PRESENCE_SENSOR, DisplayMode::CLOCK},
      {DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::ACTIVE},
      {DisplayRequestSource::MANUAL_SLEEP, DisplayMode::DISPLAY_OFF},
      {DisplayRequestSource::MEDIA_PLAYBACK, DisplayMode::COVER_ART},
      {DisplayRequestSource::SETUP_TIMEOUT, DisplayMode::SETUP_DIMMED},
      {DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE},
  };
  for (const auto &path : clear_paths) {
    DisplayModeController isolated;
    CHECK(isolated.request(path.source, path.mode));
    CHECK(isolated.resolve().winning_source == path.source);
    CHECK(isolated.clear(path.source));
    CHECK(decision_is(isolated, DisplayMode::ACTIVE));
  }

  // Cover art is a media request, not a saved presentation. Higher-priority
  // requests temporarily hide it, and clearing media restores whatever is
  // currently underneath rather than assuming the active UI.
  DisplayModeController cover_art;
  const uint32_t activation_generation = cover_art.generation();
  CHECK(cover_art.generation_is_current(activation_generation));
  CHECK(cover_art.request(DisplayRequestSource::MEDIA_PLAYBACK,
                          DisplayMode::COVER_ART));
  const auto first_art = cover_art.resolve();
  CHECK(decision_is(cover_art, DisplayMode::COVER_ART,
                    DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(!cover_art.generation_is_current(activation_generation));
  CHECK(cover_art.complete_transition(first_art));

  CHECK(cover_art.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::DIMMED));
  const uint32_t artwork_generation = cover_art.generation();
  const auto cover_over_idle = cover_art.resolve();
  CHECK(decision_is(cover_art, DisplayMode::COVER_ART,
                    DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(!cover_art.transition_required(cover_over_idle));
  CHECK(cover_art.request(DisplayRequestSource::SCREEN_SCHEDULE,
                          DisplayMode::DISPLAY_OFF));
  CHECK(!cover_art.generation_is_current(artwork_generation));
  CHECK(decision_is(cover_art, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(cover_art.clear(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(decision_is(cover_art, DisplayMode::COVER_ART,
                    DisplayRequestSource::MEDIA_PLAYBACK));

  const uint32_t dismissed_generation = cover_art.generation();
  CHECK(cover_art.clear(DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(!cover_art.transition_is_current(dismissed_generation,
                                         DisplayMode::COVER_ART));
  CHECK(decision_is(cover_art, DisplayMode::DIMMED,
                    DisplayRequestSource::IDLE_TIMER));
  CHECK(cover_art.request(DisplayRequestSource::MEDIA_PLAYBACK,
                          DisplayMode::COVER_ART));
  const uint32_t replacement_generation = cover_art.generation();
  CHECK(replacement_generation > dismissed_generation);
  CHECK(!cover_art.transition_is_current(dismissed_generation,
                                         DisplayMode::COVER_ART));
  CHECK(cover_art.transition_is_current(replacement_generation,
                                        DisplayMode::COVER_ART));

  // Each effective change invalidates older delayed work, including cover-art work.
  const DisplayTransition stale_cover_art = controller.resolve();
  CHECK(controller.clear(DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(!controller.complete_transition(stale_cover_art));
  const DisplayTransition idle_transition = controller.resolve();
  CHECK(controller.complete_transition(idle_transition));
  CHECK(controller.current_mode() == DisplayMode::DIMMED);
  CHECK(!controller.clear(DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(!controller.request(DisplayRequestSource::PRESENCE_SENSOR, DisplayMode::DIMMED));

  DisplayModeController rapid;
  CHECK(rapid.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::DISPLAY_OFF));
  const auto off_generation = rapid.resolve();
  CHECK(rapid.transition_is_current(off_generation.generation, DisplayMode::DISPLAY_OFF));
  CHECK(rapid.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::CLOCK));
  const auto clock_generation = rapid.resolve();
  CHECK(clock_generation.generation > off_generation.generation);
  CHECK(!rapid.transition_is_current(off_generation.generation, DisplayMode::DISPLAY_OFF));
  CHECK(rapid.transition_is_current(clock_generation.generation, DisplayMode::CLOCK));
  CHECK(!rapid.complete_transition(off_generation));
  CHECK(rapid.complete_transition(clock_generation.generation, DisplayMode::CLOCK));
  CHECK(!rapid.transition_required(rapid.resolve()));

  // Display-off lifecycle sequences always re-resolve current requests rather
  // than restoring the presentation that happened to be visible before them.
  DisplayModeController lifecycle;
  CHECK(lifecycle.request(DisplayRequestSource::PRESENCE_SENSOR,
                          DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(lifecycle, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::PRESENCE_SENSOR));
  CHECK(lifecycle.clear(DisplayRequestSource::PRESENCE_SENSOR));
  CHECK(decision_is(lifecycle, DisplayMode::ACTIVE));

  CHECK(lifecycle.request(DisplayRequestSource::SCREEN_SCHEDULE,
                          DisplayMode::DISPLAY_OFF));
  CHECK(lifecycle.request_active(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(lifecycle.request(DisplayRequestSource::IDLE_TIMER,
                          DisplayMode::DISPLAY_OFF));
  CHECK(lifecycle.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE));
  CHECK(decision_is(lifecycle, DisplayMode::ACTIVE,
                    DisplayRequestSource::USER_WAKE));
  CHECK(lifecycle.clear(DisplayRequestSource::USER_WAKE));
  CHECK(decision_is(lifecycle, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  // A scheduled morning wake clears the automatic request that was hidden
  // beneath the higher-priority night schedule.
  CHECK(lifecycle.clear(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(!lifecycle.request_active(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(lifecycle.clear(DisplayRequestSource::IDLE_TIMER));
  CHECK(decision_is(lifecycle, DisplayMode::ACTIVE));
  CHECK(lifecycle.request(DisplayRequestSource::SCREEN_SCHEDULE,
                          DisplayMode::DISPLAY_OFF));
  CHECK(lifecycle.request(DisplayRequestSource::SCREEN_SCHEDULE,
                          DisplayMode::CLOCK));
  CHECK(decision_is(lifecycle, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));

  // Invalid boot time fails dark even when the live night schedule would
  // otherwise select a clock presentation.
  CHECK(lifecycle.request(DisplayRequestSource::BOOT_GUARD,
                          DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(lifecycle, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::BOOT_GUARD));
  CHECK(lifecycle.clear(DisplayRequestSource::BOOT_GUARD));
  CHECK(decision_is(lifecycle, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));

  // A source change at the same visible mode still needs the adapter so it can
  // select the winning source's brightness and compatibility state.
  CHECK(rapid.clear(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(rapid.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::CLOCK));
  const auto idle_clock_generation = rapid.resolve();
  CHECK(rapid.transition_required(idle_clock_generation));
  CHECK(rapid.complete_transition(idle_clock_generation));
  CHECK(rapid.current_source() == DisplayRequestSource::IDLE_TIMER);
  CHECK(!rapid.current_takeover().has_value());

  // Requests continue to change while an interactive image modal is open.
  // Automatic idle and media remain deferred, while schedule/manual sleep can
  // still replace the modal; releasing the takeover resolves live state.
  DisplayModeController takeover;
  CHECK(takeover.begin_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(takeover.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::DIMMED));
  CHECK(takeover.request(DisplayRequestSource::MEDIA_PLAYBACK, DisplayMode::COVER_ART));
  CHECK(decision_is(takeover, DisplayMode::ACTIVE, std::nullopt,
                    DisplayTakeoverKind::INTERACTIVE));
  CHECK(takeover.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::CLOCK));
  CHECK(decision_is(takeover, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(takeover.clear(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(decision_is(takeover, DisplayMode::ACTIVE, std::nullopt,
                    DisplayTakeoverKind::INTERACTIVE));
  CHECK(takeover.clear(DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(takeover.end_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(decision_is(takeover, DisplayMode::DIMMED,
                    DisplayRequestSource::IDLE_TIMER));

  // Critical alarm takeovers remain visible while schedule, presence, media,
  // and manual requests change, then release to the current winner.
  CHECK(takeover.begin_takeover(DisplayTakeoverKind::CRITICAL));
  CHECK(takeover.request(DisplayRequestSource::PRESENCE_SENSOR,
                         DisplayMode::DISPLAY_OFF));
  CHECK(takeover.request(DisplayRequestSource::MEDIA_PLAYBACK,
                         DisplayMode::COVER_ART));
  CHECK(takeover.request(DisplayRequestSource::SCREEN_SCHEDULE,
                         DisplayMode::CLOCK));
  CHECK(takeover.request(DisplayRequestSource::MANUAL_SLEEP,
                         DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(takeover, DisplayMode::ACTIVE, std::nullopt,
                    DisplayTakeoverKind::CRITICAL));
  CHECK(takeover.clear(DisplayRequestSource::MANUAL_SLEEP));
  CHECK(takeover.end_takeover(DisplayTakeoverKind::CRITICAL));
  CHECK(decision_is(takeover, DisplayMode::CLOCK,
                    DisplayRequestSource::SCREEN_SCHEDULE));

  // The photo screensaver is an idle-owned presentation: only the idle timer and
  // the presence sensor may request it, and it loses to the same higher-priority
  // requests as clock.
  CHECK(DisplayModeController::request_is_valid(DisplayRequestSource::IDLE_TIMER,
                                                DisplayMode::PHOTOS));
  CHECK(DisplayModeController::request_is_valid(
      DisplayRequestSource::PRESENCE_SENSOR, DisplayMode::PHOTOS));
  for (DisplayRequestSource source :
       {DisplayRequestSource::SCREEN_SCHEDULE, DisplayRequestSource::MANUAL_SLEEP,
        DisplayRequestSource::MEDIA_PLAYBACK, DisplayRequestSource::USER_WAKE,
        DisplayRequestSource::BOOT_GUARD, DisplayRequestSource::SETUP_TIMEOUT}) {
    CHECK(!DisplayModeController::request_is_valid(source, DisplayMode::PHOTOS));
  }

  DisplayModeController photos;
  CHECK(photos.request(DisplayRequestSource::IDLE_TIMER, DisplayMode::PHOTOS));
  CHECK(decision_is(photos, DisplayMode::PHOTOS, DisplayRequestSource::IDLE_TIMER));

  // Eligible media outranks the photo screensaver.
  CHECK(photos.request(DisplayRequestSource::MEDIA_PLAYBACK, DisplayMode::COVER_ART));
  CHECK(decision_is(photos, DisplayMode::COVER_ART,
                    DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(photos.clear(DisplayRequestSource::MEDIA_PLAYBACK));
  CHECK(decision_is(photos, DisplayMode::PHOTOS, DisplayRequestSource::IDLE_TIMER));

  // A schedule request closes the photo screensaver; clearing it resolves back
  // from live inputs rather than a saved presentation.
  CHECK(photos.request(DisplayRequestSource::SCREEN_SCHEDULE, DisplayMode::DISPLAY_OFF));
  CHECK(decision_is(photos, DisplayMode::DISPLAY_OFF,
                    DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(photos.clear(DisplayRequestSource::SCREEN_SCHEDULE));
  CHECK(decision_is(photos, DisplayMode::PHOTOS, DisplayRequestSource::IDLE_TIMER));

  // A deliberate wake leaves the photo screensaver for the active UI.
  CHECK(photos.request(DisplayRequestSource::USER_WAKE, DisplayMode::ACTIVE));
  CHECK(decision_is(photos, DisplayMode::ACTIVE, DisplayRequestSource::USER_WAKE));

  // Nested takeovers only finish when every owner has ended its takeover.
  CHECK(controller.begin_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(controller.begin_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(controller.end_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(controller.takeover_active(DisplayTakeoverKind::INTERACTIVE));
  CHECK(controller.end_takeover(DisplayTakeoverKind::INTERACTIVE));
  CHECK(!controller.end_takeover(DisplayTakeoverKind::INTERACTIVE));

  return EXIT_SUCCESS;
}
