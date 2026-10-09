#include "gif_player.h"
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

using esphome::artwork_image::gif::Player;

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

static void fixture(const std::string &name, bool loops) {
  const auto base = std::string(GIF_FIXTURE_DIR) + "/" + name;
  auto data = read(base + ".gif");
  auto expected = read(base + ".rgb565");
  const size_t pixels = static_cast<size_t>(word(expected, 0)) * word(expected, 2);
  const size_t frames = word(expected, 4);
  std::vector<uint16_t> canvas(pixels), restore(pixels);
  Player player;
  assert(player.open(data.data(), data.size(), canvas.data(), restore.data()));
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
    if (!truncated.open(data.data(), size, canvas.data(), restore.data())) continue;
    Player::Result result;
    do { result = frame(truncated); } while (result == Player::Result::FRAME);
    assert(result == Player::Result::ERROR);
  }
}

int main() {
  fixture("disposal", true);
  fixture("interlaced", false);
  fixture("static87", false);

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
      if (!corrupt.open(mutated.data(), mutated.size(), canvas.data(), restore.data())) continue;
      for (int n = 0; n < 3; ++n) if (frame(corrupt) != Player::Result::FRAME) break;
    }
  }
}
