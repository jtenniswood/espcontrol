#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <mutex>

namespace espcontrol::badge {
constexpr size_t MAX_ROW_BADGES = 8;
using Rows = std::array<std::string, 33>;
class RowConfiguration {
 public:
  bool replace(const Rows &rows) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (rows_ == rows) return false;
    rows_ = rows;
    return true;
  }
  std::string page(int index) {
    std::lock_guard<std::mutex> lock(mutex_);
    return index >= 0 && index < static_cast<int>(rows_.size()) ? rows_[index] : "";
  }
 private:
  std::mutex mutex_;
  Rows rows_;
};
inline RowConfiguration &configuration() { static RowConfiguration value; return value; }
inline int page_for_key(const uint8_t *key, size_t length) {
  const std::string value(reinterpret_cast<const char *>(key), length);
  for (int page = 0; page <= 32; ++page) {
    if (value == "badge_row_" + std::to_string(page)) return page;
  }
  return -1;
}
}  // namespace espcontrol::badge
