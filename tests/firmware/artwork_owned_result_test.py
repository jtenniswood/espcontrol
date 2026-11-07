#!/usr/bin/env python3
"""Exercise actual ArtworkImage ownership/dispatch methods at the I/O boundary."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[2]
implementation = (root / "components/artwork_image/artwork_image.cpp").read_text()
names = ("claim_owner", "request_owned_update", "release_owner", "request_update_url",
         "cancel_update", "update", "notify_result_", "complete_service_request_",
         "cancel_service_request_")
methods = []
for name in names:
    match = re.search(rf"^(?:void|bool|std::string) ArtworkImage::{name}\(.*?^\}}",
                      implementation, re.M | re.S)
    assert match, name
    methods.append(match[0])

source = r'''
#include <cassert>
#include <cstdint>
#include <functional>
#include <string>
#include "image_result.h"
#include "transfer_observer.h"
using namespace esphome::artwork_image;
constexpr uint32_t SCHEDULER_DONT_RUN = UINT32_MAX;
#define ESP_LOGI(...) ((void)0)
#define ESP_LOGD(...) ((void)0)
#define ESP_LOGW(...) ((void)0)
std::string cap_artwork_url(const std::string& url, int) { return url; }
struct ArtworkImage;
struct ImageService {
  size_t cancels=0, completes=0, queued=0;
  static ImageService& instance() { static ImageService s; return s; }
  void request(ArtworkImage*, uint32_t, int) { ++queued; }
  void complete(ArtworkImage*) { ++completes; }
  void complete_and_request(ArtworkImage*,uint32_t,int) { ++completes; ++queued; }
  void cancel(ArtworkImage*) { ++cancels; }
};
template<typename... Args> struct Callback {
  std::function<void(Args...)> callback;
  void call(Args... args) { if(callback) callback(args...); }
};
struct ArtworkImage {
  bool service_pending_=false, service_active_=false, update_pending_=false;
  bool animation_visible_=false, animation_redraw_=false, animation_screen_active_=false;
  void* downloader_=nullptr;
  void* decoder_=nullptr;
  uint32_t service_generation_=0;
  uint32_t interval=SCHEDULER_DONT_RUN;
  uint32_t get_update_interval() const { return interval; }
  int fixed_width_=200, request_priority_=2, last_http_status_=200;
  size_t connections_ended=0;
  std::string url_, pending_url_;
  ImageResultChannel result_channel_;
  TransferStamp transfer_stamp_;
  TransferFailure transfer_failure_=TransferFailure::CONTENT;
  Callback<bool> download_finished_callback_;
  Callback<> download_error_callback_;
  bool has_on_finished_callbacks() const { return bool(download_finished_callback_.callback); }
  bool has_on_error_callbacks() const { return bool(download_error_callback_.callback); }
  bool validate_url_(const std::string& url) const { return url.rfind("https://",0)==0; }
  bool is_busy_() const { return service_pending_||service_active_; }
  void end_connection_() { ++connections_ended; }
  void queue_pending_update_(const std::string& url) { pending_url_=url; update_pending_=true; }
  void log_state_(const char*) {}
  void begin() { assert(service_pending_);service_pending_=false;service_active_=true; }
  bool claim_owner(ImageResultObserver&);
  bool request_owned_update(ImageResultObserver&,uint64_t,const std::string&,int=0);
  void release_owner(ImageResultObserver&);
  std::string request_update_url(const std::string&,int=0);
  void cancel_update();
  void update();
  void notify_result_(bool,bool);
  void complete_service_request_();
  void cancel_service_request_();
};
''' + "\n".join(methods) + r'''
struct Observer : ImageResultObserver {
  ImageResult last;
  size_t calls=0;
  std::function<void()> callback;
  void image_result(ImageResult result) override {
    last=result; ++calls;
    if(callback) callback();
  }
};
int main() {
  ArtworkImage image;
  Observer owner, other;
  assert(image.claim_owner(owner));
  assert(image.claim_owner(owner));
  assert(!image.claim_owner(other));
  assert(!image.request_owned_update(other,1,"https://image"));
  assert(!image.request_owned_update(owner,0,"https://image"));
  assert(image.request_owned_update(owner,1,"https://first"));
  const auto first=image.service_generation_;
  image.begin();
  image.notify_result_(true,false);
  assert(owner.calls==1 && owner.last.request==1 && owner.last.success && !owner.last.cached);
  assert(!image.service_active_);
  image.result_channel_.deliver(first,false,false);
  assert(owner.calls==1);

  // Cancel followed by a replacement is never healed/failed by a stale worker.
  assert(image.request_owned_update(owner,2,"https://cancelled"));
  image.begin();
  const auto stale=image.service_generation_;
  assert(image.request_owned_update(owner,3,"https://replacement"));
  assert(stale!=image.service_generation_);
  image.result_channel_.deliver(stale,true,false);
  assert(owner.calls==1);
  image.begin();
  image.notify_result_(true,true);  // Shared HTTP 304 completion.
  assert(owner.calls==2 && owner.last.request==3 && owner.last.cached);
  assert(image.request_owned_update(owner,4,"https://failed"));
  image.begin();
  image.notify_result_(false,false);
  assert(owner.calls==3 && owner.last.request==4 && !owner.last.success);
  assert(!image.request_owned_update(owner,5,"invalid"));
  assert(!image.service_pending_);

  // A callback can start a replacement. Completion of the old request must not
  // complete the replacement's queued/active ImageService ownership.
  assert(image.request_owned_update(owner,6,"https://reentrant"));
  image.begin();
  owner.callback=[&] {
    assert(image.request_owned_update(owner,7,"https://new-during-callback"));
    image.begin();
  };
  const auto completes=ImageService::instance().completes;
  image.notify_result_(true,false);
  assert(owner.last.request==6 && image.service_active_);
  assert(ImageService::instance().completes==completes);
  owner.callback={};
  image.notify_result_(true,false);
  assert(owner.last.request==7 && !image.service_active_);

  // Release/claim from inside the observer may transfer the next request to a
  // different stable slot observer without cleanup consuming that new lease.
  owner.callback=[&] {
    image.release_owner(owner);
    assert(image.claim_owner(other));
    assert(image.request_owned_update(other,70,"https://new-owner"));
    image.begin();
  };
  assert(image.request_owned_update(owner,60,"https://old-owner"));
  image.begin();
  image.notify_result_(true,false);
  owner.callback={};
  assert(image.service_active_);
  image.notify_result_(true,false);
  assert(other.last.request==70);
  image.release_owner(other);
  assert(image.claim_owner(owner));

  // TransferObserver can also retire/rebind before download notification.
  TransferObserver::instance().set([](const std::string&){return 12;},[&](const TransferNotice&){
    assert(image.request_owned_update(owner,9,"https://endpoint-replacement"));
    image.begin();
  });
  assert(image.request_owned_update(owner,8,"https://ha/api/media_player_proxy/test"));
  image.begin();
  image.transfer_stamp_=TransferObserver::instance().begin(image.url_,image.service_generation_);
  const auto calls=owner.calls;
  image.notify_result_(false,false);
  assert(owner.calls==calls && image.service_active_);
  TransferObserver::instance().set({},{});
  image.notify_result_(true,false);
  assert(owner.last.request==9);

  assert(image.request_owned_update(owner,10,"https://retired"));
  image.begin();
  image.release_owner(other); // A different observer cannot revoke this claim.
  assert(image.service_active_);
  image.release_owner(owner);
  assert(!image.result_channel_.claimed() && !image.service_active_);
  assert(image.claim_owner(other));
  assert(image.request_owned_update(other,11,"https://other"));
  image.begin();
  image.notify_result_(true,false);
  assert(other.last.request==11);
  image.release_owner(other);

  ArtworkImage legacy;
  size_t legacy_calls=0;
  legacy.download_finished_callback_.callback=[&](bool cached){assert(!cached);++legacy_calls;};
  legacy.download_error_callback_.callback=[&]{++legacy_calls;};
  assert(!legacy.claim_owner(owner));
  assert(!legacy.request_update_url("https://legacy").empty());
  legacy.begin();
  legacy.notify_result_(true,false);
  assert(legacy_calls==1);
  legacy.update();legacy.begin();legacy.notify_result_(false,false);
  assert(legacy_calls==2);
  legacy.download_finished_callback_.callback={};
  legacy.download_error_callback_.callback={};
  legacy.animation_visible_=true;
  assert(!legacy.claim_owner(owner));
  legacy.animation_visible_=false;
  legacy.interval=1000;
  assert(!legacy.claim_owner(owner));
  legacy.interval=SCHEDULER_DONT_RUN;
  assert(legacy.claim_owner(owner));

  // Direct legacy updates cannot inherit an owned request's callback token.
  assert(legacy.request_owned_update(owner,12,"https://owned"));
  legacy.request_update_url("https://unowned");
  legacy.begin();
  const auto count=owner.calls;
  legacy.notify_result_(true,false);
  assert(owner.calls==count);
  legacy.release_owner(owner);
}
'''
with tempfile.TemporaryDirectory(prefix="artwork-owned-") as temp:
    cpp = Path(temp) / "owned.cpp"
    cpp.write_text(source)
    binary = Path(temp) / "owned"
    subprocess.run([sys.argv[1], "-std=c++17", "-Wall", "-Wextra", "-Werror", "-UNDEBUG",
                    "-I" + str(root / "components/artwork_image"),
                    str(cpp), "-o", str(binary)], check=True)
    subprocess.run([str(binary)], check=True)
    print("Artwork owned result: cancellation, rebinding, cached/error and reentrant completion passed")
