"""Execute generated webhook senders and ensure their diagnostics omit secrets."""
from pathlib import Path
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))
from generate_device_slots import cfg_lines
from product_schema import slot_devices


def sender(source):
    match = re.search(r"register_webhook_sender\((\[\].*?\n\s*\})\);", source, re.S)
    assert match, "missing generated sender"
    return match[1]


devices = slot_devices()
generated = sender("\n".join(cfg_lines(devices[0])))
def normalise_indentation(value):
    return "\n".join(line.strip() for line in value.splitlines())

for device in devices:
    assert sender("\n".join(cfg_lines(device))) == generated
    source = (ROOT / "devices" / device["slug"] / "device/sensors.yaml").read_text()
    assert normalise_indentation(sender(source)) == normalise_indentation(generated), device["slug"]

code = r'''
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
std::string logs;
void record_log(const char *, const char *format, ...) {
  char buffer[512]; va_list args; va_start(args, format);
  vsnprintf(buffer, sizeof(buffer), format, args); va_end(args);
  logs += buffer; logs += '\n';
}
#define ESP_LOGW(...) record_log(__VA_ARGS__)
#define ESP_LOGI(...) record_log(__VA_ARGS__)
#define id(value) value
namespace esphome::http_request {
struct Header { std::string name, value; };
bool is_success(int status) { return status >= 200 && status < 300; }
}
struct Response { int status_code; bool ended = false; void end() { ended = true; } };
struct Http {
  bool start_failed = false;
  int status = 200;
  std::shared_ptr<Response> response;
  std::string sent_url, sent_body;
  std::vector<esphome::http_request::Header> sent_headers;
  std::shared_ptr<Response> start(const std::string &url, const std::string &,
      const std::string &body, const std::vector<esphome::http_request::Header> &headers) {
    sent_url = url; sent_body = body; sent_headers = headers;
    response = start_failed ? nullptr : std::make_shared<Response>(Response{status});
    return response;
  }
} http_req;
auto send_webhook = ''' + generated + r''';
int main() {
  const std::string url = "https://user:password@host.invalid/private-hook?token=query-secret";
  const std::string body = "body-secret";
  const std::vector<esphome::http_request::Header> headers = {{"Authorization", "Bearer header-secret"}};
  for (int status : {0, 200, 401, 500}) {
    logs.clear(); http_req.start_failed = status == 0; http_req.status = status;
    assert(send_webhook(url, "POST", body, headers) == (status == 200));
    assert(http_req.sent_url == url && http_req.sent_body == body);
    assert(http_req.sent_headers[0].value == "Bearer header-secret");
    assert(logs.find("POST") != std::string::npos);
    for (const char *secret : {"user", "password", "host.invalid", "private-hook", "query-secret", "body-secret", "header-secret"})
      assert(logs.find(secret) == std::string::npos);
    if (http_req.response) assert(http_req.response->ended);
  }
  puts("Generated webhook success/failure diagnostics preserve requests without logging secrets.");
}
'''
with tempfile.TemporaryDirectory(prefix="webhook-log-secrets-") as temporary:
    source = Path(temporary) / "test.cpp"
    binary = Path(temporary) / "test"
    source.write_text(code)
    compiler = shlex.split(os.environ.get("CXX", sys.argv[1] if len(sys.argv) > 1 else "c++"))
    subprocess.run(compiler + ["-std=c++17", "-Wall", "-Wextra", "-Werror", str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
