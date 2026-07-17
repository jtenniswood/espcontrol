#pragma once

#include <cstddef>
#include <cstdint>

namespace esphome {
namespace artwork_image {

// Higher-priority requests win. Equal-priority requests keep their original
// submission order so a busy camera page cannot starve its first tile.
constexpr bool p4_pipeline_candidate_precedes(uint8_t candidate_priority,
                                              uint64_t candidate_sequence,
                                              uint8_t best_priority,
                                              uint64_t best_sequence) {
  return candidate_priority > best_priority ||
         (candidate_priority == best_priority && candidate_sequence < best_sequence);
}

// A completion is safe to publish only while it still belongs to the current
// request generation. Closing a modal or changing cards advances the generation.
constexpr bool p4_pipeline_result_is_current(uint32_t expected_generation,
                                             uint32_t result_generation,
                                             bool cancelled) {
  return !cancelled && expected_generation == result_generation;
}

// Home Assistant's local media proxy can provide valid image bytes while the
// ESP-IDF client reports an unknown/zero status. Match the established local
// downloader behaviour without weakening status checks for other URLs.
constexpr bool p4_pipeline_http_status_is_success(int status, bool ha_media_proxy) {
  return status == 200 || status == 304 || (status <= 0 && ha_media_proxy);
}

// Modal work may cancel an active or queued tile to become responsive. The
// interrupted tile still needs another turn or its card can remain blank after
// the modal closes.
constexpr bool image_pipeline_should_requeue_interrupted_tile(bool was_active_or_queued,
                                                              bool context_active,
                                                              bool has_source_url) {
  return was_active_or_queued && context_active && has_source_url;
}

// A modal-quality image is reusable only while every part of its cache key
// still matches. Camera and image entities can keep the same entity ID while
// publishing a new source URL.
constexpr bool image_pipeline_modal_cache_matches(bool ready, bool same_image,
                                                   bool same_entity, bool same_source) {
  return ready && same_image && same_entity && same_source;
}

// A modal needs either a usable tile preview or a source it can request. With
// neither, opening it would leave an unfinishable loading state on screen.
constexpr bool image_pipeline_modal_can_open(bool tile_ready, bool has_source_url) {
  return tile_ready || has_source_url;
}

// Camera cards share one modal-quality image buffer. A delayed cleanup from a
// previously closed card must not cancel that buffer after another card starts
// using it.
constexpr bool image_pipeline_should_cancel_modal_cleanup(bool has_separate_modal_image,
                                                           bool shared_modal_in_use) {
  return has_separate_modal_image && !shared_modal_in_use;
}

// LVGL cover alignment stores scale as a whole number of fixed-point steps.
// When that scale is rounded down, the rendered image can finish one pixel
// short of the widget edge. Return the small horizontal overscan needed for a
// clipped image widget to cover the intended target width.
constexpr uint32_t cover_alignment_edge_overscan(uint32_t source_width,
                                                 uint32_t source_height,
                                                 uint32_t target_width,
                                                 uint32_t target_height,
                                                 uint32_t fractional_steps) {
  if (source_width == 0 || source_height == 0 || target_width == 0 ||
      target_height == 0 || fractional_steps == 0) {
    return 0;
  }
  uint32_t scale_x = static_cast<uint32_t>(
      static_cast<uint64_t>(target_width) * fractional_steps / source_width);
  uint32_t scale_y = static_cast<uint32_t>(
      static_cast<uint64_t>(target_height) * fractional_steps / source_height);
  uint32_t scale = scale_x > scale_y ? scale_x : scale_y;
  uint32_t rendered_width = static_cast<uint32_t>(
      static_cast<uint64_t>(source_width) * scale / fractional_steps);
  if (rendered_width >= target_width) return 0;

  uint32_t required_scale = static_cast<uint32_t>(
      (static_cast<uint64_t>(target_width) * fractional_steps + source_width - 1) /
      source_width);
  uint32_t expanded_width = static_cast<uint32_t>(
      (static_cast<uint64_t>(required_scale) * source_width + fractional_steps - 1) /
      fractional_steps);
  return expanded_width > target_width ? expanded_width - target_width : 0;
}

// A newly opened card should preempt modal-quality work left by a different
// card that is still inside its delayed cleanup window. Matching the shared
// image buffer prevents unrelated artwork downloads from being cancelled.
constexpr bool image_pipeline_should_preempt_stale_modal(bool switching_context,
                                                          bool previous_context_active,
                                                          bool previous_cleanup_pending,
                                                          bool shares_modal_image) {
  return switching_context && previous_context_active && previous_cleanup_pending &&
         shares_modal_image;
}

// Starting the next queued tile inline is safe only when download and decode
// work run on the background pipeline. Modal requests are still deferred so
// LVGL can paint the cached preview before full-resolution work starts.
constexpr bool image_pipeline_can_start_followup_inline(bool background_pipeline) {
  return background_pipeline;
}

// Reserve a known HTTP response in one allocation. Chunked responses and
// inaccurate Content-Length values retain bounded geometric growth.
constexpr size_t p4_pipeline_transfer_capacity(size_t current_capacity,
                                               size_t required_capacity,
                                               size_t reported_content_length,
                                               size_t initial_capacity,
                                               size_t maximum_capacity) {
  if (required_capacity > maximum_capacity || initial_capacity == 0) return 0;
  size_t next_capacity = current_capacity;
  if (next_capacity == 0) {
    if (reported_content_length > maximum_capacity) return 0;
    next_capacity = reported_content_length >= required_capacity &&
                            reported_content_length <= maximum_capacity
                        ? reported_content_length
                        : initial_capacity;
  }
  while (next_capacity < required_capacity && next_capacity < maximum_capacity) {
    next_capacity = next_capacity > maximum_capacity / 2
                        ? maximum_capacity
                        : next_capacity * 2;
  }
  return next_capacity >= required_capacity ? next_capacity : 0;
}

// A cached tile remains a useful immediate preview after a grid rebuild, but a
// different target size must bypass the recent-URL suppression and refresh in
// the background.
constexpr bool image_pipeline_cached_target_changed(bool image_ready,
                                                    int previous_width,
                                                    int previous_height,
                                                    int current_width,
                                                    int current_height) {
  return image_ready && current_width > 0 && current_height > 0 &&
         (previous_width != current_width || previous_height != current_height);
}

// Resizing can skip its scale/crop work only when the source and target have
// the same aspect ratio. A square source still needs cropping for a rectangular
// target even though its own width and height are equal.
constexpr bool image_resize_aspect_differs(int source_width, int source_height,
                                           int target_width, int target_height) {
  if (source_width <= 0 || source_height <= 0 || target_width <= 0 || target_height <= 0) {
    return false;
  }
  return static_cast<int64_t>(source_width) * target_height !=
         static_cast<int64_t>(source_height) * target_width;
}

struct P4CoverScalePlan {
  bool valid{false};
  uint32_t crop_width{0};
  uint32_t crop_height{0};
  uint32_t crop_x{0};
  uint32_t crop_y{0};
  uint32_t scale_units{0};
};

// PPA represents the fractional part of its scale in fixed-size steps and
// truncates arbitrary floating-point ratios. Choose a centred crop that maps
// to the exact target dimensions after that quantisation, so no unwritten
// pixels remain along the right or bottom edge of a cover image.
constexpr P4CoverScalePlan p4_cover_scale_plan(uint32_t source_width,
                                               uint32_t source_height,
                                               uint32_t target_width,
                                               uint32_t target_height,
                                               uint32_t fractional_steps,
                                               uint32_t max_scale_units) {
  P4CoverScalePlan plan{};
  if (source_width == 0 || source_height == 0 || target_width == 0 ||
      target_height == 0 || fractional_steps == 0 || max_scale_units == 0) {
    return plan;
  }

  uint32_t cover_width = source_width;
  uint32_t cover_height = source_height;
  if (static_cast<uint64_t>(source_width) * target_height >
      static_cast<uint64_t>(source_height) * target_width) {
    cover_width = static_cast<uint32_t>(
        static_cast<uint64_t>(source_height) * target_width / target_height);
    if (cover_width == 0) cover_width = 1;
  } else {
    cover_height = static_cast<uint32_t>(
        static_cast<uint64_t>(source_width) * target_height / target_width);
    if (cover_height == 0) cover_height = 1;
  }

  auto ceil_div = [](uint64_t numerator, uint64_t denominator) constexpr -> uint32_t {
    return static_cast<uint32_t>((numerator + denominator - 1) / denominator);
  };
  uint32_t min_scale_x = ceil_div(
      static_cast<uint64_t>(target_width) * fractional_steps, cover_width);
  uint32_t min_scale_y = ceil_div(
      static_cast<uint64_t>(target_height) * fractional_steps, cover_height);
  uint32_t first_scale = min_scale_x > min_scale_y ? min_scale_x : min_scale_y;
  if (first_scale == 0) first_scale = 1;

  for (uint32_t scale_units = first_scale; scale_units <= max_scale_units;
       scale_units++) {
    uint32_t crop_width = ceil_div(
        static_cast<uint64_t>(target_width) * fractional_steps, scale_units);
    uint32_t crop_height = ceil_div(
        static_cast<uint64_t>(target_height) * fractional_steps, scale_units);
    if (crop_width > cover_width || crop_height > cover_height) continue;
    if (static_cast<uint64_t>(crop_width) * scale_units / fractional_steps !=
            target_width ||
        static_cast<uint64_t>(crop_height) * scale_units / fractional_steps !=
            target_height) {
      continue;
    }
    plan.valid = true;
    plan.crop_width = crop_width;
    plan.crop_height = crop_height;
    plan.crop_x = (source_width - crop_width) / 2;
    plan.crop_y = (source_height - crop_height) / 2;
    plan.scale_units = scale_units;
    return plan;
  }
  return plan;
}

// The P4 decoder emits packed RGB565 pixels. Other configured target formats
// must stay on the software path, which performs the required conversion.
constexpr bool p4_jpeg_hardware_target_supported(bool target_is_rgb565) {
  return target_is_rgb565;
}

}  // namespace artwork_image
}  // namespace esphome
