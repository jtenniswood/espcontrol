#pragma once
#define VERSION_CODE(major, minor, patch) ((major) * 10000 + (minor) * 100 + (patch))
#ifndef TEST_ESPHOME_MINOR
#define TEST_ESPHOME_MINOR 9
#endif
#define ESPHOME_VERSION_CODE VERSION_CODE(2026, TEST_ESPHOME_MINOR, 1)
