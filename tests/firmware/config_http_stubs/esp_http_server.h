#pragma once
#include <cstring>
#include <map>
#include <string>
#include <vector>

enum { HTTP_GET, HTTP_PUT };
constexpr int HTTPD_RESP_USE_STRLEN = -1;
constexpr int HTTPD_400_BAD_REQUEST = 400, HTTPD_404_NOT_FOUND = 404,
              HTTPD_500_INTERNAL_SERVER_ERROR = 500;
struct httpd_req_t {
  int method{HTTP_GET};
  std::string path{"/api/v1/config"}, status{"200 OK"}, type;
  size_t length{0};
  bool authenticated{true};
  std::map<std::string, std::string> headers, sent_headers;
  std::map<std::string, const char *> borrowed_headers;
  std::vector<uint8_t> response;
};
inline int httpd_resp_set_status(httpd_req_t *r, const char *status) { r->status=status; return 0; }
inline int httpd_resp_set_type(httpd_req_t *r, const char *type) { r->type=type; return 0; }
inline int httpd_resp_set_hdr(httpd_req_t *r, const char *key, const char *value) {
  r->borrowed_headers[key]=value; return 0;
}
inline int httpd_resp_send(httpd_req_t *r, const char *body, int length) {
  for (auto &[key,value]:r->borrowed_headers) r->sent_headers[key]=value;
  r->borrowed_headers.clear();
  if (length==HTTPD_RESP_USE_STRLEN) length=std::strlen(body);
  if (length) r->response.assign(body,body+length); else r->response.clear();
  return 0;
}
inline int httpd_resp_send_err(httpd_req_t *r, int code, const char *message) {
  r->status=std::to_string(code); return httpd_resp_send(r,message,HTTPD_RESP_USE_STRLEN);
}
