cmake_minimum_required(VERSION 3.20)
project(power_surface_render_tests LANGUAGES C CXX ASM)
enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
set(ESPCONTROL_LVGL_DIR "" CACHE PATH "LVGL source from the repository-pinned ESPHome build")
if(NOT EXISTS "${ESPCONTROL_LVGL_DIR}/lvgl.h")
  message(FATAL_ERROR "Set ESPCONTROL_LVGL_DIR to the build's managed_components/lvgl__lvgl directory")
endif()
set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lv_conf.h" CACHE PATH "" FORCE)
set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
add_subdirectory("${ESPCONTROL_LVGL_DIR}" lvgl)
set(ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(GENERATED "${CMAKE_CURRENT_BINARY_DIR}/power_surface_render_test.cpp")
add_custom_command(OUTPUT "${GENERATED}"
  COMMAND ${Python3_EXECUTABLE} "${ROOT}/tests/firmware/generate_power_ui_test.py"
    --root "${ROOT}" --output "${GENERATED}" --native-surface
  DEPENDS power_surface_render_test.cpp.in ../firmware/generate_power_ui_test.py
    "${ROOT}/components/espcontrol/button_grid_power.h"
    "${ROOT}/components/espcontrol/button_grid_sensor_driver.h"
    "${ROOT}/components/espcontrol/button_grid_subpages.h"
    "${ROOT}/components/espcontrol/button_grid_display.h"
  VERBATIM)
add_executable(power_surface_render_test "${GENERATED}")
target_compile_features(power_surface_render_test PRIVATE cxx_std_17)
target_include_directories(power_surface_render_test PRIVATE "${ROOT}/components/espcontrol")
target_link_libraries(power_surface_render_test PRIVATE lvgl)
if(MSVC)
  target_compile_options(power_surface_render_test PRIVATE /UNDEBUG)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(power_surface_render_test PRIVATE -UNDEBUG)
endif()
option(ESPCONTROL_RENDER_SANITIZERS "Run native rendering under ASan/UBSan" OFF)
if(ESPCONTROL_RENDER_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  foreach(target lvgl power_surface_render_test)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
  endforeach()
  target_link_options(power_surface_render_test PRIVATE -fsanitize=address,undefined)
endif()
add_test(NAME power_surface_render_test COMMAND power_surface_render_test)
set_tests_properties(power_surface_render_test PROPERTIES TIMEOUT 90)
if(ESPCONTROL_RENDER_SANITIZERS AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  set_tests_properties(power_surface_render_test PROPERTIES ENVIRONMENT
    "ASAN_OPTIONS=detect_leaks=1:abort_on_error=1;UBSAN_OPTIONS=halt_on_error=1:print_stacktrace=1")
endif()
