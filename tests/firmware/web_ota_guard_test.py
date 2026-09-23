"""Run the production OTA guard and URL decoder against equivalent request paths."""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "components/web_server_idf/web_server_idf.cpp").read_text()
utils = (ROOT / "components/web_server_idf/utils.cpp").read_text()
start = source.index("#ifdef USE_WEBSERVER_OTA_DISABLED", source.index("esp_err_t AsyncWebServer::request_post_handler"))
guard = source[start:source.index("#endif", start) + len("#endif")]
start = source.index("StringRef AsyncWebServerRequest::url_to(")
url_to = source[start:source.index("\n}\n", start) + 2]
start = utils.index("size_t url_decode(")
decode = utils[start:utils.index("\n}\n", start) + 2]
harness = r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <span>
#include <string_view>
using StringRef = std::string_view;
struct httpd_req_t { const char *uri; int status = 0; const char *body = nullptr; };
constexpr int ESP_OK = 0, HTTPD_RESP_USE_STRLEN = -1;
int httpd_resp_set_status(httpd_req_t *r, const char *status) {
  r->status = std::strcmp(status, "403 Forbidden") == 0 ? 403 : 500;
  return 0;
}
int httpd_resp_send(httpd_req_t *r, const char *body, int) { r->body = body; return 0; }
// Only the hex conversion primitive is doubled; decode and path extraction are production code.
size_t parse_hex(const char *s, size_t, uint8_t *out, size_t) {
  auto digit = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  int hi = digit(s[0]);
  if (hi < 0) return 0;
  int lo = digit(s[1]);
  if (lo < 0) return 0;
  *out = (hi << 4) | lo;
  return 2;
}
class AsyncWebServerRequest {
 public:
  static constexpr size_t URL_BUF_SIZE = 513;
  explicit AsyncWebServerRequest(httpd_req_t *r) : req_(r) {}
  StringRef url_to(std::span<char, URL_BUF_SIZE> buffer) const;
 private:
  httpd_req_t *req_;
};
''' + decode + '\n' + url_to + '\nint dispatch(httpd_req_t *r) {\n' + guard + r'''
  (void) r;
  return 42;  // Reached normal body/upload dispatch.
}
int main() {
  for (const char *uri : {"/update", "/update?source=fallback", "/up%64ate",
       "/%75pdate?source=test", "%2fupdate", "%2Fupdate",
       "/%75%70%64%61%74%65", "/update?next=%2Fwifi"}) {
    httpd_req_t r{uri};
#ifdef USE_WEBSERVER_OTA_DISABLED
    assert(dispatch(&r) == ESP_OK && r.status == 403);
    assert(std::strcmp(r.body, "Browser firmware uploads are disabled") == 0);
#else
    assert(dispatch(&r) == 42 && r.status == 0);
#endif
  }
  for (const char *uri : {"/", "/wifi", "/api/v1/config", "/update_frequency",
       "/update-old", "/update/", "/wifi?next=/update", "/up%2564ate",
       "/update%3Fsource=test", "/update%00extra", "/up%ZZdate", "/up+date"}) {
    httpd_req_t r{uri};
    assert(dispatch(&r) == 42 && r.status == 0);
  }
}
'''
with tempfile.TemporaryDirectory(prefix="web-ota-guard-") as tmp:
    cpp = Path(tmp) / "test.cpp"
    cpp.write_text(harness)
    for disabled in (False, True):
        binary = Path(tmp) / ("disabled" if disabled else "default")
        command = shlex.split(os.environ.get("CXX", "c++"))
        command += ["-std=c++20", "-Wall", "-Wextra", "-Werror"]
        if disabled:
            command += ["-DUSE_WEBSERVER_OTA_DISABLED"]
        subprocess.run(command + [str(cpp), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
print("Web OTA guard tests passed (disabled and default builds).")
