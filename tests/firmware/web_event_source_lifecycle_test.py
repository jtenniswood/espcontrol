"""Exercise production SSE cleanup with a delayed HTTP-server free_ctx callback."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
header = (ROOT / "components/web_server_idf/web_server_idf.h").read_text()
source = (ROOT / "components/web_server_idf/web_server_idf.cpp").read_text()
classes = header.split("#ifdef USE_WEBSERVER\nclass AsyncEventSource;", 1)[1]
classes = "class AsyncEventSource;" + classes.split("#endif  // USE_WEBSERVER", 1)[0]
implementation = source.split("#ifdef USE_WEBSERVER\nAsyncEventSource::~", 1)[1]
implementation = "AsyncEventSource::~" + implementation.split("\n#endif\n\n#ifdef USE_WEBSERVER_OTA", 1)[0]

harness = r'''
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <functional>
#include <span>
#include <string>
#include <utility>
#include <vector>
#define VERSION_CODE(major, minor, patch) (major * 10000 + minor * 100 + patch)
template<typename... T> void log_message(T &&...) {}
#define ESP_LOGD(...) log_message(__VA_ARGS__)
#define ESP_LOGW(...) log_message(__VA_ARGS__)
#define ESP_LOGV(...) log_message(__VA_ARGS__)
#define ESP_LOGE(...) log_message(__VA_ARGS__)
#define CRLF_STR "\r\n"
constexpr const char *TAG = "web_server_idf";
using httpd_handle_t = void *;
constexpr int ESP_OK = 0, ESP_FAIL = -1;
constexpr int HTTP_GET = 1, HTTPD_SOCK_ERR_TIMEOUT = -3;
constexpr int HTTPD_SOCK_ERR_FAIL = -2, HTTPD_SOCK_ERR_INVALID = -1;
constexpr const char *HTTPD_200 = "200 OK";
constexpr size_t CRLF_LEN = 2;
struct httpd_req_t {
  httpd_handle_t handle = reinterpret_cast<void *>(1);
  void *sess_ctx = nullptr;
  void (*free_ctx)(void *) = nullptr;
  int fd = 17;
};
int httpd_resp_set_status(httpd_req_t *, const char *) { return ESP_OK; }
int httpd_resp_set_type(httpd_req_t *, const char *) { return ESP_OK; }
int httpd_resp_set_hdr(httpd_req_t *, const char *, const char *) { return ESP_OK; }
int httpd_resp_send_chunk(httpd_req_t *, const char *, size_t) { return ESP_OK; }
int httpd_req_to_sockfd(httpd_req_t *req) { return req->fd; }
template<typename T> void httpd_sess_set_send_override(httpd_handle_t, int, T) {}
int nonblocking_send(httpd_handle_t, int, const char *, size_t, int) { return 0; }
uint32_t millis() { return 100; }
int send_result = 100000, sends = 0, close_attempts = 0, queued_closes = 0;
bool queue_available = true;
int shutdowns = 0;
void *session_context = nullptr;
std::vector<std::pair<void (*)(void *), void *>> close_work;
constexpr int SHUT_RDWR = 2;
int shutdown(int fd, int how) { assert(fd == 17 && how == SHUT_RDWR); ++shutdowns; return 0; }
void *httpd_sess_get_ctx(httpd_handle_t, int fd) { assert(fd == 17); return session_context; }
void run_close_work() {
  auto pending = std::move(close_work);
  close_work.clear();
  for (auto &work : pending) work.first(work.second);
}
std::string received;
int httpd_socket_send(httpd_handle_t, int fd, const char *data, size_t size, int) {
  assert(fd == 17);
  ++sends;
  if (send_result <= 0) return send_result;
  const int count = std::min(size, static_cast<size_t>(send_result));
  received.append(data, count);
  return count;
}
int httpd_queue_work(httpd_handle_t, void (*callback)(void *), void *arg) {
  ++close_attempts;
  if (!queue_available) return ESP_FAIL;
  ++queued_closes;
  close_work.emplace_back(callback, arg);
  // As in IDF, acceptance does not free sess_ctx synchronously.
  return ESP_OK;
}
namespace esphome {
namespace json {
template<typename... T> using SerializationBuffer = std::string;
struct JsonBuilder { std::string serialize() { return "{}"; } };
}
namespace web_server {
struct WebServer {
  bool include_internal_ = true;
  std::vector<std::function<void()>> pending;
  std::string get_config_json() { return "{}"; }
  void run_deferred() {
    auto callbacks = std::move(pending);
    for (auto &callback : callbacks) callback();
  }
  void enable_loop_soon_any_context() {}
};
struct ListEntitiesIterator {
  int advances = 0;
  bool done = true;
  template<typename T> ListEntitiesIterator(WebServer *, T *) {}
  void begin(bool) {}
  bool completed() const { return done; }
  void try_advance(int) { ++advances; }
};
}
struct Scheduler {
  void set_timeout(web_server::WebServer *ws, const char *name, uint32_t timeout,
                   std::function<void()> callback) {
    assert(name == nullptr && timeout == 0);  // Never cancel another pending client.
    ws->pending.push_back(std::move(callback));
  }
};
struct Application { Scheduler scheduler; } App;
namespace web_server_idf {
class AsyncWebServerRequest {
 public:
  static constexpr size_t URL_BUF_SIZE = 513;
  httpd_req_t *req;
  operator httpd_req_t *() const { return req; }
  int method() const { return HTTP_GET; }
  std::string url_to(std::span<char, URL_BUF_SIZE>) const { return "/events"; }
};
class AsyncWebHandler {
 public:
  virtual ~AsyncWebHandler() = default;
  virtual bool canHandle(AsyncWebServerRequest *) const { return false; }
  virtual void handleRequest(AsyncWebServerRequest *) {}
};
struct HttpHeader { const char *name; const char *value; };
struct DefaultHeaders {
  std::vector<HttpHeader> headers_;
  static DefaultHeaders &Instance() { static DefaultHeaders headers; return headers; }
};
''' + classes.replace(" protected:", " public:") + '\n' + implementation + r'''
}  // namespace web_server_idf
}  // namespace esphome
using namespace esphome::web_server_idf;
int generated = 0;
#if ESPHOME_VERSION_CODE >= VERSION_CODE(2026, 10, 0)
void generate(esphome::web_server::WebServer *, void *, esphome::json::JsonBuilder &) { ++generated; }
#else
std::string generate(esphome::web_server::WebServer *, void *) { ++generated; return "{}"; }
#endif
struct Connection {
  esphome::web_server::WebServer ws;
  httpd_req_t raw;
  AsyncWebServerRequest request{&raw};
  AsyncEventSource events{"/events", &ws};
  explicit Connection(bool handoff = true) {
    send_result = 100000;
    sends = close_attempts = queued_closes = generated = shutdowns = 0;
    assert(close_work.empty());
    queue_available = true;
    events.handleRequest(&request);
    session_context = raw.sess_ctx;
    assert(events.count() == 0);  // httpd must not mutate the main-loop vector.
    if (handoff) ws.run_deferred();
    received.clear();
  }
  AsyncEventSourceResponse *session() { return events.sessions_.front(); }
  void server_closed() {
    assert(raw.sess_ctx != nullptr && raw.free_ctx != nullptr);
    raw.free_ctx(raw.sess_ctx);
    raw.sess_ctx = nullptr;
    session_context = nullptr;
    run_close_work();
    assert(!events.loop());
  }
  ~Connection() { if (raw.sess_ctx != nullptr) server_closed(); }
};
void retained_until_server_close(Connection &c) {
  assert(c.events.count() == 1);
  assert(c.session()->fd_.load() == 17);
  assert(queued_closes == 1);
  const int previous_sends = sends;
  c.session()->entities_iterator_.done = false;
  c.session()->deferrable_send_state(&c, "state_detail_all", generate);
  assert(!c.session()->try_send_nodefer("new", 3, "state"));
  for (int i = 0; i < 10; ++i) assert(c.events.loop());
  assert(c.events.count() == 1 && queued_closes == 1);
  assert(sends == previous_sends && generated == 0);
  assert(c.session()->entities_iterator_.advances == 0);
  assert(shutdowns == 0);
  run_close_work();
  assert(shutdowns == 1);
  c.server_closed();
}
int main() {
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_FAIL;
    c.session()->try_send_nodefer("broken", 6, "state");
    c.raw.free_ctx(c.raw.sess_ctx);
    c.raw.sess_ctx = nullptr;
    assert(c.events.loop() && c.events.count() == 1);  // Queued callback still owns the response.
    httpd_req_t replacement;
    AsyncWebServerRequest request{&replacement};
    send_result = 100000;
    c.events.handleRequest(&request);
    session_context = replacement.sess_ctx;
    c.ws.run_deferred();
    run_close_work();
    assert(shutdowns == 0 && c.events.loop() && c.events.count() == 1);
    replacement.free_ctx(replacement.sess_ctx);
    session_context = nullptr;
    assert(!c.events.loop());
  }
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_FAIL;
    c.session()->try_send_nodefer("broken", 6, "state");
    session_context = &c;  // Reused fd with a different session identity.
    run_close_work();
    assert(shutdowns == 0);
  }
  {
    Connection c(false);
    httpd_req_t second;
    AsyncWebServerRequest request{&second};
    c.events.handleRequest(&request);
    assert(c.events.count() == 0 && c.ws.pending.size() == 2);
    c.ws.run_deferred();
    assert(c.events.count() == 2);
    c.raw.free_ctx(c.raw.sess_ctx);
    c.raw.sess_ctx = nullptr;
    assert(c.events.loop() && c.events.count() == 1);
    second.free_ctx(second.sess_ctx);
    assert(!c.events.loop());
  }
  {
    Connection c(false);
    int connected = 0;
    c.events.onConnect([&](AsyncEventSourceClient *) { ++connected; });
    c.ws.run_deferred();
    assert(connected == 1 && c.events.count() == 1);
  }
  {
    Connection c(false);
    int connected = 0;
    c.events.onConnect([&](AsyncEventSourceClient *) { ++connected; });
    c.raw.free_ctx(c.raw.sess_ctx);  // Disconnect before main-loop handoff.
    c.raw.sess_ctx = nullptr;
    c.ws.run_deferred();
    assert(connected == 0 && !c.events.loop());
  }
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_TIMEOUT;
    assert(c.session()->try_send_nodefer("stalled", 7, "state"));
    // This path used to zero fd_ and delete the object while raw.sess_ctx
    // still pointed to it; the HTTP server would later call free_ctx on it.
    for (int i = 0; i < 2600; ++i) assert(c.events.loop());
    retained_until_server_close(c);
  }
  for (int failure : {HTTPD_SOCK_ERR_FAIL, HTTPD_SOCK_ERR_INVALID, 0}) {
    Connection c;
    send_result = failure;
    c.session()->try_send_nodefer("broken", 6, "state");
    // Fatal sends used to leave the socket open indefinitely.
    retained_until_server_close(c);
  }
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_FAIL;
    c.session()->deferrable_send_state(&c, "state_detail_all", generate);
    assert(c.session()->deferred_queue_.empty());
    generated = 0;
    retained_until_server_close(c);
  }
  {
    Connection c;
    queue_available = false;
    send_result = HTTPD_SOCK_ERR_FAIL;
    c.session()->try_send_nodefer("retry", 5, "state");
    assert(close_attempts == 1 && queued_closes == 0);
    const int previous_sends = sends;
    for (int i = 0; i < 3; ++i) assert(c.events.loop());
    assert(close_attempts == 4 && sends == previous_sends);
    queue_available = true;
    assert(c.events.loop());
    retained_until_server_close(c);
  }
  {
    Connection c;
    send_result = 3;
    assert(c.session()->try_send_nodefer("one\ntwo", 7, "state"));
    for (int i = 0; i < 100; ++i) c.events.loop();
    assert(received == "00000026\r\nevent: state\r\ndata: one\r\ndata: two\r\n\r\n\r\n");
    assert(queued_closes == 0);
  }
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_TIMEOUT;
    c.session()->try_send_nodefer("pending", 7, "state");
    c.session()->deferrable_send_state(&c, "state_detail_all", generate);
    assert(c.session()->deferred_queue_.size() == 1);
    send_result = HTTPD_SOCK_ERR_FAIL;
    c.session()->process_deferred_queue_();
    generated = 0;
    retained_until_server_close(c);
  }
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_TIMEOUT;
    assert(c.session()->try_send_nodefer("recover", 7, "state"));
    for (int i = 0; i < 100; ++i) c.events.loop();
    send_result = 100000;
    c.events.loop();
    assert(c.session()->consecutive_send_failures_ == 0);
    assert(c.session()->event_buffer_.empty() && queued_closes == 0);
    assert(c.session()->try_send_nodefer("next", 4, "state"));
  }
  {
    Connection c;
    send_result = HTTPD_SOCK_ERR_TIMEOUT;
    c.session()->try_send_nodefer("pending", 7, "state");
    c.session()->deferrable_send_state(&c, "state_detail_all", generate);
    assert(c.session()->deferred_queue_.size() == 1);
    generated = 0;
    const int previous_sends = sends;
    c.raw.free_ctx(c.raw.sess_ctx);  // Browser disconnects with data pending.
    c.raw.sess_ctx = nullptr;
    assert(!c.events.loop());
    assert(sends == previous_sends && queued_closes == 0 && generated == 0);
  }
}
'''
with tempfile.TemporaryDirectory(prefix="web-event-source-") as tmp:
    cpp = Path(tmp) / "test.cpp"
    cpp.write_text(harness)
    for version in (20260901, 20261000):
        binary = Path(tmp) / str(version)
        command = shlex.split(os.environ.get("CXX", "c++"))
        command += ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-fsanitize=address",
                    "-fno-omit-frame-pointer", f"-DESPHOME_VERSION_CODE={version}"]
        subprocess.run(command + [str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print("Web event-source lifecycle tests passed (ESPHome 2026.9 and 2026.10).")
