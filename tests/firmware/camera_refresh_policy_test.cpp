#include <cassert>
#include <cstdint>
#include "camera_refresh_policy.h"

using namespace espcontrol::camera;

int main() {
  assert(valid_trigger("binary_sensor.front_door_motion"));
  assert(valid_trigger("event.doorbell"));
  for (const char *bad : {"", "camera.front", "event.", "event.Door", "event.x,y", "binary_sensor.x=y"})
    assert(!valid_trigger(bad));
  assert(refresh_mode("timer") == RefreshMode::OFF);
  assert(refresh_interval_ms("invalid") == 10000);

  RefreshSchedule periodic;
  periodic.mode = RefreshMode::PERIODIC;
  periodic.begin(100, false);
  assert(periodic.due(100, true));
  periodic.started();
  assert(!periodic.due(60000, true)); // A slow download must never create a backlog.
  periodic.finished(60000, true);
  assert(!periodic.due(69999, true));
  assert(periodic.due(70000, true));
  assert(!periodic.due(70000, false));
  periodic.close();
  assert(!periodic.due(100000, true));

  RefreshSchedule activity;
  activity.mode = RefreshMode::ACTIVITY;
  activity.begin(100, false);
  assert(!activity.due(100, true));
  activity.started(); // Initial open fetch is independent of mode.
  activity.finished(500, true);
  activity.activate(1000);
  assert(!activity.due(1000, true)); // Reuse a very recent snapshot.
  assert(activity.due(5500, true));
  activity.started();
  activity.activate(10000); // Extend the window, without restarting an active download.
  assert(!activity.due(12000, true));
  activity.finished(12000, true);
  assert(!activity.due(16999, true));
  assert(activity.due(17000, true));
  assert(activity.enabled(39999));
  assert(!activity.enabled(40000));
  activity.close();
  activity.activate(50000); // Hidden activity cannot queue work for later.
  assert(!activity.window);
  activity.begin(60000, true); // Opening during known active motion starts one window.
  assert(activity.enabled(60000));
  assert(!activity.enabled(90000));
  activity.begin(100000, false);
  assert(!activity.enabled(100000));

  RefreshSchedule failures;
  failures.mode = RefreshMode::PERIODIC;
  failures.interval_ms = 5000;
  failures.begin(0, false);
  for (uint32_t delay : {5000U, 10000U, 20000U, 30000U, 30000U}) {
    const uint32_t now = failures.next_due;
    failures.started();
    failures.finished(now, false);
    assert(failures.next_due == now + delay);
  }
  failures.finished(failures.next_due, true);
  assert(failures.failures == 0);
  failures.interval_ms = 30000;
  failures.finished(500000, false);
  assert(failures.next_due == 530000); // Errors never speed up a slower configured interval.

  activity.begin(100, false);
  activity.activate(100);
  activity.started();
  activity.finished(200, false);
  activity.activate(201);
  assert(!activity.due(201, true)); // Trigger storms cannot defeat backoff.
  assert(activity.due(5200, true));
  activity.finished(10000, false);
  activity.finished(20000, false);
  assert(!activity.due(40000, true)); // Backoff cannot prolong the activity window.

  ActivityTrigger motion;
  assert(!motion.observe("on", true, 1)); // Retained baseline is not an edge.
  assert(motion.on(1));
  assert(!motion.observe("on", true, 1));
  assert(!motion.observe("off", true, 1));
  assert(motion.observe("on", true, 1));
  assert(!motion.observe("unknown", true, 1));
  assert(!motion.on(1));
  assert(!motion.observe("on", true, 1));
  assert(!motion.observe("off", true, 1));
  assert(!motion.observe("on", true, 2)); // Reconnect snapshot must not replay activity.
  assert(!motion.on(1));

  ActivityTrigger doorbell;
  assert(!doorbell.observe("2026-09-10T10:00:00+00:00", false, 1));
  assert(!doorbell.observe("2026-09-10T10:00:00+00:00", false, 1));
  assert(doorbell.observe("2026-09-10T10:01:00+00:00", false, 1));
  assert(!doorbell.observe("2026-09-10T10:02:00+00:00", false, 2));
  assert(!doorbell.observe("unavailable", false, 2));
  assert(!doorbell.observe("2026-09-10T10:03:00+00:00", false, 2));

  ImageRevision image;
  assert(image.observe("2026-09-10T10:00:00+00:00"));
  image.tile_requested = image.latest;
  assert(image.observe("2026-09-10T10:01:00+00:00"));
  assert(image.observe("2026-09-10T10:02:00+00:00"));
  image.tile_applied = image.tile_requested;
  assert(image.tile_dirty()); // Completion of an older revision cannot acknowledge newer ones.
  image.tile_requested = image.latest;
  image.tile_applied = image.tile_requested;
  assert(!image.tile_dirty()); // One followup acknowledges all coalesced revisions.
  assert(!image.observe("2026-09-10T10:02:00+00:00"));
  assert(!image.observe("unknown"));
  assert(image.modal_dirty());
  image.modal_requested = image.latest;
  image.modal_applied = image.modal_requested;
  assert(!image.modal_dirty());

  const uint32_t near_wrap = UINT32_MAX - 1000;
  periodic.begin(near_wrap, false);
  periodic.started();
  periodic.finished(near_wrap, true);
  assert(!periodic.due(near_wrap + 9999, true));
  assert(periodic.due(near_wrap + 10000, true));
  activity.begin(near_wrap, false);
  activity.activate(near_wrap);
  assert(activity.enabled(near_wrap + 29999));
  assert(!activity.enabled(near_wrap + 30000));
}
