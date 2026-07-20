#ifndef ESPCONTROL_CALENDAR_AGENDA_RUNTIME_H
#define ESPCONTROL_CALENDAR_AGENDA_RUNTIME_H

#pragma once

// Device-side calendar agenda fetcher. Calls Home Assistant's
// calendar.get_events for one or more calendar entities, accumulates the
// returned events into the host-tested espcontrol::AgendaList, and invokes a
// ready callback with the finalized, time-ordered, day-grouped list.
//
// The action-response JSON support (USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON)
// is enabled by the espcontrol component. When it is unavailable this degrades
// to a no-op so non-API builds still compile.

#include <functional>
#include <string>
#include <vector>

#include "calendar_agenda.h"
#include "button_grid_ha.h"

#ifdef USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON
#include "esphome/components/json/json_util.h"
#endif

namespace espcontrol {

using AgendaReadyCallback = std::function<void(const AgendaList &)>;

class AgendaFetcher {
 public:
  void set_entities(std::vector<std::string> entities) {
    this->entities_ = std::move(entities);
  }
  void set_max_events(std::size_t max_events) { this->max_events_ = max_events; }
  void set_on_ready(AgendaReadyCallback callback) { this->on_ready_ = std::move(callback); }
  bool has_on_ready() const { return static_cast<bool>(this->on_ready_); }
  // Last completed fetch's events; empty while a refresh is mid-flight.
  const AgendaList &last_result() const { return this->accumulated_; }

  bool has_entities() const { return !this->entities_.empty(); }

  // Start a fetch covering [start_datetime, end_datetime] (Home Assistant local
  // "YYYY-MM-DD HH:MM:SS" strings). now_epoch filters out events that already
  // ended. A new refresh supersedes any in-flight one.
  void refresh(const std::string &start_datetime, const std::string &end_datetime,
               int64_t now_epoch) {
#ifdef USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON
    if (this->entities_.empty()) {
      this->pending_ = 0;
      this->accumulated_.clear();
      if (this->on_ready_) this->on_ready_(this->accumulated_);
      return;
    }
    ESP_LOGI("agenda", "Fetching events from %u calendar(s)",
             (unsigned) this->entities_.size());
    const uint32_t generation = ++this->generation_;
    this->accumulated_.clear();
    this->now_epoch_ = now_epoch;
    this->pending_ = static_cast<int>(this->entities_.size());

    for (std::size_t entity_index = 0; entity_index < this->entities_.size();
         entity_index++) {
      const std::string &entity = this->entities_[entity_index];
      const uint32_t call_id = next_agenda_call_id_();
      esphome::api::HomeassistantActionRequest req;
      if (!ha_action_begin(req, "calendar.get_events", false, 3, call_id)) {
        this->finish_one_(generation);
        continue;
      }
      // Ask Home Assistant to return the service response (the events); without
      // this the call still runs but no data comes back to the device. The
      // response template renders the events into a compact JSON array
      // Home-Assistant-side — the same mechanism the weather forecast card
      // uses — so the device receives a small, versioned-envelope-free payload
      // under the "response" key.
      req.wants_response = true;
      static const char *const kTemplate =
          "{% set ns = namespace(items=[]) %}"
          "{% set data = response if response is defined and response is not none else {} %}"
          "{% for cal, body in data.items() %}"
          "{% for e in body['events'] | default([]) %}"
          "{% set ns.items = ns.items + [{'s': e.start | string, 'e': e.end | default('') | string, 't': e.summary | string, 'l': e.location | default('') | string}] %}"
          "{% endfor %}{% endfor %}"
          "{{ ns.items | tojson }}";
      req.response_template = decltype(req.response_template)(kTemplate);
      ha_action_add_entity(req, entity);
      ha_action_add_data(req, "start_date_time", start_datetime.c_str());
      ha_action_add_data(req, "end_date_time", end_datetime.c_str());

      auto *self = this;
      const uint8_t source = static_cast<uint8_t>(entity_index & 0xFF);
      const bool registered = ha_register_action_response_callback(
          call_id,
          [self, generation, source](const esphome::api::ActionResponse &response) {
            self->handle_response_(generation, source, response);
          });
      if (!registered) {
        this->finish_one_(generation);
        continue;
      }
      if (!ha_action_send(req)) {
        ha_cancel_action_response_callback(call_id, "send failed");
        this->finish_one_(generation);
      }
    }
#else
    (void) start_datetime;
    (void) end_datetime;
    (void) now_epoch;
    if (this->on_ready_) this->on_ready_(this->accumulated_);
#endif
  }

 protected:
#ifdef USE_API_HOMEASSISTANT_ACTION_RESPONSES_JSON
  static uint32_t next_agenda_call_id_() {
    static uint32_t call_id = 600000;
    if (call_id == 0) call_id = 600000;
    return call_id++;
  }

  void handle_response_(uint32_t generation, uint8_t source,
                        const esphome::api::ActionResponse &response) {
    if (generation != this->generation_) {
      ESP_LOGD("agenda", "Dropping superseded get_events response");
      return;
    }
    if (response.is_success()) {
      // The response template renders a JSON array of {s: start, t: summary}
      // under the "response" key. Home Assistant parses template output that
      // looks like JSON back into a native value, so the array can arrive
      // either as a JSON array directly or as its string form; accept both.
      JsonObjectConst root = response.get_json();
      JsonVariantConst rendered = root["response"];
      JsonDocument parsed_doc;
      JsonArrayConst items;
      if (rendered.is<JsonArrayConst>()) {
        items = rendered.as<JsonArrayConst>();
      } else if (rendered.is<const char *>()) {
        const char *payload = rendered.as<const char *>();
        if (payload != nullptr && payload[0] != '\0') {
          parsed_doc = esphome::json::parse_json(
              reinterpret_cast<const uint8_t *>(payload), strlen(payload));
          items = parsed_doc.as<JsonArrayConst>();
        }
      }
      if (items.isNull()) {
        ESP_LOGW("agenda", "get_events response had no usable payload");
      } else {
        size_t added = 0;
        for (JsonObjectConst item : items) {
          const char *start = item["s"] | "";
          const char *end = item["e"] | "";
          const char *summary = item["t"] | "";
          const char *location = item["l"] | "";
          this->accumulated_.add(start, end[0] != '\0' ? end : nullptr,
                                 std::string(summary), std::string(location),
                                 source);
          added++;
        }
        ESP_LOGD("agenda", "Parsed %u event(s) from calendar response", (unsigned) added);
      }
    } else {
      ESP_LOGW("agenda", "calendar.get_events failed: %s",
               response.get_error_message().c_str());
    }
    this->finish_one_(generation);
  }

  void finish_one_(uint32_t generation) {
    if (generation != this->generation_) return;
    if (this->pending_ > 0) this->pending_--;
    if (this->pending_ > 0) return;
    this->accumulated_.finalize(this->max_events_, this->now_epoch_);
    ESP_LOGI("agenda", "Agenda ready: %u event(s) after fetch",
             (unsigned) this->accumulated_.size());
    if (this->on_ready_) this->on_ready_(this->accumulated_);
  }
#endif

  std::vector<std::string> entities_;
  std::size_t max_events_{kAgendaMaxEvents};
  AgendaReadyCallback on_ready_;
  AgendaList accumulated_;
  int pending_{0};
  uint32_t generation_{0};
  int64_t now_epoch_{0};
};

}  // namespace espcontrol

#endif  // ESPCONTROL_CALENDAR_AGENDA_RUNTIME_H
