#pragma once

#include <cstdint>
#include <string_view>

namespace espcontrol::pages {
class PageImageEnvironment {
 public:
  virtual ~PageImageEnvironment()=default;
  // Borrowed settings last for this main-loop call. Advance generation when
  // the HA endpoint, base URL or HTTP credentials change.
  virtual std::string_view base_url()const=0;
  virtual uint32_t endpoint_generation()const=0;
  // Preserve the existing modal/headroom/decoder policy before a request.
  virtual bool request_allowed(uint16_t width,uint16_t height)const=0;
};
using PageCoverArtEnvironment=PageImageEnvironment;
} // namespace espcontrol::pages
