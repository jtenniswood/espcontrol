#pragma once
#include <memory>
#include <optional>
#include "esp_http_server.h"
namespace esphome {
class StringRef {
 public:
  explicit StringRef(const char *text):text_(text){}
  const char *c_str() const {return text_;}
 private:
  const char *text_;
};
namespace web_server_idf {
struct RequestBodyContext {virtual ~RequestBodyContext()=default;};
class AsyncWebServerRequest {
 public:
  static constexpr size_t URL_BUF_SIZE=128;
  explicit AsyncWebServerRequest(httpd_req_t &request):raw_(request){}
  int method() const {return raw_.method;}
  StringRef url_to(char *buffer) const {std::strcpy(buffer,raw_.path.c_str());return StringRef(buffer);}
  std::optional<std::string> get_header(const char *key) const {
    auto it=raw_.headers.find(key);
    if(it==raw_.headers.end())return {}; return it->second;
  }
  size_t contentLength() const {return raw_.length;}
  bool authenticate(const char *,const char *) const {return raw_.authenticated;}
  void requestAuthentication() {raw_.status="401 Unauthorized";}
  operator httpd_req_t *() {return &raw_;}
  void set_body_context(std::unique_ptr<RequestBodyContext> context) {body_=std::move(context);}
  RequestBodyContext *body_context() {return body_.get();}
 private:
  httpd_req_t &raw_;
  std::unique_ptr<RequestBodyContext> body_;
};
class AsyncWebHandler {
 public:
  virtual ~AsyncWebHandler()=default;
  virtual bool canHandle(AsyncWebServerRequest *) const {return false;}
  virtual size_t maximumBodySize() const {return 0;}
  virtual bool canReceiveBody(AsyncWebServerRequest *) {return false;}
  virtual void handleBody(AsyncWebServerRequest *,uint8_t *,size_t,size_t,size_t) {}
  virtual void handleRequest(AsyncWebServerRequest *) {}
};
class AsyncWebServer {public:void addHandler(AsyncWebHandler *) {}};
}  // namespace web_server_idf
}  // namespace esphome
