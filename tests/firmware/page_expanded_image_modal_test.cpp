#include <algorithm>
#include <array>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>
#include <vector>

#include "button_grid_subpage_parser.h"
#include "lvgl.h"
#include "page_card_dependencies.h"
#include "page_entity_observer_set.h"
#include "panel_config_document.h"
using namespace espcontrol;
using namespace espcontrol::pages;
using namespace espcontrol::configuration;
static size_t lv_live_bytes = 0, lv_allocation_calls = 0, cpp_allocation_calls = 0, cpp_live_bytes = 0;
struct alignas(16) CppAllocation {
  size_t bytes;
};
void* operator new(size_t bytes) {
  ++cpp_allocation_calls;
  auto* header = static_cast<CppAllocation*>(std::malloc(sizeof(CppAllocation) + (bytes ? bytes : 1)));
  assert(header);
  header->bytes = bytes;
  cpp_live_bytes += bytes;
  return header + 1;
}
void* operator new[](size_t bytes) {
  return ::operator new(bytes);
}
void operator delete(void* pointer) noexcept {
  if (pointer) {
    auto* header = static_cast<CppAllocation*>(pointer) - 1;
    cpp_live_bytes -= header->bytes;
    std::free(header);
  }
}
void operator delete[](void* pointer) noexcept {
  ::operator delete(pointer);
}
void operator delete(void* pointer, std::size_t) noexcept {
  ::operator delete(pointer);
}
void operator delete[](void* pointer, std::size_t) noexcept {
  ::operator delete(pointer);
}
struct alignas(16) Allocation {
  size_t bytes;
};
extern "C" {
void lv_mem_init() {}
void lv_mem_deinit() {}
lv_mem_pool_t lv_mem_add_pool(void*, size_t) {
  return nullptr;
}
void lv_mem_remove_pool(lv_mem_pool_t) {}
void* lv_malloc_core(size_t size) {
  ++lv_allocation_calls;
  auto* p = static_cast<Allocation*>(std::malloc(size + sizeof(Allocation)));
  assert(p);
  p->bytes = size;
  lv_live_bytes += size;
  return p + 1;
}
void lv_free_core(void* ptr) {
  if (!ptr) return;
  auto* p = static_cast<Allocation*>(ptr) - 1;
  lv_live_bytes -= p->bytes;
  std::free(p);
}
void* lv_realloc_core(void* ptr, size_t size) {
  void* p = lv_malloc_core(size);
  if (ptr) {
    std::memcpy(p, ptr, std::min(size, (static_cast<Allocation*>(ptr) - 1)->bytes));
    lv_free_core(ptr);
  }
  return p;
}
void lv_mem_monitor_core(lv_mem_monitor_t* out) {
  *out = {};
}
lv_result_t lv_mem_test_core() {
  return LV_RESULT_OK;
}
}

void lv_label_set_display_text(lv_obj_t* label, const char* text) {
  lv_label_set_text(label, text);
}

#include "page_expanded_image_view.h"
struct TestTheme {
  uint32_t surface_primary{0x212121}, overlay{0x101010}, border{0x424242}, text_primary{0xFFFFFF};
};
TestTheme theme;
const auto& current_theme() {
  return theme;
}
uint32_t theme_display_color(uint32_t c) {
  return c;
}
std::string clock_label;
void set_clock_bar_modal_label(const std::string& s) {
  clock_label = s;
}
struct Coordinator {
  bool busy{false};
  bool dispatching() const {
    return busy;
  }
} coordinator;
auto& ha_read_coordinator() {
  return coordinator;
}
bool online = true, enough_heap = true, style_ok = true, shell_ok = true;
bool ha_api_state_connected() {
  return online;
}
bool ha_internal_heap_available(const char*) {
  return enough_heap;
}
void apply_icon_width_compensation(lv_obj_t*) {}
ControlModalStateService<lv_obj_t> service;
auto& control_modal_active() {
  return service.active();
}
auto& control_modal_service() {
  return service;
}
struct ControlModalLayout {
  int short_side{480}, back_size{42}, back_inset_x{8}, back_inset_y{8};
};
struct ControlModalShell {
  lv_obj_t *overlay{nullptr}, *panel{nullptr}, *close_btn{nullptr};
  ControlModalLayout layout;
};
void control_modal_style_translucent_chrome_button(lv_obj_t* b) {
  lv_obj_set_style_bg_opa(b, LV_OPA_70, LV_PART_MAIN);
}
bool control_modal_uses_compact_portrait_tuning(const ControlModalLayout&) {
  return false;
}
int control_modal_scaled_px(int px, int side) {
  return px * side / 480;
}
void control_modal_block_close_for(uint32_t ms) {
  service.block_close_for(lv_tick_get(), ms);
}
ControlModalShell control_modal_open_shell(ControlModalKind kind, lv_obj_t*, int, const lv_font_t*,
                                           void (*close)()) {
  ControlModalShell s;
  if (!shell_ok) return s;
  s.overlay = lv_obj_create(lv_layer_top());
  lv_obj_set_size(s.overlay, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(s.overlay, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(s.overlay, 0, LV_PART_MAIN);
  s.panel = lv_obj_create(s.overlay);
  lv_obj_set_size(s.panel, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(s.panel, 0, LV_PART_MAIN);
  lv_obj_set_style_border_width(s.panel, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(s.panel, 12, LV_PART_MAIN);
  s.close_btn = lv_button_create(s.panel);
  lv_obj_set_size(s.close_btn, 42, 42);
  lv_obj_add_event_cb(
      s.close_btn,
      [](lv_event_t*) {
        if (auto cb = service.active().close_callback) cb();
      },
      LV_EVENT_CLICKED, nullptr);
  service.set_active(kind, s.overlay, close, ControlModalDismissPolicy::DISMISS);
  return s;
}
#include "page_expanded_image_modal.h"
struct Arena : PageArenaAllocator {
  size_t live{0};
  bool fail{false};
  void* allocate(size_t n) override {
    if (fail) return nullptr;
    ++live;
    return std::malloc(n);
  }
  void release(void* p, size_t) override {
    assert(live);
    --live;
    std::free(p);
  }
};
struct Transport : PageEntitySubscriptions {
  struct Entry {
    const char* entity;
    const char* attr;
    void* owner;
    uint32_t revision, channel;
    Deliver deliver;
  };
  std::vector<Entry> entries;
  uint32_t connection{1};
  bool subscribe(const char* e, const char* a, void* o, uint32_t r, uint32_t c, Deliver d) override {
    entries.push_back({e, a, o, r, c, d});
    return true;
  }
  void release(void* o) override {
    entries.erase(std::remove_if(entries.begin(), entries.end(), [&](auto x) { return x.owner == o; }),
                  entries.end());
  }
  uint32_t connection_generation() const override {
    return connection;
  }
  void send(const char* attr, std::string_view value) {
    const auto count = cpp_allocation_calls;
    for (auto e : entries)
      if (std::strcmp(e.entity, "camera.front") == 0 && std::strcmp(e.attr, attr) == 0)
        e.deliver(e.owner, e.revision, e.channel, value);
    assert(count == cpp_allocation_calls);
  }
};
struct Document {
  std::array<uint8_t, 8192> bytes{};
  alignas(std::max_align_t) std::array<uint8_t, 32768> memory{};
  PageStore store;
  Document(const DeviceUiLimits& limits, uint32_t generation = 17) {
    PanelConfigWriter w(bytes.data(), bytes.size());
    assert(w.begin() == PanelConfigStatus::OK);
    assert(w.append_device_profile(reinterpret_cast<const uint8_t*>(limits.device_profile),
                                   std::strlen(limits.device_profile)) == PanelConfigStatus::OK);
    const std::string rows[] = {"camera.front;;Auto;Auto;;;image;;image_modal_mode=fit",
                                ";Rooms;;;indicator;;subpage;;"};
    for (size_t i = 0; i < 2; ++i)
      assert(w.append_button(i + 1, reinterpret_cast<const uint8_t*>(rows[i].data()), rows[i].size()) ==
             PanelConfigStatus::OK);
    const std::string child = "~B,1|image,image.hidden,Hidden,Auto,Auto,,,,";
    assert(w.append_subpage(2, reinterpret_cast<const uint8_t*>(child.data()), child.size()) ==
           PanelConfigStatus::OK);
    size_t size = 0;
    assert(w.finish(&size) == PanelConfigStatus::OK);
    assert(PageStore::stage_v1(bytes.data(), size, limits, generation, memory.data(), memory.size(),
                               &store) == PageStoreStatus::OK);
  }
};
struct View final : PageExpandedImageView {
  PageModalToken token;
  PageExpandedImageSurface frame;
  PageImageTileSource source;
  std::array<uint8_t, 8 * 8 * 2> pixels{};
  lv_image_dsc_t image{};
  size_t binds{0}, unbinds{0};
  uint64_t frame_revision{0};
  uint16_t width{0}, height{0};
  uint32_t background{0};
  bool available{true}, accept{true};
  std::function<void()> on_bind, on_surface;
  View() {
    image.header.magic = LV_IMAGE_HEADER_MAGIC;
    image.header.cf = LV_COLOR_FORMAT_RGB565;
    image.header.w = image.header.h = 8;
    image.header.stride = 16;
    image.data_size = pixels.size();
    image.data = pixels.data();
  }
  ~View() override {
    lv_image_cache_drop(&image);
  }
  bool ready() const override {
    return available;
  }
  bool bind(PageModalToken t, PageImageTileSource s) override {
    token = t;
    source = s;
    frame = {};
    frame.policy.bound = true;
    frame.policy.status = PageImageTileStatus::LOADING;
    ++binds;
    if (on_bind) on_bind();
    return accept;
  }
  void unbind(PageModalToken t) override {
    if (!PageModalImageIdentity::same(token, t)) return;
    token = {};
    source = {};
    frame = {};
    ++unbinds;
  }
  void size(PageModalToken, uint16_t w, uint16_t h, uint32_t bg) override {
    width = w;
    height = h;
    background = bg;
  }
  PageExpandedImageSurface surface(PageModalToken t) override {
    const auto result = PageModalImageIdentity::same(token, t) ? frame : PageExpandedImageSurface{};
    if (on_surface) on_surface();
    return result;
  }
  uint64_t revision() const override {
    return binds + unbinds + frame_revision;
  }
  void ready_frame(bool preview = false) {
    ++frame_revision;
    frame.policy.status = PageImageTileStatus::READY;
    frame.policy.asset = preview ? 0 : 1;
    frame.policy.preview =
        preview ? PageBindingToken{token.configuration, 1, 0, token.card, 0, false} : PageBindingToken{};
    frame.preview_asset = preview ? 1 : 0;
    frame.source = &image;
  }
};
struct Harness {
  Document document;
  Arena arena;
  Transport transport;
  PageCardDependencies deps;
  PageEntityStateRuntime graph;
  View view;
  PageExpandedImageModalPort port;
  PageModalRuntime modal;
  bool covered{false};
  PageModalToken styled{};
  CardId hidden_card{NO_CARD};
  lv_font_t hidden_font{};

  Harness(const DeviceUiLimits& limits)
      : document(limits),
        graph(deps, arena, transport, {256, 128, 32768}),
        port(arena, graph, view, style(this), this, [](void*, PageBindingToken t) { return t.binding != 0; }),
        modal(port) {
    hidden_card = document.store.page(1)->first_card;
    hidden_font = *LV_FONT_DEFAULT;
    hidden_font.line_height += 2;
    assert(port.install(modal, true));
    assert(graph.prepare(document.store) && modal.prepare(document.store));
    graph.activate_prepared(document.store);
    modal.activate_prepared(document.store);
    transport.send("", "idle");
  }
  ~Harness() {
    modal.retire_active();
    modal.loop();
    graph.retire_active();
    assert(!covered);
  }
  static PageExpandedImageModalStyle style(Harness* h) {
    PageExpandedImageModalStyle s;
    s.owner = h;
    s.resolve = [](void* owner, PageModalToken token, const ParsedCfg&,
                   PageExpandedImageModalEnvironment& e) {
      auto& h = *static_cast<Harness*>(owner);
      assert(token.opening && token.configuration == h.document.store.generation() &&
             token.kind == ControlModalKind::IMAGE_CARD);
      h.styled = token;
      e.label_font = token.card == h.hidden_card ? &h.hidden_font : LV_FONT_DEFAULT;
      e.icon_font = LV_FONT_DEFAULT;
      e.background = 0x123456;
      return style_ok;
    };
    s.covered = [](void* o, bool value) { static_cast<Harness*>(o)->covered = value; };
    return s;
  }
  PageModalToken open(CardId card = 0) {
    assert(modal.request_card({document.store.generation(), card}));
    modal.loop();
    if (modal.token().opening) assert(PageModalImageIdentity::same(styled, modal.token()));
    return modal.token();
  }
  lv_obj_t* panel() {
    return lv_obj_get_child(service.active().overlay, 0);
  }
  lv_obj_t* back() {
    auto* object = lv_obj_get_child(panel(), 2);
    assert(lv_obj_check_type(object, &lv_button_class));
    return object;
  }
  lv_obj_t* surface() {
    return lv_obj_get_child(panel(), 0);
  }
  lv_obj_t* loading() {
    auto* object = lv_obj_get_child(panel(), 1);
    assert(lv_obj_get_child_count(object) == 2);
    return object;
  }
  void close() {
    const auto count = cpp_allocation_calls;
    modal.dismiss();
    assert(count == cpp_allocation_calls);
    modal.loop();
  }
};
void lifetime(const DeviceUiLimits& limits) {
  Harness h(limits);
  auto token = h.open();
  assert(token.opening && h.covered && h.view.binds == 1);
  assert(clock_label == "camera.front" && h.view.source.fit);
  assert(h.view.width && h.view.height && h.view.background == 0x123456);
  assert(!lv_obj_has_flag(h.loading(), LV_OBJ_FLAG_HIDDEN));
  assert(std::strcmp(lv_label_get_text(lv_obj_get_child(h.loading(), 1)), "Loading") == 0);
  h.transport.send("friendly_name", std::string_view("New camera\0ignored", 18));
  h.modal.loop();
  assert(clock_label == "New camera");
  h.transport.send("friendly_name", "");
  h.modal.loop();
  assert(clock_label == "camera.front");
  h.view.ready_frame(true);
  h.modal.loop();
  assert(lv_obj_has_flag(h.loading(), LV_OBJ_FLAG_HIDDEN));
  const auto preview = h.view.frame;
  h.view.ready_frame();
  assert(!preview.same_frame(h.view.frame));
  h.view.frame.policy.status = PageImageTileStatus::UNAVAILABLE;
  h.modal.loop();
  assert(!lv_obj_has_flag(h.loading(), LV_OBJ_FLAG_HIDDEN));
  assert(std::strcmp(lv_label_get_text(lv_obj_get_child(h.loading(), 1)), "Unavailable") == 0);
  // A stray click without a press cannot close an opening.
  lv_obj_send_event(h.back(), LV_EVENT_CLICKED, nullptr);
  assert(h.modal.interactive(token));
  lv_obj_send_event(h.back(), LV_EVENT_PRESSED, nullptr);
  lv_obj_send_event(h.back(), LV_EVENT_CLICKED, nullptr);
  assert(!h.modal.valid(token) && !h.covered);
  h.modal.loop();
  for (size_t n = 0; n < 10; ++n) {
    token = h.open();
    assert(token.opening);
    h.view.ready_frame();
    h.modal.loop();
    h.close();
    if (n == 9) {
      assert(h.view.binds == h.view.unbinds);
    }
  }
  const auto heap = cpp_live_bytes, lv = lv_live_bytes, channels = h.transport.entries.size(),
             arenas = h.arena.live;
  const auto* count = std::getenv("ESPCONTROL_TEST_CYCLES");
  const size_t cycles = count ? std::strtoul(count, nullptr, 10) : 1000;
  for (size_t n = 0; n < cycles; ++n) {
    token = h.open();
    h.view.ready_frame();
    h.modal.loop();
    h.close();
  }
  assert(cpp_live_bytes == heap && lv_live_bytes == lv && h.transport.entries.size() == channels &&
         h.arena.live == arenas && h.view.binds == h.view.unbinds);
  token = h.open(h.document.store.page(1)->first_card);
  assert(h.view.source.entity == "image.hidden" && clock_label == "Hidden");
  auto* caption = lv_obj_get_child(h.loading(), 1);
  assert(lv_obj_get_style_text_font(caption, LV_PART_MAIN) == &h.hidden_font);
  h.modal.loop();
  assert(PageModalImageIdentity::same(h.styled, token) &&
         lv_obj_get_style_text_font(caption, LV_PART_MAIN) == &h.hidden_font);

  h.close();
}
void retirement_and_failures() {
  Harness h(DEVICE_UI_PROFILES[8]);
  auto token = h.open();
  auto* old = h.back();
  lv_obj_send_event(old, LV_EVENT_PRESSED, nullptr);
  coordinator.busy = true;
  h.modal.notify_closed(token);
  h.modal.loop();
  assert(!h.modal.valid(token) && !h.covered && h.modal.memory().retired_contexts == 1);
  lv_obj_send_event(old, LV_EVENT_CLICKED, nullptr);
  assert(!h.modal.token().opening);
  coordinator.busy = false;
  h.modal.loop();
  h.view.on_bind = [&] { assert(h.modal.notify_closed(h.modal.token())); };
  h.open();
  h.view.on_bind = nullptr;
  assert(h.view.binds == h.view.unbinds && !h.covered);
  h.view.accept = false;
  h.open();
  h.view.accept = true;
  assert(!h.modal.token().opening && !h.covered);
  shell_ok = false;
  h.open();
  shell_ok = true;
  assert(!h.modal.token().opening && !h.covered);
  style_ok = false;
  h.open();
  style_ok = true;
  assert(!h.modal.token().opening);
  enough_heap = false;
  h.open();
  enough_heap = true;
  assert(!h.modal.token().opening);
  h.arena.fail = true;
  h.open();
  h.arena.fail = false;
  assert(!h.modal.token().opening);
  token = h.open();
  ++h.transport.connection;
  h.modal.loop();
  assert(!h.modal.valid(token) && !h.covered);
  token = h.open();
  Document replacement(DEVICE_UI_PROFILES[8], 0);
  assert(h.modal.prepare(replacement.store));
  h.modal.discard_prepared();
  assert(h.modal.valid(token));
  assert(h.modal.prepare(replacement.store));
  h.modal.activate_prepared(replacement.store);
  std::memset(h.document.memory.data(), 0xa5, h.document.memory.size());
  h.modal.loop();
  assert(!h.covered);
  h.modal.retire_active();
  h.graph.retire_active();
}
void drawing_and_revocation(lv_display_t* display) {
  Harness h(DEVICE_UI_PROFILES[8]);
  auto token = h.open();
  h.view.ready_frame(true);
  h.modal.loop();
  struct Draw {
    Harness* h;
    size_t images{0};
    bool close{false};
  } draw{&h};
  lv_obj_add_flag(h.surface(), LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(
      h.surface(),
      [](lv_event_t* e) {
        auto& d = *static_cast<Draw*>(lv_event_get_user_data(e));
        if (lv_draw_task_get_type(lv_event_get_draw_task(e)) != LV_DRAW_TASK_TYPE_IMAGE) return;
        ++d.images;
        if (d.close) {
          d.h->modal.notify_closed(d.h->modal.token());
          d.h->modal.loop();
          assert(d.h->modal.memory().retired_contexts == 1);
        }
      },
      LV_EVENT_DRAW_TASK_ADDED, &draw);
  lv_obj_invalidate(h.surface());
  lv_refr_now(display);
  assert(draw.images > 0);
  draw.images = 0;
  h.view.on_surface = [&] { h.view.ready_frame(h.view.frame.policy.asset != 0); };
  lv_obj_invalidate(h.surface());
  lv_refr_now(display);
  assert(draw.images == 0);
  h.view.on_surface = nullptr;
  // Revoke the opening during the second coherent frame read. Its returned
  // descriptor can still look unchanged, so draw must recheck admission too.
  size_t reads = 0;
  h.view.on_surface = [&] {
    if (++reads == 2) h.modal.notify_closed(token);
  };
  lv_obj_invalidate(h.surface());
  lv_refr_now(display);
  assert(!h.modal.valid(token) && draw.images == 0);
  h.view.on_surface = nullptr;
  h.modal.loop();
  token = h.open();
  h.view.ready_frame();
  h.modal.loop();
  draw.images = 0;
  draw.close = true;
  lv_obj_add_flag(h.surface(), LV_OBJ_FLAG_SEND_DRAW_TASK_EVENTS);
  lv_obj_add_event_cb(
      h.surface(),
      [](lv_event_t* e) {
        auto& d = *static_cast<Draw*>(lv_event_get_user_data(e));
        if (lv_draw_task_get_type(lv_event_get_draw_task(e)) != LV_DRAW_TASK_TYPE_IMAGE) return;
        ++d.images;
        d.h->modal.notify_closed(d.h->modal.token());
        d.h->modal.loop();
        assert(d.h->modal.memory().retired_contexts == 1);
      },
      LV_EVENT_DRAW_TASK_ADDED, &draw);
  lv_obj_invalidate(h.surface());
  lv_refr_now(display);
  assert(draw.images > 0 && !h.modal.valid(token));
  h.modal.loop();
}
void static_view_does_not_redraw(lv_display_t* display) {
  Harness h(DEVICE_UI_PROFILES[8]);
  h.open();
  h.view.ready_frame();
  h.modal.loop();
  size_t draws = 0;
  lv_obj_add_event_cb(
      h.surface(), [](lv_event_t* event) { ++*static_cast<size_t*>(lv_event_get_user_data(event)); },
      LV_EVENT_DRAW_MAIN, &draws);
  lv_refr_now(display);
  assert(draws);
  draws = 0;
  const auto cpp = cpp_allocation_calls, lv = lv_allocation_calls;
  for (size_t n = 0; n < 1000; ++n) h.modal.loop();
  lv_refr_now(display);
  assert(draws == 0 && cpp_allocation_calls == cpp && lv_allocation_calls == lv);
  // Source changes (including preview animation) still request a new frame.
  h.view.ready_frame(true);
  h.modal.loop();
  lv_refr_now(display);
  assert(draws > 0);
  h.close();
}
void geometry_and_theme(lv_display_t* display) {
  for (const auto& size : {std::pair<int, int>{480, 480},
                           {480, 800},
                           {800, 480},
                           {1024, 600},
                           {1280, 800},
                           {800, 1280},
                           {720, 720}}) {
    lv_display_set_resolution(display, size.first, size.second);
    Harness h(DEVICE_UI_PROFILES[8]);
    h.open();
    assert(h.view.width == size.first && h.view.height == size.second);
    assert(lv_obj_get_width(h.loading()) == size.first && lv_obj_get_height(h.loading()) == size.second);
    assert(lv_obj_get_style_text_font(lv_obj_get_child(h.loading(), 1), LV_PART_MAIN) == LV_FONT_DEFAULT);
    theme = {0xFFFFFF, 0xF5F5F5, 0xDDDDDD, 0x333333};
    h.modal.loop();
    assert(lv_color_eq(lv_obj_get_style_bg_color(h.panel(), LV_PART_MAIN), lv_color_hex(theme.overlay)));
    assert(lv_color_eq(lv_obj_get_style_bg_color(h.loading(), LV_PART_MAIN),
                       lv_color_hex(theme.surface_primary)));
    assert(lv_color_eq(lv_obj_get_style_text_color(lv_obj_get_child(h.loading(), 1), LV_PART_MAIN),
                       lv_color_hex(theme.text_primary)));
    h.close();
    theme = {};
  }
  lv_display_set_resolution(display, 480, 480);
}
int main() {
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  lv_init();
  auto* display = lv_display_create(480, 480);
  assert(display);
  lv_timer_pause(lv_display_get_refr_timer(display));
  static std::array<uint16_t, 1280 * 32> buffer{};
  lv_display_set_buffers(display, buffer.data(), nullptr, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display,
                          [](lv_display_t* d, const lv_area_t*, uint8_t*) { lv_display_flush_ready(d); });
  for (const auto& limits : DEVICE_UI_PROFILES) {
    const auto* selected = std::getenv("ESPCONTROL_TEST_DEVICE");
    if (selected && std::strcmp(selected, limits.device_profile) != 0) continue;
    std::printf("Testing %s\n", limits.device_profile);
    lifetime(limits);
    std::printf("%s: demanded image UI, loading/preview, gestures and stable cycles passed\n",
                limits.device_profile);
  }
  retirement_and_failures();
  drawing_and_revocation(display);
  geometry_and_theme(display);
  static_view_does_not_redraw(display);
  lv_display_delete(display);
  lv_deinit();
  assert(lv_live_bytes == 0);
}
