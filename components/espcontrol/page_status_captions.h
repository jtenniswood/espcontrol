#pragma once
namespace espcontrol::pages {
struct PageStatusCaptions {
  // Return the original input or a firmware-lifetime string. Supply both callbacks for a locale.
  const char* (*text)(const char*){nullptr};
  const char* (*key)(const char*){nullptr};
};
}  // namespace espcontrol::pages
