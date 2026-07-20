#pragma once

// Shared, mostly pure helpers for Home Assistant media-player grouping.
// Included by button_grid.h after the Home Assistant action boundary.

constexpr uint64_t MEDIA_PLAYER_FEATURE_GROUPING = 524288ULL;
constexpr const char *DEFAULT_MEDIA_SPEAKER_DISCOVERY_ENTITY = "sensor.speaker_group";

struct MediaGroupDiscoveryItem {
  std::string entity_id;
  std::string friendly_name;
  int volume_pct = 0;
  bool volume_known = false;
};

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

inline std::vector<std::string> media_group_parse_discovery_data(const std::string &raw) {
  std::vector<std::string> out;
  std::string ids = raw.substr(0, raw.find('|'));
  size_t start = 0;
  while (start <= ids.size()) {
    size_t end = ids.find(',', start);
    std::string entity_id = media_group_trim(ids.substr(
      start, end == std::string::npos ? std::string::npos : end - start));
    if (!entity_id.empty() && entity_id.rfind("media_player.", 0) != 0) {
      entity_id = "media_player." + entity_id;
    }
    media_group_append_unique(out, entity_id);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  return out;
}

inline std::vector<MediaGroupDiscoveryItem> media_group_parse_discovery_items(
    const std::string &raw) {
  std::vector<MediaGroupDiscoveryItem> out;
  size_t p1 = raw.find('|');
  size_t p2 = p1 == std::string::npos ? std::string::npos : raw.find('|', p1 + 1);
  size_t p3 = p2 == std::string::npos ? std::string::npos : raw.find('|', p2 + 1);
  std::string ids = raw.substr(0, p1);
  std::string names = p1 == std::string::npos ? "" : raw.substr(
    p1 + 1, p2 == std::string::npos ? std::string::npos : p2 - p1 - 1);
  std::string volumes = p2 == std::string::npos ? "" : raw.substr(
    p2 + 1, p3 == std::string::npos ? std::string::npos : p3 - p2 - 1);
  auto split = [](const std::string &value) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (start <= value.size()) {
      size_t end = value.find(',', start);
      parts.push_back(media_group_trim(value.substr(
        start, end == std::string::npos ? std::string::npos : end - start)));
      if (end == std::string::npos) break;
      start = end + 1;
    }
    return parts;
  };
  std::vector<std::string> entity_ids = split(ids);
  std::vector<std::string> friendly_names = split(names);
  std::vector<std::string> volume_values = split(volumes);
  for (size_t i = 0; i < entity_ids.size(); i++) {
    std::string entity_id = entity_ids[i];
    if (!entity_id.empty() && entity_id.rfind("media_player.", 0) != 0) {
      entity_id = "media_player." + entity_id;
    }
    if (!media_group_valid_entity_id(entity_id)) continue;
    MediaGroupDiscoveryItem item;
    item.entity_id = entity_id;
    if (i < friendly_names.size()) item.friendly_name = friendly_names[i];
    if (i < volume_values.size()) {
      char *end = nullptr;
      float volume = std::strtof(volume_values[i].c_str(), &end);
      if (end && end != volume_values[i].c_str()) {
        item.volume_pct = std::max(0, std::min(100, (int) std::lround(volume * 100.0f)));
        item.volume_known = true;
      }
    }
    out.push_back(std::move(item));
  }
  return out;
}

inline std::string media_group_discovery_entity(const std::string &configured) {
  std::string entity_id = media_group_trim(configured);
  return entity_id.empty() ? DEFAULT_MEDIA_SPEAKER_DISCOVERY_ENTITY : entity_id;
}

inline const char *media_group_discovery_attribute(const std::string &entity_id) {
  return entity_id == DEFAULT_MEDIA_SPEAKER_DISCOVERY_ENTITY ? "data" : "entity_id";
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
