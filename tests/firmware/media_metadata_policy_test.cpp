#include <cassert>

#include "media_metadata_policy.h"

int main() {
  using espcontrol::media::MediaItemKind;
  using espcontrol::media::media_item_kind;
  using espcontrol::media::media_metadata_clear_decision;
  using espcontrol::media::should_replace_media_metadata_identity;

  assert(media_item_kind("library://track/456") == MediaItemKind::TRACK);
  assert(media_item_kind("audiobookshelf://audiobook/book-id") ==
         MediaItemKind::AUDIOBOOK);
  assert(media_item_kind("library://podcast_episode/42") ==
         MediaItemKind::PODCAST);
  assert(media_item_kind("library://radio/1") == MediaItemKind::RADIO);
  assert(media_item_kind("library://movie/1") == MediaItemKind::VIDEO);
  assert(media_item_kind("library://playlist/1") == MediaItemKind::COLLECTION);
  assert(media_item_kind("https://example.test/audio", "audiobook") ==
         MediaItemKind::AUDIOBOOK);
  assert(media_item_kind("library://audiobook/1", "music") ==
         MediaItemKind::AUDIOBOOK);
  assert(media_item_kind("opaque-content-id", "music") ==
         MediaItemKind::UNKNOWN);

  const auto same_artist_track = media_metadata_clear_decision(
    "library://track/1", MediaItemKind::TRACK,
    "library://track/2", MediaItemKind::TRACK);
  assert(same_artist_track.item_changed);
  assert(same_artist_track.clear_title);
  assert(!same_artist_track.clear_grouping);

  for (MediaItemKind next : {
         MediaItemKind::AUDIOBOOK,
         MediaItemKind::PODCAST,
         MediaItemKind::RADIO,
         MediaItemKind::VIDEO,
       }) {
    const auto changed_kind = media_metadata_clear_decision(
      "library://track/1", MediaItemKind::TRACK,
      "library://item/2", next);
    assert(changed_kind.clear_title);
    assert(changed_kind.clear_grouping);
  }

  const auto unknown_kind = media_metadata_clear_decision(
    "library://track/1", MediaItemKind::TRACK,
    "opaque-content-id", MediaItemKind::UNKNOWN);
  assert(unknown_kind.clear_title);
  assert(!unknown_kind.clear_grouping);

  const auto audiobook_to_track = media_metadata_clear_decision(
    "library://audiobook/1", MediaItemKind::AUDIOBOOK,
    "library://track/2", MediaItemKind::TRACK);
  assert(audiobook_to_track.clear_title);
  assert(audiobook_to_track.clear_grouping);

  const auto unchanged_item = media_metadata_clear_decision(
    "library://track/1", MediaItemKind::TRACK,
    "library://track/1", MediaItemKind::TRACK);
  assert(!unchanged_item.item_changed);
  assert(!unchanged_item.clear_title);
  assert(!unchanged_item.clear_grouping);

  const auto missing_next_item = media_metadata_clear_decision(
    "library://track/1", MediaItemKind::TRACK,
    "", MediaItemKind::UNKNOWN);
  assert(!missing_next_item.item_changed);
  assert(!missing_next_item.clear_title);
  assert(!missing_next_item.clear_grouping);

  std::string remembered_content_id = "library://track/1";
  MediaItemKind remembered_kind = MediaItemKind::TRACK;
  assert(!should_replace_media_metadata_identity(""));
  if (should_replace_media_metadata_identity("")) {
    remembered_content_id.clear();
    remembered_kind = MediaItemKind::UNKNOWN;
  }
  const auto item_after_empty_gap = media_metadata_clear_decision(
    remembered_content_id, remembered_kind,
    "library://audiobook/2", MediaItemKind::AUDIOBOOK);
  assert(item_after_empty_gap.item_changed);
  assert(item_after_empty_gap.clear_title);
  assert(item_after_empty_gap.clear_grouping);
  assert(should_replace_media_metadata_identity("library://audiobook/2"));

  const auto initial_value = media_metadata_clear_decision(
    "", MediaItemKind::UNKNOWN,
    "library://audiobook/1", MediaItemKind::AUDIOBOOK);
  assert(!initial_value.item_changed);
  assert(!initial_value.clear_title);
  assert(!initial_value.clear_grouping);

  return 0;
}
