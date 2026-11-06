#include "legacy_card_config.h"

#include <algorithm>

namespace espcontrol {
namespace {

int hex_digit(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'A' && value <= 'F') return value - 'A' + 10;
  if (value >= 'a' && value <= 'f') return value - 'a' + 10;
  return -1;
}

char hex_char(unsigned value) {
  return value < 10 ? static_cast<char>('0' + value)
                    : static_cast<char>('A' + value - 10);
}

std::string decode_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (size_t index = 0; index < value.size();) {
    if (value[index] == '%' && index + 2 < value.size()) {
      const int high = hex_digit(value[index + 1]);
      const int low = hex_digit(value[index + 2]);
      if (high >= 0 && low >= 0) {
        out.push_back(static_cast<char>((high << 4) | low));
        index += 3;
        continue;
      }
    }
    out.push_back(value[index++]);
  }
  return out;
}

std::string encode_field(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  for (const unsigned char ch : value) {
    if (ch == '%' || ch == ',' || ch == ';' || ch == '|' || ch == ':') {
      out.push_back('%');
      out.push_back(hex_char((ch >> 4) & 0x0F));
      out.push_back(hex_char(ch & 0x0F));
    } else {
      out.push_back(static_cast<char>(ch));
    }
  }
  return out;
}

std::vector<std::string> split(const std::string &value, char delimiter) {
  std::vector<std::string> fields;
  size_t start = 0;
  while (start <= value.size()) {
    size_t end = value.find(delimiter, start);
    if (end == std::string::npos) end = value.size();
    fields.push_back(value.substr(start, end - start));
    if (end == value.size()) break;
    start = end + 1;
  }
  return fields;
}

std::string join(const std::vector<std::string> &fields, char delimiter) {
  std::string out;
  for (size_t index = 0; index < fields.size(); ++index) {
    if (index > 0) out.push_back(delimiter);
    out += fields[index];
  }
  return out;
}

bool clear_option(std::string &options, const std::string &asset_id) {
  std::vector<std::string> tokens = split(options, ',');
  const std::string target = "bg_image=" + asset_id;
  const auto original_size = tokens.size();
  tokens.erase(std::remove(tokens.begin(), tokens.end(), target), tokens.end());
  if (tokens.size() == original_size) return false;
  // An empty options field is represented by an empty string, not one empty
  // token retained from split().
  if (tokens.size() == 1 && tokens.front().empty()) options.clear();
  else options = join(tokens, ',');
  return true;
}

bool clear_button(std::string &button, bool compact, const std::string &asset_id) {
  const char delimiter = compact ? ',' : ';';
  std::vector<std::string> fields = split(button, delimiter);
  if (fields.size() <= 8) return false;
  std::string options = compact ? decode_field(fields[8]) : fields[8];
  if (!clear_option(options, asset_id)) return false;
  fields[8] = compact ? encode_field(options) : options;
  button = join(fields, delimiter);
  return true;
}

bool clear_document(std::string &config, const std::string &asset_id, bool subpage) {
  if (config.empty() || asset_id.empty()) return false;
  const bool compact = config.front() == '~';
  if (!subpage) {
    std::string body = compact ? config.substr(1) : config;
    if (!clear_button(body, compact, asset_id)) return false;
    config = compact ? "~" + body : body;
    return true;
  }

  std::string body = compact ? config.substr(1) : config;
  std::vector<std::string> segments = split(body, '|');
  bool changed = false;
  // Segment zero is the subpage order/navigation header.
  for (size_t index = 1; index < segments.size(); ++index) {
    changed = clear_button(segments[index], compact, asset_id) || changed;
  }
  if (changed) config = (compact ? "~" : "") + join(segments, '|');
  return changed;
}

bool continuation_byte(unsigned char value) { return (value & 0xC0) == 0x80; }

}  // namespace

bool clear_card_background_reference(std::string &config, const std::string &asset_id) {
  return clear_document(config, asset_id, false);
}

bool clear_subpage_background_references(std::string &config, const std::string &asset_id) {
  return clear_document(config, asset_id, true);
}

bool card_config_references_asset(const std::string &config, const std::string &asset_id) {
  std::string updated = config;
  return clear_card_background_reference(updated, asset_id);
}

bool subpage_config_references_asset(const std::string &config, const std::string &asset_id) {
  std::string updated = config;
  return clear_subpage_background_references(updated, asset_id);
}

bool split_subpage_config(const std::string &config, size_t chunk_count,
                          size_t chunk_bytes, std::vector<std::string> &out) {
  out.clear();
  if (chunk_count == 0 || chunk_bytes == 0 || config.size() > chunk_count * chunk_bytes) {
    return false;
  }
  size_t start = 0;
  while (start < config.size()) {
    size_t end = std::min(config.size(), start + chunk_bytes);
    while (end > start && end < config.size() &&
           continuation_byte(static_cast<unsigned char>(config[end]))) {
      --end;
    }
    if (end == start) return false;
    out.push_back(config.substr(start, end - start));
    start = end;
  }
  while (out.size() < chunk_count) out.emplace_back();
  return out.size() == chunk_count;
}

}  // namespace espcontrol
