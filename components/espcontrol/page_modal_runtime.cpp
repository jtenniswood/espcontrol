#include "page_modal_runtime.h"

#include <cassert>

namespace espcontrol::pages {
namespace {struct Guard{bool &value;bool previous;explicit Guard(bool &v):value(v),previous(v){value=true;}~Guard(){value=previous;}};}
PageModalRuntime::~PageModalRuntime() {
  // As with the renderer/coordinator, the application owner outlives dispatch.
  assert(!changing_);dismiss(PageModalClose::SHUTDOWN);
  assert(!retired_||port_.ready());collect_retired();
}
bool PageModalRuntime::prepare(const PageStore &store) {
  if(changing_)return false;
  discard_prepared();if(!store.source_document()||store.page_count()==0)return false;
  prepared_generation_=store.generation();prepared_=true;return true;
}
void PageModalRuntime::discard_prepared(){prepared_generation_=0;prepared_=false;}
void PageModalRuntime::activate_prepared(const PageStore &store) {
  if(!prepared_||!page_prepared_generation_matches(prepared_generation_,store.generation()))return;
  assert(!changing_);Guard guard(changing_);
  dismiss(PageModalClose::REPLACED);active_store_=store;discard_prepared();
}
void PageModalRuntime::retire_active(){dismiss(PageModalClose::SHUTDOWN);active_store_={};}
bool PageModalRuntime::valid_card(ConfigurationHandle card, ControlModalKind requested,
                                  bool verify_kind) const {
  if (requested == ControlModalKind::NONE || active_store_.card(card) == nullptr ||
      (verify_kind && port_.kind(active_store_, card, requested) != requested) ||
      !port_.connected_for(requested))
    return false;
  for(PageId page=0;page<active_store_.page_count();++page) {
    const auto *definition=active_store_.page(page);
    if(definition->navigable&&card.index>=definition->first_card&&
        card.index-definition->first_card<definition->card_count)return true;
  }
  return false;
}
bool PageModalRuntime::valid_request(const Request& request, bool verify_kind) const {
  if (request.from_local) {
    const auto& origin = request.local_origin;
    const auto* page = active_store_.page(origin.page);
    return request.pending && request.opening && origin.binding && request.card.index == NO_CARD &&
           request.card.generation == origin.configuration &&
           origin.configuration == active_store_.generation() && active_store_.source_document() && page &&
           page->navigable && request.kind != ControlModalKind::NONE &&
           (!verify_kind || port_.local_kind(request.kind) == request.kind) &&
           port_.connected_for(request.kind) &&
           request.connection == port_.connection_generation_for(request.kind) &&
           (request.local_admitted || port_.valid_local_origin(origin, request.kind));
  }

  return request.pending && request.opening != 0 &&
         request.connection == port_.connection_generation_for(request.kind) &&
         valid_card(request.card, request.kind, verify_kind) &&
         (!request.from_binding ||
          (request.origin.binding != 0 && !request.origin.back && port_.valid_origin(request.origin)));
}
bool PageModalRuntime::request(PageBindingToken origin) { return request(origin, ControlModalKind::NONE); }
bool PageModalRuntime::request(PageBindingToken origin, ControlModalKind requested) {
  if (!next_opening_) return false;
  const ConfigurationHandle card{origin.configuration, origin.card};
  if (!active_store_.card(card)) return false;
  const auto selected = port_.kind(active_store_, card, requested);
  if (selected == ControlModalKind::NONE) return false;
  const Request request{{origin.configuration, origin.card},
                        origin,
                        next_opening_,
                        port_.connection_generation_for(selected),
                        selected,
                        true,
                        true};
  // Family was resolved before capturing its epoch; loop validates it again.
  if (!valid_request(request, false)) return false;
  ++next_opening_;
  pending_ = request;
  return true;
}
bool PageModalRuntime::request_local(PageLocalModalOrigin origin, ControlModalKind requested) {
  if (!next_opening_ || !origin.binding || !active_store_.source_document()) return false;
  const auto selected = port_.local_kind(requested);
  if (selected == ControlModalKind::NONE || selected != requested) return false;
  Request request;
  request.card = {origin.configuration, NO_CARD};
  request.opening = next_opening_;
  request.connection = port_.connection_generation_for(selected);
  request.kind = selected;
  request.pending = true;
  request.local_origin = origin;
  request.from_local = true;
  if (!valid_request(request, false)) return false;
  ++next_opening_;
  pending_ = request;
  return true;
}
bool PageModalRuntime::request_card(ConfigurationHandle card) {
  return request_card(card, ControlModalKind::NONE);
}
bool PageModalRuntime::request_card(ConfigurationHandle card, ControlModalKind requested) {
  if (!next_opening_) return false;
  if (!active_store_.card(card)) return false;
  const auto selected = port_.kind(active_store_, card, requested);
  if (selected == ControlModalKind::NONE) return false;
  const Request request{card,     {},   next_opening_, port_.connection_generation_for(selected),
                        selected, true, false};
  // Family was resolved before capturing its epoch; loop validates it again.
  if (!valid_request(request, false)) return false;
  ++next_opening_;
  pending_ = request;
  return true;
}
bool PageModalRuntime::cancel_request(uint64_t opening) {
  if (!opening || !pending_.pending || pending_.opening != opening) return false;
  pending_ = {};
  return true;
}
bool PageModalRuntime::valid(PageModalToken token)const {
  return context_ && token.opening != 0 && token.opening == token_.opening &&
         token.configuration == token_.configuration && token.card == token_.card &&
         token.kind == token_.kind && token.configuration == active_store_.generation() &&
         port_.connected_for(token.kind) && connection_ == port_.connection_generation_for(token.kind);
}
bool PageModalRuntime::can_finish_retired(PageModalToken token) const {
  const auto& retired = last_retirement_.token;
  return collecting_retired_ && retired_finish_allowed_ && port_.ready() && !modal_callback_dispatching() &&
         token.opening && token.opening == retired.opening && token.configuration == retired.configuration &&
         token.card == retired.card && token.kind == retired.kind &&
         (last_retirement_.reason == PageModalClose::USER ||
          last_retirement_.reason == PageModalClose::SWITCH) &&
         active_store_.card({token.configuration, token.card}) && port_.connected_for(token.kind) &&
         connection_ == port_.connection_generation_for(token.kind);
}
void PageModalRuntime::retire_context(PageModalClose reason) {
  if(!context_)return;
  assert(retired_==nullptr);Guard guard(changing_);
  last_retirement_ = {token_, reason, port_.dismiss_policy(context_), port_.source_revision(context_)};
  retired_finish_allowed_ = reason == PageModalClose::USER || reason == PageModalClose::SWITCH;
  retired_=context_;context_=nullptr;token_={};
  port_.retire(retired_,reason);
}
void PageModalRuntime::dismiss(PageModalClose reason) {
  if (reason != PageModalClose::USER && reason != PageModalClose::SWITCH) retired_finish_allowed_ = false;
  if (creating_) {
    create_cancelled_ = true;
    create_cancel_reason_ = reason;
  }
  pending_ = {};
  retire_context(reason);
}
bool PageModalRuntime::notify_closed(PageModalToken token,PageModalClose reason) {
  // A disconnection invalidates public tokens before loop can perform cleanup;
  // close notification still identifies this exact opening without using a
  // replacement grid binding or a recycled context address.
  if(!context_||token.opening==0||token.opening!=token_.opening||token.configuration!=token_.configuration||
      token.card!=token_.card||token.kind!=token_.kind)return false;
  retire_context(reason);return true;
}
bool PageModalRuntime::display_takeover(bool preserve_policy_active) {
  retired_finish_allowed_ = false;
  if (creating_) {
    create_cancelled_ = true;
    create_cancel_reason_ = PageModalClose::TAKEOVER;
  }
  pending_={};if(!context_)return false;
  if(preserve_policy_active&&port_.dismiss_policy(context_)==ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER)return true;
  retire_context(PageModalClose::TAKEOVER);return false;
}
void PageModalRuntime::collect_retired() {
  if(!retired_||modal_callback_dispatching()||!port_.ready())return;
  Guard guard(changing_);
  void* context = retired_;
  retired_ = nullptr;
  collecting_retired_ = true;
  port_.destroy(context);
  collecting_retired_ = false;
  retired_finish_allowed_ = false;
}
void PageModalRuntime::loop() {
  if(changing_||modal_callback_dispatching()||!port_.ready())return;
  Guard guard(changing_);collect_retired();
  if (context_ &&
      (!port_.connected_for(token_.kind) || connection_ != port_.connection_generation_for(token_.kind)))
    retire_context(PageModalClose::DISCONNECTED);
  if(context_&&!port_.visible(context_))retire_context(PageModalClose::USER);
  collect_retired();
  if(context_) {
    const auto current=token_;port_.update(context_,current);
    // update can synchronously close the modal while replaying initial values.
    if(context_&&!valid(current))retire_context(PageModalClose::DISCONNECTED);
    if(context_&&!port_.visible(context_))retire_context(PageModalClose::USER);
    collect_retired();
  }
  if(!pending_.pending)return;
  auto request = pending_;
  pending_ = {};
  if (!valid_request(request)) return;
  // An accepted local navigation keeps its captured origin while its own
  // previous overlay retires. Takeover/configuration dismissal still cancels it.
  if (request.from_local) request.local_admitted = true;
  const auto card=request.card;
  const auto requested_kind =
      request.from_local ? port_.local_kind(request.kind) : port_.kind(active_store_, card, request.kind);
  if (requested_kind == ControlModalKind::NONE || requested_kind != request.kind) return;
  if (context_ && !request.from_local && token_.configuration == card.generation &&
      token_.card == card.index && token_.kind == requested_kind)
    return;
  retire_context(PageModalClose::SWITCH);collect_retired();
  if(pending_.pending)return; // A reentrant replacement request supersedes this one.
  if(retired_) {pending_=request;return;} // Never exceed one allocated context.
  const PageModalToken next{card.generation, request.opening, card.index, requested_kind};
  creating_ = true;
  create_cancelled_ = false;
  void* created = request.from_local ? port_.create_local(active_store_, request.local_origin, next)
                                     : port_.create(active_store_, card, next);
  creating_ = false;
  if(!created){++memory_.failures;return;}
  context_ = created;
  token_ = next;
  connection_ = request.connection;
  // A fallible create may pump a producer/reconnect. Never initialize an old
  // request as though it belonged to the new connection.
  const bool initialized = !create_cancelled_ && valid(next) && port_.initialize(created, next);
  if(!initialized)++memory_.failures;
  if(context_==created&&(!initialized||!valid(next)||!port_.visible(created))) {
    if(initialized)++memory_.failures;
    retire_context(create_cancelled_ ? create_cancel_reason_
                   : !port_.connected_for(next.kind) ||
                           connection_ != port_.connection_generation_for(next.kind)
                       ? PageModalClose::DISCONNECTED
                       : PageModalClose::FAILED);
  } else if(context_==created)++memory_.openings;
  collect_retired();
}
PageModalMemory PageModalRuntime::memory()const {
  auto result=memory_;result.fixed_bytes=sizeof(*this);result.active_contexts=context_?1:0;
  result.retired_contexts=retired_?1:0;result.pending_requests=pending_.pending?1:0;return result;
}
}  // namespace espcontrol::pages
