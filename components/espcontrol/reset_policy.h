#pragma once

#include <cstdint>
#include <cstring>
#include <string>

namespace espcontrol::reset {
enum class Mode : uint8_t { NONE = 0, CUSTOMIZATION = 1, FACTORY = 2 };
enum class Stage : uint8_t { COMPLETE = 0, REQUESTED = 1, PANEL_CLEARED = 2, PREFERENCES_CLEARED = 3 };
struct Journal {
  uint32_t magic{0x52535431};
  uint32_t epoch{0};
  Mode mode{Mode::NONE};
  Stage stage{Stage::COMPLETE};
  uint8_t reserved[2]{};
  bool pending() const { return stage != Stage::COMPLETE; }
  bool valid() const {
    return magic == 0x52535431 && reserved[0] == 0 && reserved[1] == 0 &&
           static_cast<unsigned>(stage) <= 3 &&
           (pending() ? (mode == Mode::CUSTOMIZATION || mode == Mode::FACTORY) : mode == Mode::NONE);
  }
};
inline Mode parse_mode(const std::string &mode) {
  return mode == "customization" ? Mode::CUSTOMIZATION : mode == "factory" ? Mode::FACTORY : Mode::NONE;
}
// ESPHome 2026.8.2 WiFiComponent::start and APIServer::setup. Keep this adapter
// separate: these preference keys are an upstream compatibility boundary.
inline uint32_t wifi_preference_key(bool compiled_networks, uint32_t config_hash) {
  return compiled_networks ? config_hash : 88491487UL;
}
// ESPHome 2026.8.2 RestoringGlobalsComponent<bool> uses this salt XOR
// the generated name hash for the P4-86 factory_wifi_reset_done global.
// Clearing it would run the one-time Wi-Fi wipe again after a partial reset.
inline constexpr uint32_t FACTORY_WIFI_RESET_DONE_KEY = 1944399030U ^ 1124703304U;
inline bool preserve_key(Mode mode, const std::string &ns, const std::string &key, uint32_t wifi_key) {
  if (ns == "espcontrol_rst") return true;
  if (mode == Mode::FACTORY) return false;
  if (ns == "espcontrol_cfg") return false;
  if (ns != "esphome") return true;  // Platform network/calibration records.
  return key == std::to_string(wifi_key) || key == "88491486" ||
         key == std::to_string(FACTORY_WIFI_RESET_DONE_KEY);
}
class Storage {
 public:
  virtual ~Storage() = default;
  virtual bool read(Journal &journal) = 0;
  virtual bool write(const Journal &journal) = 0;
  virtual bool clear_panel() = 0;
  virtual bool clear_preferences(Mode mode) = 0;
  virtual bool verify(Mode mode) = 0;
};
enum class Result { ACCEPTED, CONFLICT, FAILED };
inline Result request(Storage &storage, Journal &journal, Mode mode) {
  if (mode == Mode::NONE) return Result::CONFLICT;
  if (journal.pending()) return journal.mode == mode ? Result::ACCEPTED : Result::CONFLICT;
  if (journal.epoch == UINT32_MAX) return Result::FAILED;
  Journal next = journal;
  ++next.epoch;
  next.mode = mode;
  next.stage = Stage::REQUESTED;
  if (!storage.write(next)) return Result::FAILED;
  journal = next;
  return Result::ACCEPTED;
}
inline bool resume(Storage &storage, Journal &journal) {
  if (!journal.valid()) return false;
  if (!journal.pending()) return true;
  auto advance = [&](Stage stage) {
    Journal next = journal;
    next.stage = stage;
    if (stage == Stage::COMPLETE) next.mode = Mode::NONE;
    if (!storage.write(next)) return false;
    journal = next;
    return true;
  };
  if (journal.stage == Stage::REQUESTED &&
      (!storage.clear_panel() || !advance(Stage::PANEL_CLEARED))) return false;
  if (journal.stage == Stage::PANEL_CLEARED &&
      (!storage.clear_preferences(journal.mode) || !advance(Stage::PREFERENCES_CLEARED))) return false;
  return storage.verify(journal.mode) && advance(Stage::COMPLETE);
}
inline bool same_origin(const std::string &origin, const std::string &host,
                        const std::string &fetch_site, const std::string &intent) {
  if (host.empty() || intent != "reset" || (!fetch_site.empty() && fetch_site != "same-origin")) return false;
  return origin.empty() || origin == "http://" + host;
}
inline bool write_requires_epoch(const std::string &uri) {
  const auto path = uri.substr(0, uri.find('?'));
  // ESPHome owns these forms. They do not edit the panel configuration and
  // cannot supply EspControl's editing-session header.
  if (path == "/wifisave" || path == "/update") return false;
  // Standard ESPHome control clients do not participate in web editing
  // sessions. Keep operational entity actions usable without an epoch.
  // Text, number, select and switch routes contain the panel's settings and
  // continue to require it, as do native configuration and update endpoints.
  for (const char *prefix : {"/light/", "/button/", "/fan/", "/cover/",
                             "/climate/", "/lock/", "/valve/", "/alarm_control_panel/"}) {
    if (path.compare(0, std::strlen(prefix), prefix) == 0) return false;
  }
  return true;
}
inline bool allow_web_write(bool initialized, bool reset_pending, const std::string &uri,
                            bool epoch_supplied, bool epoch_matches) {
  // A supplied stale epoch is always rejected, including operational actions
  // queued by an editor before reset. Pending reset blocks every mutation.
  return initialized && !reset_pending &&
         (epoch_supplied ? epoch_matches : !write_requires_epoch(uri));
}
}  // namespace espcontrol::reset
