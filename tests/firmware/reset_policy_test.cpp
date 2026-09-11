#include "reset_interlock.h"
#include <cassert>
#include <future>
#include <map>
#include <vector>
using namespace espcontrol::reset;
struct MemoryStorage : Storage {
  Journal saved;
  std::map<std::pair<std::string, std::string>, std::string> records{
      {{"esphome", "88491487"}, "wifi secret"}, {{"esphome", "88491486"}, "HA secret"},
      {{"esphome", "layout"}, "legacy"}, {{"esphome", "volume"}, "0.9"},
      {{"espcontrol_cfg", "slot_a"}, "old"}, {{"espcontrol_cfg", "slot_b"}, "older"},
      {{"espcontrol_rst", "journal"}, "intent"}, {{"wifi", "platform"}, "network"}};
  bool slot_a = true, slot_b = true;
  int calls = 0, fail_at = -1;
  bool power_cut_after_write = false;
  bool step() { return ++calls != fail_at; }
  bool read(Journal &j) override { j = saved; return true; }
  bool write(const Journal &j) override {
    if (!step()) { if (power_cut_after_write) saved = j; return false; }
    saved = j; return true;
  }
  bool clear_panel() override {
    if (!step()) return false;
    slot_a = false;
    if (!step()) return false;
    slot_b = false;
    return true;
  }
  bool clear_preferences(Mode mode) override {
    for (auto it = records.begin(); it != records.end();) {
      if (!preserve_key(mode, it->first.first, it->first.second, 88491487)) {
        if (!step()) return false;
        it = records.erase(it);
      } else ++it;
    }
    return true;
  }
  bool verify(Mode mode) override {
    if (!step() || slot_a || slot_b) return false;
    for (auto &r : records) if (!preserve_key(mode, r.first.first, r.first.second, 88491487)) return false;
    return true;
  }
};
void interrupted_resets_resume(Mode mode) {
  for (int failure = 1; failure < 23; ++failure) {
    for (bool after_write : {false, true}) {
      MemoryStorage s;
      Journal j;
      assert(request(s, j, mode) == Result::ACCEPTED);
      assert(request(s, j, mode) == Result::ACCEPTED && j.epoch == 1);
      assert(request(s, j, mode == Mode::FACTORY ? Mode::CUSTOMIZATION : Mode::FACTORY) == Result::CONFLICT);
      s.calls = 0; s.fail_at = failure; s.power_cut_after_write = after_write;
      resume(s, j);
      // New boot uses only the durable journal and remaining storage.
      s.read(j); s.fail_at = -1;
      assert(resume(s, j));
      assert(!j.pending() && j.epoch == 1 && !s.slot_a && !s.slot_b);
      assert(resume(s, j));
      assert(s.records.count({"esphome", "88491487"}) == (mode == Mode::CUSTOMIZATION ? 1 : 0));
      assert(s.records.count({"esphome", "88491486"}) == (mode == Mode::CUSTOMIZATION ? 1 : 0));
      if (mode == Mode::CUSTOMIZATION) assert(s.records.at({"esphome", "88491487"}) == "wifi secret");
      assert(!s.records.count({"esphome", "layout"}) && !s.records.count({"espcontrol_cfg", "slot_b"}));
    }
  }
}
void installations_and_resets_are_exclusive() {
  for (bool coprocessor : {false, true}) {
    MemoryStorage storage; Journal journal; OperationInterlock gate;
    assert(gate.begin_installation(coprocessor));
    assert(gate.record(storage, journal, Mode::FACTORY) == Result::CONFLICT);
    assert(!journal.pending() && !gate.pending());
    if (coprocessor) gate.set_coprocessor_busy(false); else gate.set_ota_busy(false);
    assert(gate.record(storage, journal, Mode::FACTORY) == Result::ACCEPTED);
    assert(!gate.begin_installation(false) && !gate.begin_installation(true));
    assert(gate.record(storage, journal, Mode::FACTORY) == Result::ACCEPTED);
    assert(gate.record(storage, journal, Mode::CUSTOMIZATION) == Result::CONFLICT);
  }
  for (bool persisted : {false, true}) {
    MemoryStorage storage; Journal journal; OperationInterlock gate;
    storage.fail_at = 1; storage.power_cut_after_write = persisted;
    assert(gate.record(storage, journal, Mode::FACTORY) == Result::FAILED);
    assert(gate.pending() && !gate.begin_installation(false) && !gate.begin_installation(true));
    assert(gate.record(storage, journal, Mode::CUSTOMIZATION) == Result::FAILED);
    // The scheduled restart resolves both an uncommitted write and failed
    // readback after a successful commit, without a physical power cycle.
    storage.fail_at = -1; storage.read(journal);
    assert(resume(storage, journal));
    assert(storage.slot_a == !persisted && !journal.pending());
  }
  struct PausedStorage : MemoryStorage {
    std::promise<void> writing;
    std::shared_future<void> proceed;
    bool write(const Journal &value) override {
      writing.set_value(); proceed.wait();
      return MemoryStorage::write(value);
    }
  } storage;
  std::promise<void> proceed; storage.proceed = proceed.get_future();
  OperationInterlock gate; Journal journal;
  auto resetting = std::async(std::launch::async, [&] { return gate.record(storage, journal, Mode::FACTORY); });
  storage.writing.get_future().wait();
  auto installing = std::async(std::launch::async, [&] { return gate.begin_installation(false); });
  proceed.set_value();
  assert(resetting.get() == Result::ACCEPTED && !installing.get());
}
int main() {
  installations_and_resets_are_exclusive();
  interrupted_resets_resume(Mode::CUSTOMIZATION);
  interrupted_resets_resume(Mode::FACTORY);
  assert(wifi_preference_key(false, 1234) == 88491487);
  assert(wifi_preference_key(true, 1234) == 1234);
  assert(preserve_key(Mode::CUSTOMIZATION, "esphome", "1234", 1234));
  assert(!preserve_key(Mode::CUSTOMIZATION, "esphome", "88491487", 1234));
  assert(same_origin("http://panel.local", "panel.local", "same-origin", "reset"));
  assert(same_origin("", "panel.local", "", "reset"));
  assert(!same_origin("http://evil.test", "panel.local", "", "reset"));
  assert(!same_origin("null", "panel.local", "", "reset"));
  assert(!same_origin("http://panel.local", "panel.local", "cross-site", "reset"));
  assert(!same_origin("http://panel.local", "panel.local", "", ""));
  assert(!write_requires_epoch("/wifisave"));
  assert(!write_requires_epoch("/wifisave?ssid=setup"));
  assert(!write_requires_epoch("/update"));
  assert(write_requires_epoch("/update/firmware/install"));
  assert(write_requires_epoch("/wifisave/config"));
  assert(write_requires_epoch("/api/v1/config"));
  assert(write_requires_epoch("/text/layout/set?value=old"));
  MemoryStorage s; Journal j;
  s.fail_at = 1;
  assert(request(s, j, Mode::FACTORY) == Result::FAILED && !j.pending());
  j.magic = 0;
  assert(!resume(s, j));
}
