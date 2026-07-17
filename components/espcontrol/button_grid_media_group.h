#pragma once

// Shared, mostly pure helpers for Home Assistant media-player grouping.
// Included by button_grid.h after the Home Assistant action boundary.

constexpr uint64_t MEDIA_PLAYER_FEATURE_GROUPING = 524288ULL;

inline std::string media_group_trim(std::string value) {
  size_t first = value.find_first_not_of(" \t\r\n");
  if (first == std::string::npos) return "";
  size_t last = value.find_last_not_of(" \t\r\n");
  value = value.substr(first, last - first + 1);
  if (value.size() >= 2 &&
      ((value.front() == '\'' && value.back() == '\'') ||
       (value.front() == '"' && value.back() == '"'))) {
    value = value.substr(1, value.size() - 2);
  }
  return value;
}

inline bool media_group_valid_entity_id(const std::string &entity_id) {
  constexpr const char *prefix = "media_player.";
  if (entity_id.rfind(prefix, 0) != 0 || entity_id.size() <= std::strlen(prefix)) return false;
  for (size_t i = std::strlen(prefix); i < entity_id.size(); i++) {
    unsigned char ch = static_cast<unsigned char>(entity_id[i]);
    if (!std::isalnum(ch) && ch != '_') return false;
  }
  return true;
}

inline void media_group_append_unique(std::vector<std::string> &out,
                                      const std::string &value) {
  std::string entity_id = media_group_trim(value);
  if (!media_group_valid_entity_id(entity_id)) return;
  if (std::find(out.begin(), out.end(), entity_id) == out.end()) out.push_back(entity_id);
}

inline std::vector<std::string> media_group_parse_entity_list(const char *raw,
                                                              size_t raw_size) {
  std::vector<std::string> out;
  if (!raw || raw_size == 0) return out;
  std::string token;
  char quote = 0;
  bool escaped = false;
  for (size_t i = 0; i < raw_size; i++) {
    char ch = raw[i];
    if (escaped) {
      token.push_back(ch);
      escaped = false;
      continue;
    }
    if (ch == '\\' && quote != 0) {
      escaped = true;
      continue;
    }
    if ((ch == '\'' || ch == '"')) {
      if (quote == 0) quote = ch;
      else if (quote == ch) quote = 0;
      else token.push_back(ch);
      continue;
    }
    if (quote == 0 && (ch == ',' || ch == '[' || ch == ']')) {
      media_group_append_unique(out, token);
      token.clear();
      continue;
    }
    token.push_back(ch);
  }
  media_group_append_unique(out, token);
  return out;
}

inline std::vector<std::string> media_group_parse_entity_list(const std::string &raw) {
  return media_group_parse_entity_list(raw.data(), raw.size());
}

inline uint64_t media_group_parse_supported_features(const std::string &raw) {
  std::string value_text = media_group_trim(raw);
  char *end = nullptr;
  unsigned long long value = std::strtoull(value_text.c_str(), &end, 10);
  return end && end != value_text.c_str() ? static_cast<uint64_t>(value) : 0;
}

inline bool media_grouping_supported(uint64_t supported_features) {
  return (supported_features & MEDIA_PLAYER_FEATURE_GROUPING) != 0;
}

inline std::vector<std::string> media_group_merge_candidates(
    const std::string &primary,
    const std::vector<std::string> &helper_members,
    const std::vector<std::string> &current_members) {
  std::vector<std::string> out;
  media_group_append_unique(out, primary);
  for (const std::string &entity_id : helper_members) media_group_append_unique(out, entity_id);
  for (const std::string &entity_id : current_members) media_group_append_unique(out, entity_id);
  return out;
}

struct MediaGroupVolumeState {
  std::string entity_id;
  int volume_pct = 0;
  bool volume_known = false;
  bool available = true;
};

inline bool media_group_mean_volume(const std::vector<MediaGroupVolumeState> &members,
                                    int *mean_pct) {
  if (!mean_pct || members.empty()) return false;
  int total = 0;
  int count = 0;
  for (const MediaGroupVolumeState &member : members) {
    if (!member.available) continue;
    if (!member.volume_known) return false;
    total += std::max(0, std::min(100, member.volume_pct));
    count++;
  }
  if (count == 0) return false;
  *mean_pct = static_cast<int>(std::lround(static_cast<double>(total) / count));
  return true;
}

inline std::vector<int> media_group_delta_volumes(
    const std::vector<MediaGroupVolumeState> &members,
    int requested_mean,
    int maximum_volume) {
  std::vector<int> out;
  int current_mean = 0;
  if (!media_group_mean_volume(members, &current_mean)) return out;
  maximum_volume = std::max(1, std::min(100, maximum_volume));
  requested_mean = std::max(0, std::min(maximum_volume, requested_mean));
  int delta = requested_mean - current_mean;
  out.reserve(members.size());
  for (const MediaGroupVolumeState &member : members) {
    int value = member.volume_pct + delta;
    value = std::max(0, std::min(maximum_volume, value));
    out.push_back(value);
  }
  return out;
}

inline std::string media_group_members_json(const std::vector<std::string> &members) {
  std::string json = "[";
  bool first = true;
  for (const std::string &member : members) {
    if (!media_group_valid_entity_id(member)) continue;
    if (!first) json += ",";
    first = false;
    json += "\"" + member + "\"";
  }
  json += "]";
  return json;
}

inline uint32_t next_media_group_call_id() {
  static uint32_t call_id = 500000;
  return call_id++;
}

inline bool send_media_group_join_action(
    const std::string &primary,
    const std::vector<std::string> &selected_members,
    HomeAssistantActionResponseCallback callback,
    uint32_t *sent_call_id = nullptr) {
  if (!media_group_valid_entity_id(primary)) return false;
  std::vector<std::string> members;
  for (const std::string &entity_id : selected_members) {
    if (entity_id != primary) media_group_append_unique(members, entity_id);
  }
  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_media_group_call_id();
  if (!ha_action_begin(req, "media_player.join", false, 1, call_id)) return false;
  ha_action_add_entity(req, primary);
  req.data_template.init(1);
  req.variables.init(1);
  std::string members_json = media_group_members_json(members);
  ha_action_add_variable(req, "group_members_json", members_json.c_str());
  ha_action_add_data_template(req, "group_members", "{{ group_members_json | from_json }}");
  if (!ha_register_action_response_callback(call_id, std::move(callback))) return false;
  if (!ha_action_send(req)) {
    ha_cancel_action_response_callback(call_id, "send failed");
    return false;
  }
  if (sent_call_id) *sent_call_id = call_id;
  return true;
}

inline bool send_media_group_unjoin_action(
    const std::string &entity_id,
    HomeAssistantActionResponseCallback callback,
    uint32_t *sent_call_id = nullptr) {
  if (!media_group_valid_entity_id(entity_id)) return false;
  esphome::api::HomeassistantActionRequest req;
  uint32_t call_id = next_media_group_call_id();
  if (!ha_action_begin(req, "media_player.unjoin", false, 1, call_id)) return false;
  ha_action_add_entity(req, entity_id);
  if (!ha_register_action_response_callback(call_id, std::move(callback))) return false;
  if (!ha_action_send(req)) {
    ha_cancel_action_response_callback(call_id, "send failed");
    return false;
  }
  if (sent_call_id) *sent_call_id = call_id;
  return true;
}
