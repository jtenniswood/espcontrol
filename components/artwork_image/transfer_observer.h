#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <utility>

namespace esphome::artwork_image {
enum class TransferFailure { NONE, TRANSPORT, HTTP, CONTENT, RESOURCE };
struct TransferStamp {
  std::string origin;
  uint32_t endpoint_generation{0};
  uint32_t request_generation{0};
};
struct TransferNotice {
  TransferStamp request;
  int status{0};
  TransferFailure failure{TransferFailure::NONE};
};

// Main-loop-only observer. Download workers never invoke application callbacks.
// Capturing before the request prevents a late result from healing/failing a
// replacement endpoint. No URL query, credentials or token is retained here.
class TransferObserver {
 public:
  using Capture = std::function<uint32_t(const std::string &)>;
  using Notify = std::function<void(const TransferNotice &)>;
  static TransferObserver &instance() { static TransferObserver observer; return observer; }
  void set(Capture capture, Notify notify) { capture_ = std::move(capture); notify_ = std::move(notify); }
  TransferStamp begin(const std::string &url, uint32_t generation) const {
    if (!capture_) return {};
    const size_t scheme = url.find("://");
    if (scheme == std::string::npos) return {};
    const size_t path = url.find('/', scheme + 3);
    if (path == std::string::npos) return {};
    if (url.compare(path, sizeof("/api/camera_proxy/") - 1, "/api/camera_proxy/") != 0 &&
        url.compare(path, sizeof("/api/image_proxy/") - 1, "/api/image_proxy/") != 0 &&
        url.compare(path, sizeof("/api/media_player_proxy/") - 1, "/api/media_player_proxy/") != 0) return {};
    const std::string origin = url.substr(0, path);
    const uint32_t epoch = capture_(origin);
    return epoch ? TransferStamp{origin, epoch, generation} : TransferStamp{};
  }
  void complete(TransferStamp &stamp, uint32_t generation, int status,
                TransferFailure failure, bool superseded = false) const {
    const auto completed = std::move(stamp);
    stamp = {};
    if (notify_ && !superseded && completed.endpoint_generation != 0 &&
        completed.request_generation == generation)
      notify_({completed, status, failure});
  }
 private:
  Capture capture_;
  Notify notify_;
};
}  // namespace esphome::artwork_image
