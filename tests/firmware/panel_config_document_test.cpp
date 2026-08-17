#include <array>
#include <cstdint>
#include <cstring>
#include <stdexcept>

#include "panel_config_document.h"

using espcontrol::configuration::PanelConfigReader;
using espcontrol::configuration::PanelConfigRecord;
using espcontrol::configuration::PanelConfigRecordType;
using espcontrol::configuration::PanelConfigStatus;
using espcontrol::configuration::PanelConfigWriter;

namespace {

void expect(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}

void expect_status(PanelConfigStatus actual, PanelConfigStatus expected,
                   const char *message) {
  expect(actual == expected, message);
}

const uint8_t *bytes(const char *value) {
  return reinterpret_cast<const uint8_t *>(value);
}

void canonical_document_round_trips() {
  std::array<uint8_t, 256> buffer{};
  PanelConfigWriter writer(buffer.data(), buffer.size());
  expect_status(writer.begin(), PanelConfigStatus::OK, "writer starts");
  expect_status(writer.append_device_profile(bytes("esp32-p4-86"), 11),
                PanelConfigStatus::OK, "device profile is written");
  expect_status(writer.append_button(1, bytes("light.kitchen;Kitchen"), 21),
                PanelConfigStatus::OK, "button is written");
  expect_status(writer.append_subpage(2, bytes("1|Living room|light.sofa"), 24),
                PanelConfigStatus::OK, "subpage is written");
  expect_status(
      writer.append_setting(bytes("button_on_color"), 15, bytes("0073FF"), 6),
      PanelConfigStatus::OK, "colour setting is written");
  expect_status(
      writer.append_setting(bytes("button_order"), 12, bytes("1,2,3"), 5),
      PanelConfigStatus::OK, "order setting is written");
  size_t document_size = 0;
  expect_status(writer.finish(&document_size), PanelConfigStatus::OK,
                "writer finishes");

  const uint8_t fixture[] = {
      0x45, 0x50, 0x43, 0x46, 0x01, 0x00, 0x10, 0x00, 0x71, 0x00, 0x00, 0x00,
      0x05, 0x00, 0x00, 0x00, 0x01, 0x0b, 0x00, 'e',  's',  'p',  '3',  '2',
      '-',  'p',  '4',  '-',  '8',  '6',  0x02, 0x16, 0x00, 0x01, 'l',  'i',
      'g',  'h',  't',  '.',  'k',  'i',  't',  'c',  'h',  'e',  'n',  ';',
      'K',  'i',  't',  'c',  'h',  'e',  'n',  0x03, 0x19, 0x00, 0x02, '1',
      '|',  'L',  'i',  'v',  'i',  'n',  'g',  ' ',  'r',  'o',  'o',  'm',
      '|',  'l',  'i',  'g',  'h',  't',  '.',  's',  'o',  'f',  'a',  0x04,
      0x16, 0x00, 0x0f, 'b',  'u',  't',  't',  'o',  'n',  '_',  'o',  'n',
      '_',  'c',  'o',  'l',  'o',  'r',  '0',  '0',  '7',  '3',  'F',  'F',
      0x04, 0x12, 0x00, 0x0c, 'b',  'u',  't',  't',  'o',  'n',  '_',  'o',
      'r',  'd',  'e',  'r',  '1',  ',',  '2',  ',',  '3',
  };
  expect(document_size == sizeof(fixture),
         "fixture size matches canonical document");
  expect(std::memcmp(buffer.data(), fixture, sizeof(fixture)) == 0,
         "writer produces the shared codec fixture");

  PanelConfigReader reader(buffer.data(), document_size);
  expect_status(reader.begin(), PanelConfigStatus::OK, "reader starts");
  PanelConfigRecord record;
  expect_status(reader.next(&record), PanelConfigStatus::OK,
                "device record reads");
  expect(record.type == PanelConfigRecordType::DEVICE_PROFILE &&
             record.value_size == 11,
         "device record is intact");
  expect_status(reader.next(&record), PanelConfigStatus::OK,
                "button record reads");
  expect(record.type == PanelConfigRecordType::BUTTON && record.slot == 1,
         "button record is intact");
  expect_status(reader.next(&record), PanelConfigStatus::OK,
                "subpage record reads");
  expect(record.type == PanelConfigRecordType::SUBPAGE && record.slot == 2,
         "subpage record is intact");
  expect_status(reader.next(&record), PanelConfigStatus::OK,
                "setting record reads");
  expect_status(reader.next(&record), PanelConfigStatus::OK,
                "second setting record reads");
  expect_status(reader.next(&record), PanelConfigStatus::END,
                "reader reaches the end");
}

void malformed_documents_are_rejected() {
  std::array<uint8_t, 64> buffer{};
  PanelConfigWriter writer(buffer.data(), buffer.size());
  expect_status(writer.begin(), PanelConfigStatus::OK,
                "writer starts for bounds test");
  expect_status(writer.append_button(1, bytes("x"), 1), PanelConfigStatus::OK,
                "records may precede device profile");
  expect_status(writer.append_button(1, bytes("x"), 1),
                PanelConfigStatus::INVALID_ARGUMENT,
                "duplicate button slots are rejected");

  std::array<uint8_t, 20> too_small_buffer{};
  PanelConfigWriter too_small_writer(too_small_buffer.data(),
                                     too_small_buffer.size());
  expect_status(too_small_writer.begin(), PanelConfigStatus::OK,
                "small writer starts");
  expect_status(too_small_writer.append_device_profile(bytes("a"), 1),
                PanelConfigStatus::OK, "small writer records profile");
  expect_status(too_small_writer.append_button(1, bytes("x"), 1),
                PanelConfigStatus::BUFFER_TOO_SMALL,
                "slot capacity failures report their buffer status");

  const uint8_t malformed[] = {
      0x45, 0x50, 0x43, 0x46, 0x01, 0x00, 0x10, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x7f, 0x01, 0x00, 'x',
  };
  PanelConfigReader reader(malformed, sizeof(malformed));
  expect_status(reader.validate(), PanelConfigStatus::INVALID_DOCUMENT,
                "unknown records are rejected");

  const uint8_t invalid_utf8[] = {
      0x45, 0x50, 0x43, 0x46, 0x01, 0x00, 0x10, 0x00, 0x04, 0x00,
      0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x01, 0x00, 0xff,
  };
  PanelConfigReader invalid_utf8_reader(invalid_utf8, sizeof(invalid_utf8));
  expect_status(invalid_utf8_reader.validate(),
                PanelConfigStatus::INVALID_DOCUMENT,
                "invalid UTF-8 is rejected");
}

} // namespace

int main() {
  canonical_document_round_trips();
  malformed_documents_are_rejected();
  return 0;
}
