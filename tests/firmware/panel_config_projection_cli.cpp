#include <cassert>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <new>
#include <string>
#include <vector>
#include "panel_config_projection.h"
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
  std::vector<uint8_t> input(std::istreambuf_iterator<char>(std::cin), {});
  const auto original = input;
  size_t needed = 0;
  PanelConfigProjectionStatus status;
  { Guard guard; status = panel_config_v2_to_v1_workspace_bytes(input.data(), input.size(), &needed); }
  if (status != PanelConfigProjectionStatus::OK) needed = 16384;
  std::vector<uint64_t> scratch((needed + 7) / 8 + 1);
  std::vector<uint8_t> output(PANEL_CONFIG_HEADER_SIZE + PANEL_CONFIG_MAX_RECORD_COUNT * (3 + PANEL_CONFIG_MAX_RECORD_BODY_BYTES), 'E');
  const std::string mode = argc > 1 ? argv[1] : "";
  void *workspace = scratch.data(); size_t workspace_size = needed, capacity = output.size(), used = 12345;
  auto *destination = output.data(); auto *result = &used;
  if (mode == "short") --workspace_size;
  if (mode == "unaligned") workspace = static_cast<uint8_t *>(workspace) + 1;
  if (mode == "small-output") capacity = 16;
  if (mode == "input-output-alias") destination = input.data();
  if (mode == "input-workspace-alias") workspace = input.data();
  if (mode == "output-workspace-alias") workspace = output.data();
  if (mode == "result-input-alias") result = reinterpret_cast<size_t *>(input.data());
  if (mode == "result-output-alias") result = reinterpret_cast<size_t *>(output.data());
  if (mode == "result-workspace-alias") result = reinterpret_cast<size_t *>(workspace);
  if (mode == "null-result") result = nullptr;
  { Guard guard; status = panel_config_v2_to_v1(input.data(), input.size(), destination, capacity, workspace, workspace_size, result); }
  assert(input == original);
  if (status != PanelConfigProjectionStatus::OK) {
    if (status != PanelConfigProjectionStatus::INVALID_ARGUMENT || mode == "unaligned") {
      assert(used == 0); assert(output[0] != 'E');
    }
    std::cerr << unsigned(status) << '\n'; return 2;
  }
  assert(mode.empty() && used <= output.size());
  PanelConfigReader reader(output.data(), used);
  { Guard guard; assert(reader.validate() == PanelConfigStatus::OK); }
  std::cout.write(reinterpret_cast<const char *>(output.data()), used);
}
