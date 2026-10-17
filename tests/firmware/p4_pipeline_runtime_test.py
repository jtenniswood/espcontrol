#!/usr/bin/env python3
"""Exercise the production P4 worker against IDF-style ignored callback errors."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
production = (root / "components/artwork_image/artwork_image.cpp").read_text()
pipeline = production.split("struct P4PipelineJob {", 1)[1].split("\n#endif", 1)[0]
pipeline = "struct P4PipelineJob {" + pipeline.replace(" private:", " public:")
source = r'''
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <new>
#include <string>
#include <strings.h>
#include <utility>
#include <vector>
#include "image_pipeline_policy.h"
using namespace esphome::artwork_image;
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_NO_MEM = 1, ESP_ERR_INVALID_SIZE = 2;
constexpr int ESP_ERR_HTTP_FETCH_HEADER = 3, ESP_ERR_HTTP_MAX_REDIRECT = 4;
constexpr int ESP_ERR_HTTP_EAGAIN = 5, ESP_ERR_HTTP_READ_TIMEOUT = 6;
constexpr int ESP_ERR_HTTP_CONNECTION_CLOSED = 7;
constexpr int HTTP_METHOD_GET = 0, HTTP_AUTH_TYPE_NONE = 0;
constexpr int HTTP_EVENT_ON_HEADER = 1, HTTP_EVENT_ON_DATA = 2, HTTP_EVENT_DISCONNECTED = 3;
constexpr int MALLOC_CAP_SPIRAM = 1, MALLOC_CAP_8BIT = 2;
constexpr int LOCAL_ARTWORK_HTTP_TIMEOUT_MS = 6500;
#define ESP_LOGE(...) ((void)0)
uint32_t millis() { return 100; }
struct ArtworkImage {};
namespace http_request { struct Header { std::string name, value; }; }
using SemaphoreHandle_t = void *;
using TaskHandle_t = void *;
using BaseType_t = int;
constexpr int pdPASS = 1, pdTRUE = 1, portMAX_DELAY = -1;
void *xSemaphoreCreateMutex() { return reinterpret_cast<void *>(1); }
int xSemaphoreTake(void *, int) { return 1; }
void xSemaphoreGive(void *) {}
int xTaskCreate(void (*)(void *), const char *, int, void *, int, void **task) {
  *task = reinterpret_cast<void *>(2); return pdPASS;
}
void xTaskNotifyGive(void *) {}
void ulTaskNotifyTake(int, int) {}
static int allocations = 0;
static bool fail_alloc = false;
void *heap_caps_malloc(size_t size, int) { return std::malloc(size); }
void heap_caps_free(void *data) { std::free(data); }
size_t heap_caps_get_free_size(int) { return 32 * 1024 * 1024; }
void *heap_caps_realloc(void *data, size_t size, int) {
  ++allocations;
  return fail_alloc ? nullptr : std::realloc(data, size);
}
struct Client;
using esp_http_client_handle_t = Client *;
struct esp_http_client_event_t {
  int event_id; void *user_data; void *data = nullptr; int data_len = 0;
  Client *client = nullptr; const char *header_key = nullptr, *header_value = nullptr;
};
struct esp_http_client_config_t {
  const char *url = nullptr; int method = 0, timeout_ms = 0;
  bool disable_auto_redirect = false; int max_redirection_count = 0, auth_type = 0;
  int (*event_handler)(esp_http_client_event_t *) = nullptr;
  void *user_data = nullptr; bool keep_alive_enable = false; int buffer_size = 0;
};
struct Response {
  int status = 200; std::string mime = "image/jpeg", body = std::string(5000, 'x');
  int64_t reported_size = 5000;
  bool truncated = false, data_with_headers = false;
};
struct Client {
  void *context = nullptr;
  int (*handler)(esp_http_client_event_t *) = nullptr;
  size_t response = 0, offset = 0, cached = 0;
  bool closed = false;
};
static std::vector<Response> responses;
static int reads = 0, network_operations = 0, closes = 0;
static std::function<void()> on_read, on_cleanup;
int emit(Client *client, int id, const char *data = nullptr, size_t size = 0) {
  esp_http_client_event_t event{id, client->context, const_cast<char *>(data), static_cast<int>(size), client};
  if (id == HTTP_EVENT_ON_HEADER) {
    event.header_key = "Content-Type"; event.header_value = responses.at(client->response).mime.c_str();
  }
  return client->handler(&event);
}
Client *esp_http_client_init(esp_http_client_config_t *config) {
  ++network_operations;
  assert(config->disable_auto_redirect && config->timeout_ms == 6500);
  return new Client{config->user_data, config->event_handler};
}
int esp_http_client_set_user_data(Client *client, void *context) { client->context = context; return ESP_OK; }
int esp_http_client_set_url(Client *client, const char *) {
  ++network_operations;
  assert(client->context);  // Change context before a URL change emits events.
  assert(client->cached == 0);
  emit(client, HTTP_EVENT_DISCONNECTED);
  client->response = client->offset = 0;
  return ESP_OK;
}
int esp_http_client_set_method(Client *, int) { return ESP_OK; }
int esp_http_client_set_header(Client *, const char *, const char *) { return ESP_OK; }
int esp_http_client_delete_header(Client *, const char *) { return ESP_OK; }
int esp_http_client_get_status_code(Client *client) { return responses.at(client->response).status; }
int64_t esp_http_client_get_content_length(Client *client) { return responses.at(client->response).reported_size; }
int esp_http_client_open(Client *client, int) {
  ++network_operations; client->closed = false; client->offset = 0; return ESP_OK;
}
int64_t esp_http_client_fetch_headers(Client *client) {
  ++network_operations;
  emit(client, HTTP_EVENT_ON_HEADER);
  const auto &response = responses.at(client->response);
  if (response.data_with_headers && !response.body.empty()) {
    // Like IDF, discard the callback's error, including when headers and body
    // arrive in one packet. The worker must inspect the latched failure.
    emit(client, HTTP_EVENT_ON_DATA, response.body.data(), response.body.size());
    client->offset = response.body.size();
    client->cached = response.body.size();
  }
  return std::max<int64_t>(0, response.reported_size);
}
bool esp_http_client_is_complete_data_received(Client *client) {
  const auto &response = responses.at(client->response);
  return !response.truncated && client->offset == response.body.size();
}
int esp_http_client_read(Client *client, char *, int length) {
  ++network_operations; ++reads;
  if (on_read) { auto callback = std::move(on_read); on_read = {}; callback(); }
  if (client->cached) {
    const size_t size = std::min<size_t>(length, client->cached);
    client->cached -= size;
    return size;  // IDF does not emit ON_DATA twice for cached body bytes.
  }
  const auto &response = responses.at(client->response);
  const size_t size = std::min<size_t>(length, response.body.size() - client->offset);
  if (size == 0) return 0;
  emit(client, HTTP_EVENT_ON_DATA, response.body.data() + client->offset, size);
  client->offset += size;
  return size;
}
int esp_http_client_close(Client *client) {
  ++network_operations; ++closes; client->closed = true;
  emit(client, HTTP_EVENT_DISCONNECTED); return ESP_OK;
}
int esp_http_client_clear_response_buffer(Client *client) { client->cached = 0; return ESP_OK; }
int esp_http_client_set_redirection(Client *client) {
  ++network_operations; ++client->response; return ESP_OK;
}
void esp_http_client_cleanup(Client *client) {
  ++network_operations;
  if (on_cleanup) { auto callback = std::move(on_cleanup); on_cleanup = {}; callback(); }
  esp_http_client_close(client);
  delete client;
}
// Deliberately no esp_http_client_cancel_request stub: cancellation must never
// operate on a worker-owned client from the main task.
''' + pipeline + r'''
int main() {
  P4ImagePipeline &pipeline = P4ImagePipeline::instance();
  ArtworkImage owner;
  uint32_t generation = 0;
  auto run = [&]() {
    std::vector<http_request::Header> headers;
    assert(pipeline.submit(&owner, ++generation, 1, "http://example.local/image.jpg", headers));
    P4PipelineJob *job = pipeline.next_job_();
    assert(job);
    auto *result = pipeline.perform_(job);
    pipeline.active_ = nullptr;
    delete job;
    return result;
  };
  auto reset = [&] {
    pipeline.reset_client_();
    responses = {Response{}};
    reads = allocations = closes = 0;
    fail_alloc = false;
  };
  reset();
  for (int i = 0; i < 100; ++i) {
    auto *result = run();
    assert(result && result->error == ESP_OK && result->size == 5000);
    assert(std::string(reinterpret_cast<char *>(result->data), result->size) == responses[0].body);
    assert(pipeline.client_ && pipeline.client_->context == nullptr);
    delete result;
  }
  // Chunked transfers and body bytes delivered with headers both stay intact.
  reset(); responses[0].reported_size = -1;
  auto *result = run();
  assert(result->error == ESP_OK && result->size == 5000); delete result;
  reset(); responses[0].data_with_headers = true;
  result = run();
  assert(result->error == ESP_OK && result->size == 5000 && reads > 0); delete result;
  // Stop after the first rejected chunk even though IDF ignores ESP_FAIL.
  reset(); responses[0].reported_size = 3 * 1024 * 1024;
  result = run();
  assert(result->error == ESP_ERR_INVALID_SIZE && reads == 1 && allocations == 0);
  assert(!pipeline.client_ && closes == 1); delete result;
  reset(); fail_alloc = true;
  result = run();
  assert(result->error == ESP_ERR_NO_MEM && reads == 1 && allocations == 1);
  assert(!pipeline.client_); delete result;
  reset(); fail_alloc = true; responses[0].data_with_headers = true;
  result = run();
  assert(result->error == ESP_ERR_NO_MEM && reads == 0 && allocations == 1); delete result;
  // Cancellation during a read and during cleanup never touches the client.
  reset();
  auto cancel_without_network = [&] {
    const int before = network_operations;
    pipeline.cancel(&owner);
    assert(network_operations == before);
  };
  on_read = cancel_without_network;
  on_cleanup = cancel_without_network;
  assert(run() == nullptr && !pipeline.client_ && reads == 1 && allocations == 0);
  reset();
  responses[0].status = 302; responses[0].mime = "image/gif";
  responses[0].body = "redirect page"; responses[0].data_with_headers = true;
  responses.push_back(Response{});
  result = run();
  assert(result->error == ESP_OK && result->size == 5000);
  assert(std::string(reinterpret_cast<char *>(result->data), result->size) == responses[1].body);
  delete result;
  reset(); responses.assign(4, Response{});
  for (auto &response : responses) response.status = 302;
  result = run();
  assert(result->error == ESP_ERR_HTTP_MAX_REDIRECT && !pipeline.client_); delete result;
  reset(); responses[0].truncated = true;
  result = run();
  assert(result->error == ESP_ERR_HTTP_CONNECTION_CLOSED && !pipeline.client_); delete result;
  reset(); responses[0].status = 304;
  result = run();
  assert(result->error == ESP_OK && result->status == 304 && result->size == 0 && reads == 0);
  assert(pipeline.client_->closed && !pipeline.client_->context); delete result;
  responses[0] = Response{};
  result = run();
  assert(result->error == ESP_OK && result->size == 5000); delete result;
  reset(); responses[0].status = 500;
  responses[0].data_with_headers = true;
  result = run();
  assert(result->status == 500 && result->size == 0 && reads == 0 && pipeline.client_->cached == 0); delete result;
  pipeline.reset_client_();
  std::puts("P4 worker: success/reuse, chunked, rejection, cancellation/cleanup, redirects, truncation and HTTP errors passed");
}
'''
with tempfile.TemporaryDirectory(prefix="p4-pipeline-runtime-") as directory:
    temp = Path(directory)
    (temp / "test.cpp").write_text(source)
    executable = temp / "test"
    subprocess.run(shlex.split(os.environ.get("CXX", "c++")) + [
        "-std=c++17", "-Wall", "-Wextra", "-Werror", "-I", str(root / "components/artwork_image"),
        str(temp / "test.cpp"), "-o", str(executable)
    ], check=True)
    subprocess.run([str(executable)], check=True)
