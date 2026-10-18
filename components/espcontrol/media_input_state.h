#pragma once

#include <string_view>

#include "media_metadata_policy.h"
#include "media_playlist_match.h"

namespace espcontrol::media {
enum class InputLabel : uint8_t { NONE, TV, LINE_IN };
inline const char* input_label_text(InputLabel label) {
  return label == InputLabel::TV ? "TV" : label == InputLabel::LINE_IN ? "Line-in" : "";
}
inline bool media_kind_token_starts(std::string_view value, std::string_view prefix) {
  return value.size() >= prefix.size() && media_kind_token_is(value.substr(0, prefix.size()), prefix);
}
inline InputLabel content_input_label(std::string_view content) {
  if (media_kind_token_starts(content, "x_rincon_stream:")) return InputLabel::LINE_IN;
  if (media_kind_token_starts(content, "x_sonos_htastream:") ||
      media_kind_token_starts(content, "x_rincon_htastream:"))
    return InputLabel::TV;
  for (size_t offset = 0; offset < content.size(); ++offset)
    if (media_kind_token_starts(content.substr(offset), ":spdif")) return InputLabel::TV;
  return InputLabel::NONE;
}
inline InputLabel source_input_label(std::string_view source) {
  if (media_kind_token_is(source, "line_in")) return InputLabel::LINE_IN;
  if (media_kind_token_is(source, "tv") || media_kind_token_starts(source, "hdmi")) return InputLabel::TV;
  return InputLabel::NONE;
}
inline bool external_input_source(std::string_view source) {
  source = playlist_trim(source);
  return media_kind_token_is(source, "tv") || media_kind_token_starts(source, "hdmi") ||
         (media_kind_token_is(source, "line_in") && source[4] != '_');
}

// Configuration-owned input reconciliation. Values are already HTML-decoded and
// have unknown/unavailable mapped to empty. Raw source bytes stay in the shared
// cache; only a synthesized TV/Line-in label or an explicit clear is kept here.
struct MediaInputState {
  bool known{false}, external{false}, present{false}, source_observed{false}, content_observed{false};
  bool synthetic{false};
  InputLabel content_label{InputLabel::NONE}, override_label{InputLabel::NONE};
  std::string_view effective_source(std::string_view cached) const {
    if (!known) return {};
    return synthetic ? std::string_view(input_label_text(override_label)) : cached;
  }
  void clear_source() {
    known = external = present = source_observed = false;
    synthetic = true;
    override_label = InputLabel::NONE;
  }
  void state_observed() {
    source_observed = content_observed = false;
  }
  void invalidate_content() {
    content_observed = false;
    content_label = InputLabel::NONE;
  }
  void current_content(bool has_content) {
    if (external && !source_observed && has_content) clear_source();
  }
  // Return whether primary cover-art metadata/progress needs invalidating.
  bool source(std::string_view next, std::string_view previous_cached) {
    const bool use_content =
        content_observed && content_label != InputLabel::NONE && content_label != source_input_label(next);
    const auto reconciled = use_content ? std::string_view(input_label_text(content_label)) : next;
    const bool next_external = external_input_source(next) || use_content;
    const bool changed =
        !known || reconciled != effective_source(previous_cached) || next_external != external;
    known = source_observed = true;
    external = next_external;
    present = !reconciled.empty();
    synthetic = use_content;
    override_label = use_content ? content_label : InputLabel::NONE;
    return changed && external;
  }
  bool content(std::string_view next) {
    content_observed = !next.empty();
    const auto label = content_input_label(next);
    if (!next.empty()) content_label = label;
    if (label != InputLabel::NONE) {
      const bool changed = !external;
      const bool replace =
          media_content_id_should_replace_external_source(source_observed, external, present);
      known = external = source_observed = true;
      if (replace) {
        synthetic = present = true;
        override_label = label;
      }
      return changed;
    }
    if (external && !next.empty())
      clear_source();
    else
      current_content(!next.empty());
    return false;
  }
};
}  // namespace espcontrol::media
