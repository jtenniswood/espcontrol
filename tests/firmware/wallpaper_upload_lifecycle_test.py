#!/usr/bin/env python3
"""Run the production raw-body dispatcher through success and aborted uploads."""
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'components/web_server_idf/web_server_idf.cpp').read_text()
start = source.index('esp_err_t AsyncWebServer::handle_raw_body_(')
end = source.index('\nesp_err_t AsyncWebServer::request_handler(', start)
header = (root / 'components/web_server_idf/web_server_idf.h').read_text()
handler_start = header.index('class AsyncWebHandler {')
handler_end = header.index('\n#ifdef USE_WEBSERVER', handler_start)
production_handler = header[handler_start:handler_end]
fixture = r'''
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <string>
#include <vector>
#define ESP_LOGW(...) ((void)0)
using esp_err_t = int;
constexpr int ESP_OK = 0, ESP_FAIL = -1, ESP_ERR_TIMEOUT = -2;
constexpr int HTTPD_SOCK_ERR_TIMEOUT = -3, HTTPD_408_REQ_TIMEOUT = 408, HTTPD_400_BAD_REQUEST = 400;
struct httpd_req_t { size_t content_len; int mode; int reads = 0; };
struct AsyncWebServerRequest { explicit AsyncWebServerRequest(httpd_req_t *) {} };
int httpd_resp_send_err(httpd_req_t *, int, const char *) { return 0; }
int httpd_req_recv(httpd_req_t *request, char *, size_t length) {
  if (request->reads++ && request->mode) return request->mode == 1 ? HTTPD_SOCK_ERR_TIMEOUT : -1;
  return static_cast<int>(length);
}
'''
fixture += production_handler
fixture += r'''
class AsyncWebServer {
 public:
  std::vector<AsyncWebHandler *> handlers_;
  int request_handler_(AsyncWebServerRequest *) { return ESP_OK; }
  esp_err_t handle_raw_body_(httpd_req_t *, const char *);
};
'''
fixture += source[start:end]
fixture += r'''
class Upload : public AsyncWebHandler {
 public:
  bool canHandle(AsyncWebServerRequest *) const override { return true; }
  size_t maximumBodySize() const override { return 4096; }
  bool canReceiveBody(AsyncWebServerRequest *) override { return authorized; }
  void handleBody(AsyncWebServerRequest *, uint8_t *, size_t, size_t, size_t) override {
    buffer.resize(4096); ++chunks;
  }
  void handleRequest(AsyncWebServerRequest *) override { ++requests; }
  void handleBodyEnd(AsyncWebServerRequest *) override { buffer.clear(); ++ends; }
  std::vector<uint8_t> buffer;
  int chunks = 0, requests = 0, ends = 0;
  bool authorized = true;
};
int main() {
  for (int mode : {0, 1, 2}) {
    Upload upload; AsyncWebServer server; server.handlers_.push_back(&upload);
    httpd_req_t request{4096, mode};
    const int result = server.handle_raw_body_(&request, "application/vnd.espcontrol.wallpaper");
    assert(upload.ends == 1 && upload.buffer.empty());
    assert(mode ? upload.requests == 0 : upload.requests == 1);
    assert(mode ? result != ESP_OK : result == ESP_OK);
    assert(mode ? upload.chunks == 1 : upload.chunks == 4);
  }
  Upload upload; AsyncWebServer server; server.handlers_.push_back(&upload);
  httpd_req_t too_large{8192, 0};
  assert(server.handle_raw_body_(&too_large, "application/vnd.espcontrol.wallpaper") == ESP_FAIL);
  assert(upload.ends == 1 && upload.chunks == 0);
  upload.authorized = false;
  httpd_req_t unauthorized{4096, 0};
  assert(server.handle_raw_body_(&unauthorized, "application/vnd.espcontrol.wallpaper") == ESP_OK);
  assert(upload.ends == 2 && upload.chunks == 0);
}
'''
with tempfile.TemporaryDirectory(prefix='wallpaper-upload-') as temp:
    path = Path(temp)
    (path / 'test.cpp').write_text(fixture)
    subprocess.run([os.environ.get('CXX', 'c++'), '-std=c++17', '-Wall', '-Wextra', '-Werror',
                    '-Wno-unused-parameter', str(path / 'test.cpp'), '-o', str(path / 'test')], check=True)
    subprocess.run([str(path / 'test')], check=True)
print('Raw upload buffers are released on success, timeout, disconnect, size rejection and denied authorization.')
