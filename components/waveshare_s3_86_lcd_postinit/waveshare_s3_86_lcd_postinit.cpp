#include "waveshare_s3_86_lcd_postinit.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

namespace esphome::waveshare_s3_86_lcd_postinit {

static const char *const TAG = "waveshare_s3_86_lcd_postinit";

static constexpr uint8_t LCD_CS_PIN = 0;
static constexpr uint8_t LCD_SDA_PIN = 1;
static constexpr uint8_t LCD_SCL_PIN = 2;

void WaveshareS386LcdPostinit::setup() {
  if (this->pca9554_ == nullptr) {
    ESP_LOGE(TAG, "PCA9554 parent is not configured");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "Re-sending Waveshare ST7701S init table after RGB setup");
  this->pca9554_->pin_mode(LCD_CS_PIN, gpio::FLAG_OUTPUT);
  this->pca9554_->pin_mode(LCD_SDA_PIN, gpio::FLAG_OUTPUT);
  this->pca9554_->pin_mode(LCD_SCL_PIN, gpio::FLAG_OUTPUT);
  this->pca9554_->digital_write(LCD_CS_PIN, true);
  this->pca9554_->digital_write(LCD_SCL_PIN, true);

  this->write_command_(0x11);
  delay(120);
  const uint8_t cmd_ff_page_10[] = {0x77, 0x01, 0x00, 0x00, 0x10};
  this->write_command_data_(0xFF, cmd_ff_page_10, sizeof(cmd_ff_page_10));
  const uint8_t cmd_c0[] = {0x3B, 0x00};
  this->write_command_data_(0xC0, cmd_c0, sizeof(cmd_c0));
  const uint8_t cmd_c1[] = {0x0D, 0x02};
  this->write_command_data_(0xC1, cmd_c1, sizeof(cmd_c1));
  const uint8_t cmd_c2[] = {0x21, 0x08};
  this->write_command_data_(0xC2, cmd_c2, sizeof(cmd_c2));
  const uint8_t cmd_cd[] = {0x08};
  this->write_command_data_(0xCD, cmd_cd, sizeof(cmd_cd));
  const uint8_t cmd_b0_gamma[] = {0x00, 0x11, 0x18, 0x0E, 0x11, 0x06, 0x07, 0x08,
                                  0x07, 0x22, 0x04, 0x12, 0x0F, 0xAA, 0x31, 0x18};
  this->write_command_data_(0xB0, cmd_b0_gamma, sizeof(cmd_b0_gamma));
  const uint8_t cmd_b1_gamma[] = {0x00, 0x11, 0x19, 0x0E, 0x12, 0x07, 0x08, 0x08,
                                  0x08, 0x22, 0x04, 0x11, 0x11, 0xA9, 0x32, 0x18};
  this->write_command_data_(0xB1, cmd_b1_gamma, sizeof(cmd_b1_gamma));
  const uint8_t cmd_ff_page_11[] = {0x77, 0x01, 0x00, 0x00, 0x11};
  this->write_command_data_(0xFF, cmd_ff_page_11, sizeof(cmd_ff_page_11));
  const uint8_t cmd_b0[] = {0x60};
  this->write_command_data_(0xB0, cmd_b0, sizeof(cmd_b0));
  const uint8_t cmd_b1[] = {0x30};
  this->write_command_data_(0xB1, cmd_b1, sizeof(cmd_b1));
  const uint8_t cmd_b2[] = {0x87};
  this->write_command_data_(0xB2, cmd_b2, sizeof(cmd_b2));
  const uint8_t cmd_b3[] = {0x80};
  this->write_command_data_(0xB3, cmd_b3, sizeof(cmd_b3));
  const uint8_t cmd_b5[] = {0x49};
  this->write_command_data_(0xB5, cmd_b5, sizeof(cmd_b5));
  const uint8_t cmd_b7[] = {0x85};
  this->write_command_data_(0xB7, cmd_b7, sizeof(cmd_b7));
  const uint8_t cmd_b8[] = {0x21};
  this->write_command_data_(0xB8, cmd_b8, sizeof(cmd_b8));
  const uint8_t cmd_c1_page_11[] = {0x78};
  this->write_command_data_(0xC1, cmd_c1_page_11, sizeof(cmd_c1_page_11));
  const uint8_t cmd_c2_page_11[] = {0x78};
  this->write_command_data_(0xC2, cmd_c2_page_11, sizeof(cmd_c2_page_11));
  delay(20);
  const uint8_t cmd_e0[] = {0x00, 0x1B, 0x02};
  this->write_command_data_(0xE0, cmd_e0, sizeof(cmd_e0));
  const uint8_t cmd_e1[] = {0x08, 0xA0, 0x00, 0x00, 0x07, 0xA0, 0x00, 0x00, 0x00, 0x44, 0x44};
  this->write_command_data_(0xE1, cmd_e1, sizeof(cmd_e1));
  const uint8_t cmd_e2[] = {0x11, 0x11, 0x44, 0x44, 0xED, 0xA0, 0x00,
                            0x00, 0xEC, 0xA0, 0x00, 0x00};
  this->write_command_data_(0xE2, cmd_e2, sizeof(cmd_e2));
  const uint8_t cmd_e3[] = {0x00, 0x00, 0x11, 0x11};
  this->write_command_data_(0xE3, cmd_e3, sizeof(cmd_e3));
  const uint8_t cmd_e4[] = {0x44, 0x44};
  this->write_command_data_(0xE4, cmd_e4, sizeof(cmd_e4));
  const uint8_t cmd_e5[] = {0x0A, 0xE9, 0xD8, 0xA0, 0x0C, 0xEB, 0xD8, 0xA0,
                            0x0E, 0xED, 0xD8, 0xA0, 0x10, 0xEF, 0xD8, 0xA0};
  this->write_command_data_(0xE5, cmd_e5, sizeof(cmd_e5));
  const uint8_t cmd_e6[] = {0x00, 0x00, 0x11, 0x11};
  this->write_command_data_(0xE6, cmd_e6, sizeof(cmd_e6));
  const uint8_t cmd_e7[] = {0x44, 0x44};
  this->write_command_data_(0xE7, cmd_e7, sizeof(cmd_e7));
  const uint8_t cmd_e8[] = {0x09, 0xE8, 0xD8, 0xA0, 0x0B, 0xEA, 0xD8, 0xA0,
                            0x0D, 0xEC, 0xD8, 0xA0, 0x0F, 0xEE, 0xD8, 0xA0};
  this->write_command_data_(0xE8, cmd_e8, sizeof(cmd_e8));
  const uint8_t cmd_eb[] = {0x02, 0x00, 0xE4, 0xE4, 0x88, 0x00, 0x40};
  this->write_command_data_(0xEB, cmd_eb, sizeof(cmd_eb));
  const uint8_t cmd_ec[] = {0x3C, 0x00};
  this->write_command_data_(0xEC, cmd_ec, sizeof(cmd_ec));
  const uint8_t cmd_ed[] = {0xAB, 0x89, 0x76, 0x54, 0x02, 0xFF, 0xFF, 0xFF,
                            0xFF, 0xFF, 0xFF, 0x20, 0x45, 0x67, 0x98, 0xBA};
  this->write_command_data_(0xED, cmd_ed, sizeof(cmd_ed));
  const uint8_t cmd_ff_page_00[] = {0x77, 0x01, 0x00, 0x00, 0x00};
  this->write_command_data_(0xFF, cmd_ff_page_00, sizeof(cmd_ff_page_00));
  const uint8_t cmd_36[] = {0x08};
  this->write_command_data_(0x36, cmd_36, sizeof(cmd_36));
  const uint8_t cmd_3a[] = {0x66};
  this->write_command_data_(0x3A, cmd_3a, sizeof(cmd_3a));
  this->write_command_(0x21);
  this->write_command_(0x29);
  delay(20);
}

void WaveshareS386LcdPostinit::write_9bit_(uint16_t value) {
  this->pca9554_->digital_write(LCD_CS_PIN, false);
  for (int bit = 8; bit >= 0; bit--) {
    this->pca9554_->digital_write(LCD_SDA_PIN, (value & (1u << bit)) != 0);
    this->pca9554_->digital_write(LCD_SCL_PIN, false);
    delayMicroseconds(2);
    this->pca9554_->digital_write(LCD_SCL_PIN, true);
    delayMicroseconds(2);
  }
  this->pca9554_->digital_write(LCD_CS_PIN, true);
  delayMicroseconds(2);
}

void WaveshareS386LcdPostinit::write_command_(uint8_t command) { this->write_9bit_(command); }

void WaveshareS386LcdPostinit::write_data_(uint8_t data) { this->write_9bit_(0x100 | data); }

void WaveshareS386LcdPostinit::write_command_data_(uint8_t command, const uint8_t *data, size_t length) {
  this->write_command_(command);
  for (size_t i = 0; i < length; i++) {
    this->write_data_(data[i]);
  }
}

void WaveshareS386LcdPostinit::dump_config() {
  ESP_LOGCONFIG(TAG, "Waveshare ESP32-S3 86 Box LCD post-init");
  ESP_LOGCONFIG(TAG, "  Expander: PCA9554");
  if (this->is_failed()) {
    ESP_LOGE(TAG, "  Setup failed");
  }
}

float WaveshareS386LcdPostinit::get_setup_priority() const {
  return 235.0f;
}

}  // namespace esphome::waveshare_s3_86_lcd_postinit
