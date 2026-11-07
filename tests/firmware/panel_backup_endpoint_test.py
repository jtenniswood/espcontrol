"""Exercise the production HTTP handler with transport/ESPHome test doubles."""
import os
from pathlib import Path
import re
import shlex
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
HEADER = ROOT / "components/espcontrol/panel_backup_endpoint.h"
PREFIX = r'''#include <algorithm>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <ctime>
#include <functional>
#include <map>
#include <string>
#include <vector>
#include "panel_backup.h"
#include "panel_backup_document.h"
#include "nested_page_fixture.h"
#include "panel_config_http_context.h"
#define USE_WEBSERVER
#define USE_WEBSERVER_AUTH
constexpr int HTTP_GET=1, HTTP_POST=2, ESP_OK=0, HTTPD_RESP_USE_STRLEN=-1;
thread_local bool on_main_loop=false;
namespace esphome {
struct Scheduler {
  bool delay=false;
  std::function<void()> pending;
  void set_timeout(void *, const char *, unsigned, std::function<void()> callback) {
    if (delay) { pending=std::move(callback); return; }
    std::thread worker([callback]() { on_main_loop=true; callback(); });
    worker.join();
  }
};
struct Application { Scheduler scheduler; } App;
}
struct httpd_req_t {
  int status=200, chunks=0, fail_after=-1;
  bool complete=false;
  std::string body, type;
  std::map<std::string,std::string> headers;
};
int httpd_resp_set_status(httpd_req_t *r, const char *v) { r->status=std::atoi(v); return 0; }
int httpd_resp_send(httpd_req_t *r, const char *v, int n) {
  r->body.assign(v,n==HTTPD_RESP_USE_STRLEN?std::strlen(v):size_t(n));
  return 0;
}
int httpd_resp_set_type(httpd_req_t *r, const char *v) { r->type=v; return 0; }
int httpd_resp_set_hdr(httpd_req_t *r, const char *k, const char *v) { r->headers[k]=v; return 0; }
int httpd_resp_send_chunk(httpd_req_t *r, const char *v, size_t n) {
  if (r->fail_after>=0 && r->chunks>=r->fail_after) return -1;
  ++r->chunks;
  if (v==nullptr) r->complete=true;
  else r->body.append(v,n);
  return 0;
}
// JSON allocation itself is covered by the pinned ESPHome build; these doubles
// let the handler's error, authorization and streaming paths run on the host.
struct JsonObject;
struct JsonValue {
  template<class T> JsonValue &operator=(const T &) { return *this; }
  template<class T> T to() { return T(); }
};
struct JsonObject { JsonValue operator[](const char *) { return {}; } };
bool allocation_failed=false;
struct JsonDocument {
  explicit JsonDocument(void *) {}
  JsonValue operator[](const char *) { return {}; }
  bool overflowed() const { return allocation_failed; }
};
void serializeJson(JsonObject, std::string &out) { out="{}"; }
namespace esphome::json { void *heap_json_allocator() { return nullptr; } }
namespace esphome::web_server_idf {
class AsyncWebServerRequest {
 public:
  static constexpr size_t URL_BUF_SIZE=128;
  int method_value=HTTP_GET, challenges=0, auth_calls=0;
  bool authorized=true;
  std::string path="/api/v1/backup";
  httpd_req_t response;
  int method() const { return method_value; }
  std::string url_to(char *) const { return path; }
  operator httpd_req_t *() { return &response; }
  bool authenticate(const char *user, const char *password) {
    ++auth_calls;
    assert(std::string(user)=="user" && std::string(password)=="pass");
    return authorized;
  }
  void requestAuthentication() { ++challenges; response.status=401; }
  void send(int status, const char *, const char *body) { response.status=(status==200||status==404||status==409)?status:500; response.body=body; }
};
class AsyncWebHandler {
 public:
  virtual ~AsyncWebHandler()=default;
  virtual bool canHandle(AsyncWebServerRequest *) const=0;
  virtual void handleRequest(AsyncWebServerRequest *)=0;
};
class AsyncWebServer {
 public:
  std::vector<AsyncWebHandler *> handlers;
  ~AsyncWebServer() { for(auto *h:handlers) delete h; }
  void addHandler(AsyncWebHandler *h) { handlers.push_back(h); }
};
}
namespace espcontrol {
struct TestIdentity {
  bool available=true;
  bool ready() const { return available; }
  const char *username() const { return "user"; }
  const char *password() const { return "pass"; }
  std::string saved_name() const { return "Kitchen"; }
  std::string target_hostname() const { return "kitchen-ab1234"; }
  std::string suffix() const { return "ab1234"; }
};
TestIdentity identity;
TestIdentity *panel_identity=&identity;
}
namespace espcontrol::configuration {
class ConfigurationService {
 public:
  bool readable=true;
  size_t loads=0;
  uint16_t version=1;
  std::vector<uint8_t> document;
  struct Result { bool success; size_t document_size; uint16_t document_version; bool ok() const { return success; } };
  Result load(uint8_t *out,size_t capacity) {
    assert(on_main_loop);
    ++loads;
    assert(capacity>=document.size());
    std::copy(document.begin(),document.end(),out);
    return {readable,document.size(),version};
  }
};
enum class ConfigurationDispatchStatus { COMPLETED, BUSY };
class ConfigurationMainLoop {
 public:
  ConfigurationService &service;
  bool busy=false;
  struct Transaction { ConfigurationDispatchStatus dispatch; ConfigurationService::Result result; };
  Transaction load(uint8_t *out,size_t capacity) {
    if(busy)return {ConfigurationDispatchStatus::BUSY,{false,0,1}};
    Transaction result{ConfigurationDispatchStatus::COMPLETED,{false,0,1}};
    std::thread worker([&]{on_main_loop=true;result.result=service.load(out,capacity);});worker.join();return result;
  }
};
bool native_allocation_failed=false;
int native_remaining=-1;
size_t native_buffers=0;
uint8_t *allocate_native(size_t size) {
  if(native_allocation_failed||native_remaining==0)return nullptr;
  if(native_remaining>0)--native_remaining;
  auto *result=static_cast<uint8_t*>(std::malloc(size));
  if(result)++native_buffers;
  return result;
}
void release_native(uint8_t *buffer) {--native_buffers;std::free(buffer);}
}
'''
SUFFIX = r'''
int main() {
  using namespace espcontrol;
  using namespace espcontrol::backup;
  using namespace espcontrol::configuration;
  using esphome::web_server_idf::AsyncWebServerRequest;
  PanelBackupHandler handler;
  AsyncWebServerRequest request;
  assert(handler.canHandle(&request));
  request.method_value=HTTP_POST;
  assert(!handler.canHandle(&request));
  request.method_value=HTTP_GET; request.path="/api/v1/backup-extra";
  assert(!handler.canHandle(&request));
  request={}; request.authorized=false;
  handler.handleRequest(&request);
  assert(request.response.status==401 && request.challenges==1);
  assert(!request.response.complete);
  request={}; handler.handleRequest(&request);
  assert(request.response.status==503);
  assert(request.auth_calls==1);
  uint8_t buffer[12000]{};
  ConfigurationService service;
  service.document.resize(sizeof(buffer));
  PanelConfigWriter writer(service.document.data(),service.document.size());
  const std::string profile="panel-test", card="light.test;Test;Auto;Auto;;;;;"+std::string(3500,'x');
  assert(writer.begin()==PanelConfigStatus::OK);
  assert(writer.append_device_profile(reinterpret_cast<const uint8_t *>(profile.data()),profile.size())==PanelConfigStatus::OK);
  assert(writer.append_button(1,reinterpret_cast<const uint8_t *>(card.data()),card.size())==PanelConfigStatus::OK);
  size_t size=0; assert(writer.finish(&size)==PanelConfigStatus::OK); service.document.resize(size);
  ConfigurationMainLoop transactions{service};
  bind_panel_config_http_context(service,transactions,sizeof(buffer),allocate_native,release_native,"user","pass");
  int snapshots=0;
  configure("panel-test",9,[&](JsonObject,JsonObject){ assert(on_main_loop); ++snapshots; });
  request={}; request.authorized=false; handler.handleRequest(&request);
  assert(service.loads==0 && snapshots==0 && request.response.status==401);
  native_allocation_failed=true;
  request={};handler.handleRequest(&request);
  assert(request.response.status==503&&service.loads==0&&native_buffers==0);
  native_allocation_failed=false;transactions.busy=true;
  request={};handler.handleRequest(&request);
  assert(request.response.status==503&&service.loads==0&&native_buffers==0);
  transactions.busy=false;service.version=2;
  request={};handler.handleRequest(&request);
  assert(request.response.status==500&&!request.response.complete&&snapshots==0&&native_buffers==0);
  service.version=3;request={};handler.handleRequest(&request);
  assert(request.response.status==422&&!request.response.complete&&snapshots==0&&native_buffers==0);
  service.version=2;
  const auto original_document=service.document;
  const auto &graph_limits=DEVICE_UI_PROFILES[8];
  service.document=pages::test::wire(pages::test::authored_nested(graph_limits));
  configure(graph_limits.device_profile,graph_limits.slot_count,[&](JsonObject,JsonObject){assert(on_main_loop);++snapshots;});
  request={};handler.handleRequest(&request);
  assert(request.response.status==200&&request.response.complete&&snapshots==1&&native_buffers==0);
  assert(request.response.body.find("\"version\":3")!=std::string::npos);
  assert(request.response.body.find("\"document_version\":2")!=std::string::npos);
  const int graph_chunks=request.response.chunks;
  for(int fail_after=0;fail_after<graph_chunks;++fail_after) {
    request={};request.response.fail_after=fail_after;handler.handleRequest(&request);
    assert(!request.response.complete&&native_buffers==0);
  }
  const int graph_snapshots=snapshots;
  for(int allowance:{0,1}) {
    native_remaining=allowance;
    request={};handler.handleRequest(&request);
    assert(request.response.status==503&&!request.response.complete&&native_buffers==0&&snapshots==graph_snapshots);
  }
  native_remaining=-1;
  configure("wrong-device",9,[&](JsonObject,JsonObject){++snapshots;});
  request={};handler.handleRequest(&request);
  assert(request.response.status==500&&!request.response.complete&&native_buffers==0&&snapshots==graph_snapshots);
  configure(graph_limits.device_profile,graph_limits.slot_count,[&](JsonObject,JsonObject){assert(on_main_loop);++snapshots;});
  esphome::App.scheduler.delay=true;
  { AsyncWebServerRequest timed_out;handler.handleRequest(&timed_out);
    assert(timed_out.response.status==503&&!timed_out.response.complete&&native_buffers==0); }
  assert(esphome::App.scheduler.pending&&snapshots==graph_snapshots);
  esphome::App.scheduler.pending();esphome::App.scheduler.pending={};
  assert(snapshots==graph_snapshots&&native_buffers==0);
  esphome::App.scheduler.delay=false;
  service.document=original_document;
  configure("panel-test",9,[&](JsonObject,JsonObject){assert(on_main_loop);++snapshots;});
  snapshots=0;
  service.version=1;
  service.readable=false;
  request={}; handler.handleRequest(&request);
  assert(request.response.status==503 && snapshots==0);
  service.readable=true; service.document[0]='X';
  request={}; handler.handleRequest(&request);
  assert(request.response.status==500 && snapshots==0);
  service.document[0]='E'; context().slots=0;
  request={}; handler.handleRequest(&request);
  assert(request.response.status==500 && snapshots==0);
  context().slots=9; allocation_failed=true;
  request={}; handler.handleRequest(&request);
  assert(request.response.status==500 && !request.response.complete);
  allocation_failed=false; snapshots=0;
  request={}; handler.handleRequest(&request);
  assert(request.response.status==200 && request.response.complete);
  assert(snapshots==1 && request.response.chunks>2);
  assert(request.response.type=="application/json");
  assert(request.response.headers["Cache-Control"]=="no-store");
  assert(request.response.headers["Content-Disposition"]=="attachment; filename=\"espcontrol-backup.json\"");
  assert(request.response.body.find("\"format\":\"espcontrol.backup\"")!=std::string::npos);
  assert(request.response.body.substr(request.response.body.size()-3)=="\"}}");
  request={}; request.response.fail_after=0; handler.handleRequest(&request);
  assert(!request.response.complete && request.response.chunks==0);
  // Main-loop delay must return a real 503, and a late callback must not
  // capture settings or retain the destroyed HTTP request.
  const int before_timeout=snapshots;
  esphome::App.scheduler.delay=true;
  { AsyncWebServerRequest timed_out; handler.handleRequest(&timed_out);
    assert(timed_out.response.status==503 && !timed_out.response.complete); }
  assert(snapshots==before_timeout);
  assert(esphome::App.scheduler.pending);
  esphome::App.scheduler.pending();
  esphome::App.scheduler.pending={};
  assert(snapshots==before_timeout);
  esphome::App.scheduler.delay=false;
  request={}; handler.handleRequest(&request);
  assert(request.response.status==200 && request.response.complete);
  esphome::web_server_idf::AsyncWebServer server;
  register_panel_backup_endpoint(server); register_panel_backup_endpoint(server);
  assert(server.handlers.size()==1);
  assert(native_buffers==0);
}
'''

with tempfile.TemporaryDirectory(prefix="backup-endpoint-") as temporary:
    folder = Path(temporary)
    # Keep the complete production handler body, replacing only its includes
    # with the transport and allocator boundary doubles above.
    header = re.sub(r"^#include .*\n", "", HEADER.read_text(), flags=re.MULTILINE).replace("#pragma once", "")
    source = folder / "test.cpp"
    source.write_text(PREFIX + header + SUFFIX)
    binary = folder / "test"
    subprocess.run([os.environ.get("CXX", "c++")] + shlex.split(os.environ.get("CXXFLAGS", "")) + ["-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG", "-pthread",
                    "-I", str(ROOT / "components/espcontrol"), "-I", str(ROOT / "tests/firmware"), str(source), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
print("Backup HTTP authorization, errors, headers and streaming checks passed.")
