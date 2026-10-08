#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>

namespace esphome {
template<class... Args> void log(const char *, const char *, Args...) {}
#define ESP_LOGD(...) esphome::log(__VA_ARGS__)
#define ESP_LOGI(...) esphome::log(__VA_ARGS__)
#define ESP_LOGW(...) esphome::log(__VA_ARGS__)
#define ESP_LOGE(...) esphome::log(__VA_ARGS__)
#define ESP_LOGV(...) esphome::log(__VA_ARGS__)
#define ESP_LOGCONFIG(...) esphome::log(__VA_ARGS__)
#define LOG_STR(value) value
unsigned elapsed = 0;
void delay(unsigned ms) { elapsed += ms; }
struct Application { unsigned feeds = 0; void feed_wdt() { ++feeds; } } App;
namespace gpio {
enum Flags { FLAG_OUTPUT = 1, FLAG_INPUT = 2, FLAG_PULLUP = 4, INTERRUPT_FALLING_EDGE = 8 };
}
struct InternalGPIOPin {
    void pin_mode(int) {}
    void setup() {}
    void digital_write(bool);
};
namespace touchscreen {
struct Display {
    unsigned get_native_height() { return 1280; }
    unsigned get_native_width() { return 800; }
};
struct Touchscreen {
    virtual ~Touchscreen() = default;
    virtual void setup() {}
    virtual void dump_config() {}
    virtual void update_touches() {}
    bool swap_x_y_ = false, failed = false, attached = false;
    int x_raw_min_ = 0, y_raw_min_ = 0, x_raw_max_ = 0, y_raw_max_ = 0;
    Display display;
    Display *get_display() { return &display; }
    void mark_failed(const char *) { failed = true; }
    bool is_failed() const { return failed; }
    void attach_interrupt_(InternalGPIOPin *, int) { attached = true; }
    void add_raw_touch_position_(int, int, int) {}
};
}
namespace i2c {
enum ErrorCode { ERROR_OK = 0, ERROR_TIMEOUT = 3, ERROR_UNKNOWN = 6 };
struct Bus {
    std::map<uint32_t, std::array<uint8_t, 128>> ram;
    std::array<uint8_t, 4> page_register{};
    uint32_t page = 0;
    unsigned count = 0, uploads = 0, verified = 0, starts = 0, status_reads = 0;
    unsigned ready_after = 0, ready_at = 0;
    bool corrupt = false, corrupt_once = false, read_error = false, write_error = false;
    bool invalid_page_width = false, never_ready = false, started = false;
    ErrorCode write(uint8_t reg, const uint8_t *data, size_t len) {
        if (write_error) return ERROR_TIMEOUT;
        if (reg == 0xf0) {
            // Model a controller that ACKs short page selectors but ignores them.
            if (len != 4) { invalid_page_width = true; return ERROR_OK; }
            std::memcpy(page_register.data(), data, 4);
            page = uint32_t(data[0]) | (uint32_t(data[1]) << 8) |
                   (uint32_t(data[2]) << 16) | (uint32_t(data[3]) << 24);
        } else if (reg == 0x80) {
            count = data[0];
        } else if (reg == 0xe0) {
            if (data[0] == 0x88) {
                started = false;
            } else if (data[0] == 0 && uploads > 0) {
                ++starts;
                started = true;
                ready_at = elapsed + ready_after;
            }
        } else if (reg < 0x80 && len == 4) {
            std::memcpy(ram[page].data() + reg, data, 4);
            ++uploads;
        }
        return ERROR_OK;
    }
    ErrorCode read(uint8_t reg, uint8_t *data, size_t len) {
        if (read_error) return ERROR_TIMEOUT;
        std::memset(data, 0, len);
        if (reg == 0xf0) std::memcpy(data, page_register.data(), len);
        else if (reg == 0xb0) {
            ++status_reads;
            if (started && count > 0 && !never_ready && elapsed >= ready_at)
                std::memset(data, 0x5a, len);
        } else if (reg < 0x80) {
            assert(len == 4);
            ++verified;
            std::memcpy(data, ram[page].data() + reg, len);
            if (corrupt || corrupt_once) { data[0] ^= 1; corrupt_once = false; }
        }
        return ERROR_OK;
    }
};
Bus *bus = nullptr;
struct I2CDevice {
    ErrorCode write_register(uint8_t reg, const uint8_t *data, size_t len) {
        return bus->write(reg, data, len);
    }
    ErrorCode read_register(uint8_t reg, uint8_t *data, size_t len) {
        return bus->read(reg, data, len);
    }
};
}
void InternalGPIOPin::digital_write(bool high) {
    if (!high && i2c::bus != nullptr) {
        i2c::bus->count = 0;
        i2c::bus->uploads = 0;
        i2c::bus->started = false;
    }
}
}

#include "gsl3680_under_test.cpp"
unsigned int gsl_mask_tiaoping() { return 0; }
void gsl_alg_id_main(gsl_touch_info *) {}
void gsl_DataInit(unsigned int *) {}

struct Fixture {
    esphome::i2c::Bus bus;
    esphome::InternalGPIOPin reset, interrupt;
    esphome::gsl3680::GSL3680 driver;
    Fixture() {
        esphome::elapsed = 0;
        esphome::App.feeds = 0;
        esphome::i2c::bus = &bus;
        driver.set_reset_pin(&reset);
        driver.set_interrupt_pin(&interrupt);
    }
};

int main() {
    using namespace esphome::gsl3680;
    // ACKs alone cannot make a panel start: it needs the touch-count register
    // and complete page selectors. The original issue's driver fails here.
    {
        Fixture f;
        f.driver.setup();
        assert(!f.driver.failed && f.driver.attached);
        assert(f.bus.count == TOUCH_MAX_POINTS);
        assert(!f.bus.invalid_page_width);
        unsigned words = 0;
        for (const auto &word : GSLX680_FW) if (word.offset != 0xf0) ++words;
        assert(f.bus.verified == words);
        assert(esphome::App.feeds > 100);
    }
    // Accept a healthy controller that becomes ready after the old 40ms check.
    {
        Fixture f;
        f.bus.ready_after = 150;
        f.driver.setup();
        assert(!f.driver.failed && f.driver.attached);
        assert(f.bus.status_reads > 1);
    }
    // Corruption must not pass into readiness/touch setup, despite successful ACKs.
    {
        Fixture f;
        f.bus.corrupt = true;
        f.driver.setup();
        assert(f.driver.failed && !f.driver.attached);
        assert(f.bus.status_reads == 0);
        assert(f.bus.verified == 2); // One rejected word on each bounded attempt.
    }
    // Retry a transient upload/readback problem from the whole boot sequence.
    {
        Fixture f;
        f.bus.corrupt_once = true;
        f.driver.setup();
        assert(!f.driver.failed && f.driver.attached);
        assert(f.bus.status_reads == 1);
    }
    // Keep a real non-starting controller failed, with a bounded retry budget.
    {
        Fixture f;
        f.bus.never_ready = true;
        f.driver.setup();
        assert(f.driver.failed && !f.driver.attached);
        assert(f.bus.status_reads == 20);
        assert(esphome::elapsed < 2000);
    }
    for (bool write : {false, true}) {
        Fixture f;
        f.bus.read_error = !write;
        f.bus.write_error = write;
        f.driver.setup();
        assert(f.driver.failed && !f.driver.attached);
        assert(f.bus.starts == 0);
    }
    std::puts("GSL3680 startup checks passed");
}
