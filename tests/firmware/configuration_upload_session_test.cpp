#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "configuration_upload_session.h"

namespace {

using namespace espcontrol::configuration;

bool sequential_upload_completes_without_growth() {
  std::array<uint8_t, 16> buffer{};
  ConfigurationUploadSession upload(buffer.data(), buffer.size());
  const uint8_t first[] = {1, 2};
  const uint8_t second[] = {3, 4};
  return upload.begin(7, 12, 1, 4) == UploadStatus::OK &&
         upload.append(7, 0, first, sizeof(first)) == UploadStatus::OK &&
         upload.inspect(7) == UploadStatus::INCOMPLETE &&
         upload.append(7, 2, second, sizeof(second)) == UploadStatus::OK &&
         upload.inspect(7) == UploadStatus::OK &&
         upload.expected_revision() == 12 && upload.document_version() == 1 &&
         std::memcmp(buffer.data(), "\x01\x02\x03\x04", 4) == 0;
}

bool newest_begin_coalesces_stale_work() {
  std::array<uint8_t, 8> buffer{};
  ConfigurationUploadSession upload(buffer.data(), buffer.size());
  const uint8_t stale[] = {1, 2};
  const uint8_t current[] = {8, 9};
  if (upload.begin(1, 3, 1, 4) != UploadStatus::OK ||
      upload.append(1, 0, stale, sizeof(stale)) != UploadStatus::OK ||
      upload.begin(2, 3, 1, 2) != UploadStatus::OK) {
    return false;
  }
  return upload.append(1, 2, stale, sizeof(stale)) ==
             UploadStatus::STALE_UPLOAD &&
         upload.append(2, 0, current, sizeof(current)) == UploadStatus::OK &&
         upload.inspect(2) == UploadStatus::OK && buffer[0] == 8 &&
         buffer[1] == 9;
}

bool invalid_sizes_and_order_are_rejected() {
  std::array<uint8_t, 4> buffer{};
  ConfigurationUploadSession upload(buffer.data(), buffer.size());
  const uint8_t bytes[] = {1, 2, 3};
  return upload.begin(1, 0, 1, 5) == UploadStatus::TOO_LARGE &&
         upload.begin(0, 0, 1, 1) == UploadStatus::INVALID_ARGUMENT &&
         upload.begin(2, 0, 1, 3) == UploadStatus::OK &&
         upload.append(2, 1, bytes, 1) == UploadStatus::OUT_OF_ORDER &&
         upload.append(2, 0, bytes, sizeof(bytes)) == UploadStatus::OK &&
         upload.append(2, 3, bytes, 1) == UploadStatus::INVALID_ARGUMENT;
}

}  // namespace

int main() {
  return sequential_upload_completes_without_growth() &&
                 newest_begin_coalesces_stale_work() &&
                 invalid_sizes_and_order_are_rejected()
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
