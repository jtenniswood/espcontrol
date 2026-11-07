#include "gif_player.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using esphome::artwork_image::gif::Player;


static bool reference_required(const uint8_t *data,size_t size) {
  if(!data || size<4 || size>Player::MAX_BYTES) return true;
  for(size_t i=0;i+3<size;++i)
    if(data[i]==0x21 && data[i+1]==0xf9 && data[i+2]==4 && ((data[i+3]>>2)&7)==3) return true;
  return false;
}
static bool required(const uint8_t *data,size_t size,size_t budget=3) {
  size_t position=0,calls=0;
  while(true) {
    const auto before=position;
    const auto result=Player::scan_restore_canvas(data,size,position,budget);
    assert(position>=before && position-before<=budget);
    if(result!=Player::RestoreScan::MORE) return result==Player::RestoreScan::REQUIRED;
    assert(position>before && ++calls<=size);
  }
}
static void scanner_boundaries() {
  uint8_t short_data[4]={0,0,0,0};size_t cursor=0;
  assert(Player::scan_restore_canvas(short_data,4,cursor,0)==Player::RestoreScan::REQUIRED);
  cursor=2;assert(Player::scan_restore_canvas(short_data,4,cursor,1)==Player::RestoreScan::REQUIRED);
  for(size_t offset=0;offset<75;++offset) for(unsigned packed=0;packed<256;++packed) {
    std::vector<uint8_t> bytes(offset+4,0x21);
    bytes[offset]=0x21;bytes[offset+1]=0xf9;bytes[offset+2]=4;bytes[offset+3]=packed;
    for(size_t budget=1;budget<=17;++budget)
      assert(required(bytes.data(),bytes.size(),budget)==reference_required(bytes.data(),bytes.size()));
  }
  uint32_t seed=2175;
  for(size_t n=0;n<1000;++n) {
    std::vector<uint8_t> bytes(4+n%257);
    for(auto &b:bytes) {seed=seed*1664525u+1013904223u;b=seed>>24;}
    for(size_t budget=1;budget<=13;++budget)
      assert(required(bytes.data(),bytes.size(),budget)==reference_required(bytes.data(),bytes.size()));
  }
  std::vector<uint8_t> largest(Player::MAX_BYTES,0x21);
  assert(!required(largest.data(),largest.size(),65536));
  largest[largest.size()-4]=0x21;largest[largest.size()-3]=0xf9;
  largest[largest.size()-2]=4;largest.back()=3<<2;
  assert(required(largest.data(),largest.size(),65536));
}

static std::vector<uint8_t> read(const std::string &path) {
  std::ifstream file(path, std::ios::binary);
  assert(file.good());
  return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

static uint16_t word(const std::vector<uint8_t> &data, size_t offset) {
  return data.at(offset) | (data.at(offset + 1) << 8);
}

static Player::Result frame(Player &player) {
  // Tiny budgets prove that a partially decoded LZW string survives yielding.
  for (int i = 0; i < 100000; ++i) {
    auto result = player.step(3);
    if (result != Player::Result::MORE) return result;
  }
  assert(false && "GIF failed to make bounded progress");
  return Player::Result::ERROR;
}

static void fixture(const std::string &name, bool loops, bool needs_restore) {
  const auto base = std::string(GIF_FIXTURE_DIR) + "/" + name;
  auto data = read(base + ".gif");
  auto expected = read(base + ".rgb565");
  const size_t pixels = static_cast<size_t>(word(expected, 0)) * word(expected, 2);
  const size_t frames = word(expected, 4);
  std::vector<uint16_t> canvas(pixels), restore(pixels);
  Player player;
  assert(required(data.data(),data.size()) == needs_restore);
  if(needs_restore) {
    Player rejected;assert(rejected.open(data.data(),data.size(),canvas.data(),nullptr));
    Player::Result result=Player::Result::MORE;
    for(size_t n=0;n<frames+1;++n) {result=frame(rejected);if(result!=Player::Result::FRAME)break;}
    assert(result==Player::Result::ERROR);
  }
  assert(player.open(data.data(), data.size(), canvas.data(), needs_restore ? restore.data() : nullptr));
  assert(player.width() == word(expected, 0) && player.height() == word(expected, 2));
  for (size_t cycle = 0; cycle < (loops ? 2u : 1u); ++cycle) {
    for (size_t n = 0; n < frames; ++n) {
      assert(frame(player) == Player::Result::FRAME);
      assert(player.has_more_frames() == (n + 1 < frames));
      for (size_t i = 0; i < pixels; ++i)
        assert(canvas[i] == word(expected, 6 + (n * pixels + i) * 2));
      if (loops) assert(player.delay_ms() == (n + 1) * 100);
    }
  }
  assert(frame(player) == Player::Result::END);
  assert(frame(player) == Player::Result::END);

  // Every truncated prefix must either fail to open or terminate safely.
  for (size_t size = 0; size < data.size(); ++size) {
    Player truncated;
    if (!truncated.open(data.data(), size, canvas.data(), required(data.data(),size) ? restore.data() : nullptr)) continue;
    Player::Result result;
    do { result = frame(truncated); } while (result == Player::Result::FRAME);
    assert(result == Player::Result::ERROR);
  }
}


static void conservative_and_mutated() {
  assert(required(nullptr,0));
  const uint8_t minimal[4]={0,0,0,0};
  for(size_t n=0;n<4;++n) assert(required(minimal,n));
  assert(required(minimal,Player::MAX_BYTES+1));
  for(size_t offset=0;offset<40;++offset) {
    std::vector<uint8_t> raw(offset+4,0);
    raw[offset]=0x21;raw[offset+1]=0xf9;raw[offset+2]=4;raw[offset+3]=3<<2;
    assert(required(raw.data(),raw.size()));
  }
  // A pattern inside irrelevant trailing bytes must conservatively retain the
  // backup. Actual bitmap compositing remains equal to the independent golden.
  auto original=read(std::string(GIF_FIXTURE_DIR)+"/static87.gif");
  auto golden=read(std::string(GIF_FIXTURE_DIR)+"/static87.rgb565");
  auto trailing=original;
  trailing.insert(trailing.end(),{0x21,0xf9,4,3<<2});
  assert(required(trailing.data(),trailing.size()));
  uint16_t canvas[4],backup[4];Player player;
  Player optional;assert(optional.open(trailing.data(),trailing.size(),canvas,nullptr));
  assert(frame(optional)==Player::Result::FRAME);
  for(int i=0;i<4;++i) assert(canvas[i]==word(golden,6+i*2));
  assert(frame(optional)==Player::Result::END);
  assert(player.open(trailing.data(),trailing.size(),canvas,backup));
  assert(frame(player)==Player::Result::FRAME);
  for(int i=0;i<4;++i) assert(canvas[i]==word(golden,6+i*2));
  assert(frame(player)==Player::Result::END);
  // Even a caller violating the immutable-source contract cannot cause a null
  // restore-canvas write after the initial scan.
  auto changed=read(std::string(GIF_FIXTURE_DIR)+"/disposal.gif");
  size_t control=changed.size();
  for(size_t i=0;i+3<changed.size();++i) {
    if(changed[i]==0x21 && changed[i+1]==0xf9 && changed[i+2]==4 && ((changed[i+3]>>2)&7)==3) {
      assert(control==changed.size());control=i+3;changed[control]=(changed[control]&~0x1c)|(1<<2);
    }
  }
  assert(control<changed.size() && !required(changed.data(),changed.size()));
  uint16_t large[64];Player mutated;
  assert(mutated.open(changed.data(),changed.size(),large,nullptr));
  changed[control]=(changed[control]&~0x1c)|(3<<2);
  Player::Result result=Player::Result::MORE;
  for(int i=0;i<8;++i) {result=frame(mutated);if(result!=Player::Result::FRAME)break;}
  assert(result==Player::Result::ERROR);
}

int main() {
  scanner_boundaries();
  conservative_and_mutated();
  fixture("disposal", true, true);
  fixture("interlaced", false, false);
  fixture("static87", false, false);
  fixture("disposal-no-restore", true, false);

  // Plain-text graphics are omitted, but consume their own control extension.
  // Compare following bitmap pixels with the independent static fixture.
  const auto original_static = read(std::string(GIF_FIXTURE_DIR) + "/static87.gif");
  const auto static_pixels = read(std::string(GIF_FIXTURE_DIR) + "/static87.rgb565");
  const size_t palette_end = 13 + 3u * (1u << ((original_static[10] & 7) + 1));
  const std::vector<uint8_t> text{0x21,0xf9,4,1,30,0,1,0,
                                0x21,0x01,12,0,0,0,0,2,0,2,0,1,1,1,0,1,'A',0};
  for (bool between : {false, true}) {
    auto with_text = original_static;
    with_text[4] = '9';
    if (between) {
      with_text.pop_back();
      with_text.insert(with_text.end(), text.begin(), text.end());
      with_text.insert(with_text.end(), original_static.begin() + palette_end, original_static.end());
    } else {
      with_text.insert(with_text.begin() + palette_end, text.begin(), text.end());
    }
    uint16_t canvas[4], restore[4];
    Player player;
    assert(player.open(with_text.data(), with_text.size(), canvas, restore));
    for (int n = 0; n < (between ? 2 : 1); ++n) {
      assert(frame(player) == Player::Result::FRAME);
      assert(player.delay_ms() == 100);
      assert(player.has_more_frames() == (between && n == 0));
      for (int pixel = 0; pixel < 4; ++pixel) assert(canvas[pixel] == word(static_pixels, 6 + pixel * 2));
    }
    assert(frame(player) == Player::Result::END);
  }

  auto data = read(std::string(GIF_FIXTURE_DIR) + "/static87.gif");
  uint16_t width, height;
  assert(Player::dimensions(data.data(), data.size(), width, height));
  auto oversized = data;
  oversized[6] = oversized[7] = 255;
  assert(!Player::dimensions(oversized.data(), oversized.size(), width, height));
  assert(!Player::dimensions(data.data(), Player::MAX_BYTES + 1, width, height));
  data[0] = 'X';
  assert(!Player::dimensions(data.data(), data.size(), width, height));

  // Exercise corrupt descriptors, palettes, LZW data and sub-block lengths
  // under ASan/UBSan without accepting an unbounded playback loop.
  const auto original = read(std::string(GIF_FIXTURE_DIR) + "/static87.gif");
  for (size_t offset = 0; offset < original.size(); ++offset) {
    for (unsigned bit = 0; bit < 8; ++bit) {
      auto mutated = original;
      mutated[offset] ^= 1u << bit;
      if (!Player::dimensions(mutated.data(), mutated.size(), width, height)) continue;
      std::vector<uint16_t> canvas(static_cast<size_t>(width) * height), restore(canvas.size());
      Player corrupt;
      if (!corrupt.open(mutated.data(), mutated.size(), canvas.data(), required(mutated.data(),mutated.size()) ? restore.data() : nullptr)) continue;
      for (int n = 0; n < 3; ++n) if (frame(corrupt) != Player::Result::FRAME) break;
    }
  }
}
