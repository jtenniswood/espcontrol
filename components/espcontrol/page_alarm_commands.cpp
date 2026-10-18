#include "page_alarm_commands.h"

#include <cassert>
#include <cstring>
#include <string>

#include "button_grid_subpage_parser.h"

namespace espcontrol::pages {
namespace {
const char* mode_name(alarm::Action action) {
  switch (action) {
    case alarm::Action::AWAY:
      return "away";
    case alarm::Action::HOME:
      return "home";
    case alarm::Action::NIGHT:
      return "night";
    case alarm::Action::VACATION:
      return "vacation";
    case alarm::Action::DISARM:
      return "disarm";
    default:
      return "";
  }
}
bool visible(const std::string& options, alarm::Action action) {
  const auto selected = cfg_option_value(options, "actions");
  uint8_t mask = 0;
  for (size_t start = 0; start <= selected.size();) {
    const auto end = selected.find('|', start);
    const auto mode = alarm::action(
        std::string_view(selected).substr(start, end == std::string::npos ? std::string::npos : end - start));
    if (mode != alarm::Action::NONE) mask |= 1u << static_cast<uint8_t>(mode);
    if (end == std::string::npos) break;
    start = end + 1;
  }
  if (!mask)
    mask = (1u << static_cast<uint8_t>(alarm::Action::AWAY)) |
           (1u << static_cast<uint8_t>(alarm::Action::HOME)) |
           (1u << static_cast<uint8_t>(alarm::Action::DISARM));
  size_t count = 0;
  for (size_t index = 0; index < card_runtime_alarm_action_mode_count(); ++index) {
    const auto mode = alarm::action(card_runtime_alarm_action_mode_at(index));
    if (mode == alarm::Action::NONE || !(mask & (1u << static_cast<uint8_t>(mode)))) continue;
    if (++count > card_runtime_alarm_max_visible_actions()) break;
    if (mode == action) return true;
  }
  return false;
}
}  // namespace
PageAlarmCommands::PageAlarmCommands(PageEntityStateRuntime& graph, PageAlarmStateRuntime& state,
                                     PageAlarmAckRuntime& acknowledgements, PageAlarmCommandPort& port)
    : graph_(graph), state_(state), acknowledgements_(acknowledgements), port_(port) {}
PageAlarmCommands::~PageAlarmCommands() {
  assert(!dispatching_);
  cancel();
}
bool PageAlarmCommands::prepare(const PageStore& store) {
  discard_prepared();
  if (!store.source_document() || !store.page_count()) return false;
  staged_ = store;
  return true;
}
void PageAlarmCommands::discard_prepared() {
  staged_ = {};
}
void PageAlarmCommands::activate_prepared(const PageStore& store) {
  if (!staged_.source_document() ||
      !page_prepared_generation_matches(staged_.generation(), store.generation()))
    return;
  cancel();
  active_ = store;
  staged_ = {};
}
void PageAlarmCommands::retire_active() {
  cancel();
  active_ = {};
}
void PageAlarmCommands::wipe(Pending& pending) {
  volatile char* bytes = pending.code;
  for (size_t i = 0; i < sizeof(pending.code); ++i) bytes[i] = 0;
  pending.origin = {};
  pending.action = alarm::Action::NONE;
  pending.code_size = 0;
  pending.present = false;
}
void PageAlarmCommands::cancel() {
  if (pending_.present) ++cancelled_;
  wipe(pending_);
}
void PageAlarmCommands::cancel(PageModalToken token) {
  const auto pending = pending_.origin.modal;
  if (pending_.present && token.opening && pending.opening == token.opening &&
      pending.configuration == token.configuration && pending.card == token.card &&
      pending.kind == token.kind)
    cancel();
}
bool PageAlarmCommands::current(const Pending& pending) {
  if (!pending.present || !port_.connected() || !port_.service_available() ||
      pending.origin.connection != graph_.connection_generation())
    return false;
  const auto handle = pending.origin.card();
  const auto* card = active_.card(handle);
  if (!card) return false;
  const bool modal = pending.origin.modal.opening != 0;
  if (modal) {
    if (pending.origin.binding.binding ||
        (pending.origin.modal.kind != ControlModalKind::ALARM_PIN &&
         pending.origin.modal.kind != ControlModalKind::ALARM_CONTROL) ||
        !port_.valid(pending.origin.modal))
      return false;
  } else if (!pending.origin.binding.binding || pending.origin.binding.back ||
             !port_.valid(pending.origin.binding))
    return false;
  state_.loop();
  const auto snapshot = state_.read(handle);
  if (!snapshot.valid || !snapshot.command_available || snapshot.entity.empty()) return false;
  const auto primary = graph_.read_attribute(handle, snapshot.entity, {});
  return primary.status != PageEntityStatus::EXHAUSTED && primary.status != PageEntityStatus::STALE;
}
bool PageAlarmCommands::queue(PageAlarmCommandOrigin origin, alarm::Action action, std::string_view code) {
  Pending next;
  next.origin = origin;
  next.action = action;
  next.present = true;
  const auto reject = [&]() {
    wipe(next);
    ++rejected_;
    return false;
  };
  if (action < alarm::Action::AWAY || action > alarm::Action::DISARM || code.size() > 16) return reject();
  for (char ch : code)
    if (ch < '0' || ch > '9') return reject();
  if (!current(next)) return reject();
  const auto handle = origin.card();
  const auto kind = PageAlarmStateRuntime::classify(active_, handle.index);
  const auto* card = active_.card(handle);
  const std::string options(PageAlarmStateRuntime::effective_options(active_, handle.index));
  // A live ALARM_CONTROL opening for an action card was admitted by the modal
  // family as a critical panel. Its selected PIN action can finish after the
  // countdown ends; configuration/connection/opening guards remain mandatory.
  const bool critical_panel = origin.modal.opening && origin.modal.kind == ControlModalKind::ALARM_CONTROL;
  if (kind == PageAlarmSourceKind::ACTION && !critical_panel) {
    if (action != alarm::action(active_.field(*card, CardField::SENSOR))) return reject();
  } else if (kind == PageAlarmSourceKind::CONTROL || critical_panel) {
    if (!origin.modal.opening) return reject();
    const auto snapshot = state_.read(handle);
    if (!visible(options, action) && !(action == alarm::Action::DISARM && alarm::delay(snapshot.state)))
      return reject();
  } else
    return reject();
  if (cfg_option_value(options, action == alarm::Action::DISARM ? "pin_disarm" : "pin_arm") != "0" &&
      code.empty())
    return reject();
  next.code_size = code.size();
  if (!code.empty()) std::memcpy(next.code, code.data(), code.size());
  wipe(pending_);
  pending_ = next;
  wipe(next);
  return true;
}
PageAlarmCommandResult PageAlarmCommands::loop() {
  if (dispatching_ || !pending_.present) return PageAlarmCommandResult::NONE;
  if (!current(pending_)) {
    cancel();
    return PageAlarmCommandResult::CANCELLED;
  }
  if (!port_.ready()) return PageAlarmCommandResult::NONE;
  // Own the entity through a reentrant send, which can retire the PageStore.
  // This is an explicit user command, never frame/navigation maintenance.
  const auto handle = pending_.origin.card();
  const std::string entity(PageAlarmStateRuntime::effective_entity(active_, handle.index));
  Pending command = pending_;
  wipe(pending_);
  dispatching_ = true;
  const bool sent = port_.send(entity, card_runtime_alarm_action_service(mode_name(command.action)),
                               {command.code, command.code_size});
  dispatching_ = false;
  if (sent) {
    ++sent_;
    acknowledgements_.accepted(handle, command.origin.connection, command.action, command.code_size != 0);
  }
  wipe(command);
  return sent ? PageAlarmCommandResult::SENT : PageAlarmCommandResult::FAILED;
}
PageAlarmCommandMemory PageAlarmCommands::memory() const {
  return {sizeof(*this), pending_.present ? size_t(1) : size_t(0), pending_.code_size, sent_, cancelled_,
          rejected_};
}
}  // namespace espcontrol::pages
