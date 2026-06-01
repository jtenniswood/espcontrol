#pragma once

#include "esphome/components/pca9554/pca9554.h"
#include "esphome/core/component.h"
#include <cstddef>

namespace esphome::waveshare_s3_86_lcd_postinit {

class WaveshareS386LcdPostinit : public Component {
 public:
  void set_pca9554(pca9554::PCA9554Component *pca9554) { this->pca9554_ = pca9554; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

 protected:
  void write_9bit_(uint16_t value);
  void write_command_(uint8_t command);
  void write_data_(uint8_t data);
  void write_command_data_(uint8_t command, const uint8_t *data, size_t length);

  pca9554::PCA9554Component *pca9554_{nullptr};
};

}  // namespace esphome::waveshare_s3_86_lcd_postinit
