#pragma once

#include "esphome/core/application.h"
#include "esphome/core/component.h"
#include "esphome/core/helpers.h"
#include "esphome/components/i2c/i2c.h"
#include "esphome/components/touchscreen/touchscreen.h"
#include "esphome/core/hal.h"

#include "gsl3670_firmware.h"
#include "gsl_point_id.h"

namespace esphome {
namespace gsl3670 {

#define TOUCH_MAX_POINTS 5

constexpr static const char *const TAG = "touchscreen.gsl3670";

class GSL3670 : public touchscreen::Touchscreen, public i2c::I2CDevice {
    public:
        void setup() override;
        void update_touches() override;

        void set_interrupt_pin(InternalGPIOPin *pin) { this->interrupt_pin_ = pin; }
        // Reset is optional. Accept any GPIOPin so it can live on an I2C
        // expander (e.g. Seeed reTerminal D1001) or be omitted entirely.
        void set_reset_pin(GPIOPin *pin) { this->reset_pin_ = pin; }

    protected:
        InternalGPIOPin *interrupt_pin_{};
        GPIOPin *reset_pin_{nullptr};

        esphome::i2c::ErrorCode reset();
        esphome::i2c::ErrorCode init();
        esphome::i2c::ErrorCode read_configuration();
        esphome::i2c::ErrorCode clear_registers();
        esphome::i2c::ErrorCode load_firmware();
        esphome::i2c::ErrorCode start();
        esphome::i2c::ErrorCode read_ram();
};

}
}
