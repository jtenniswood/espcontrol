#!/usr/bin/env python3
"""Exercise the real P4 HTTP receiver with oversized GIF and split data events."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/artwork_image/artwork_image.cpp").read_text()
transfer = re.search(r"^struct P4PipelineTransfer \{.*?^\};", source, re.M | re.S)
receiver = re.search(r"^  static esp_err_t http_event_\([^\n]*\) \{.*?^  \}", source, re.M | re.S)
assert transfer and receiver

cpp = r'''
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <strings.h>
#include <unordered_map>
#include "image_pipeline_policy.h"
using namespace esphome::artwork_image;
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1;
constexpr int HTTP_EVENT_ON_HEADER = 1, HTTP_EVENT_ON_DATA = 2;
constexpr int MALLOC_CAP_SPIRAM = 1, MALLOC_CAP_8BIT = 2;
#define ESP_LOGE(...) ((void)0)
uint32_t millis() { return 1; }
struct Client { int64_t length; };
int64_t esp_http_client_get_content_length(Client *client) { return client->length; }
struct esp_http_client_event_t {
  int event_id = HTTP_EVENT_ON_DATA;
  void *user_data = nullptr;
  void *data = nullptr;
  int data_len = 0;
  Client *client = nullptr;
  const char *header_key = nullptr, *header_value = nullptr;
};
struct P4PipelineJob { std::atomic<bool> cancelled{false}; };
size_t free_psram = 32 * 1024 * 1024;
std::unordered_map<void *, size_t> allocations;
size_t heap_caps_get_free_size(int) { return free_psram; }
void *heap_caps_realloc(void *pointer, size_t size, int) {
  const size_t previous = pointer ? allocations.at(pointer) : 0;
  if (size > previous && size - previous > free_psram) return nullptr;
  void *result = std::realloc(pointer, size);
  if (!result) return nullptr;
  if (pointer) allocations.erase(pointer);
  allocations[result] = size;
  free_psram = free_psram + previous - size;
  return result;
}
void release(void *pointer) {
  if (!pointer) return;
  free_psram += allocations.at(pointer); allocations.erase(pointer); std::free(pointer);
}
''' + transfer[0] + "\nstruct Receiver {\n" + receiver[0] + r'''
};
int main() {
  uint8_t gif[] = {'G', 'I', 'F', '8', '9', 'a'};
  uint8_t png[] = {0x89, 'P', 'N', 'G', 0, 0};
  Client client{3 * 1024 * 1024};
  P4PipelineJob job;
  // This is the observed bytes=0 failure path in the old 2 MiB receiver.
  assert(p4_pipeline_transfer_capacity(0, sizeof(gif), client.length, 16384,
                                      IMAGE_PIPELINE_STANDARD_TRANSFER_LIMIT_BYTES) == 0);
  P4PipelineTransfer transfer; transfer.job = &job;
  esp_http_client_event_t event; event.user_data = &transfer; event.client = &client;
  event.data = gif; event.data_len = sizeof(gif);
  assert(Receiver::http_event_(&event) == ESP_OK);
  assert(transfer.size == sizeof(gif) && transfer.capacity == static_cast<size_t>(client.length));
  assert(transfer.gif_response && !transfer.allocation_failed);
  release(transfer.data);

  // Split magic bytes must establish the GIF limit before rejecting its length.
  transfer = {}; transfer.job = &job; event.data_len = 2;
  assert(Receiver::http_event_(&event) == ESP_OK);
  event.data = gif + 2; event.data_len = 4;
  assert(Receiver::http_event_(&event) == ESP_OK && transfer.gif_response);
  assert(!std::memcmp(transfer.data, gif, sizeof(gif))); release(transfer.data);

  // Ordinary images keep the original bound and report a size error accurately.
  transfer = {}; transfer.job = &job; event.data = png; event.data_len = sizeof(png);
  assert(Receiver::http_event_(&event) == ESP_FAIL);
  assert(transfer.size_limit_exceeded && !transfer.allocation_failed && !transfer.data);

  // Correct MIME supports a first chunk too short to contain the signature.
  transfer = {}; transfer.job = &job;
  event.event_id = HTTP_EVENT_ON_HEADER; event.header_key = "Content-Type";
  event.header_value = "image/gif; charset=binary";
  assert(Receiver::http_event_(&event) == ESP_OK && transfer.gif_response);
  event.event_id = HTTP_EVENT_ON_DATA; event.data = gif; event.data_len = 1;
  assert(Receiver::http_event_(&event) == ESP_OK);
  release(transfer.data);

  // An oversized GIF remains bounded and does not masquerade as heap exhaustion.
  transfer = {}; transfer.job = &job; client.length = 9 * 1024 * 1024;
  event.data_len = sizeof(gif);
  assert(Receiver::http_event_(&event) == ESP_FAIL);
  assert(transfer.size_limit_exceeded && !transfer.allocation_failed);

  // Larger GIF transfers cannot consume the reserve needed for decode and UI.
  transfer = {}; transfer.job = &job; client.length = 3 * 1024 * 1024;
  free_psram = client.length + IMAGE_PIPELINE_P4_GIF_PSRAM_HEADROOM_BYTES - 1;
  assert(Receiver::http_event_(&event) == ESP_FAIL);
  assert(transfer.allocation_failed && !transfer.size_limit_exceeded && !transfer.data);
  assert(allocations.empty());
}
'''

with tempfile.TemporaryDirectory(prefix="p4-gif-transfer-") as directory:
    temp = Path(directory)
    (temp / "test.cpp").write_text(cpp)
    executable = temp / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror",
        "-I", str(ROOT / "components/artwork_image"),
        str(temp / "test.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
print("P4 GIF receiver: larger responses, fragmented signature, MIME, size limits and memory reserve passed")
