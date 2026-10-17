"""Exercise production upload dispatch before and after multipart completion."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
server = (ROOT / "components/web_server_idf/web_server_idf.cpp").read_text()
method = re.search(
    r"esp_err_t AsyncWebServer::handle_multipart_upload_\([^\n]+\) \{.*?^}",
    server, re.M | re.S,
)
assert method, "Production multipart upload method not found"

# The existing header-fragment test covers MultipartReader. This harness drives
# its data/part/body lifecycle independently to exercise the real HTTP handler's
# decision to finalise firmware, including errors after an otherwise valid file.
source = r'''
#include <algorithm>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#define ESP_LOGE(...) do {} while (false)
#define ESP_LOGW(...) do {} while (false)
#define ESP_LOGV(...) do {} while (false)
using esp_err_t = int;
constexpr int ESP_OK=0, ESP_FAIL=-1, ESP_ERR_TIMEOUT=-2;
constexpr int HTTPD_400_BAD_REQUEST=400, HTTPD_404_NOT_FOUND=404;
constexpr int HTTPD_408_REQ_TIMEOUT=408, HTTPD_SOCK_ERR_TIMEOUT=-3;
struct httpd_req_t {
  std::string body;
  size_t content_len, offset=0;
  int status=200;
};
int httpd_req_recv(httpd_req_t *r,char *out,size_t) {
  if (r->body[r->offset]=='T') return HTTPD_SOCK_ERR_TIMEOUT;
  *out=r->body[r->offset++];
  return 1;
}
void httpd_resp_send_err(httpd_req_t *r,int status,const char *) {r->status=status;}
void vTaskDelay(int) {}
class AsyncWebServerRequest {
 public:
  explicit AsyncWebServerRequest(httpd_req_t *) {}
};
class AsyncWebHandler {
 public:
  size_t bytes=0, finalisations=0, requests=0;
  bool canHandle(AsyncWebServerRequest *) {return true;}
  void handleUpload(AsyncWebServerRequest *,const std::string &,size_t,
                    uint8_t *,size_t count,bool final) {
    bytes+=count;
    if (final) ++finalisations;
  }
  void handleRequest(AsyncWebServerRequest *) {++requests;}
};
bool parse_multipart_boundary(const char *,const char **start,size_t *length) {
  *start="test";*length=4;return true;
}
class MultipartReader {
 public:
  struct Part {std::string filename="firmware.bin";} part;
  explicit MultipartReader(const std::string &) {}
  void set_data_callback(std::function<void(const uint8_t *,size_t)> callback) {data_=callback;}
  void set_part_complete_callback(std::function<void()> callback) {part_=callback;}
  bool has_file() const {return current_file_;}
  const Part &get_current_part() const {return part;}
  bool is_complete() const {return body_complete_ && file_complete_;}
  size_t parse(const char *data,size_t len) {
    switch (*data) {
      case 'D': current_file_=has_data_=true;data_(reinterpret_cast<const uint8_t *>("F"),1);break;
      case 'P': part_();file_complete_=file_complete_ || (current_file_ && has_data_);
                current_file_=has_data_=false;break;
      case 'N': current_file_=has_data_=false;part_();break;
      case 'E': body_complete_=true;break;
      case 'X': return 0;
    }
    return len;
  }
 private:
  std::function<void(const uint8_t *,size_t)> data_;
  std::function<void()> part_;
  bool current_file_=false,has_data_=false,file_complete_=false,body_complete_=false;
};
class AsyncWebServer {
 public:
  std::vector<AsyncWebHandler *> handlers_;
  esp_err_t handle_multipart_upload_(httpd_req_t *,const char *);
};
'''
source += method.group(0) + "\n"
source += r'''
bool check(const char *name,const std::string &body,bool success,int status=400) {
  AsyncWebHandler handler;
  AsyncWebServer server;
  server.handlers_.push_back(&handler);
  httpd_req_t request{body,body.size()};
  int result=server.handle_multipart_upload_(&request,"multipart/form-data; boundary=test");
  const bool passed=success
    ? result==ESP_OK && request.status==200 && handler.finalisations==1 && handler.requests==1
    : result!=ESP_OK && request.status==status && handler.finalisations==0 && handler.requests==0;
  if (!passed) std::cerr << name << ": status=" << request.status
      << " finalisations=" << handler.finalisations << " requests=" << handler.requests << '\n';
  return passed;
}
int main() {
  if (!check("complete file without terminal body","DP",false)) return 1;
  if (!check("malformed body after complete file","DPX",false)) return 1;
  if (!check("timeout after complete file","DPT",false,408)) return 1;
  if (!check("truncated file","D",false)) return 1;
  if (!check("no firmware file","NE",false)) return 1;
  if (!check("second nonempty firmware file","DPDPE",false)) return 1;
  if (!check("complete fragmented file","DDPE",true)) return 1;
  if (!check("fields before and after file","NDPNE",true)) return 1;
}
'''
with tempfile.TemporaryDirectory(prefix="multipart-completion-") as directory:
    directory = Path(directory)
    cpp, exe = directory / "test.cpp", directory / "test"
    cpp.write_text(source)
    subprocess.run([
        os.environ.get("CXX", "c++"), "-std=c++20", "-Wall", "-Wextra", "-Werror",
        str(cpp), "-o", str(exe),
    ], check=True)
    subprocess.run([str(exe)], check=True)
print("Multipart upload completion regression passed.")
