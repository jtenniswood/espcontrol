#pragma once

#include "esphome/components/pca9554/pca9554.h"
#include "esphome/core/component.h"

namespace esphome::waveshare_s3_86_lcd_preinit {

class WaveshareS386LcdPreinit : public Component {
 public:
  void set_pca9554(pca9554::PCA9554Component *pca9554) { this->pca9554_ = pca9554; }

  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override;

 protected:
  pca9554::PCA9554Component *pca9554_{nullptr};
};

}  // namespace esphome::waveshare_s3_86_lcd_preinit
