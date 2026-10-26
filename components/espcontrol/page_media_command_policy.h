#pragma once

#include "page_card_commands.h"

namespace espcontrol::pages {
inline std::string_view page_media_command_mode(std::string_view raw) {
  for (const auto* mode : CARD_CONTRACT_MEDIA_MODES)
    if (raw == mode) return raw;
  return CARD_CONTRACT_MEDIA_DEFAULT_MODE;
}
inline bool page_media_command_token(std::string_view options, std::string_view token) {
  for (size_t start = 0; start <= options.size();) {
    const auto end = options.find(',', start);
    if (options.substr(start, end == std::string_view::npos ? end : end - start) == token) return true;
    if (end == std::string_view::npos) break;
    start = end + 1;
  }
  return false;
}
class PageMediaCommandPolicy final : public PageCardControlPolicy {
 public:
  PageMediaCommandPolicy(PageEntityStateRuntime& graph, PageMediaStateRuntime& media)
      : graph_(graph), media_(media) {}
  bool admit(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             PageCardCommandValue& value) override {
    return valid(store, origin, kind, value);
  }
  bool valid(const PageStore& store, PageBindingToken origin, PageCardCommandKind kind,
             const PageCardCommandValue& value) override {
    if (kind != PageCardCommandKind::MEDIA_ACTION && kind != PageCardCommandKind::MEDIA_SEEK) return false;
    const ConfigurationHandle handle{origin.configuration, origin.card};
    const auto* card = store.card(handle);
    if (!card || store.field(*card, CardField::TYPE) != "media") return false;
    const auto entity = store.field(*card, CardField::ENTITY);
    if (entity.size() <= 13 || entity.substr(0, 13) != "media_player.") return false;
    const auto mode = page_media_command_mode(store.field(*card, CardField::SENSOR));
    const bool now_playing =
        mode == "now_playing" &&
        !page_media_command_token(store.field(*card, CardField::OPTIONS), MEDIA_COVER_ART_OPTION);
    if (kind == PageCardCommandKind::MEDIA_ACTION)
      return mode == "play_pause" || mode == "previous" || mode == "next" || mode == "playlist" ||
             (now_playing && store.field(*card, CardField::PRECISION) == "play_pause");
    if (mode != "position" && !(now_playing && store.field(*card, CardField::PRECISION) == "progress"))
      return false;
    // Read-only policy queries never pump producer callbacks/reannouncements.
    PageMediaPlaybackSnapshot snapshot;
    if (!media_.playback(handle, entity, snapshot) || !snapshot.available || !snapshot.has_duration ||
        !value.write_revision || snapshot.seek_revision != value.write_revision ||
        snapshot.duration != value.number || value.integer < 0 || value.integer > 100)
      return false;
    for (const auto* attr : {"", "media_duration", "media_content_id"})
      if (graph_.read_attribute(handle, entity, attr).status == PageEntityStatus::EXHAUSTED) return false;
    int seconds = 0;
    return media::seek_seconds(snapshot.duration, static_cast<uint8_t>(value.integer), seconds);
  }
  void discard(PageBindingToken, PageCardCommandKind, const PageCardCommandValue&) override {}

 private:
  PageEntityStateRuntime& graph_;
  PageMediaStateRuntime& media_;
};
}  // namespace espcontrol::pages
