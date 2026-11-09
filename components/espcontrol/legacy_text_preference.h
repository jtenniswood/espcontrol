#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace espcontrol::configuration {

// TemplateText's deployed preference is one length byte followed by max_length
// bytes. Drain the SDK queue around each write, and compare durable storage
// rather than the SDK saver's cached string (which can outlive a failed sync).
// Backend copies save() data; neither stack window escapes this call.
template <typename Backend, typename Sync>
bool persist_legacy_text(Backend &backend, Sync sync, const char *value,
                         size_t size, size_t maximum) {
  if (maximum > UINT8_MAX || size > maximum || (!value && size))
    return false;
  if (!sync())
    return false;
  uint8_t window[UINT8_MAX + 1]{};
  const size_t bytes = maximum + 1;
  if (backend.load(window, bytes) && window[0] == size &&
      (!size || std::memcmp(window + 1, value, size) == 0))
    return true;
  std::memset(window, 0, bytes);
  window[0] = static_cast<uint8_t>(size);
  if (size)
    std::memcpy(window + 1, value, size);
  return backend.save(window, bytes) && sync();
}

} // namespace espcontrol::configuration
