#include "gsl3670.h"

namespace esphome {
namespace gsl3670 {

#define STOP_ON_I2C_ERROR(code, value) code = value; if (code != esphome::i2c::ERROR_OK) { ESP_LOGE(TAG, "I2C Error: %d", code); return code; }

void GSL3670::setup() {

    ESP_LOGD(TAG, "Setup start");
    this->x_raw_max_ = this->swap_x_y_? this->get_display()->get_native_height(): this->get_display()->get_native_width() ;
    this->y_raw_max_ = this->swap_x_y_? this->get_display()->get_native_width() : this->get_display()->get_native_height();

    if (this->reset_pin_ != nullptr) {
        this->reset_pin_->pin_mode(esphome::gpio::FLAG_OUTPUT);
        this->reset_pin_->setup();
        this->reset_pin_->digital_write(false);
        this->reset_pin_->digital_write(true);
    }

    auto err = this->init();
    if (err != esphome::i2c::ERROR_OK) {
        this->mark_failed(LOG_STR("I2C init error"));
        return;
    }

    this->interrupt_pin_->pin_mode(gpio::FLAG_INPUT | gpio::FLAG_PULLUP);
    this->interrupt_pin_->setup();
    this->attach_interrupt_(this->interrupt_pin_, gpio::INTERRUPT_FALLING_EDGE);

    gsl_DataInit(gsl_config_data_id);
    ESP_LOGI(TAG, "Setup complete");
}

esphome::i2c::ErrorCode GSL3670::init() {
    auto err = esphome::i2c::ERROR_OK;

    STOP_ON_I2C_ERROR(err, this->read_configuration());
    STOP_ON_I2C_ERROR(err, this->clear_registers());
    STOP_ON_I2C_ERROR(err, this->reset());
    STOP_ON_I2C_ERROR(err, this->load_firmware());
    STOP_ON_I2C_ERROR(err, this->start());
    STOP_ON_I2C_ERROR(err, this->read_ram());

    return err;
}

esphome::i2c::ErrorCode GSL3670::reset() {
    if (this->reset_pin_ != nullptr) {
        this->reset_pin_->digital_write(false);
        esphome::delay(20);
        this->reset_pin_->digital_write(true);
        esphome::delay(20);
    }

    auto err = esphome::i2c::ERROR_OK;
    uint8_t write_buf[4];

    write_buf[0] = 0x88;
    STOP_ON_I2C_ERROR(err, this->write_register(0xe0, (uint8_t *)&write_buf, 1));
    esphome::delay(10);
    write_buf[0] = 0x04;
    STOP_ON_I2C_ERROR(err, this->write_register(0xe4, (uint8_t *)&write_buf, 1));
    esphome::delay(10);

    write_buf[0] = 0x00;
    write_buf[1] = 0x00;
    write_buf[2] = 0x00;
    write_buf[3] = 0x00;
    STOP_ON_I2C_ERROR(err, this->write_register(0xbc, (uint8_t *)&write_buf, 4));
    esphome::delay(10);

    ESP_LOGD(TAG, "Reset complete");
    return err;

}

esphome::i2c::ErrorCode GSL3670::read_configuration() {
    auto err = esphome::i2c::ERROR_OK;

    uint8_t buf[4];
    uint8_t write[4] = {0x12, 0x34, 0x56, 0x00};

    esphome::delay(50);
    STOP_ON_I2C_ERROR(err, this->read_register(0xf0, (uint8_t *)&buf, 4));
    ESP_LOGD(TAG, "Read configuration #1: %x, %x, %x, %x", buf[0], buf[1], buf[2], buf[3]);
    esphome::delay(20);
    STOP_ON_I2C_ERROR(err, this->write_register(0xf0, (uint8_t *)&write, 4));
    esphome::delay(20);
    STOP_ON_I2C_ERROR(err, this->read_register(0xf0, (uint8_t *)&buf, 4));
    esphome::delay(20);
    ESP_LOGD(TAG, "Read configuration #2: %x, %x, %x, %x", buf[0], buf[1], buf[2], buf[3]);
    if (buf[0] != write[0]) {
        ESP_LOGE(TAG, "Invalid configuration byte returned, got 0x%x, expected 0x12", buf[0]);
        return esphome::i2c::ERROR_UNKNOWN;
    }
    return err;
}

esphome::i2c::ErrorCode GSL3670::clear_registers() {
    uint8_t clear_reg_regs[4] = {0xe0, 0x88, 0xe4, 0xe0};
    uint8_t clear_reg_data[4] = {0x88, 0x01, 0x04, 0x00};

    auto err = esphome::i2c::ERROR_OK;

    for (int i = 0; i < 4; i++) {
        STOP_ON_I2C_ERROR(err, this->write_register(clear_reg_regs[i], (uint8_t *)&clear_reg_data[i], 1));
        esphome::delay(20);
    }
    ESP_LOGD(TAG, "Clear registers complete");
    return err;

}

esphome::i2c::ErrorCode GSL3670::load_firmware() {
    ESP_LOGD(TAG,"Load firmware start");
    uint8_t addr;
    uint8_t wrbuf[4];
    uint16_t source_len = sizeof(GSLX680_FW) / sizeof(struct fw_data);
    auto err = esphome::i2c::ERROR_OK;

    for(int i = 0; i < source_len; i++) {
        addr = GSLX680_FW[i].offset;
        wrbuf[0] = (uint8_t)(GSLX680_FW[i].val & 0x000000ff);
        wrbuf[1] = (uint8_t)((GSLX680_FW[i].val & 0x0000ff00) >> 8);
        wrbuf[2] = (uint8_t)((GSLX680_FW[i].val & 0x00ff0000) >> 16);
        wrbuf[3] = (uint8_t)((GSLX680_FW[i].val & 0xff000000) >> 24);
        STOP_ON_I2C_ERROR(err, this->write_register(addr, (uint8_t *)&wrbuf, addr == 0xf0? 1: 4));
    }
    ESP_LOGD(TAG,"Load firmware complete");
    return err;
}

esphome::i2c::ErrorCode GSL3670::start() {
    uint8_t write_buf[1] = {0x00};
    uint8_t addr = 0xe0;

    auto err = esphome::i2c::ERROR_OK;
    STOP_ON_I2C_ERROR(err, this->write_register(addr, (uint8_t *)&write_buf, 1));
    esphome::delay(10);
    ESP_LOGD(TAG,"Start chipset complete");

    return err;

}

esphome::i2c::ErrorCode GSL3670::read_ram() {
    uint8_t buf[4];

    auto err = esphome::i2c::ERROR_OK;
    esphome::delay(30);
    STOP_ON_I2C_ERROR(err, this->read_register(0xb0, (uint8_t *)&buf, 4));
    ESP_LOGD(TAG, "Read RAM: %x, %x, %x, %x", buf[0], buf[1], buf[2], buf[3]);
    for (int i = 0; i < 4; i++) {
        if (buf[i] != 0x5a) {
            ESP_LOGE(TAG, "Unexpected byte in read_ram: got 0x%x, expected 0x5a", buf[i]);
            return esphome::i2c::ERROR_UNKNOWN;
        }
    }
    return err;
}


void GSL3670::update_touches() {
    uint8_t touch_data[24];
    struct gsl_touch_info cinfo = {0};

    auto err = this->read_register(0x80, (uint8_t *)&touch_data, 24);
    if (err != esphome::i2c::ERROR_OK) {
        ESP_LOGE(TAG, "I2C error in update_touches");
        return;
    }
    ESP_LOGV(TAG, "update_touches: %x %x %x %x %x %x %x %x", touch_data[0], touch_data[1], touch_data[2], touch_data[3], touch_data[4], touch_data[5], touch_data[6], touch_data[7]);

    uint16_t x1 = ((touch_data[7] & 0x0f) << 8) | touch_data[6];
    uint16_t y1 = (touch_data[5] << 8) | touch_data[4];
    uint16_t x2 = ((touch_data[11] & 0x0f) << 8) | touch_data[10];
    uint16_t y2 = (touch_data[9] << 8) | touch_data[8];

    cinfo.x[0] = x1;
    cinfo.y[0] = y1;
    cinfo.id[0] = ((touch_data[7] & 0xf0) >> 4);
    cinfo.x[1] = x2;
    cinfo.y[1] = y2;
    cinfo.id[1] = ((touch_data[11] & 0xf0) >> 4);
    cinfo.finger_num = (touch_data[3] << 24) | (touch_data[2] << 16) | (touch_data[1] << 8) | touch_data[0];

    gsl_alg_id_main(&cinfo);
    unsigned int mask = gsl_mask_tiaoping();

    if ((mask > 0) && (mask < 0xffffffff)) {
        uint8_t buf[4] = {0xa, 0x0, 0x0, 0x0};
        this->write_register(0xf0, (uint8_t *)&buf, 4);
        buf[0] = (uint8_t)(mask & 0xff);
        buf[1] = (uint8_t)((mask >> 8) & 0xff);
        buf[2] = (uint8_t)((mask >> 16) & 0xff);
        buf[3] = (uint8_t)((mask >> 24) & 0xff);
        this->write_register(0x8, (uint8_t *)&buf, 4);
    }

    ESP_LOGV(TAG, "update_touches: touch [%d] %dx%d (%d)", cinfo.finger_num, cinfo.x[0], cinfo.y[0], mask);

    if (cinfo.finger_num == 1) {
        // Report only first finger for now
        this->add_raw_touch_position_(0, cinfo.x[0], cinfo.y[0]);
    }
}

}
}
