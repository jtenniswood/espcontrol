"""Run the production request entry point against stalled/failed worker queues.

The HTTP stub records synchronous requests. P4 cover art, public tile artwork,
and an unavailable local pipeline must all return without calling it.
"""
from pathlib import Path
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
implementation_path = (Path(sys.argv[2]) if len(sys.argv) > 2 else
                       root / "components/artwork_image/artwork_image.cpp")
implementation = implementation_path.read_text()
start = implementation.index("void ArtworkImage::start_update_()")
end = implementation.index("bool ArtworkImage::can_use_p4_pipeline", start)
production = implementation[start:end]

source = r'''
#include <cassert>
#include <cstdint>
#include <ctime>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#define ESP_LOGD(...)
#define ESP_LOGI(...)
#define ESP_LOGW(...)
#define ESP_LOGE(...)
static uint32_t millis() { return 1; }
namespace esphome::artwork_image {
namespace http_request {
struct Header { std::string name, value; };
struct HttpContainer { int status_code = 200; size_t content_length = 100; };
struct Parent {
  int synchronous_requests = 0;
  std::shared_ptr<HttpContainer> get(const std::string &, const std::vector<Header> &,
                                  std::initializer_list<const char *>) {
    ++synchronous_requests;
    return std::make_shared<HttpContainer>();
  }
};
}
enum ImageFormat { AUTO, JPEG, PNG, BMP };
enum class TransferFailure { CONTENT, RESOURCE, TRANSPORT, HTTP, NONE };
static constexpr int HTTP_CODE_OK = 200, HTTP_CODE_NOT_MODIFIED = 304;
static constexpr const char *CONTENT_TYPE_HEADER_NAME = "content-type";
static bool is_ha_media_proxy_url(const std::string &url) {
  return url.find("/api/media_player_proxy/") != std::string::npos;
}
struct TransferObserver {
  static TransferObserver &instance() { static TransferObserver observer; return observer; }
  int begin(const std::string &, uint32_t) { return 1; }
  void complete(int, uint32_t, int, TransferFailure) {}
};
struct Value { std::string value() { return "test"; } };
struct Callback { void call(bool) {} };
struct ArtworkImage {
  http_request::Parent *parent_;
  std::string url_;
  int transfer_stamp_ = 0, last_http_status_ = 0;
  uint32_t service_generation_ = 1, request_started_ms_ = 0;
  uint32_t response_ready_ms_ = 0, first_byte_ms_ = 0;
  uint32_t transfer_complete_ms_ = 0, decode_started_ms_ = 0, last_data_millis_ = 0;
  size_t peak_download_buffer_size_ = 0, completed_transfer_bytes_ = 0;
  bool last_error_was_ha_media_proxy_ = false;
  TransferFailure transfer_failure_ = TransferFailure::CONTENT;
  ImageFormat format_ = AUTO;
  std::vector<std::pair<std::string, Value>> request_headers_;
  std::shared_ptr<http_request::HttpContainer> downloader_;
  struct Buffer { size_t size() { return 0; } } download_buffer_;
  Callback download_finished_callback_;
  time_t start_time_ = 0;
  bool eligible_for_p4_pipeline = false, p4_queue_available = false;
  bool background_queue_available = true;
  int p4_submits = 0, background_submits = 0, failures = 0;
  void start_update_();
  void log_state_(const char *) {}
  bool can_use_p4_pipeline(const std::string &) { return eligible_for_p4_pipeline; }
  bool start_p4_pipeline_(std::vector<http_request::Header> &) {
    ++p4_submits; return p4_queue_available;
  }
  bool start_background_transfer_(std::vector<http_request::Header> &&) {
    ++background_submits; return background_queue_available;
  }
  // Allows the regression to run against the pre-fix implementation too.
  bool start_s3_transfer_(std::vector<http_request::Header> &&headers) {
    return start_background_transfer_(std::move(headers));
  }
  bool should_use_local_idf_url_(const std::string &url) {
    return url.find("192.168.") != std::string::npos;
  }
  std::shared_ptr<http_request::HttpContainer> get_local_idf_(
      const std::string &url, const std::vector<http_request::Header> &headers) {
    return parent_->get(url, headers, {});
  }
  void fail_download_() { ++failures; }
  void end_connection_() {}
  bool has_newer_pending_update_() { return false; }
  void complete_service_request_() {}
  size_t get_sane_content_length_() { return 100; }
  void enable_loop() {}
  ImageFormat detect_format_() { return JPEG; }
  bool create_decoder_(ImageFormat, size_t) { return true; }
};
'''
source += production
source += r'''
}
int main() {
  using namespace esphome::artwork_image;
  for (const std::string url : {"http://192.168.1.2/api/media_player_proxy/test",
                                "https://example.com/cover.jpg"}) {
    http_request::Parent parent;
    ArtworkImage image;
    image.parent_ = &parent;
    image.url_ = url;
    // A submitted worker may remain blocked in HTTP for seconds. The entry
    // point must return before any HTTP transaction runs on this caller.
    image.start_update_();
    assert(image.background_submits == 1);
    assert(parent.synchronous_requests == 0 && image.failures == 0);

    image.background_submits = 0;
    image.background_queue_available = false;
    image.start_update_();
    assert(image.background_submits == 1 && image.failures == 1);
    assert(parent.synchronous_requests == 0);
    assert(image.transfer_failure_ == TransferFailure::RESOURCE);

#ifdef CONFIG_IDF_TARGET_ESP32P4
    image.eligible_for_p4_pipeline = true;
    image.background_queue_available = true;
    image.background_submits = 0;
    image.start_update_();
    assert(image.p4_submits == 1 && image.background_submits == 1);
    assert(parent.synchronous_requests == 0);

    image.p4_queue_available = true;
    image.background_submits = 0;
    image.start_update_();
    assert(image.p4_submits == 2 && image.background_submits == 0);
    assert(parent.synchronous_requests == 0);
#endif
  }
}
'''
with tempfile.TemporaryDirectory(prefix="artwork-transfer-routing-") as directory:
    temp = Path(directory)
    cpp = temp / "check.cpp"
    cpp.write_text(source)
    for target in ("ESP32P4", "ESP32S3"):
        binary = temp / target
        subprocess.run([sys.argv[1] if len(sys.argv) > 1 else "c++",
                        "-std=c++17", "-Wall", "-Wextra", "-Werror",
                        "-DUSE_ESP_IDF", f"-DCONFIG_IDF_TARGET_{target}",
                        str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
        print(f"{target}: artwork never falls back to synchronous HTTP")
