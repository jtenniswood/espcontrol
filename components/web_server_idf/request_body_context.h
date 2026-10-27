#pragma once

namespace esphome::web_server_idf {

// A raw-body handler can attach temporary state to one request. The request
// owns it through receive, dispatch and response, including interrupted reads.
class RequestBodyContext {
public:
  virtual ~RequestBodyContext() = default;
};

} // namespace esphome::web_server_idf
