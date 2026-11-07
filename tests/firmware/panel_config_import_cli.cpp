#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>
#include "panel_config_import.h"
using namespace espcontrol::configuration;
static bool forbidden = false;
void *operator new(size_t size) {
  if (forbidden) std::abort();
  if (void *p = std::malloc(size ? size : 1)) return p;
  throw std::bad_alloc();
}
void *operator new[](size_t size) { return ::operator new(size); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }
struct Guard { Guard() { assert(!forbidden); forbidden = true; } ~Guard() { forbidden = false; } };
int main(int argc, char **argv) {
  std::vector<uint8_t> input(std::istreambuf_iterator<char>(std::cin), {}); const auto original = input;
  PanelConfigImportBounds bounds; PanelConfigImportStatus status;
  { Guard guard; status = panel_config_v1_to_v2_bounds(input.data(), input.size(), &bounds); }
  if (status != PanelConfigImportStatus::OK) { bounds.workspace_bytes = 16384; bounds.output_bytes = 16384; }
  std::vector<uint64_t> scratch((bounds.workspace_bytes + 7) / 8 + 1);
  std::vector<uint8_t> output(bounds.output_bytes, 'E');
  const std::string mode = argc > 1 ? argv[1] : "";
  void *workspace = scratch.data(); size_t workspace_size = bounds.workspace_bytes, capacity = output.size(), used = 12345;
  auto *destination = output.data(); auto *result = &used;
  if (mode == "short") workspace_size = panel_config_import_detail::FIXED - 1;
  if (mode == "table-full") workspace_size = panel_config_import_detail::FIXED + 16;
  if (mode == "finish-short") workspace_size = panel_config_import_detail::FIXED + 512;
  if (mode == "budget") workspace_size = std::min(workspace_size, panel_config_import_detail::FIXED + size_t(32768));
  if (workspace_size < bounds.workspace_bytes) {
    std::vector<uint64_t> tight((workspace_size + 7) / 8 + 1, UINT64_C(0xA5A5A5A5A5A5A5A5));
    scratch.swap(tight); workspace = scratch.data();
  }
  if (mode == "unaligned") workspace = static_cast<uint8_t *>(workspace) + 1;
  if (mode == "small-output") capacity = 16;
  if (mode == "input-output-alias") destination = input.data();
  if (mode == "input-workspace-alias") workspace = input.data();
  if (mode == "output-workspace-alias") workspace = output.data();
  if (mode == "result-input-alias") result = reinterpret_cast<size_t *>(input.data());
  if (mode == "result-output-alias") result = reinterpret_cast<size_t *>(output.data());
  if (mode == "result-workspace-alias") result = reinterpret_cast<size_t *>(workspace);
  if (mode == "null-result") result = nullptr;
  if (mode == "null-bounds" || mode == "bounds-input-alias") {
    auto *sizing_result = mode == "null-bounds" ? nullptr : reinterpret_cast<PanelConfigImportBounds *>(input.data());
    { Guard guard; status = panel_config_v1_to_v2_bounds(input.data(), input.size(), sizing_result); }
    assert(status == PanelConfigImportStatus::INVALID_ARGUMENT && input == original); return 2;
  }
  { Guard guard; status = panel_config_v1_to_v2(input.data(), input.size(), destination, capacity, workspace, workspace_size, result); }
  assert(input == original);
  if (workspace_size < bounds.workspace_bytes) {
    const auto *canary = reinterpret_cast<const uint8_t *>(scratch.data());
    for (size_t i = workspace_size; i < scratch.size() * 8; ++i) assert(canary[i] == 0xA5);
  }
  if (status != PanelConfigImportStatus::OK) {
    if (status != PanelConfigImportStatus::INVALID_ARGUMENT || mode == "unaligned") { assert(used == 0); assert(output[0] != 'E'); }
    std::cerr << unsigned(status) << '\n'; return 2;
  }
  assert((mode.empty() || mode == "budget") && used <= output.size());
  PanelConfigV2Reader reader(output.data(), used); size_t bytes = 0;
  { Guard guard; assert(reader.workspace_bytes(&bytes) == PanelConfigStatus::OK); }
  std::vector<uint64_t> validation((bytes + 7) / 8 + 1);
  { Guard guard; assert(reader.validate(validation.data(), validation.size() * 8) == PanelConfigStatus::OK); }
  assert(reader.record_count() <= bounds.max_records);
  std::cout.write(reinterpret_cast<const char *>(output.data()), used);
}
