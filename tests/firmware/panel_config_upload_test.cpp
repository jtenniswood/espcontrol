#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <memory>
#include <unordered_map>
#include <vector>

#include "panel_config_upload.h"
#include "request_body_context.h"

using namespace espcontrol::configuration;

static std::unordered_map<uint8_t *, size_t> live;
static bool fail_allocation = false;
static size_t calls = 0;
static uint8_t *allocate(size_t size) {
  ++calls;
  if (fail_allocation)
    return nullptr;
  auto *data = static_cast<uint8_t *>(std::malloc(size));
  assert(data);
  live.emplace(data, size);
  return data;
}
static void release(uint8_t *data) {
  assert(live.erase(data) == 1);
  std::free(data);
}

// This is the same polymorphic request-owned composition used by the handler.
struct Upload final : esphome::web_server_idf::RequestBodyContext {
  explicit Upload(size_t size) : body(size, allocate, release) {}
  PanelConfigUpload body;
};

int main() {
  std::vector<uint8_t> document(40932);
  for (size_t i = 0; i < document.size(); ++i)
    document[i] = static_cast<uint8_t>(i % 251);
  for (size_t chunk : {size_t(1), size_t(7), size_t(1024), document.size()}) {
    const auto before = calls;
    {
      std::unique_ptr<esphome::web_server_idf::RequestBodyContext> context(
          new Upload(document.size()));
      auto &body = static_cast<Upload *>(context.get())->body;
      assert(body.ready() && !body.complete());
      for (size_t offset = 0; offset < document.size();) {
        const auto count = std::min(chunk, document.size() - offset);
        assert(body.append(document.data() + offset, count, offset,
                           document.size()));
        offset += count;
        assert(body.complete() == (offset == document.size()));
      }
      assert(body.size() == document.size());
      assert(std::equal(document.begin(), document.end(), body.data()));
      assert(live.size() == 1 && calls == before + 1);
    }
    assert(live.empty());
  }

  // Interrupted requests and two interleaved bodies cannot retain or share
  // storage, including a request that never receives its first chunk.
  for (size_t cycle = 0; cycle < 100; ++cycle) {
    std::unique_ptr<esphome::web_server_idf::RequestBodyContext> first(
        new Upload(document.size()));
    std::unique_ptr<esphome::web_server_idf::RequestBodyContext> second(
        new Upload(8));
    auto &one = static_cast<Upload *>(first.get())->body;
    auto &two = static_cast<Upload *>(second.get())->body;
    assert(one.data() != two.data() && live.size() == 2);
    assert(one.append(document.data(), 3, 0, document.size()));
    const uint8_t other[] = {9, 8, 7, 6, 5, 4, 3, 2};
    assert(two.append(other, 8, 0, 8) && two.complete());
    first.reset(); // Socket closes before handleRequest(): no document save.
    assert(live.size() == 1 &&
           std::equal(std::begin(other), std::end(other), two.data()));
    second.reset();
    assert(live.empty());
    {
      std::unique_ptr<esphome::web_server_idf::RequestBodyContext> unread(
          new Upload(32));
    }
    assert(live.empty());
  }

  // Reject discontinuous, repeated, oversized, null and changed-total chunks.
  for (size_t kind = 0; kind < 5; ++kind) {
    {
      Upload upload(8);
      auto &body = upload.body;
      assert(body.append(document.data(), 3, 0, 8));
      const bool accepted = kind == 0   ? body.append(document.data(), 1, 4, 8)
                            : kind == 1 ? body.append(document.data(), 3, 0, 8)
                            : kind == 2 ? body.append(document.data(), 6, 3, 8)
                            : kind == 3 ? body.append(nullptr, 1, 3, 8)
                                        : body.append(document.data(), 1, 3, 9);
      assert(!accepted && !body.ready() && !body.complete() &&
             body.size() == 3);
      assert(!body.append(document.data(), 5, 3, 8));
    }
    assert(live.empty());
  }
  const auto before_empty = calls;
  {
    Upload empty(0);
    assert(!empty.body.ready() && !empty.body.complete());
  }
  assert(calls == before_empty && live.empty());
  fail_allocation = true;
  {
    Upload failed(40932);
    assert(!failed.body.ready() && !failed.body.complete());
  }
  assert(live.empty());
  fail_allocation = false;
  {
    Upload retry(8);
    assert(retry.body.ready());
  }
  assert(live.empty());
}
