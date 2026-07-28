// =============================================================================
// CONFIG API - HTTP endpoints for reading and writing the panel configuration
// =============================================================================
// Serves the espcontrol.backup v2 envelope over HTTP so Home Assistant
// (rest_command:/rest:) and headless clients (curl, scripts) can read and write
// the panel config without the browser setup page.
//
// Auth: none of its own. Handlers register through
// esphome::web_server_base::global_web_server_base->add_handler(), which wraps
// them in ESPHome's AuthMiddlewareHandler when `web_server: auth:` is
// configured. With no auth configured these endpoints are open, exactly like
// the rest of the web_server REST API already is (include_internal: true).
//
// Body reading: the vendored web_server_idf fork only parses
// application/x-www-form-urlencoded bodies. For any other content type it falls
// through to the GET handler (web_server_idf.cpp:263) *before* consuming the
// socket or applying its size cap, so an application/json body is still unread
// when handleRequest() runs and we read it ourselves via httpd_req_recv().
// A guard in scripts/check_firmware_ha_bindings.py fails the build if that
// fallthrough ever disappears upstream.
// =============================================================================
// A macro guard, not #pragma once. The espcontrol component adds its own source
// directory to the include path (cg.add_build_flag("-I<component dir>")) while
// ESPHome separately copies component headers into the build tree and includes
// them from src/esphome.h. Those are two different file paths to the same
// header, so #pragma once does not deduplicate them and every definition here
// would collide. A macro guard is content-based and handles it.
#ifndef ESPCONTROL_CONFIG_API_H
#define ESPCONTROL_CONFIG_API_H

// Included first so USE_WEBSERVER is defined regardless of include order.
#include "esphome/core/defines.h"

#ifdef USE_WEBSERVER
#include <esp_heap_caps.h>
#include <esp_http_server.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <atomic>
#include <cctype>
#include <cinttypes>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "esphome/components/json/json_util.h"
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "esphome/core/application.h"
#include "esphome/core/log.h"
#include "esphome/core/preferences.h"
#include "entity_backup_map_generated.h"
// For ParsedCfg / parse_cfg, used to expose card configs as parsed fields
// alongside the raw stored string.
#include "button_grid_config.h"

#ifdef USE_TEXT
#include "esphome/components/text/text.h"
#endif
#ifdef USE_SELECT
#include "esphome/components/select/select.h"
#endif
#ifdef USE_NUMBER
#include "esphome/components/number/number.h"
#endif
#ifdef USE_SWITCH
#include "esphome/components/switch/switch.h"
#endif

// Largest request body we will read, from the espcontrol component's
// config_api_max_body option.
//
// The default suits every shipping profile and no device overrides it. The
// hard upper bound on an envelope is ~20 KB, on the widest panel (20 slots x
// (255-byte raw + its parsed fields) + 8 subpage chunks x 255 + settings), and a
// measured 9-slot envelope is under 4 KB. The buffer is allocated at the actual
// content_len and freed when the request ends, so a generous cap costs nothing at
// rest - it exists to reject an absurd body before reading a byte of it.
#ifndef ESPCONTROL_CONFIG_API_MAX_BODY
#define ESPCONTROL_CONFIG_API_MAX_BODY 32768
#endif

namespace espcontrol_config_api {

static const char *const TAG = "config_api";

enum class BodyRead : uint8_t { OK, EMPTY, TOO_LARGE, TIMEOUT, FAILED, NO_MEMORY };

// Request body buffer, allocated in PSRAM.
//
// This is deliberately not a std::string. Measured on the S3: free *internal*
// heap while serving a request is only ~45 KB, and reading a 20 KB body into
// internal RAM left just 24 KB free - too little to leave for WiFi/lwIP, before
// ArduinoJson has even run. PSRAM has room to spare and ArduinoJson's parser
// takes a pointer+size anyway, so a raw buffer suits the caller better too.
class Body {
 public:
  Body() = default;
  Body(const Body &) = delete;
  Body &operator=(const Body &) = delete;
  ~Body() {
    if (this->data_ != nullptr)
      heap_caps_free(this->data_);
  }

  bool allocate(size_t size) {
    // +1 so the buffer is always NUL-terminated for C-string consumers.
    this->data_ = static_cast<char *>(heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM));
    if (this->data_ == nullptr) {
      // Fall back to internal RAM: better a tight allocation than a hard failure
      // on a board without usable PSRAM.
      ESP_LOGW(TAG, "PSRAM alloc of %zu bytes failed; falling back to internal", size + 1);
      this->data_ = static_cast<char *>(heap_caps_malloc(size + 1, MALLOC_CAP_INTERNAL));
    }
    if (this->data_ == nullptr)
      return false;
    this->size_ = size;
    this->data_[size] = '\0';
    return true;
  }

  char *data() { return this->data_; }
  const char *c_str() const { return this->data_ == nullptr ? "" : this->data_; }
  size_t size() const { return this->size_; }

 protected:
  char *data_{nullptr};
  size_t size_{0};
};

// Reads the full request body into `out`. Loops because httpd_req_recv() may
// return a short read; the fork's own single-shot call (web_server_idf.cpp:277)
// does not loop and should not be copied. content_len is checked against the cap
// before a single byte is read, so an oversized body costs nothing.
inline BodyRead read_body(httpd_req_t *req, Body &out) {
  const size_t total = req->content_len;
  if (total == 0)
    return BodyRead::EMPTY;
  if (total > (size_t) ESPCONTROL_CONFIG_API_MAX_BODY)
    return BodyRead::TOO_LARGE;
  if (!out.allocate(total))
    return BodyRead::NO_MEMORY;

  size_t received = 0;
  int timeouts = 0;
  while (received < total) {
    const int ret = httpd_req_recv(req, out.data() + received, total - received);
    if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
      // recv_wait_timeout is 5s; bound the retries so a stalled client cannot
      // pin the httpd task forever.
      if (++timeouts > 8) {
        ESP_LOGW(TAG, "body read timed out after %zu/%zu bytes", received, total);
        return BodyRead::TIMEOUT;
      }
      continue;
    }
    if (ret <= 0) {
      ESP_LOGW(TAG, "body read failed at %zu/%zu bytes (ret=%d)", received, total, ret);
      return BodyRead::FAILED;
    }
    received += static_cast<size_t>(ret);
  }
  return BodyRead::OK;
}

// ---------------------------------------------------------------------------
// Entity resolution and typed read/write
// ---------------------------------------------------------------------------
// Entities are resolved by NAME rather than object_id: web_server matches names
// first and logs object_id URLs as deprecated (removal in 2026.7.0). We iterate
// rather than using App.get_*_by_key(), whose include_internal defaults to false
// while every config entity here is internal: true.

enum class WriteResult : uint8_t {
  OK,
  UNCHANGED,
  NOT_PRESENT,    // device-conditional entity (screen_rotation, screen_theme)
  UNKNOWN_FIELD,  // no such envelope field anywhere - a client error, not a device difference
  BAD_VALUE,      // not coercible, or not a valid select option
  OUT_OF_RANGE,   // number outside min/max
  TOO_LONG,       // text over max_length / under min_length
  WRONG_DOMAIN,
};

inline const char *write_result_name(WriteResult result) {
  switch (result) {
    case WriteResult::OK: return "ok";
    case WriteResult::UNCHANGED: return "unchanged";
    case WriteResult::NOT_PRESENT: return "not_present";
    case WriteResult::UNKNOWN_FIELD: return "unknown_field";
    case WriteResult::BAD_VALUE: return "bad_value";
    case WriteResult::OUT_OF_RANGE: return "out_of_range";
    case WriteResult::TOO_LONG: return "too_long";
    case WriteResult::WRONG_DOMAIN: default: return "wrong_domain";
  }
}

// A write outcome that left the entity holding the requested value. Callers use
// this rather than `== OK` so an already-correct value is not reported as failure.
inline bool write_succeeded(WriteResult result) {
  return result == WriteResult::OK || result == WriteResult::UNCHANGED;
}

#define ESPCONTROL_DEFINE_FINDER(fn, ns, type, getter) \
  inline esphome::ns::type *fn(const char *name) { \
    for (auto *obj : esphome::App.getter()) { \
      if (obj->get_name() == name) return obj; \
    } \
    return nullptr; \
  }

#ifdef USE_TEXT
ESPCONTROL_DEFINE_FINDER(find_text, text, Text, get_texts)
#endif
#ifdef USE_SELECT
ESPCONTROL_DEFINE_FINDER(find_select, select, Select, get_selects)
#endif
#ifdef USE_NUMBER
ESPCONTROL_DEFINE_FINDER(find_number, number, Number, get_numbers)
#endif
#ifdef USE_SWITCH
ESPCONTROL_DEFINE_FINDER(find_switch, switch_, Switch, get_switches)
#endif

#undef ESPCONTROL_DEFINE_FINDER

// Renders a number without trailing zeros, so 600.0 reads as "600" and a step of
// 0.5 still round-trips. Keeps the envelope stable against the browser's output.
inline std::string format_number(float value) {
  char buf[32];
  if (value == (float) (long long) value) {
    snprintf(buf, sizeof(buf), "%lld", (long long) value);
  } else {
    snprintf(buf, sizeof(buf), "%.3f", value);
    std::string out(buf);
    while (!out.empty() && out.back() == '0') out.pop_back();
    if (!out.empty() && out.back() == '.') out.pop_back();
    return out;
  }
  return std::string(buf);
}

// Reads an entity's current value as a string. `found` distinguishes "absent on
// this device" from "present but empty".
inline std::string read_entity(const char *domain, const char *name, bool *found) {
  *found = false;
#ifdef USE_TEXT
  if (strcmp(domain, "text") == 0) {
    auto *obj = find_text(name);
    if (obj == nullptr) return "";
    *found = true;
    return obj->state;
  }
#endif
#ifdef USE_SELECT
  if (strcmp(domain, "select") == 0) {
    auto *obj = find_select(name);
    if (obj == nullptr) return "";
    *found = true;
    // current_option(), not .state: Select::state is deprecated and goes away in
    // ESPHome 2026.7.0.
    return obj->current_option();
  }
#endif
#ifdef USE_NUMBER
  if (strcmp(domain, "number") == 0) {
    auto *obj = find_number(name);
    if (obj == nullptr) return "";
    *found = true;
    if (std::isnan(obj->state)) return "";
    return format_number(obj->state);
  }
#endif
#ifdef USE_SWITCH
  if (strcmp(domain, "switch") == 0) {
    auto *obj = find_switch(name);
    if (obj == nullptr) return "";
    *found = true;
    return obj->state ? "true" : "false";
  }
#endif
  return "";
}

inline bool parse_bool(const std::string &value, bool *out) {
  if (value == "true" || value == "on" || value == "1" || value == "yes") { *out = true; return true; }
  if (value == "false" || value == "off" || value == "0" || value == "no") { *out = false; return true; }
  return false;
}

// Writes an entity. Always goes through make_call().perform() so the YAML
// set_action:/on_value: side effects still fire - theme.yaml rewrites the three
// colour entities and rebuilds the grid, button_order.yaml refreshes the grid,
// core_infra.yaml rebuilds the artwork base URL. publish_state() would skip all
// of that and leave the panel visually stale.
//
// Writes are verified by reading back, because TextCall::validate_() drops an
// over-long or too-short value with only an ESP_LOGW and no error signal - a
// 271-char card config into a max_length:255 entity is otherwise a silent no-op.
#ifdef USE_TEXT
// Split out from write_entity() so the subpage chunk writer, which has already
// resolved its entities, does not have to look each one up again by name.
inline WriteResult write_text(esphome::text::Text *obj, const std::string &value,
                              std::string &detail, bool dry_run = false) {
  if (obj == nullptr) return WriteResult::NOT_PRESENT;
  if (obj->state == value) return WriteResult::UNCHANGED;
  const int len = (int) value.size();
  if (len > obj->traits.get_max_length()) {
    detail = "length " + std::to_string(len) + " exceeds max_length " +
             std::to_string(obj->traits.get_max_length());
    return WriteResult::TOO_LONG;
  }
  if (len < obj->traits.get_min_length()) {
    detail = "length " + std::to_string(len) + " below min_length " +
             std::to_string(obj->traits.get_min_length());
    return WriteResult::TOO_LONG;
  }
  if (dry_run)
    return WriteResult::OK;
  obj->make_call().set_value(value).perform();
  if (obj->state != value) {
    detail = "write was rejected by the entity";
    return WriteResult::TOO_LONG;
  }
  return WriteResult::OK;
}
#endif

inline WriteResult write_entity(const char *domain, const char *name,
                                const std::string &value, std::string &detail,
                                bool dry_run = false) {
#ifdef USE_TEXT
  if (strcmp(domain, "text") == 0)
    return write_text(find_text(name), value, detail, dry_run);
#endif
#ifdef USE_SELECT
  if (strcmp(domain, "select") == 0) {
    auto *obj = find_select(name);
    if (obj == nullptr) return WriteResult::NOT_PRESENT;
    if (obj->current_option() == value) return WriteResult::UNCHANGED;
    // Validate up front so we can report the valid options instead of letting
    // SelectCall drop the value silently.
    //
    // The option list is budgeted rather than dumped whole. The timezone select has
    // hundreds of entries, and listing them all produced a multi-kilobyte detail
    // string that was then copied into the response report - enough to exhaust
    // internal heap on a rejected import. operator new cannot throw in this build,
    // so a failed allocation aborts the firmware rather than failing the request.
    // Every option is still compared; the budget only limits what is quoted back.
    static constexpr size_t OPTIONS_DETAIL_BUDGET = 160;
    bool valid = false;
    std::string options;
    size_t option_count = 0;
    bool listed_all = true;
    for (const char *option : obj->traits.get_options()) {
      option_count++;
      if (value == option) valid = true;
      if (options.size() >= OPTIONS_DETAIL_BUDGET) {
        listed_all = false;
        continue;
      }
      if (!options.empty()) options += ", ";
      options += option;
    }
    if (!valid) {
      detail = "not a valid option; " + std::to_string(option_count) + " available";
      if (!options.empty()) {
        detail += listed_all ? ": " : ", starting with: ";
        detail += options;
        if (!listed_all) detail += ", ...";
      }
      return WriteResult::BAD_VALUE;
    }
    if (dry_run)
      return WriteResult::OK;
    obj->make_call().set_option(value).perform();
    return obj->current_option() == value ? WriteResult::OK : WriteResult::BAD_VALUE;
  }
#endif
#ifdef USE_NUMBER
  if (strcmp(domain, "number") == 0) {
    auto *obj = find_number(name);
    if (obj == nullptr) return WriteResult::NOT_PRESENT;
    char *end = nullptr;
    const float parsed = strtof(value.c_str(), &end);
    if (end == value.c_str() || std::isnan(parsed)) {
      detail = "not a number";
      return WriteResult::BAD_VALUE;
    }
    if (parsed < obj->traits.get_min_value() || parsed > obj->traits.get_max_value()) {
      detail = "outside range " + format_number(obj->traits.get_min_value()) + ".." +
               format_number(obj->traits.get_max_value());
      return WriteResult::OUT_OF_RANGE;
    }
    if (obj->state == parsed) return WriteResult::UNCHANGED;
    if (dry_run)
      return WriteResult::OK;
    obj->make_call().set_value(parsed).perform();
    return WriteResult::OK;
  }
#endif
#ifdef USE_SWITCH
  if (strcmp(domain, "switch") == 0) {
    auto *obj = find_switch(name);
    if (obj == nullptr) return WriteResult::NOT_PRESENT;
    bool target = false;
    if (!parse_bool(value, &target)) {
      detail = "expected a boolean";
      return WriteResult::BAD_VALUE;
    }
    if (obj->state == target) return WriteResult::UNCHANGED;
    if (dry_run)
      return WriteResult::OK;
    // turn_on()/turn_off(), not publish_state(), so on_turn_on/on_turn_off fire.
    if (target) obj->turn_on(); else obj->turn_off();
    return WriteResult::OK;
  }
#endif
  detail = std::string("unsupported domain ") + domain;
  return WriteResult::WRONG_DOMAIN;
}

// Looks up an envelope field. Follows backupAliases so legacy keys still resolve.
inline const EntityBackupField *lookup_field(const char *section, const std::string &field) {
  for (size_t i = 0; i < ENTITY_BACKUP_FIELD_COUNT; i++) {
    const auto &candidate = ENTITY_BACKUP_FIELDS[i];
    if (strcmp(candidate.section, section) == 0 && field == candidate.field)
      return &candidate;
  }
  for (size_t i = 0; i < ENTITY_BACKUP_ALIAS_COUNT; i++) {
    const auto &alias = ENTITY_BACKUP_ALIASES[i];
    if (strcmp(alias.section, section) == 0 && field == alias.field)
      return lookup_field(section, alias.target);
  }
  return nullptr;
}

// ---------------------------------------------------------------------------
// Derived field rules
// ---------------------------------------------------------------------------
// Six envelope fields are not a plain 1:1 entity read/write. Each rule below
// cites the JavaScript it mirrors; entity_names.json marks the field
// `derived: true` and build.py's validator keeps the two lists in step.
//
// Four of the six are label-vs-token or normalisation mismatches. The other two
// (clock_screensaver, schedule_enabled) are derived on READ only, because the
// firmware already maintains each pair in both directions:
//   clock_screensaver <-> screensaver_action   display.yaml:359-383, :458-475
//   schedule_enabled  <-> schedule_trigger     backlight_schedule.yaml:103-127, :178-187
// so a plain turn_on()/turn_off() through write_entity() lets the YAML do the
// mirroring. We read from the partner because that is what the browser exports
// (clock_bar_state.ts), and because at boot a RESTORE_DEFAULT_OFF switch can
// briefly disagree with its restored partner until the partner's on_value fires.

// Mirrors the String(v).toLowerCase().replace(/[\s-]+/g, "_") that the JS
// normalisers all start with (model/settings.ts:82, :91, :99). Leading and
// trailing separator runs collapse to a single "_" exactly as the regex does.
inline std::string canonical_token(const std::string &value) {
  std::string out;
  out.reserve(value.size());
  size_t i = 0;
  while (i < value.size()) {
    const unsigned char c = (unsigned char) value[i];
    if (isspace(c) || c == '-') {
      out += '_';
      while (i < value.size() && (isspace((unsigned char) value[i]) || value[i] == '-')) i++;
      continue;
    }
    out += (char) tolower(c);
    i++;
  }
  return out;
}

// model/settings.ts:98-103
inline std::string normalize_screensaver_action(const std::string &value) {
  const std::string action = canonical_token(value);
  if (action == "screen_dimmed" || action == "dimmed" || action == "dim") return "dim";
  if (action == "clock") return "clock";
  return "off";
}

// model/settings.ts:105-110. The select's options are display labels, so the
// envelope's canonical token has to be mapped back before writing.
inline const char *screensaver_action_option(const std::string &canonical) {
  if (canonical == "dim") return "Screen Dimmed";
  if (canonical == "clock") return "Clock";
  return "Display Off";
}

// model/settings.ts:81-88
inline std::string normalize_schedule_mode(const std::string &value) {
  const std::string mode = canonical_token(value);
  if (mode == "screen_dimmed" || mode == "dimmed" || mode == "always_on" || mode == "always")
    return "screen_dimmed";
  if (mode == "clock") return "clock";
  return "screen_off";
}

// model/settings.ts:112-117. Deliberately lossy in this direction: the entity also
// offers "Always On", which normalises to screen_dimmed, so a round-trip through
// the envelope rewrites it to "Screen Dimmed". The browser behaves the same way.
inline const char *schedule_mode_option(const std::string &canonical) {
  if (canonical == "screen_dimmed") return "Screen Dimmed";
  if (canonical == "clock") return "Clock";
  return "Screen off";
}

// model/settings.ts:276-279. Note this one does NOT canonicalise - it is an exact
// match against the three accepted tokens, with anything else meaning disabled.
inline std::string normalize_screensaver_mode(const std::string &value) {
  if (value == "sensor" || value == "timer" || value == "disabled") return value;
  return "disabled";
}

// model/settings.ts:90-96. schedule_enabled only breaks the tie when the trigger
// itself is unrecognised.
inline std::string normalize_schedule_trigger(const std::string &value, bool schedule_enabled) {
  const std::string trigger = canonical_token(value);
  if (trigger == "sensor") return "sensor";
  if (trigger == "time" || trigger == "timer") return "time";
  if (trigger == "disabled" || trigger == "off") return "disabled";
  return schedule_enabled ? "time" : "disabled";
}

// Reads a derived field. Returns false when `field` is not derived, leaving
// out/found untouched so the caller falls through to the plain map read.
inline bool derived_read(const char *section, const std::string &field, std::string *out,
                         bool *found) {
  const bool settings = strcmp(section, "settings") == 0;
  const bool screen = strcmp(section, "screen") == 0;

  // app_backup.ts -> getActiveScreensaverMode() (screensaver_state.ts).
  if (settings && field == "screensaver_mode") {
    const std::string raw = read_entity("text", "Screensaver Mode", found);
    if (*found) *out = normalize_screensaver_mode(raw);
    return true;
  }
  // app_backup.ts: the envelope carries off/dim/clock, the select stores
  // "Display Off"/"Screen Dimmed"/"Clock".
  if (settings && field == "screensaver_action") {
    const std::string raw = read_entity("select", "Screen Saver: Action", found);
    if (*found) *out = normalize_screensaver_action(raw);
    return true;
  }
  // clock_bar_state.ts: clockScreensaverOn tracks the action, not the switch.
  if (settings && field == "clock_screensaver") {
    const std::string raw = read_entity("select", "Screen Saver: Action", found);
    if (*found) *out = normalize_screensaver_action(raw) == "clock" ? "true" : "false";
    return true;
  }
  // app_backup.ts: same label-vs-token split as screensaver_action.
  if (screen && field == "schedule_mode") {
    const std::string raw = read_entity("select", "Screen: Schedule Mode", found);
    if (*found) *out = normalize_schedule_mode(raw);
    return true;
  }
  // clock_bar_state.ts / model/settings.ts:191: the trigger is authoritative.
  if (screen && field == "schedule_enabled") {
    const std::string raw = read_entity("text", "Screen: Schedule Trigger", found);
    if (!*found) return true;
    bool switch_found = false;
    const std::string sw = read_entity("switch", "Screen: Schedule Enabled", &switch_found);
    *out = normalize_schedule_trigger(raw, switch_found && sw == "true") != "disabled" ? "true"
                                                                                      : "false";
    return true;
  }
  return false;
}

// Writes a derived field. Returns false when `field` is not derived.
inline bool derived_write(const char *section, const std::string &field, const std::string &value,
                          WriteResult *result, std::string &detail, bool dry_run = false) {
  const bool settings = strcmp(section, "settings") == 0;
  const bool screen = strcmp(section, "screen") == 0;

  if (settings && field == "screensaver_mode") {
    *result =
        write_entity("text", "Screensaver Mode", normalize_screensaver_mode(value), detail, dry_run);
    return true;
  }
  if (settings && field == "screensaver_action") {
    *result = write_entity("select", "Screen Saver: Action",
                           screensaver_action_option(normalize_screensaver_action(value)), detail,
                           dry_run);
    return true;
  }
  if (screen && field == "schedule_mode") {
    *result = write_entity("select", "Screen: Schedule Mode",
                           schedule_mode_option(normalize_schedule_mode(value)), detail, dry_run);
    return true;
  }

  // The two mirrored switches. The plain switch write is normally all that is
  // needed, but if switch and partner have diverged the switch already holds the
  // requested state, write_entity() short-circuits to UNCHANGED, and the YAML
  // mirror never fires - leaving the envelope still reporting the old value.
  // Reconcile against the partner in that case so the write is observable.
  const bool clock_pair = settings && field == "clock_screensaver";
  const bool schedule_pair = screen && field == "schedule_enabled";
  if (clock_pair || schedule_pair) {
    bool want = false;
    if (!parse_bool(value, &want)) {
      detail = "expected a boolean";
      *result = WriteResult::BAD_VALUE;
      return true;
    }
    const char *switch_name = clock_pair ? "Screen Saver: Clock" : "Screen: Schedule Enabled";
    *result = write_entity("switch", switch_name, want ? "true" : "false", detail, dry_run);
    // The reconciliation below is a second write, so it is skipped when validating:
    // pass 1 only has to establish that the boolean is well formed.
    if (*result == WriteResult::UNCHANGED && !dry_run) {
      bool found = false;
      std::string current;
      derived_read(section, field, &current, &found);
      if (found && (current == "true") != want) {
        if (clock_pair) {
          *result = write_entity("select", "Screen Saver: Action",
                                 screensaver_action_option(want ? "clock" : "off"), detail, dry_run);
        } else {
          *result = write_entity("text", "Screen: Schedule Trigger", want ? "time" : "disabled",
                                 detail, dry_run);
        }
      }
    }
    return true;
  }
  return false;
}

// ---------------------------------------------------------------------------
// Envelope-field level read/write
// ---------------------------------------------------------------------------

// Reads an envelope field. `found` is false for a field whose entity is absent on
// this device - `theme` on a non-epaper panel, `screen_rotation` where the profile
// does not expose it - which the emitters treat as "omit" rather than "null".
inline std::string read_field(const char *section, const std::string &field, bool *found) {
  *found = false;
  std::string out;
  if (derived_read(section, field, &out, found))
    return out;
  const EntityBackupField *entry = lookup_field(section, field);
  if (entry == nullptr)
    return "";
  return read_entity(entry->domain, entry->entity_name, found);
}

inline WriteResult write_field(const char *section, const std::string &field,
                               const std::string &value, std::string &detail,
                               bool dry_run = false) {
  // model/settings.ts:302-309: clock_brightness is the legacy pre-day/night key,
  // and the night value falls back to it when clock_brightness_night is absent.
  // A single set therefore has to move both, or the panel keeps a night
  // brightness the envelope no longer mentions. Import resolves the explicit
  // day/night keys first and only falls back to this when neither is present.
  if (strcmp(section, "settings") == 0 && field == "clock_brightness") {
    const WriteResult day = write_field(section, "clock_brightness_day", value, detail, dry_run);
    if (!write_succeeded(day))
      return day;
    std::string night_detail;
    const WriteResult night =
        write_field(section, "clock_brightness_night", value, night_detail, dry_run);
    if (!write_succeeded(night)) {
      detail = night_detail;
      return night;
    }
    return (day == WriteResult::OK || night == WriteResult::OK) ? WriteResult::OK
                                                                : WriteResult::UNCHANGED;
  }

  WriteResult result = WriteResult::UNKNOWN_FIELD;
  if (derived_write(section, field, value, &result, detail, dry_run))
    return result;
  const EntityBackupField *entry = lookup_field(section, field);
  if (entry == nullptr) {
    detail = std::string("no envelope field ") + section + "." + field;
    return WriteResult::UNKNOWN_FIELD;
  }
  return write_entity(entry->domain, entry->entity_name, value, detail, dry_run);
}

// Card configs are written as the exact stored string, which is what GET's `raw`
// returns. That gives a byte-exact round-trip with no C++ encoder: the firmware
// has parse_cfg() but no serialiser.
inline WriteResult write_card(int slot, const std::string &value, std::string &detail,
                             bool dry_run = false) {
#ifdef USE_TEXT
  const std::string name = "Button " + std::to_string(slot) + " Config";
  auto *entity = find_text(name.c_str());
  if (entity == nullptr) {
    detail = "no card slot " + std::to_string(slot) + " on this device";
    return WriteResult::NOT_PRESENT;
  }
  return write_text(entity, value, detail, dry_run);
#else
  detail = "text entities are not compiled in";
  return WriteResult::NOT_PRESENT;
#endif
}

// Writes a pre-serialised subpage config, split across the slot's chunk entities.
// The firmware joins chunks by plain concatenation (button_grid_grid.h:1350-1357),
// so the only constraint on a split point is that it must not cut a UTF-8
// sequence - the equivalent of splitSubpageConfigChunks (model/subpage.ts:271-299).
// The chunk count is per device (4 on the S3, 8 elsewhere) and is discovered by
// probing the templated names rather than hardcoded.
inline WriteResult write_subpage(int slot, const std::string &value, std::string &detail,
                                bool dry_run = false) {
#ifdef USE_TEXT
  esphome::text::Text *chunks[ENTITY_SLOT_TEMPLATE_COUNT];
  size_t chunk_count = 0;
  size_t capacity = 0;
  for (size_t t = 0; t < ENTITY_SLOT_TEMPLATE_COUNT; t++) {
    const auto &tpl = ENTITY_SLOT_TEMPLATES[t];
    if (strncmp(tpl.key, "subpage_config", 14) != 0) continue;
    const std::string name = std::string(tpl.name_prefix) + std::to_string(slot) + tpl.name_suffix;
    auto *entity = find_text(name.c_str());
    // Stop at the first gap: a device with fewer chunks simply has fewer entities.
    if (entity == nullptr) break;
    chunks[chunk_count++] = entity;
    capacity += (size_t) entity->traits.get_max_length();
  }
  if (chunk_count == 0) {
    detail = "no subpage slot " + std::to_string(slot) + " on this device";
    return WriteResult::NOT_PRESENT;
  }
  // Checked before writing anything, so an over-long value cannot leave a
  // half-written subpage behind.
  if (value.size() > capacity) {
    detail = "length " + std::to_string(value.size()) + " exceeds the " +
             std::to_string(chunk_count) + " chunk entities' combined " +
             std::to_string(capacity) + " bytes";
    return WriteResult::TOO_LONG;
  }

  size_t offset = 0;
  bool changed = false;
  for (size_t i = 0; i < chunk_count; i++) {
    const size_t limit = (size_t) chunks[i]->traits.get_max_length();
    size_t take = value.size() - offset > limit ? limit : value.size() - offset;
    if (offset + take < value.size()) {
      // Back off over UTF-8 continuation bytes so a multi-byte character is not
      // split across two entities.
      while (take > 0 && ((unsigned char) value[offset + take] & 0xC0) == 0x80) take--;
      if (take == 0) {
        detail = "a single character is longer than the chunk length";
        return WriteResult::TOO_LONG;
      }
    }
    std::string chunk_detail;
    const WriteResult result =
        write_text(chunks[i], value.substr(offset, take), chunk_detail, dry_run);
    if (!write_succeeded(result)) {
      detail = "chunk " + std::to_string(i + 1) + ": " + chunk_detail;
      return result;
    }
    if (result == WriteResult::OK) changed = true;
    offset += take;
  }
  return changed ? WriteResult::OK : WriteResult::UNCHANGED;
#else
  detail = "text entities are not compiled in";
  return WriteResult::NOT_PRESENT;
#endif
}

// Highest N for which "Button N Config" exists. Lets the API discover the slot
// count without a per-device constant.
inline int discover_slot_count() {
#ifdef USE_TEXT
  int count = 0;
  for (int slot = 1; slot <= 64; slot++) {
    const std::string name = "Button " + std::to_string(slot) + " Config";
    if (find_text(name.c_str()) == nullptr) break;
    count = slot;
  }
  return count;
#else
  return 0;
#endif
}

// Exact path match, tolerating a query string. Deliberately not the loose
// strncmp() prefix test used by the older local_* handlers, where
// "/local_sensorsXYZ" also matched and could shadow a later handler.
inline bool path_is(const esphome::StringRef &url, const char *path) {
  const size_t len = strlen(path);
  if (strncmp(url.c_str(), path, len) != 0)
    return false;
  const char next = url.c_str()[len];
  return next == '\0' || next == '?';
}

// Appends `value` as a quoted, escaped JSON string. Shared by the chunked
// envelope writer and the small fixed-size set/import responses.
inline void append_json_string(std::string &out, const std::string &value) {
  out += '"';
  for (const char c : value) {
    switch (c) {
      case '"': out += "\\\""; break;
      case '\\': out += "\\\\"; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default:
        if ((unsigned char) c < 0x20) {
          char esc[7];
          snprintf(esc, sizeof(esc), "\\u%04x", (unsigned char) c);
          out += esc;
        } else {
          out += c;
        }
    }
  }
  out += '"';
}

// Semicolon-joins warning text, so several independent complaints from one import
// arrive as one readable line.
inline void append_warning(std::string *warning, const std::string &text) {
  if (!warning->empty())
    *warning += "; ";
  *warning += text;
}

inline void send_json(httpd_req_t *req, const char *status, const std::string &body) {
  httpd_resp_set_status(req, status);
  httpd_resp_set_type(req, "application/json");
  // We bypass the fork's init_response_(), so DefaultHeaders are not applied.
  // Mirror web_server_base.h's Access-Control-Allow-Origin explicitly.
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  const esp_err_t err = httpd_resp_send(req, body.c_str(), HTTPD_RESP_USE_STRLEN);
  if (err != ESP_OK)
    ESP_LOGE(TAG, "httpd_resp_send failed: %d", err);
}

// Headers for a chunked JSON response. Must be set before the first chunk goes out,
// so every status decision has to be made up front.
inline void begin_json_stream(httpd_req_t *req, const char *status) {
  httpd_resp_set_status(req, status);
  httpd_resp_set_type(req, "application/json");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
}

inline void send_error(httpd_req_t *req, const char *status, const char *reason) {
  std::string body = "{\"ok\":false,\"error\":\"";
  body += reason;
  body += "\"}";
  send_json(req, status, body);
}

// Maps a body-read failure onto its HTTP status and sends the response.
// Returns false so callers can `if (!ok) return;`.
inline bool report_body_error(httpd_req_t *req, BodyRead result) {
  switch (result) {
    case BodyRead::OK:
      return true;
    case BodyRead::EMPTY:
      send_error(req, "400 Bad Request", "empty_body");
      return false;
    case BodyRead::TOO_LARGE:
      send_error(req, "413 Payload Too Large", "body_too_large");
      return false;
    case BodyRead::TIMEOUT:
      send_error(req, "408 Request Timeout", "body_read_timeout");
      return false;
    case BodyRead::NO_MEMORY:
      send_error(req, "500 Internal Server Error", "out_of_memory");
      return false;
    case BodyRead::FAILED:
    default:
      send_error(req, "400 Bad Request", "body_read_failed");
      return false;
  }
}

// ---------------------------------------------------------------------------
// Chunked JSON response writer
// ---------------------------------------------------------------------------
// Streams the envelope so peak heap stays at one buffer instead of a ~20 KB
// string. This is the key difference from LocalSensorHandler, which builds a
// whole std::string and single-shots it.
class JsonStream {
 public:
  // A null req puts the stream in hash-only mode: nothing is sent and the bytes
  // are folded into an FNV-1a digest instead. That is how the ETag is computed -
  // run the same emitters twice, once to hash and once to send - which keeps the
  // digest defined by the actual serialisation rather than a parallel
  // hand-maintained one that could drift from it.
  explicit JsonStream(httpd_req_t *req) : req_(req) { this->buf_.reserve(FLUSH_AT + 256); }

  void raw(const char *text) { this->buf_ += text; this->maybe_flush_(); }
  void raw(const std::string &text) { this->buf_ += text; this->maybe_flush_(); }
  void raw(char c) { this->buf_ += c; this->maybe_flush_(); }

  void string(const std::string &value) {
    append_json_string(this->buf_, value);
    this->maybe_flush_();
  }

  // "name":<value> pair, comma-separated automatically within the current object.
  void key(const char *name) {
    if (!this->first_) this->buf_ += ',';
    this->first_ = false;
    this->buf_ += '"';
    this->buf_ += name;
    this->buf_ += "\":";
    this->maybe_flush_();
  }
  void pair(const char *name, const std::string &value) { this->key(name); this->string(value); }
  void pair_raw(const char *name, const std::string &value) { this->key(name); this->raw(value); }

  // On close, the container we just finished is itself an element of its parent,
  // so the parent always needs a separator before whatever comes next. That makes
  // first_ = false correct at every depth and removes any need to track nesting.
  void open_object() { this->raw('{'); this->first_ = true; }
  void close_object() { this->raw('}'); this->first_ = false; }
  void open_array() { this->raw('['); this->first_ = true; }
  void close_array() { this->raw(']'); this->first_ = false; }
  // Separator for array elements, which have no key to hang the comma off.
  void next_element() { if (!this->first_) this->raw(','); this->first_ = false; }

  bool finish() {
    this->flush_();
    if (this->failed_) return false;
    if (this->req_ == nullptr) return true;
    return httpd_resp_send_chunk(this->req_, nullptr, 0) == ESP_OK;
  }

  uint32_t hash() const { return this->hash_; }
  bool failed() const { return this->failed_; }

 protected:
  static constexpr size_t FLUSH_AT = 768;

  void maybe_flush_() { if (this->buf_.size() >= FLUSH_AT) this->flush_(); }
  void flush_() {
    if (this->failed_ || this->buf_.empty()) return;
    if (this->req_ == nullptr) {
      for (const char c : this->buf_) {
        this->hash_ ^= (uint8_t) c;
        this->hash_ *= 16777619u;
      }
      this->buf_.clear();
      return;
    }
    if (httpd_resp_send_chunk(this->req_, this->buf_.data(), this->buf_.size()) != ESP_OK) {
      ESP_LOGW(TAG, "chunk send failed");
      this->failed_ = true;
    }
    this->buf_.clear();
  }

  httpd_req_t *req_;
  std::string buf_;
  uint32_t hash_{2166136261u};  // FNV-1a offset basis
  bool first_{true};
  bool failed_{false};
};

// Device slug, supplied from YAML at registration (${device_slug}). Used for the
// envelope's `device` / `source.device` so an import can refuse a foreign config.
inline std::string &device_slug() {
  static std::string slug;
  return slug;
}

// switch and number emit as JSON primitives, everything else as a string. Derived
// fields keep their entity's domain, which is what makes clock_screensaver come
// out as a bare true/false rather than "true".
inline void emit_typed(JsonStream &json, const char *name, const char *domain,
                       const std::string &value) {
  if (strcmp(domain, "switch") == 0 || strcmp(domain, "number") == 0) {
    json.pair_raw(name, value.empty() ? "null" : value);
  } else {
    json.pair(name, value);
  }
}

// Emits a section's fields into the object the caller has already opened. Shared
// with the root section, whose fields sit at the top level of the envelope rather
// than in a nested object of their own.
inline void emit_section_fields(JsonStream &json, const char *section) {
  for (size_t i = 0; i < ENTITY_BACKUP_FIELD_COUNT; i++) {
    const auto &field = ENTITY_BACKUP_FIELDS[i];
    if (strcmp(field.section, section) != 0) continue;
    bool found = false;
    const std::string value = read_field(section, field.field, &found);
    if (!found) continue;  // device-conditional entity; omit rather than emit null
    emit_typed(json, field.field, field.domain, value);
  }
  // Aliases the browser's export literal also writes. Emitting them from their
  // target keeps a device-produced envelope field-for-field comparable with a
  // browser-produced one; inbound-only aliases are deliberately not emitted.
  for (size_t i = 0; i < ENTITY_BACKUP_ALIAS_COUNT; i++) {
    const auto &alias = ENTITY_BACKUP_ALIASES[i];
    if (!alias.exported || strcmp(alias.section, section) != 0) continue;
    const EntityBackupField *target = lookup_field(section, alias.target);
    if (target == nullptr) continue;
    bool found = false;
    const std::string value = read_field(section, alias.target, &found);
    if (!found) continue;
    emit_typed(json, alias.field, target->domain, value);
  }
}

inline void emit_section(JsonStream &json, const char *section) {
  json.open_object();
  emit_section_fields(json, section);
  json.close_object();
}

inline void emit_cards(JsonStream &json, int slots, int only_slot) {
  json.open_array();
#ifdef USE_TEXT
  for (int slot = 1; slot <= slots; slot++) {
    if (only_slot > 0 && slot != only_slot) continue;
    const std::string name = "Button " + std::to_string(slot) + " Config";
    auto *entity = find_text(name.c_str());
    if (entity == nullptr) continue;
    json.next_element();
    json.open_object();
    json.pair_raw("slot", std::to_string(slot));
    // `raw` is the exact stored string. Import prefers it, which gives a
    // byte-exact round-trip with no C++ encoder. The parsed fields below are for
    // clients that want to edit a single field without knowing the codec.
    json.pair("raw", entity->state);
    const ParsedCfg cfg = parse_cfg(entity->state);
    json.pair("entity", cfg.entity);
    json.pair("label", cfg.label);
    json.pair("icon", cfg.icon);
    json.pair("icon_on", cfg.icon_on);
    json.pair("sensor", cfg.sensor);
    json.pair("unit", cfg.unit);
    json.pair("type", cfg.type);
    json.pair("precision", cfg.precision);
    json.pair("options", cfg.options);
    json.close_object();
  }
#endif
  json.close_array();
}

inline void emit_subpages(JsonStream &json, int slots, int only_slot) {
  json.open_object();
#ifdef USE_TEXT
  for (int slot = 1; slot <= slots; slot++) {
    if (only_slot > 0 && slot != only_slot) continue;
    // Chunks are joined by plain concatenation, matching how the grid reads them.
    std::string joined;
    for (size_t t = 0; t < ENTITY_SLOT_TEMPLATE_COUNT; t++) {
      const auto &tpl = ENTITY_SLOT_TEMPLATES[t];
      if (strncmp(tpl.key, "subpage_config", 14) != 0) continue;
      const std::string name = std::string(tpl.name_prefix) + std::to_string(slot) + tpl.name_suffix;
      auto *entity = find_text(name.c_str());
      if (entity == nullptr) continue;
      joined += entity->state;
    }
    if (joined.empty()) continue;
    const std::string slot_key = std::to_string(slot);
    json.pair(slot_key.c_str(), joined);
  }
#endif
  json.close_object();
}

// Emits every config-bearing part of the envelope into an already-open object.
// `section` selects one part or "all"; `only_slot` narrows cards/subpages.
//
// Deliberately excludes the meta block. The ETag is an FNV-1a digest of exactly
// these bytes, and meta carries `exported_at` and the ETag itself - hashing either
// would make the digest change on every request, or be self-referential.
inline void emit_config(JsonStream &json, const std::string &section, int slots, int only_slot) {
  const bool all = section == "all";
  if (all || section == "root") {
    // Root-level envelope fields (button_order and the three colours) sit at the
    // top level of the envelope, not in a nested object.
    emit_section_fields(json, "root");
  }
  if (all || section == "settings") {
    json.key("settings");
    emit_section(json, "settings");
  }
  if (all || section == "screen") {
    json.key("screen");
    emit_section(json, "screen");
  }
  if (all || section == "cards") {
    json.key("buttons");
    emit_cards(json, slots, only_slot);
  }
  if (all || section == "subpages") {
    json.key("subpages");
    emit_subpages(json, slots, only_slot);
  }
}

// FNV-1a over the full config serialisation. Two consecutive reads with no change
// in between produce the same value, which is what makes `?if_match=` on import a
// usable guard against the browser UI editing concurrently.
inline uint32_t config_etag(int slots) {
  JsonStream hasher(nullptr);
  hasher.open_object();
  emit_config(hasher, "all", slots, 0);
  hasher.close_object();
  hasher.finish();
  return hasher.hash();
}

inline std::string etag_hex(uint32_t etag) {
  char buf[16];
  snprintf(buf, sizeof(buf), "%08" PRIx32, etag);
  return std::string(buf);
}

// ISO 8601 UTC, matching the browser's `new Date().toISOString()`
// (app_backup.ts) except for the milliseconds, which no consumer reads - import
// only does String(data.exported_at || ""). Empty when SNTP has not synced yet,
// which is honest rather than emitting a 1970 timestamp.
inline std::string iso8601_utc_now() {
  const time_t now = ::time(nullptr);
  if (now < 1600000000)  // 2020-09-13; the clock is clearly unset before this
    return "";
  struct tm utc {};
  gmtime_r(&now, &utc);
  char buf[32];
  if (strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0)
    return "";
  return std::string(buf);
}

// ---------------------------------------------------------------------------
// Config keys
// ---------------------------------------------------------------------------
// A key names one writable thing in the envelope. Bare names are root fields
// because that is how the envelope itself spells them (button_order and the three
// colours live at the top level), so `?key=button_order` reads naturally while
// `root.button_order` stays available for clients that prefer to be explicit.
struct ConfigKey {
  enum class Kind : uint8_t { FIELD, CARD, SUBPAGE };
  Kind kind{Kind::FIELD};
  const char *section{"root"};
  std::string field;
  int slot{0};
};

// Accepts "[3]" and ".3". Slots are 1-based, matching the entity names.
inline bool parse_slot_suffix(const std::string &rest, int *slot) {
  std::string digits;
  if (rest.size() >= 3 && rest.front() == '[' && rest.back() == ']') {
    digits = rest.substr(1, rest.size() - 2);
  } else if (rest.size() >= 2 && rest.front() == '.') {
    digits = rest.substr(1);
  } else {
    return false;
  }
  for (const char c : digits) {
    if (isdigit((unsigned char) c) == 0) return false;
  }
  *slot = atoi(digits.c_str());
  return *slot >= 1;
}

// settings.<field> | screen.<field> | root.<field> | <field>
// buttons[N] | buttons.N | subpages[N] | subpages.N
inline bool parse_config_key(const std::string &key, ConfigKey *out) {
  if (key.empty())
    return false;
  static const struct {
    const char *prefix;
    const char *section;
  } SECTIONS[] = {{"settings.", "settings"}, {"screen.", "screen"}, {"root.", "root"}};
  for (const auto &candidate : SECTIONS) {
    const size_t len = strlen(candidate.prefix);
    if (key.compare(0, len, candidate.prefix) != 0) continue;
    if (key.size() == len) return false;
    out->kind = ConfigKey::Kind::FIELD;
    out->section = candidate.section;
    out->field = key.substr(len);
    return true;
  }
  if (key.compare(0, 7, "buttons") == 0 && parse_slot_suffix(key.substr(7), &out->slot)) {
    out->kind = ConfigKey::Kind::CARD;
    return true;
  }
  if (key.compare(0, 8, "subpages") == 0 && parse_slot_suffix(key.substr(8), &out->slot)) {
    out->kind = ConfigKey::Kind::SUBPAGE;
    return true;
  }
  if (key.find('.') != std::string::npos || key.find('[') != std::string::npos)
    return false;
  out->kind = ConfigKey::Kind::FIELD;
  out->section = "root";
  out->field = key;
  return true;
}

inline WriteResult apply_key(const ConfigKey &key, const std::string &value, std::string &detail,
                            bool dry_run = false) {
  switch (key.kind) {
    case ConfigKey::Kind::CARD:
      return write_card(key.slot, value, detail, dry_run);
    case ConfigKey::Kind::SUBPAGE:
      return write_subpage(key.slot, value, detail, dry_run);
    case ConfigKey::Kind::FIELD:
    default:
      return write_field(key.section, key.field, value, detail, dry_run);
  }
}

// Coerces a JSON scalar to the string form the entity layer expects. Order
// matters: ArduinoJson's is<float>() is true for integers, and is<bool>() only for
// real booleans, so booleans and strings have to be tested first.
inline bool json_scalar_to_string(JsonVariantConst value, std::string *out) {
  if (value.is<bool>()) {
    *out = value.as<bool>() ? "true" : "false";
    return true;
  }
  if (value.is<const char *>()) {
    const char *text = value.as<const char *>();
    *out = text == nullptr ? "" : text;
    return true;
  }
  if (value.is<float>()) {
    *out = format_number(value.as<float>());
    return true;
  }
  return false;
}

// Ranks outcomes so a batch can report the worst one as its HTTP status while
// still returning every per-key result in the body.
inline int result_severity(WriteResult result) {
  switch (result) {
    case WriteResult::OK:
    case WriteResult::UNCHANGED:
      return 0;
    case WriteResult::NOT_PRESENT:  // absent on this device: reported, not an error
      return 1;
    case WriteResult::UNKNOWN_FIELD:
      return 2;
    default:
      return 3;
  }
}

inline const char *status_for_result(WriteResult worst) {
  switch (result_severity(worst)) {
    case 0:
    case 1:
      return "200 OK";
    case 2:
      return "404 Not Found";
    default:
      return "400 Bad Request";
  }
}

// Builds the per-key result array and the applied/skipped/failed tally shared by
// the set and (later) import responses.
class WriteReport {
 public:
  void add(const std::string &key, WriteResult result, const std::string &detail) {
    if (result_severity(result) > result_severity(this->worst_))
      this->worst_ = result;
    if (result == WriteResult::OK) {
      this->applied_++;
    } else if (result == WriteResult::UNCHANGED) {
      // Counted apart from applied_: a caller replaying a whole envelope wants to
      // see how much actually moved, and every real write costs an NVS save.
      this->unchanged_++;
    } else if (result == WriteResult::NOT_PRESENT) {
      this->skipped_++;
    } else {
      this->failed_++;
    }
    // The tally above always counts every key; the per-key list stops growing at the
    // cap. Callers see `results_truncated` and can still trust the counts.
    if (this->entries_.size() >= MAX_REPORT_ENTRIES) {
      this->truncated_ = true;
      return;
    }
    this->entries_.push_back(Entry{key, clamp_detail(detail), result});
  }

  WriteResult worst() const { return this->worst_; }
  bool ok() const { return this->failed_ == 0; }
  size_t count() const {
    return this->applied_ + this->unchanged_ + this->skipped_ + this->failed_;
  }

  // Streams the tally and the per-key results into an already-open JSON object.
  //
  // Deliberately not a std::string. A 65-key report is ~3.5 KB and the previous
  // version built it in one buffer, which ABORTED the firmware on a rejected
  // import: by the time pass 1 finishes, internal RAM is fragmented down to a
  // ~10 KB largest free block, and a growing string asks for one contiguous piece
  // bigger than that. malloc returns null, operator new cannot throw in this build,
  // and the panel panics. Chunked through JsonStream the largest allocation is one
  // short key or detail, which a fragmented heap can always satisfy - the same
  // reason handle_get_() never materialises the envelope.
  void stream(JsonStream &json) const {
    json.pair_raw("ok", this->ok() ? "true" : "false");
    json.pair_raw("applied", std::to_string(this->applied_));
    json.pair_raw("unchanged", std::to_string(this->unchanged_));
    json.pair_raw("skipped", std::to_string(this->skipped_));
    json.pair_raw("failed", std::to_string(this->failed_));
    if (this->truncated_)
      json.pair_raw("results_truncated", "true");
    json.key("results");
    json.open_array();
    for (const auto &entry : this->entries_) {
      json.next_element();
      json.open_object();
      json.pair("key", entry.key);
      json.pair("result", write_result_name(entry.result));
      if (!entry.detail.empty())
        json.pair("detail", entry.detail);
      json.close_object();
    }
    json.close_array();
  }

 protected:
  struct Entry {
    std::string key;
    std::string detail;
    WriteResult result;
  };

  // A full envelope on the widest panel is ~130 keys; the cap is a runaway bound,
  // not an expected limit. Each entry is its own small allocation.
  static constexpr size_t MAX_REPORT_ENTRIES = 256;
  static constexpr size_t MAX_DETAIL_BYTES = 200;

  // Truncates on a UTF-8 boundary so a clipped detail is still valid JSON text.
  static std::string clamp_detail(const std::string &detail) {
    if (detail.size() <= MAX_DETAIL_BYTES)
      return detail;
    size_t end = MAX_DETAIL_BYTES;
    while (end > 0 && ((unsigned char) detail[end] & 0xC0) == 0x80) end--;
    return detail.substr(0, end) + "...";
  }

  std::vector<Entry> entries_;
  size_t applied_{0};
  size_t unchanged_{0};
  size_t skipped_{0};
  size_t failed_{0};
  bool truncated_{false};
  WriteResult worst_{WriteResult::OK};
};

// ---------------------------------------------------------------------------
// Main-loop handoff
// ---------------------------------------------------------------------------
// Entity writes MUST NOT run on the httpd task.
//
// ESPHome's own web_server never writes from the request handler - it defers to
// the main loop and answers immediately (web_server.cpp:756,
// `this->defer([obj, action]() { execute_switch_action(obj, action); })`). That is
// not a style choice. A template entity's set_action:/on_value: is arbitrary YAML,
// and on this project that YAML drives LVGL and the scheduler: display.yaml:160-167
// runs `script.execute: clock_bar_apply`, then `delay: 50ms`, then
// `script.execute: refresh_button_grid`. A delay: makes the automation an async
// state machine that only the main loop pumps, and LVGL is single-threaded.
//
// Doing it from httpd wedges the panel. Observed on hardware: a set of
// settings.clock_bar never returned, and the device stayed pingable with no HTTP
// and no serial output until it was power-cycled.
//
// We cannot fire-and-forget like web_server does, because reporting what actually
// happened is the point of this API - TextCall::validate_() drops an over-long
// value with only a log line, so every write is verified by reading back. So the
// handler hands the whole batch to the main loop as ONE scheduled job and waits
// for it. One handoff per request rather than one per key, and the response still
// carries real per-key results.
//
// The cost is that the single httpd task is blocked for the duration, so a large
// import briefly stalls other HTTP requests including the setup page's /events
// stream. That is inherent to a synchronous, verified write API.

// Shared state for one main-loop handoff. Heap-owned via shared_ptr and captured
// by value, so a timeout can abandon the job instead of leaving the callback
// holding a dangling reference to the handler's stack.
struct MainLoopTask {
  std::function<void()> work;
  std::atomic<bool> done{false};
};

inline bool run_on_main_loop(const char *name, std::function<void()> &&work, uint32_t timeout_ms) {
  auto task = std::make_shared<MainLoopTask>();
  task->work = std::move(work);
  esphome::App.scheduler.set_timeout(nullptr, name, 0, [task]() {
    task->work();
    task->done.store(true, std::memory_order_release);
  });
  const TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
  while (!task->done.load(std::memory_order_acquire)) {
    if ((int32_t) (xTaskGetTickCount() - deadline) >= 0)
      return false;
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  return true;
}

struct WriteJob {
  std::vector<std::pair<std::string, std::string>> writes;
  WriteReport report;
  // atomic: validate the whole batch before writing any of it. dry_run: validate
  // and never write. aborted: an atomic batch that failed validation, so nothing
  // was written and `report` holds the validation errors.
  bool atomic{false};
  bool dry_run{false};
  bool aborted{false};
  // Flush preferences to NVS when the batch finishes. Import sets this so a
  // restore is durable without a follow-up /apply; a plain set does not, because
  // ESPHome batches preference writes on purpose and a chatty automation calling
  // set in a loop should not commit to flash on every key.
  bool sync{false};
};

inline void apply_one(WriteReport &report, const std::string &key, const std::string &value,
                      bool dry_run = false) {
  ConfigKey parsed;
  if (!parse_config_key(key, &parsed)) {
    report.add(key, WriteResult::UNKNOWN_FIELD, "unparseable key");
    return;
  }
  std::string detail;
  const WriteResult result = apply_key(parsed, value, detail, dry_run);
  if (!write_succeeded(result)) {
    ESP_LOGW(TAG, "set %s: %s%s%s", key.c_str(), write_result_name(result),
             detail.empty() ? "" : " - ", detail.c_str());
  }
  report.add(key, result, detail);
}

inline bool run_writes_on_main_loop(const std::shared_ptr<WriteJob> &job, uint32_t timeout_ms) {
  return run_on_main_loop(
      "espcontrol_config_write",
      [job]() {
        // Pass 1 - validate. This is what makes atomicity real: every check
        // write_entity() would make is made here with the write itself skipped, so
        // a value that validates cannot then fail in pass 2 for a reason we could
        // have caught. Crucially it is the SAME code path with dry_run threaded
        // through, not a parallel validator that could drift from it.
        //
        // Pass 1 is skipped for a non-atomic batch, where per-key best-effort is
        // the point and a second traversal would buy nothing.
        if (job->dry_run || job->atomic) {
          WriteReport validation;
          for (const auto &entry : job->writes)
            apply_one(validation, entry.first, entry.second, /*dry_run=*/true);
          if (job->dry_run || !validation.ok()) {
            job->aborted = !job->dry_run;
            job->report = std::move(validation);
            return;
          }
        }

        // Pass 2 - apply.
        size_t since_feed = 0;
        for (const auto &entry : job->writes) {
          apply_one(job->report, entry.first, entry.second);
          // A full envelope is ~95-135 writes and several kick refresh_button_grid,
          // so the batch can outlive the watchdog window (30 s on the S3) by itself.
          if (++since_feed >= 16) {
            since_feed = 0;
            esphome::App.feed_wdt();
          }
        }
        // One sync for the whole batch rather than per key.
        if (job->sync)
          esphome::global_preferences->sync();
      },
      timeout_ms);
}

// Long enough for a full envelope's worth of grid-refreshing writes, short enough
// that a genuinely stuck main loop still produces a response rather than hanging
// the client.
static constexpr uint32_t WRITE_TIMEOUT_MS = 20000;

// ---------------------------------------------------------------------------
// Envelope import
// ---------------------------------------------------------------------------

// Mirrors validateBackupEnvelope (model/backup.ts:85-105) including its exact
// messages, so the backup-contract fixture can assert the two agree, plus one
// refusal the browser does not need: v1.
//
// v1 is rejected deliberately. Migrating it means the v1->v2 layout remap in
// planBackupButtonLayout (model/backup.ts:186-266) - 80 lines of grid fitting that
// must not be reimplemented in C++ and kept in step. The browser importer already
// does it correctly, so v1 users are pointed there.
inline bool validate_envelope(JsonObject root, std::string *message) {
  const int version = root["version"].is<int>()          ? root["version"].as<int>()
                      : root["version"].is<const char *>()
                          ? atoi(root["version"].as<const char *>())
                          : 0;
  if (version < 1) {
    *message = "Invalid config file - missing required fields";
    return false;
  }
  if (version > 2) {
    *message = "Backup was created by a newer version of EspControl";
    return false;
  }
  if (version >= 2) {
    const char *format = root["format"].as<const char *>();
    if (format == nullptr || strcmp(format, "espcontrol.backup") != 0) {
      *message = "Invalid config file - unsupported backup format";
      return false;
    }
  }
  if (!root["buttons"].is<JsonArray>()) {
    *message = "Invalid config file - missing required fields";
    return false;
  }
  if (version < 2) {
    *message = "Version 1 backups must be restored from the web configurator, which "
               "remaps the button layout";
    return false;
  }
  return true;
}

// Flattens an envelope into the ordered (key, value) list the write job consumes.
//
// Order matters and is not the envelope's own. Settings and screen first, then
// cards, then subpages, then the root colours, and button_order last. The colours
// go through theme.yaml, which rewrites three entities and refreshes the grid, and
// button_order refreshes the grid again on a 750 ms debounce - putting them last
// means one final rebuild that reflects every card change, instead of a rebuild
// part way through against a half-applied layout.
using WriteList = std::vector<std::pair<std::string, std::string>>;

// Each section is its own function, and each is noinline. That is not cosmetic:
// as one body this exceeded the Xtensa l32r literal-window and failed to link with
// "dangerous relocation: literal target out of range". Splitting gives each its own
// literal pool.
#define ESPCONTROL_FLATTEN __attribute__((noinline)) inline

// model/settings.ts:302-309: the explicit day/night keys win over the legacy
// clock_brightness, whose whole purpose is to be the fallback when they are absent.
// Resolved here, where the envelope is interpreted, rather than in write_field.
ESPCONTROL_FLATTEN void flatten_settings(JsonObject root, WriteList &out) {
  if (!root["settings"].is<JsonObject>())
    return;
  JsonObjectConst settings = root["settings"].as<JsonObjectConst>();
  const bool has_explicit =
      !settings["clock_brightness_day"].isNull() || !settings["clock_brightness_night"].isNull();
  for (JsonPairConst entry : settings) {
    if (has_explicit && strcmp(entry.key().c_str(), "clock_brightness") == 0)
      continue;
    std::string value;
    if (!json_scalar_to_string(entry.value(), &value))
      continue;  // nested/null values are not envelope scalars; ignore quietly
    std::string key = "settings.";
    key += entry.key().c_str();
    out.emplace_back(std::move(key), std::move(value));
  }
}

ESPCONTROL_FLATTEN void flatten_screen(JsonObject root, WriteList &out) {
  if (!root["screen"].is<JsonObject>())
    return;
  for (JsonPairConst entry : root["screen"].as<JsonObjectConst>()) {
    std::string value;
    if (!json_scalar_to_string(entry.value(), &value))
      continue;
    std::string key = "screen.";
    key += entry.key().c_str();
    out.emplace_back(std::move(key), std::move(value));
  }
}

ESPCONTROL_FLATTEN void flatten_cards(JsonObject root, int slots, WriteList &out,
                                      std::string *warning) {
  if (!root["buttons"].is<JsonArray>())
    return;
  int index = 0;
  int over = 0;
  int unencodable = 0;
  for (JsonVariantConst entry : root["buttons"].as<JsonArrayConst>()) {
    // Slot comes from the element when present (that is what GET emits) and
    // otherwise from position, so a browser-exported array still lands correctly.
    const int slot = entry["slot"].is<int>() ? entry["slot"].as<int>() : index + 1;
    index++;
    if (slot < 1 || (slots > 0 && slot > slots)) {
      over++;
      continue;
    }
    // `raw` is preferred and stored verbatim, which is what makes the round-trip
    // byte-exact with no C++ encoder. A browser-exported envelope has no `raw`, and
    // rebuilding one from the parsed fields would mean porting
    // serializeButtonConfig - so those cards are reported, not guessed at.
    const char *raw = nullptr;
    if (entry["raw"].is<const char *>()) {
      raw = entry["raw"].as<const char *>();
    } else if (entry.is<const char *>()) {
      raw = entry.as<const char *>();
    }
    if (raw != nullptr) {
      out.emplace_back("buttons[" + std::to_string(slot) + "]", raw);
    } else {
      // Anything that did not yield a string. Deliberately not `is<JsonObject>()`:
      // on a JsonVariantConst that is always false, because JsonObject is the
      // mutable type - the const overload would be is<JsonObjectConst>(). Counting
      // "produced no raw string" instead sidesteps that whole trap.
      unencodable++;
    }
  }
  if (over > 0)
    append_warning(warning, std::to_string(over) + " card(s) outside this device's " +
                                std::to_string(slots) + " slots were skipped");
  if (unencodable > 0)
    append_warning(warning, std::to_string(unencodable) + " card(s) had no `raw` string and were "
                                                         "skipped: this firmware stores card "
                                                         "configs verbatim and cannot re-encode "
                                                         "parsed fields");
}

ESPCONTROL_FLATTEN void flatten_subpages(JsonObject root, WriteList &out) {
  if (!root["subpages"].is<JsonObject>())
    return;
  for (JsonPairConst entry : root["subpages"].as<JsonObjectConst>()) {
    // subpage_objects are deliberately unsupported: no C++ subpage serialiser
    // exists, so only the pre-serialised string form is accepted.
    if (!entry.value().is<const char *>())
      continue;
    std::string key = "subpages[";
    key += entry.key().c_str();
    key += "]";
    out.emplace_back(std::move(key), entry.value().as<const char *>());
  }
}

// Root fields last, button_order dead last - see the ordering note above.
ESPCONTROL_FLATTEN void flatten_root_fields(JsonObject root, WriteList &out) {
  for (size_t i = 0; i < ENTITY_BACKUP_FIELD_COUNT; i++) {
    const auto &field = ENTITY_BACKUP_FIELDS[i];
    if (strcmp(field.section, "root") != 0) continue;
    if (strcmp(field.field, "button_order") == 0) continue;
    std::string value;
    if (root[field.field].isNull() || !json_scalar_to_string(root[field.field], &value))
      continue;
    std::string key = "root.";
    key += field.field;
    out.emplace_back(std::move(key), std::move(value));
  }
  std::string order;
  if (!root["button_order"].isNull() && json_scalar_to_string(root["button_order"], &order))
    out.emplace_back("root.button_order", std::move(order));
}

#undef ESPCONTROL_FLATTEN

inline void flatten_envelope(JsonObject root, int slots, WriteList &out, std::string *warning) {
  flatten_settings(root, out);
  flatten_screen(root, out);
  flatten_cards(root, slots, out, warning);
  flatten_subpages(root, out);
  flatten_root_fields(root, out);
}

class ConfigApiHandler : public esphome::web_server_idf::AsyncWebHandler {
 public:
  bool canHandle(esphome::web_server_idf::AsyncWebServerRequest *request) const override {
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buf);
    // The dispatch loop applies no method filter, so check it here.
    if (request->method() == HTTP_GET)
      return path_is(url, "/api/config");
    if (request->method() == HTTP_POST) {
      return path_is(url, "/api/config/set") || path_is(url, "/api/config/import") ||
             path_is(url, "/api/config/apply") || path_is(url, "/api/config/probe");
    }
    return false;
  }

  void handleRequest(esphome::web_server_idf::AsyncWebServerRequest *request) override {
    char url_buf[esphome::web_server_idf::AsyncWebServerRequest::URL_BUF_SIZE];
    const esphome::StringRef url = request->url_to(url_buf);
    if (request->method() == HTTP_GET) {
      this->handle_get_(request);
      return;
    }
    if (path_is(url, "/api/config/set")) {
      this->handle_set_(request);
      return;
    }
    if (path_is(url, "/api/config/apply")) {
      this->handle_apply_(request);
      return;
    }
    if (path_is(url, "/api/config/import")) {
      this->handle_import_(request);
      return;
    }
    this->handle_probe_(request);
  }

 protected:
  void handle_get_(esphome::web_server_idf::AsyncWebServerRequest *request) {
    httpd_req_t *req = *request;

    std::string section = "all";
    if (request->hasParam("section")) {
      auto *param = request->getParam("section");
      if (param != nullptr && !param->value().empty()) section = param->value();
    }
    int only_slot = 0;
    if (request->hasParam("slot")) {
      auto *param = request->getParam("slot");
      if (param != nullptr) only_slot = atoi(param->value().c_str());
    }

    static const char *const SECTIONS[] = {"all", "meta", "root", "settings", "screen",
                                           "cards", "subpages"};
    bool known = false;
    for (const char *candidate : SECTIONS)
      if (section == candidate) known = true;
    if (!known) {
      send_error(req, "404 Not Found", "unknown_section");
      return;
    }

    const int slots = discover_slot_count();

    // Computed over the whole config regardless of ?section=, so the value means
    // "the state of this panel's configuration" rather than "the bytes of this
    // particular response". That is what an import's ?if_match= needs, and it makes
    // ?section=meta the cheap way to read the current ETag.
    const std::string etag = etag_hex(config_etag(slots));

    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    const std::string quoted_etag = "\"" + etag + "\"";
    httpd_resp_set_hdr(req, "ETag", quoted_etag.c_str());

    JsonStream json(req);
    const bool all = section == "all";
    json.open_object();

    if (all || section == "meta") {
      json.pair_raw("version", "2");
      json.pair("format", "espcontrol.backup");
      json.pair("device", device_slug());
      json.pair("exported_at", iso8601_utc_now());
      json.pair("etag", etag);
      json.key("source");
      json.open_object();
      json.pair("device", device_slug());
      json.pair_raw("slots", std::to_string(slots));
      json.close_object();
    }
    emit_config(json, section, slots, only_slot);

    json.close_object();
    if (!json.finish())
      ESP_LOGW(TAG, "GET /api/config: response send failed");
  }

  // POST /api/config/set
  //
  // Two request shapes, deliberately both scalar-only. Nested envelope objects are
  // /import's job; keeping set flat means one code path and a key syntax that
  // templates cleanly from a Home Assistant rest_command.
  //
  //   ?key=settings.clock_bar&value=true       no body at all
  //   {"key": "...", "value": <scalar>}        single key
  //   {"keys": {"<key>": <scalar>, ...}}       several keys in one request
  //
  // set is NOT atomic - keys are applied in the order given and a failure part way
  // through leaves earlier writes in place. /import is the endpoint with the
  // two-pass rollback. The body always carries per-key results whatever the status.
  void handle_set_(esphome::web_server_idf::AsyncWebServerRequest *request) {
    httpd_req_t *req = *request;

    // Query form. getParam() searches the POST form body first and then the URL
    // query (web_server_idf.cpp:455-475), so this also accepts an
    // application/x-www-form-urlencoded body for free.
    auto job = std::make_shared<WriteJob>();

    if (request->hasParam("key")) {
      auto *key_param = request->getParam("key");
      auto *value_param = request->getParam("value");
      if (key_param == nullptr || value_param == nullptr) {
        send_error(req, "400 Bad Request", "missing_value");
        return;
      }
      job->writes.emplace_back(key_param->value(), value_param->value());
      this->run_and_respond_(req, job);
      return;
    }

    Body body;
    if (!report_body_error(req, read_body(req, body)))
      return;

    // parse_json takes a pointer and length, so the PSRAM body is parsed in place
    // with no copy into internal RAM, and JsonDocument's allocator is PSRAM-backed
    // too (json_util.h:120-146).
    JsonDocument doc = esphome::json::parse_json(reinterpret_cast<const uint8_t *>(body.c_str()),
                                                 body.size());
    JsonObject root = doc.as<JsonObject>();
    if (root.isNull()) {
      send_error(req, "400 Bad Request", "invalid_json");
      return;
    }

    // Non-scalar values are rejected here rather than inside the job, so a bad
    // request never reaches the main loop at all.
    std::vector<std::pair<std::string, std::string>> rejected;
    if (root["key"].is<const char *>()) {
      std::string value;
      if (!json_scalar_to_string(root["value"], &value)) {
        send_error(req, "400 Bad Request", "value_must_be_a_scalar");
        return;
      }
      job->writes.emplace_back(root["key"].as<const char *>(), value);
    } else if (root["keys"].is<JsonObject>()) {
      for (JsonPairConst entry : root["keys"].as<JsonObjectConst>()) {
        std::string value;
        if (!json_scalar_to_string(entry.value(), &value)) {
          rejected.emplace_back(entry.key().c_str(), "value must be a scalar");
          continue;
        }
        job->writes.emplace_back(entry.key().c_str(), value);
      }
    } else {
      send_error(req, "400 Bad Request", "expected_key_or_keys");
      return;
    }

    if (job->writes.empty() && rejected.empty()) {
      send_error(req, "400 Bad Request", "no_keys");
      return;
    }
    this->run_and_respond_(req, job, rejected);
  }

  void run_and_respond_(httpd_req_t *req, const std::shared_ptr<WriteJob> &job,
                        const std::vector<std::pair<std::string, std::string>> &rejected = {}) {
    if (!job->writes.empty() && !run_writes_on_main_loop(job, WRITE_TIMEOUT_MS)) {
      ESP_LOGE(TAG, "write batch did not complete within %" PRIu32 " ms", WRITE_TIMEOUT_MS);
      send_error(req, "503 Service Unavailable", "write_timeout");
      return;
    }
    for (const auto &entry : rejected)
      job->report.add(entry.first, WriteResult::BAD_VALUE, entry.second);
    begin_json_stream(req, status_for_result(job->report.worst()));
    JsonStream json(req);
    json.open_object();
    job->report.stream(json);
    json.close_object();
    if (!json.finish())
      ESP_LOGW(TAG, "set: response send failed");
  }

  // POST /api/config/import
  //
  // Takes a whole espcontrol.backup v2 envelope, or any subset of one. Query flags:
  //   ?dry_run=1    validate and report, write nothing
  //   ?atomic=0     best-effort: apply what validates, report the rest
  //   ?if_match=    refuse with 412 unless the config still has this ETag
  //
  // Atomic is the default, which is the honest answer to partial application: make
  // the default not-partial. Pass 1 validates the whole envelope and pass 2 only
  // runs if nothing failed.
  void handle_import_(esphome::web_server_idf::AsyncWebServerRequest *request) {
    httpd_req_t *req = *request;

    // Defensive only: the single httpd task is blocked for the whole of an import,
    // so a second one cannot currently arrive. It costs three lines and stops this
    // becoming a silent corruption bug if the server ever grows a second worker.
    static std::atomic<bool> in_flight{false};
    bool expected = false;
    if (!in_flight.compare_exchange_strong(expected, true)) {
      send_error(req, "409 Conflict", "import_in_flight");
      return;
    }
    struct Guard {
      std::atomic<bool> *flag;
      ~Guard() { this->flag->store(false); }
    } guard{&in_flight};

    const bool dry_run = this->flag_param_(request, "dry_run", false);
    const bool atomic = this->flag_param_(request, "atomic", true);

    Body body;
    if (!report_body_error(req, read_body(req, body)))
      return;

    JsonDocument doc =
        esphome::json::parse_json(reinterpret_cast<const uint8_t *>(body.c_str()), body.size());
    JsonObject root = doc.as<JsonObject>();
    if (root.isNull()) {
      send_error(req, "400 Bad Request", "invalid_json");
      return;
    }

    std::string reject;
    if (!validate_envelope(root, &reject)) {
      // The message is the browser's, verbatim, so one contract fixture can assert
      // both implementations agree.
      std::string out = "{\"ok\":false,\"error\":\"invalid_envelope\",\"message\":";
      append_json_string(out, reject);
      out += "}";
      send_json(req, "400 Bad Request", out);
      return;
    }

    const int slots = discover_slot_count();

    if (request->hasParam("if_match")) {
      auto *param = request->getParam("if_match");
      std::string want = param == nullptr ? "" : param->value();
      // Tolerate a quoted ETag, since that is what the GET header carries.
      if (want.size() >= 2 && want.front() == '"' && want.back() == '"')
        want = want.substr(1, want.size() - 2);
      const std::string have = etag_hex(config_etag(slots));
      if (want != have) {
        std::string out = "{\"ok\":false,\"error\":\"etag_mismatch\",\"etag\":\"";
        out += have;
        out += "\"}";
        send_json(req, "412 Precondition Failed", out);
        return;
      }
    }

    auto job = std::make_shared<WriteJob>();
    job->atomic = atomic;
    job->dry_run = dry_run;
    job->sync = !dry_run;
    std::string warning;
    flatten_envelope(root, slots, job->writes, &warning);

    if (job->writes.empty()) {
      send_error(req, "400 Bad Request", "envelope_had_nothing_to_apply");
      return;
    }

    if (!run_writes_on_main_loop(job, WRITE_TIMEOUT_MS)) {
      ESP_LOGE(TAG, "import did not complete within %" PRIu32 " ms", WRITE_TIMEOUT_MS);
      send_error(req, "503 Service Unavailable", "write_timeout");
      return;
    }

    // Reported after the writes so a caller can chain read-modify-write without a
    // second GET.
    const std::string etag = etag_hex(config_etag(slots));

    const char *status = "200 OK";
    if (job->aborted) {
      status = "400 Bad Request";
    } else if (!dry_run) {
      status = status_for_result(job->report.worst());
    } else if (!job->report.ok()) {
      // A dry run that found problems still succeeded as a dry run, but the caller
      // needs a non-2xx to notice - HA's rest_command only logs non-2xx.
      status = "400 Bad Request";
    }
    ESP_LOGI(TAG, "import: %zu keys, atomic=%d dry_run=%d aborted=%d", job->writes.size(),
             (int) atomic, (int) dry_run, (int) job->aborted);

    begin_json_stream(req, status);
    JsonStream json(req);
    json.open_object();
    job->report.stream(json);
    json.pair("etag", etag);
    json.pair_raw("dry_run", dry_run ? "true" : "false");
    json.pair_raw("atomic", atomic ? "true" : "false");
    json.pair_raw("applied_any", job->aborted || dry_run ? "false" : "true");
    if (job->aborted)
      json.pair("error", "validation_failed");
    if (!warning.empty())
      json.pair("warning", warning);
    json.close_object();
    if (!json.finish())
      ESP_LOGW(TAG, "import: response send failed");
  }

  static bool flag_param_(esphome::web_server_idf::AsyncWebServerRequest *request, const char *name,
                          bool fallback) {
    if (!request->hasParam(name))
      return fallback;
    auto *param = request->getParam(name);
    if (param == nullptr)
      return fallback;
    // `?dry_run=` with an empty value reads as true. A *valueless* `?dry_run` does
    // not work and cannot be made to: presence detection goes through IDF's
    // httpd_query_key_value (utils.cpp:70), which only recognises key=value pairs,
    // so the parameter is invisible to both hasParam() and hasArg(). Callers must
    // write `?dry_run=1`.
    if (param->value().empty())
      return true;
    bool out = fallback;
    parse_bool(param->value(), &out);
    return out;
  }

  // POST /api/config/apply[?reboot=1]
  //
  // ESPHome batches preference writes, so a set followed by a power cut can lose
  // them; sync() is what the "Apply Configuration" button does before rebooting
  // (core_infra.yaml:152-154). The reboot is deferred rather than immediate so the
  // response has a chance to drain to the client first - safe_reboot() never
  // returns, and a truncated reply would leave the caller unable to tell a reboot
  // from a failure.
  void handle_apply_(esphome::web_server_idf::AsyncWebServerRequest *request) {
    httpd_req_t *req = *request;

    bool reboot = false;
    if (request->hasParam("reboot")) {
      auto *param = request->getParam("reboot");
      if (param != nullptr)
        parse_bool(param->value(), &reboot);
    }

    // sync() commits NVS and safe_reboot() tears the app down, so both belong on
    // the main loop for the same reason the writes do. An empty job gives us the
    // handoff-and-wait without any entity writes.
    auto synced = std::make_shared<std::atomic<bool>>(false);
    const bool completed = run_on_main_loop(
        "espcontrol_config_apply",
        [synced]() {
          synced->store(esphome::global_preferences->sync(), std::memory_order_relaxed);
        },
        WRITE_TIMEOUT_MS);
    if (!completed) {
      ESP_LOGE(TAG, "apply: preference sync did not complete");
      send_error(req, "503 Service Unavailable", "apply_timeout");
      return;
    }
    const bool ok = synced->load(std::memory_order_relaxed);

    std::string out = std::string("{\"ok\":") + (ok ? "true" : "false") +
                      ",\"synced\":" + (ok ? "true" : "false") +
                      ",\"reboot\":" + (reboot ? "true" : "false") + "}";
    send_json(req, ok ? "200 OK" : "500 Internal Server Error", out);

    if (reboot && ok) {
      ESP_LOGI(TAG, "apply: rebooting on request");
      // Deferred so the response has a chance to drain: safe_reboot() never
      // returns, and a truncated reply would leave the caller unable to tell a
      // reboot from a failure.
      esphome::App.scheduler.set_timeout(nullptr, "espcontrol_config_reboot", 750,
                                         []() { esphome::App.safe_reboot(); });
    }
  }

  void handle_probe_(esphome::web_server_idf::AsyncWebServerRequest *request) {
    httpd_req_t *req = *request;

    Body body;
    const BodyRead result = read_body(req, body);
    if (!report_body_error(req, result))
      return;

    const size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    const size_t largest_internal = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    const size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "probe: content_len=%zu received=%zu free_internal=%zu largest_internal=%zu free_psram=%zu",
             (size_t) req->content_len, body.size(), free_internal, largest_internal, free_psram);

    std::string out = "{\"ok\":true,\"content_len\":";
    out += std::to_string((unsigned) req->content_len);
    out += ",\"received\":";
    out += std::to_string((unsigned) body.size());
    out += ",\"free_internal\":";
    out += std::to_string((unsigned) free_internal);
    out += ",\"largest_internal\":";
    out += std::to_string((unsigned) largest_internal);
    out += ",\"free_psram\":";
    out += std::to_string((unsigned) free_psram);
    out += "}";
    send_json(req, "200 OK", out);
  }
};

}  // namespace espcontrol_config_api

// Registers the config API handler. Idempotent, so it is safe to call from both
// on_boot and a retry interval.
//
// Uses global_web_server_base->add_handler() rather than
// global_async_web_server()->addHandler(): add_handler() wraps the handler in
// AuthMiddlewareHandler when web_server auth is configured, and queues into
// handlers_ so it works before the httpd server has started (WebServerBase::init
// replays handlers_ after begin()). That also means registration does not have
// to wait for a Home Assistant API client to connect.
// `slug` comes from the ${device_slug} YAML substitution, following the pattern
// at common/device/screen_cover_art.yaml:443. It is reported as the envelope's
// `device` so an import can refuse a config from a different panel model.
inline void espcontrol_register_config_api(const std::string &slug = "") {
  static bool registered = false;
  if (!slug.empty())
    espcontrol_config_api::device_slug() = slug;
  if (registered)
    return;
  auto *base = esphome::web_server_base::global_web_server_base;
  if (base == nullptr) {
    ESP_LOGW(espcontrol_config_api::TAG, "web server base not ready; will retry");
    return;
  }
  base->add_handler(new espcontrol_config_api::ConfigApiHandler());
  registered = true;
  ESP_LOGI(espcontrol_config_api::TAG, "Config API registered at /api/config");
}
#else   // !USE_WEBSERVER
// Keeps the on_boot registration lambda compiling on a build without a web
// server, rather than making every caller guard the call site.
inline void espcontrol_register_config_api(const std::string & = "") {}
#endif  // USE_WEBSERVER

#endif  // ESPCONTROL_CONFIG_API_H
