#include <cmath>
#include <cstdlib>
#include <string>

#include "number_slider_policy.h"

namespace {

bool close_to(double actual, double expected) {
  return std::fabs(actual - expected) < 1e-9;
}

}  // namespace

int main() {
  using namespace espcontrol::number_slider;

  if (!is_number_entity("number.boiler_target")) return EXIT_FAILURE;
  if (!is_input_number_entity("input_number.test_level")) return EXIT_FAILURE;
  if (is_numeric_entity("light.kitchen")) return EXIT_FAILURE;
  if (std::string(service_for_entity("number.boiler_target")) != "number.set_value")
    return EXIT_FAILURE;
  if (std::string(service_for_entity("input_number.test_level")) !=
      "input_number.set_value") return EXIT_FAILURE;
  if (service_for_entity("sensor.temperature") != nullptr) return EXIT_FAILURE;

  const Metadata fractional{-10.0, 10.0, 0.25};
  if (!metadata_valid(fractional)) return EXIT_FAILURE;
  if (legal_step_count(fractional) != 80) return EXIT_FAILURE;
  if (slider_position_count(fractional) != 80) return EXIT_FAILURE;
  if (!close_to(value_for_position(fractional, 0), -10.0)) return EXIT_FAILURE;
  if (!close_to(value_for_position(fractional, 41), 0.25)) return EXIT_FAILURE;
  if (!close_to(value_for_position(fractional, 80), 10.0)) return EXIT_FAILURE;
  if (position_for_value(fractional, -20.0) != 0) return EXIT_FAILURE;
  if (position_for_value(fractional, 20.0) != 80) return EXIT_FAILURE;
  if (!close_to(snap_value(fractional, 0.37), 0.25)) return EXIT_FAILURE;
  if (format_value(-0.25, fractional) != "-0.25") return EXIT_FAILURE;

  const Metadata non_zero_minimum{5.0, 8.0, 0.5};
  if (position_for_value(non_zero_minimum, 6.5) != 3) return EXIT_FAILURE;
  if (!close_to(value_for_position(non_zero_minimum, 3), 6.5)) return EXIT_FAILURE;

  const Metadata large_range{-5000.0, 15000.0, 1.0};
  if (legal_step_count(large_range) != 20000) return EXIT_FAILURE;
  if (slider_position_count(large_range) != MAX_SLIDER_POSITIONS) return EXIT_FAILURE;
  if (!close_to(value_for_position(large_range, 0), -5000.0)) return EXIT_FAILURE;
  if (!close_to(value_for_position(large_range, 5000), 5000.0)) return EXIT_FAILURE;
  if (!close_to(value_for_position(large_range, 10000), 15000.0)) return EXIT_FAILURE;
  if (position_for_value(large_range, 5000.0) != 5000) return EXIT_FAILURE;

  const Metadata range_above_int_max{0.0, 3000000000.0, 1.0};
  if (legal_step_count(range_above_int_max) != 3000000000LL) return EXIT_FAILURE;
  if (slider_position_count(range_above_int_max) != MAX_SLIDER_POSITIONS)
    return EXIT_FAILURE;
  if (!close_to(value_for_position(range_above_int_max, MAX_SLIDER_POSITIONS),
                3000000000.0)) return EXIT_FAILURE;
  if (position_for_value(range_above_int_max, 3000000000.0) !=
      MAX_SLIDER_POSITIONS) return EXIT_FAILURE;

  const Metadata precise_offset{0.25, 10.25, 1.0};
  if (decimal_places(precise_offset) != 2) return EXIT_FAILURE;
  if (format_value(value_for_position(precise_offset, 0), precise_offset) != "0.25")
    return EXIT_FAILURE;
  if (format_value(value_for_position(precise_offset, 10), precise_offset) != "10.25")
    return EXIT_FAILURE;

  const Metadata precise_negative_offset{-0.25, 9.75, 1.0};
  if (decimal_places(precise_negative_offset) != 2) return EXIT_FAILURE;
  if (format_value(value_for_position(precise_negative_offset, 1),
                   precise_negative_offset) != "0.75") return EXIT_FAILURE;

  const Metadata high_significance{16777216.0, 16777217.0, 1.0};
  if (legal_step_count(high_significance) != 1) return EXIT_FAILURE;
  if (!close_to(value_for_position(high_significance, 1), 16777217.0))
    return EXIT_FAILURE;

  if (decimal_places(1.0) != 0) return EXIT_FAILURE;
  if (decimal_places(0.1) != 1) return EXIT_FAILURE;
  if (decimal_places(0.125) != 3) return EXIT_FAILURE;
  if (decimal_places(0.0000001) != 7) return EXIT_FAILURE;
  const Metadata fine_step{0.0, 2.0, 0.0000001};
  if (format_value(1.23456789, fine_step) != "1.2345679") return EXIT_FAILURE;
  const Metadata sub_micro_range{0.0000001, 0.0000002, 0.0000001};
  if (format_value(value_for_position(sub_micro_range, 0), sub_micro_range) !=
      "0.0000001") return EXIT_FAILURE;
  if (format_value(value_for_position(sub_micro_range, 1), sub_micro_range) !=
      "0.0000002") return EXIT_FAILURE;
  const Metadata float_metadata{0.0, 1.0, static_cast<double>(0.1f)};
  if (legal_step_count(float_metadata) != 10) return EXIT_FAILURE;
  if (decimal_places(float_metadata.step) != 1) return EXIT_FAILURE;
  if (format_value(-0.00000001, {0.0, 1.0, 0.1}) != "0.0") return EXIT_FAILURE;
  if (legal_step_count({0.0, 1000.0, static_cast<double>(0.1f)}) != 10000)
    return EXIT_FAILURE;

  if (metadata_valid({0.0, 10.0, 0.0})) return EXIT_FAILURE;
  if (metadata_valid({10.0, 0.0, 1.0})) return EXIT_FAILURE;
  if (metadata_valid({0.0, 10.0, -1.0})) return EXIT_FAILURE;

  return EXIT_SUCCESS;
}
