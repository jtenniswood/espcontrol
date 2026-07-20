#include <cassert>

#include "image_pipeline_policy.h"

using esphome::artwork_image::jpeg_normalize_for_hardware;
using esphome::artwork_image::p4_pipeline_candidate_precedes;
using esphome::artwork_image::p4_pipeline_http_status_is_success;
using esphome::artwork_image::p4_pipeline_result_is_current;
using esphome::artwork_image::image_pipeline_should_requeue_interrupted_tile;
using esphome::artwork_image::image_pipeline_completion_needs_recovery;
using esphome::artwork_image::image_pipeline_modal_can_open;
using esphome::artwork_image::image_pipeline_modal_cache_matches;
using esphome::artwork_image::image_pipeline_can_start_followup_inline;
using esphome::artwork_image::image_pipeline_cached_target_changed;
using esphome::artwork_image::image_resize_aspect_differs;
using esphome::artwork_image::image_pipeline_should_cancel_modal_cleanup;
using esphome::artwork_image::image_pipeline_should_preempt_stale_modal;
using esphome::artwork_image::p4_pipeline_transfer_capacity;
using esphome::artwork_image::cover_alignment_edge_overscan;
using esphome::artwork_image::p4_cover_scale_plan;
using esphome::artwork_image::p4_jpeg_hardware_target_supported;

int main() {
  // Modal work preempts queued tile work.
  assert(p4_pipeline_candidate_precedes(2, 20, 1, 10));
  assert(!p4_pipeline_candidate_precedes(1, 10, 2, 20));

  // Tiles at the same priority remain first-in, first-out.
  assert(p4_pipeline_candidate_precedes(1, 10, 1, 20));
  assert(!p4_pipeline_candidate_precedes(1, 20, 1, 10));

  // Cancelled and superseded downloads can never replace the visible image.
  assert(p4_pipeline_result_is_current(4, 4, false));
  assert(!p4_pipeline_result_is_current(4, 3, false));
  assert(!p4_pipeline_result_is_current(4, 4, true));

  // Only Home Assistant's media proxy may return valid bytes with status 0.
  assert(p4_pipeline_http_status_is_success(200, false));
  assert(p4_pipeline_http_status_is_success(304, false));
  assert(p4_pipeline_http_status_is_success(0, true));
  assert(!p4_pipeline_http_status_is_success(0, false));
  assert(!p4_pipeline_http_status_is_success(404, true));

  // Preemption must requeue both active tiles and the selected card when it was
  // waiting in the tile queue. Inactive, source-less, or idle work is discarded.
  assert(image_pipeline_should_requeue_interrupted_tile(true, true, true));
  assert(!image_pipeline_should_requeue_interrupted_tile(false, true, true));
  assert(!image_pipeline_should_requeue_interrupted_tile(true, false, true));
  assert(!image_pipeline_should_requeue_interrupted_tile(true, true, false));

  // A completed startup image is recovered only while the card has not yet
  // applied it and the downloader still represents the card's current URL.
  assert(image_pipeline_completion_needs_recovery(false, true, true));
  assert(!image_pipeline_completion_needs_recovery(true, true, true));
  assert(!image_pipeline_completion_needs_recovery(false, false, true));
  assert(!image_pipeline_completion_needs_recovery(false, true, false));

  // A changed source URL invalidates an otherwise matching modal cache entry.
  assert(image_pipeline_modal_cache_matches(true, true, true, true));
  assert(!image_pipeline_modal_cache_matches(true, true, true, false));
  assert(!image_pipeline_modal_cache_matches(false, true, true, true));

  // A card without a tile or a source cannot progress beyond modal loading.
  assert(image_pipeline_modal_can_open(true, false));
  assert(image_pipeline_modal_can_open(false, true));
  assert(!image_pipeline_modal_can_open(false, false));

  // A stale card cleanup must leave the shared modal request alone while a
  // newly opened card is using it. An otherwise idle separate buffer is safe
  // to cancel.
  assert(image_pipeline_should_cancel_modal_cleanup(true, false));
  assert(!image_pipeline_should_cancel_modal_cleanup(true, true));
  assert(!image_pipeline_should_cancel_modal_cleanup(false, false));

  // The 7-inch panel compensates media artwork to 98% width. LVGL rounds its
  // cover scale down (261/256 here), rendering 194 pixels into a 195-pixel
  // card. A one-pixel clipped overscan closes that visible right-hand seam.
  assert(cover_alignment_edge_overscan(191, 178, 195, 178, 256) == 1);
  assert(cover_alignment_edge_overscan(593, 535, 605, 535, 256) == 2);
  assert(cover_alignment_edge_overscan(195, 178, 195, 178, 256) == 0);
  assert(cover_alignment_edge_overscan(0, 178, 195, 178, 256) == 0);

  // Switching cards inside the delayed cleanup window should cancel the old
  // request only when both cards use the same modal-quality image buffer.
  assert(image_pipeline_should_preempt_stale_modal(true, true, true, true));
  assert(!image_pipeline_should_preempt_stale_modal(false, true, true, true));
  assert(!image_pipeline_should_preempt_stale_modal(true, false, true, true));
  assert(!image_pipeline_should_preempt_stale_modal(true, true, false, true));
  assert(!image_pipeline_should_preempt_stale_modal(true, true, true, false));

  // The P4 background worker can accept the next queued tile immediately.
  // Modal work remains deferred separately so the preview paints first.
  assert(image_pipeline_can_start_followup_inline(true));
  assert(!image_pipeline_can_start_followup_inline(false));

  // Known-length responses reserve once instead of repeatedly copying through
  // 16, 32, 64, and larger KiB transfer buffers. Unknown or inaccurate lengths
  // retain bounded geometric growth.
  assert(p4_pipeline_transfer_capacity(0, 4096, 250000, 16384, 2 * 1024 * 1024) == 250000);
  assert(p4_pipeline_transfer_capacity(0, 4096, 0, 16384, 2 * 1024 * 1024) == 16384);
  assert(p4_pipeline_transfer_capacity(16384, 20000, 0, 16384, 2 * 1024 * 1024) == 32768);
  assert(p4_pipeline_transfer_capacity(0, 4096, 3 * 1024 * 1024, 16384,
                                       2 * 1024 * 1024) == 0);
  assert(p4_pipeline_transfer_capacity(1024 * 1024, 3 * 1024 * 1024, 0, 16384,
                                       2 * 1024 * 1024) == 0);

  // Reusing a ready tile at a different card size keeps the preview but must
  // force a correctly sized refresh. Stable or not-yet-ready targets do not.
  assert(image_pipeline_cached_target_changed(true, 320, 240, 480, 320));
  assert(!image_pipeline_cached_target_changed(true, 320, 240, 320, 240));
  assert(!image_pipeline_cached_target_changed(false, 320, 240, 480, 320));

  // Square album artwork still needs cover scaling when its card is wide or
  // tall. Matching source/target ratios can safely skip that work.
  assert(image_resize_aspect_differs(600, 600, 480, 320));
  assert(image_resize_aspect_differs(600, 600, 320, 480));
  assert(!image_resize_aspect_differs(600, 600, 320, 320));
  assert(!image_resize_aspect_differs(1200, 800, 480, 320));
  assert(image_resize_aspect_differs(1200, 800, 320, 480));

  // PPA stores scale in sixteenth-step units. The old arbitrary ratio was
  // truncated (for example 0.672 to 0.625), leaving noisy right/bottom strips.
  // The adjusted centred crop must produce every target pixel exactly.
  const auto landscape_plan = p4_cover_scale_plan(1024, 768, 688, 504, 16, 4095);
  assert(landscape_plan.valid);
  assert(landscape_plan.crop_width * landscape_plan.scale_units / 16 == 688);
  assert(landscape_plan.crop_height * landscape_plan.scale_units / 16 == 504);
  assert(landscape_plan.crop_x == (1024 - landscape_plan.crop_width) / 2);
  assert(landscape_plan.crop_y == (768 - landscape_plan.crop_height) / 2);

  const auto portrait_plan = p4_cover_scale_plan(720, 1280, 688, 504, 16, 4095);
  assert(portrait_plan.valid);
  assert(portrait_plan.crop_width * portrait_plan.scale_units / 16 == 688);
  assert(portrait_plan.crop_height * portrait_plan.scale_units / 16 == 504);
  assert(!p4_cover_scale_plan(0, 768, 688, 504, 16, 4095).valid);

  // Packed RGB565 output is safe only for RGB565 image targets. Every other
  // configured type must fall back to the format-aware software decoder.
  assert(p4_jpeg_hardware_target_supported(true));
  assert(!p4_jpeg_hardware_target_supported(false));

  // Metadata stripping drops APPn/COM segments and keeps everything else, so
  // a JPEG whose frame header hides behind fat EXIF becomes hardware-decodable.
  {
    // SOI, APP1 (6 bytes of "metadata"), DQT (2 bytes), SOS + entropy data.
    const uint8_t jpeg[] = {
        0xFF, 0xD8,
        0xFF, 0xE1, 0x00, 0x08, 'E', 'x', 'i', 'f', 0x00, 0x00,
        0xFF, 0xDB, 0x00, 0x04, 0x01, 0x02,
        0xFF, 0xDA, 0x00, 0x02, 0xAA, 0xBB, 0xCC,
    };
    uint8_t out[sizeof(jpeg)] = {};
    const size_t stripped = jpeg_normalize_for_hardware(jpeg, sizeof(jpeg), out);
    // SOI + DQT + SOS-and-rest survive; the 10-byte APP1 segment is gone.
    assert(stripped == sizeof(jpeg) - 10);
    assert(out[0] == 0xFF && out[1] == 0xD8);
    assert(out[2] == 0xFF && out[3] == 0xDB);          // DQT immediately after SOI
    assert(out[8] == 0xFF && out[9] == 0xDA);          // then SOS
    assert(out[stripped - 1] == 0xCC);                 // entropy tail intact

    // A COM segment is dropped the same way.
    const uint8_t with_comment[] = {
        0xFF, 0xD8,
        0xFF, 0xFE, 0x00, 0x04, 'h', 'i',
        0xFF, 0xDA, 0x00, 0x02, 0x01,
    };
    uint8_t out2[sizeof(with_comment)] = {};
    assert(jpeg_normalize_for_hardware(with_comment, sizeof(with_comment), out2) ==
           sizeof(with_comment) - 6);

    // Not-a-JPEG and truncated segments are rejected rather than half-copied.
    const uint8_t not_jpeg[] = {0x00, 0x11, 0x22, 0x33};
    assert(jpeg_normalize_for_hardware(not_jpeg, sizeof(not_jpeg), out2) == 0);
    const uint8_t truncated[] = {0xFF, 0xD8, 0xFF, 0xE1, 0xFF, 0xFF};
    assert(jpeg_normalize_for_hardware(truncated, sizeof(truncated), out2) == 0);
    assert(jpeg_normalize_for_hardware(nullptr, 16, out2) == 0);

    // An 8-bit SOF1 frame is relabeled to SOF0; 12-bit precision is left
    // alone so the hardware rejects it and software handles it correctly.
    const uint8_t sof1_8bit[] = {
        0xFF, 0xD8,
        0xFF, 0xC1, 0x00, 0x0B, 0x08, 0x07, 0x80, 0x05, 0xA0, 0x03, 0x01, 0x11, 0x00,
        0xFF, 0xDA, 0x00, 0x02, 0x99,
    };
    uint8_t out3[sizeof(sof1_8bit)] = {};
    assert(jpeg_normalize_for_hardware(sof1_8bit, sizeof(sof1_8bit), out3) ==
           sizeof(sof1_8bit));
    assert(out3[2] == 0xFF && out3[3] == 0xC0);  // relabeled to baseline
    const uint8_t sof1_12bit[] = {
        0xFF, 0xD8,
        0xFF, 0xC1, 0x00, 0x0B, 0x0C, 0x07, 0x80, 0x05, 0xA0, 0x03, 0x01, 0x11, 0x00,
        0xFF, 0xDA, 0x00, 0x02, 0x99,
    };
    uint8_t out4[sizeof(sof1_12bit)] = {};
    assert(jpeg_normalize_for_hardware(sof1_12bit, sizeof(sof1_12bit), out4) ==
           sizeof(sof1_12bit));
    assert(out4[2] == 0xFF && out4[3] == 0xC1);  // 12-bit stays extended
  }
}
