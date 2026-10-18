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
  discard_prepared();if(store.generation()==0||store.page_count()==0)return false;
  prepared_generation_=store.generation();return true;
}
void PageModalRuntime::discard_prepared(){prepared_generation_=0;}
void PageModalRuntime::activate_prepared(const PageStore &store) {
  if(prepared_generation_==0||prepared_generation_!=store.generation())return;
  assert(!changing_);Guard guard(changing_);
  dismiss(PageModalClose::REPLACED);active_store_=store;prepared_generation_=0;
}
void PageModalRuntime::retire_active(){dismiss(PageModalClose::SHUTDOWN);active_store_={};}
bool PageModalRuntime::valid_card(ConfigurationHandle card)const {
  if(next_opening_==0||!port_.connected()||active_store_.card(card)==nullptr||
      port_.kind(active_store_,card)==ControlModalKind::NONE)return false;
  for(PageId page=0;page<active_store_.page_count();++page) {
    const auto *definition=active_store_.page(page);
    if(definition->navigable&&card.index>=definition->first_card&&
        card.index-definition->first_card<definition->card_count)return true;
  }
  return false;
}
bool PageModalRuntime::valid_request(const Request &request)const {
  return request.pending&&valid_card(request.card)&&(!request.from_binding||
      (request.origin.binding!=0&&!request.origin.back&&port_.valid_origin(request.origin)));
}
bool PageModalRuntime::request(PageBindingToken origin) {
  const Request request{{origin.configuration,origin.card},origin,true,true};
  if(!valid_request(request))return false;
  pending_=request;return true;
}
bool PageModalRuntime::request_card(ConfigurationHandle card) {
  const Request request{card,{},true,false};
  if(!valid_request(request))return false;
  pending_=request;return true;
}
bool PageModalRuntime::valid(PageModalToken token)const {
  return context_&&token.opening!=0&&token.opening==token_.opening&&
      token.configuration==token_.configuration&&token.card==token_.card&&token.kind==token_.kind&&
      token.configuration==active_store_.generation()&&port_.connected()&&connection_==port_.connection_generation();
}
void PageModalRuntime::retire_context(PageModalClose reason) {
  if(!context_)return;
  assert(retired_==nullptr);Guard guard(changing_);
  retired_=context_;context_=nullptr;token_={};
  port_.retire(retired_,reason);
}
void PageModalRuntime::dismiss(PageModalClose reason){pending_={};retire_context(reason);}
bool PageModalRuntime::notify_closed(PageModalToken token,PageModalClose reason) {
  // A disconnection invalidates public tokens before loop can perform cleanup;
  // close notification still identifies this exact opening without using a
  // replacement grid binding or a recycled context address.
  if(!context_||token.opening==0||token.opening!=token_.opening||token.configuration!=token_.configuration||
      token.card!=token_.card||token.kind!=token_.kind)return false;
  retire_context(reason);return true;
}
bool PageModalRuntime::display_takeover(bool preserve_policy_active) {
  pending_={};if(!context_)return false;
  if(preserve_policy_active&&port_.dismiss_policy(context_)==ControlModalDismissPolicy::PRESERVE_DURING_DISPLAY_TAKEOVER)return true;
  retire_context(PageModalClose::TAKEOVER);return false;
}
void PageModalRuntime::collect_retired() {
  if(!retired_||!port_.ready())return;
  Guard guard(changing_);void *context=retired_;retired_=nullptr;port_.destroy(context);
}
void PageModalRuntime::loop() {
  if(changing_||!port_.ready())return;
  Guard guard(changing_);collect_retired();
  if(context_&&(!port_.connected()||connection_!=port_.connection_generation()))retire_context(PageModalClose::DISCONNECTED);
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
  const auto request=pending_;pending_={};if(!valid_request(request))return;
  const auto card=request.card;
  if(context_&&token_.configuration==card.generation&&token_.card==card.index)return;
  retire_context(PageModalClose::SWITCH);collect_retired();
  if(pending_.pending)return; // A reentrant replacement request supersedes this one.
  if(retired_) {pending_=request;return;} // Never exceed one allocated context.
  const PageModalToken next{card.generation,next_opening_++,card.index,port_.kind(active_store_,card)};
  void *created=port_.create(active_store_,card,next);
  if(!created){++memory_.failures;return;}
  context_=created;token_=next;connection_=port_.connection_generation();
  const bool initialized=port_.initialize(created,next);
  if(!initialized)++memory_.failures;
  if(context_==created&&(!initialized||!valid(next)||!port_.visible(created))) {
    if(initialized)++memory_.failures;
    retire_context(!port_.connected()||connection_!=port_.connection_generation()?PageModalClose::DISCONNECTED:PageModalClose::FAILED);
  } else if(context_==created)++memory_.openings;
  collect_retired();
}
PageModalMemory PageModalRuntime::memory()const {
  auto result=memory_;result.fixed_bytes=sizeof(*this);result.active_contexts=context_?1:0;
  result.retired_contexts=retired_?1:0;result.pending_requests=pending_.pending?1:0;return result;
}
}  // namespace espcontrol::pages
