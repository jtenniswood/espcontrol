#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <limits>

#include "backlight_fade.h"

#define CHECK(condition) do { if (!(condition)) { \
  std::fprintf(stderr, "Check failed at line %d: %s\n", __LINE__, #condition); \
  return EXIT_FAILURE; } } while (false)

static bool near(float actual, float expected) {
  return std::fabs(actual - expected) < 0.00001f;
}

int main() {
  espcontrol::BacklightFade fade;
  // Regular updates give more than the old eight discrete fade-out steps.
  fade.start(0.8f, 0.0f, 1000, 400);
  float previous = fade.level(1000);
  for (uint32_t t = 1016; t < 1400; t += 16) {
    CHECK(!fade.finished(t));
    CHECK(fade.level(t) < previous);
    CHECK(fade.level(t) > 0.0f);
    previous = fade.level(t);
  }
  CHECK(fade.finished(1400));
  CHECK(fade.level(1400) == 0.0f);

  // Rendering stalls must skip intermediate brightness values, with no
  // accumulated delay: completion follows the deadline on the next update.
  fade.start(0.8f, 0.0f, 2000, 400);
  CHECK(near(fade.level(2100), 0.6f));
  CHECK(near(fade.level(2330), 0.14f));
  CHECK(!fade.finished(2330));
  CHECK(fade.finished(2480));
  CHECK(fade.level(2480) == 0.0f);
  CHECK(fade.level(5000) == 0.0f);

  // The redraw pause counts toward clock fade-in, including a redraw that
  // consumes the entire fade budget. Never replay a backlog of dim samples.
  fade.start(0.0f, 0.35f, 6000, 300);
  CHECK(near(fade.level(6050), 0.35f / 6.0f));
  CHECK(near(fade.level(6150), 0.175f));
  CHECK(!fade.finished(6299));
  CHECK(fade.finished(6300));
  CHECK(fade.level(6800) == 0.35f);

  // A replacement fade starts a fresh timeline with its own brightness.
  fade.start(0.1f, 0.8f, 7000, 400);
  CHECK(near(fade.level(7200), 0.45f));
  fade.start(0.45f, 0.0f, 7200, 400);
  CHECK(near(fade.level(7400), 0.225f));
  CHECK(fade.finished(7600));

  // millis() wraps during long device uptimes.
  const uint32_t start = std::numeric_limits<uint32_t>::max() - 99;
  fade.start(1.0f, 0.0f, start, 400);
  CHECK(near(fade.level(100), 0.5f));
  CHECK(!fade.finished(299));
  CHECK(fade.finished(300));
  CHECK(fade.level(300) == 0.0f);

  // Zero-duration transitions must snap to the target without division by zero.
  fade.start(0.0f, 0.6f, 0, 0);
  CHECK(fade.finished(0));
  CHECK(fade.level(0) == 0.6f);
  return EXIT_SUCCESS;
}
