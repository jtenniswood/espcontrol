#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

namespace storage_fixture {
struct PowerCut {};
inline std::vector<uint8_t> flash(2 * 1024 * 1024, 0xFF);
inline std::map<std::string, std::vector<uint8_t>> blobs;
inline bool has_partition = true;
inline size_t mutation = 0, cut_at = 0;
inline unsigned cut_percent = 0;
inline bool fail_read = false;
inline size_t fail_commit_at = 0;
inline size_t read_calls = 0;
inline void reset() {
  std::fill(flash.begin(), flash.end(), 0xFF);
  blobs.clear();
  has_partition = true;
  mutation = cut_at = read_calls = fail_commit_at = 0;
  cut_percent = 0;
  fail_read = false;
}
inline size_t prefix(size_t size) {
  ++mutation;
  return cut_at == mutation ? size * cut_percent / 100 : size;
}
inline void maybe_cut() {
  if (cut_at == mutation)
    throw PowerCut{};
}
} // namespace storage_fixture
