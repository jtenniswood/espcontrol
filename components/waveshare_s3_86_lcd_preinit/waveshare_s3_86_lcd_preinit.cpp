#include "waveshare_s3_86_lcd_preinit.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::waveshare_s3_86_lcd_preinit {

static const char *const TAG = "waveshare_s3_86_lcd_preinit";

static constexpr uint8_t LCD_RESET_PIN = 5;
static constexpr uint8_t LCD_STRAP_PIN = 6;

void WaveshareS386LcdPreinit::setup() {
  if (this->pca9554_ == nullptr) {
    ESP_LOGE(TAG, "PCA9554 parent is not configured");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Applying LCD reset/strap sequence through PCA9554 state cache");

  // Match the Waveshare BSP: hold strap low, pulse the LCD reset line, then release the strap.
  // Use PCA9554Component APIs so later expander-backed pins do not overwrite this state.
  this->pca9554_->pin_mode(LCD_RESET_PIN, gpio::FLAG_OUTPUT);
  this->pca9554_->pin_mode(LCD_STRAP_PIN, gpio::FLAG_OUTPUT);
  this->pca9554_->digital_write(LCD_STRAP_PIN, false);
  this->pca9554_->digital_write(LCD_RESET_PIN, true);
  delay(200);

  this->pca9554_->digital_write(LCD_RESET_PIN, false);
  delay(200);

  this->pca9554_->digital_write(LCD_RESET_PIN, true);
  delay(200);

  this->pca9554_->digital_write(LCD_STRAP_PIN, true);
}

void WaveshareS386LcdPreinit::dump_config() {
  ESP_LOGCONFIG(TAG, "Waveshare ESP32-S3 86 Box LCD pre-init");
  ESP_LOGCONFIG(TAG, "  Expander: PCA9554");
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Setup failed");
  }
}

float WaveshareS386LcdPreinit::get_setup_priority() const {
  return setup_priority::HARDWARE + 50.0f;
}

}  // namespace esphome::waveshare_s3_86_lcd_preinit
