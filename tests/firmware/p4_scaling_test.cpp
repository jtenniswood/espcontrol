#include <cassert>

#include "p4_scaling.h"

using esphome::artwork_image::p4::exact_scale_steps;

int main() {
  assert(exact_scale_steps(0, 640) == 0);
  assert(exact_scale_steps(1024, 0) == 0);

  // Exact 1/16-step ratios may use PPA.
  assert(exact_scale_steps(1024, 640) == 10);
  assert(exact_scale_steps(800, 650) == 13);
  assert(exact_scale_steps(100, 150) == 24);
  assert(exact_scale_steps(1024, 64) == 1);

  // Common non-exact tile sizes must use the complete CPU-scaled frame.
  assert(exact_scale_steps(1024, 650) == 0);
  assert(exact_scale_steps(768, 650) == 0);

  // PPA's valid range is inclusive of 1/16 and exclusive of 16x.
  assert(exact_scale_steps(16, 255) == 255);
  assert(exact_scale_steps(64, 1024) == 0);
}
