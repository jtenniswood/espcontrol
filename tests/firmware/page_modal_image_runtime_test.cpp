#include "page_modal_image_runtime.h"

#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

#include "panel_config_document.h"

using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
static size_t allocations = 0;
void* operator new(size_t n) {
  ++allocations;
  if (auto* p = std::malloc(n ? n : 1)) return p;
  std::abort();
}
void* operator new[](size_t n) {
  return ::operator new(n);
}
void operator delete(void* p) noexcept {
  std::free(p);
}
void operator delete[](void* p) noexcept {
  std::free(p);
}
void operator delete(void* p, size_t) noexcept {
  std::free(p);
}
void operator delete[](void* p, size_t) noexcept {
  std::free(p);
}
namespace {
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> storage{};
  PageStore store;
  size_t size{0};
  explicit Document(const DeviceUiLimits& limits, uint32_t generation = 17) {
    PanelConfigWriter w(bytes.data(), bytes.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    for (uint8_t slot = 1; slot <= 3; ++slot) {
      const std::string text = slot == 3 ? ";Folder;Auto;Auto;indicator;;subpage;;"
                                         : "camera." + std::string(96, 'a' + slot) + ";;;;;;image;;";
      assert(w.append_button(slot, reinterpret_cast<const uint8_t*>(text.data()), text.size()) ==
             PanelConfigStatus::OK);
    }
    const std::string children = "~B,1|image,image.hidden,Hidden,Auto,Auto,,,,";
    assert(w.append_subpage(3, reinterpret_cast<const uint8_t*>(children.data()), children.size()) ==
           PanelConfigStatus::OK);
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, generation, storage.data(), storage.size(),
                               &store) == PageStoreStatus::OK);
  }
};
struct Port final : PageModalPort {
  struct Record {
    std::string entity;
    PageModalToken token;
    bool shown{false};
  };
  alignas(Record) std::array<unsigned char, sizeof(Record)> memory{};
  Record* record{nullptr};
  PageModalRuntime* modal{nullptr};
  PageModalImageRuntime* images{nullptr};
  uint32_t connection{1}, binding{1};
  bool online{true}, busy{false}, attached{false};
  size_t created{0}, destroyed{0};
  bool valid_origin(PageBindingToken t) const override {
    return t.binding == binding;
  }
  ControlModalKind kind(const PageStore& store, ConfigurationHandle card) const override {
    return store.card(card) && store.field(*store.card(card), CardField::TYPE) == "image"
               ? ControlModalKind::IMAGE_CARD
               : ControlModalKind::NONE;
  }
  bool ready() const override {
    return !busy;
  }
  bool connected() const override {
    return online;
  }
  uint32_t connection_generation() const override {
    return connection;
  }
  void* create(const PageStore& store, ConfigurationHandle card, PageModalToken token) override {
    assert(!record);
    ++created;
    record = new (memory.data())
        Record{std::string(store.field(*store.card(card), CardField::ENTITY)), token, false};
    return record;
  }
  bool initialize(void* pointer, PageModalToken token) override {
    assert(pointer == record && modal->valid(token) && !modal->interactive(token));
    const auto before = allocations;
    assert(images->acquire(token) == 0 && allocations == before);
    record->shown = true;
    return true;
  }
  bool visible(void* pointer) const override {
    return pointer == record && record && record->shown;
  }
  void update(void*, PageModalToken) override {}
  ControlModalDismissPolicy dismiss_policy(void*) const override {
    return ControlModalDismissPolicy::DISMISS;
  }
  void retire(void* pointer, PageModalClose) override {
    assert(pointer == record);
    attached = false;
    record->shown = false;
    images->unbind(0);
  }
  void destroy(void* pointer) override {
    assert(!busy && pointer == record && !record->shown && !attached);
    record->~Record();
    record = nullptr;
    ++destroyed;
  }
};
struct Origin final : PageModalImageOrigin {
  PageModalRuntime& modal;
  Port& port;
  explicit Origin(PageModalRuntime& m, Port& p) : modal(m), port(p) {}
  bool valid(PageModalToken token) const override {
    return modal.valid(token);
  }
  bool visible(PageModalToken token) const override {
    return modal.interactive(token);
  }
  bool connected() const override {
    return port.online;
  }
  uint32_t connection_generation() const override {
    return port.connection;
  }
};
struct Requests final : PageImageRequests {
  struct Delivery {
    void* owner{nullptr};
    Receive receive{nullptr};
    uint64_t request{0};
    void reply(bool success = true, bool cached = false) const {
      if (receive) receive(owner, 0, request, success, cached);
    }
  } job;
  Port& port;
  explicit Requests(Port& p) : port(p) {}
  PageImageFrames* frames{nullptr};
  std::array<char, 4097> url{};
  size_t starts{0}, cancels{0}, releases{0};
  bool accepted{true}, synchronous{false};
  std::function<void()> on_start, on_cancel, on_release;
  bool claim_frames(PageImageFrames& owner) override {
    if (frames) return false;
    frames = &owner;
    return true;
  }
  void release_frames(PageImageFrames& owner) override {
    assert(frames == &owner);
    frames = nullptr;
  }
  bool start(size_t slot, uint64_t request, std::string_view source, PageImageOptions options, void* owner,
             Receive receive) override {
    assert(slot == 0 && source.size() < url.size() && options.priority == PageImagePriority::MODAL &&
           options.max_source_dimension == 800);
    std::memcpy(url.data(), source.data(), source.size());
    url[source.size()] = '\0';
    ++starts;
    job = {owner, receive, request};
    const auto delivery = job;
    if (on_start) on_start();
    if (synchronous) delivery.reply(true, true);
    return accepted;
  }
  void cancel(size_t slot) override {
    assert(slot == 0);
    ++cancels;
    const auto old = job;
    if (on_cancel) on_cancel();
    old.reply();
  }
  void release_surface(size_t slot) override {
    assert(slot == 0 && !port.attached);
    ++releases;
    if (on_release) on_release();
  }
};
constexpr PageImageOptions options{800, 600, 0x123456, false, true, PageImagePriority::MODAL, 800};
struct Fixture {
  Document document;
  Port port;
  PageModalRuntime modal;
  Origin origin;
  Requests requests;
  PageModalImageRuntime images;
  Fixture(const DeviceUiLimits& limits, uint64_t opening = 1, uint64_t request = 1)
      : document(limits),
        modal(port, opening),
        origin(modal, port),
        requests(port),
        images(requests, origin, request) {
    port.modal = &modal;
    port.images = &images;
    assert(modal.prepare(document.store));
    modal.activate_prepared(document.store);
  }
  ~Fixture() {
    modal.retire_active();
    modal.loop();
    images.loop();
    assert(port.created == port.destroyed);
  }
  PageModalToken open(CardId card = 0) {
    assert(modal.request_card({document.store.generation(), card}));
    modal.loop();
    assert(port.record && modal.interactive(modal.token()));
    return modal.token();
  }
  bool request(PageModalToken token) {
    return images.request(0, token, "https://test/image", options);
  }
  void success(PageModalToken token) {
    requests.job.reply();
    assert(images.snapshot(0, token).ready);
    port.attached = true;
  }
  void close() {
    const auto before = allocations;
    modal.dismiss();
    assert(allocations == before);
    modal.loop();
    images.loop();
  }
};
void identities_and_lifetime(const DeviceUiLimits& limits) {
  Fixture f(limits);
  const auto token = f.open();
  assert(f.images.bound_to(0, token) && f.requests.starts == 0);
  for (unsigned field = 0; field < 4; ++field) {
    auto wrong = token;
    switch (field) {
      case 0:
        ++wrong.configuration;
        break;
      case 1:
        ++wrong.opening;
        break;
      case 2:
        ++wrong.card;
        break;
      case 3:
        wrong.kind = ControlModalKind::CLIMATE;
        break;
    }
    assert(!f.images.request(0, wrong, "wrong", options));
    assert(!f.images.owns(0, wrong));
  }
  auto wrong = token;
  wrong.opening = 0;
  assert(f.images.acquire(wrong) == 1);
  wrong = token;
  wrong.kind = ControlModalKind::CLIMATE;
  assert(f.images.acquire(wrong) == 1);
  assert(f.request(token));
  const auto old = f.requests.job;
  f.success(token);
  assert(f.requests.frames->image_visible(0));
  ++f.port.binding;
  assert(f.modal.valid(token) &&
         f.images.snapshot(0, token).ready);  // Grid origin no longer owns an admitted modal.
  f.port.record->shown = false;
  assert(!f.requests.frames->image_visible(0));
  f.port.record->shown = true;
  const auto releases = f.requests.releases;
  f.port.busy = true;
  assert(f.modal.notify_closed(token));
  assert(!f.modal.valid(token) && !f.images.snapshot(0, token).ready && !f.port.attached);
  assert(f.requests.releases == releases && f.port.record);
  old.reply();
  f.modal.loop();
  assert(f.port.record);
  f.port.busy = false;
  f.modal.loop();
  f.images.loop();
  assert(!f.port.record && f.requests.releases == releases + 1);
  const auto next = f.open(1);
  assert(next.opening != token.opening);
  old.reply();
  assert(!f.images.snapshot(0, next).ready);
  assert(f.request(next));
  f.success(next);
  f.close();
  for (size_t n = 0; n < 1000; ++n) {
    const auto t = f.open();
    const auto before = allocations;
    assert(f.images.bound_to(0, t));
    f.images.unbind(0);
    assert(f.images.bind(0, t));
    assert(allocations == before);
    assert(f.request(t));
    f.success(t);
    f.close();
  }
  const auto child = f.document.store.page(1)->first_card;
  const auto hidden = f.open(child);
  assert(f.port.record->entity == "image.hidden");
  assert(f.request(hidden));
  f.success(hidden);
  f.close();
  assert(f.modal.memory().active_contexts == 0 && f.modal.memory().retired_contexts == 0);
  std::printf("%s: 1000 demanded modal image openings, full tokens and deferred surfaces passed\n",
              limits.device_profile);
}
void replacement_reconnect_and_reentrancy() {
  const auto& limits = DEVICE_UI_PROFILES[8];
  Fixture f(limits);
  auto token = f.open();
  assert(f.request(token));
  f.success(token);
  const auto old = f.requests.job;
  ++f.port.connection;
  assert(!f.images.snapshot(0, token).ready);
  const auto releases = f.requests.releases;
  f.images.loop();
  assert(f.requests.releases == releases && f.port.attached);
  f.modal.loop();
  f.images.loop();
  assert(!f.port.record && f.requests.releases == releases + 1);
  token = f.open();
  assert(f.request(token));
  old.reply();
  assert(f.images.snapshot(0, token).pending);
  f.success(token);
  Document next(limits, 0);
  assert(f.modal.prepare(next.store));
  f.modal.discard_prepared();
  assert(f.images.snapshot(0, token).ready);
  assert(f.modal.prepare(next.store));
  const auto before = allocations;
  f.modal.activate_prepared(next.store);
  assert(allocations == before && !f.images.snapshot(0, token).ready && !f.port.attached);
  f.modal.loop();
  f.images.loop();
  assert(f.modal.request_card({0, 0}));
  f.modal.loop();
  token = f.modal.token();
  assert(token.configuration == 0);
  bool entered = false;
  f.requests.on_start = [&] {
    if (!entered) {
      entered = true;
      f.port.busy = true;
      assert(f.modal.notify_closed(token));
      assert(f.port.record && !f.port.record->entity.empty());
      f.modal.loop();
      assert(f.port.record);
      f.port.busy = false;
    }
  };
  assert(!f.request(token));
  assert(entered && !f.modal.valid(token));
  f.requests.on_start = nullptr;
  f.modal.loop();
  f.images.loop();
  assert(f.modal.request_card({0, 1}));
  f.modal.loop();
  token = f.modal.token();
  f.requests.synchronous = true;
  f.requests.accepted = false;
  assert(f.request(token));
  assert(f.images.snapshot(0, token).ready && f.images.snapshot(0, token).cached);
  f.requests.synchronous = false;
  f.requests.accepted = true;
  // Rebinding inside a synchronous cancellation must preserve the newer modal.
  const auto prior = token;
  entered = false;
  f.requests.on_cancel = [&] {
    if (!entered) {
      entered = true;
      assert(f.modal.notify_closed(prior));
      f.modal.loop();
      assert(f.modal.request_card({0, 0}));
      f.modal.loop();
    }
  };
  assert(!f.request(prior));
  f.requests.on_cancel = nullptr;
  assert(entered && f.modal.token().opening != prior.opening && f.images.bound_to(0, f.modal.token()));
  f.modal.retire_active();
  f.images.loop();  // Retire local configuration before its arena goes away.
}
void final_nonce_and_failures() {
  const auto& limits = DEVICE_UI_PROFILES[8];
  {
    Fixture f(limits, UINT64_MAX, UINT64_MAX);
    const auto token = f.open();
    assert(token.opening == UINT64_MAX && f.request(token));
    f.success(token);
    assert(!f.images.requestable() && !f.request(token));
    assert(f.images.snapshot(0, token).ready);
    assert(!f.modal.request_card({17, 1}));
    f.close();
  }
  {
    Fixture f(limits);
    auto token = f.open();
    f.requests.accepted = false;
    assert(!f.request(token) && f.images.snapshot(0, token).failed);
    f.requests.accepted = true;
    assert(f.request(token));
    f.success(token);
    assert(f.request(token));
    const auto late = f.requests.job;
    assert(f.images.cancel_request(0, token, true));
    assert(f.images.snapshot(0, token).ready && !f.images.snapshot(0, token).pending);
    late.reply(false);
    assert(f.images.snapshot(0, token).ready);
    f.port.online = false;
    assert(!f.images.snapshot(0, token).ready);
    f.modal.loop();
    f.images.loop();
    assert(!f.port.record);
  }
}
}  // namespace
int main() {
  for (const auto& limits : DEVICE_UI_PROFILES) identities_and_lifetime(limits);
  replacement_reconnect_and_reentrancy();
  final_nonce_and_failures();
}
