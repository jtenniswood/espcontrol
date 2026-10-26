cmake_minimum_required(VERSION 3.20)
project(espcontrol_firmware_tests LANGUAGES CXX)

enable_testing()
include(CheckCXXSourceCompiles)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_executable(page_modal_admission_test page_modal_admission_test.cpp)
target_compile_features(page_modal_admission_test PRIVATE cxx_std_17)
target_compile_options(page_modal_admission_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_modal_admission_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_modal_admission_test COMMAND page_modal_admission_test)

add_executable(page_native_scene_state_test page_native_scene_state_test.cpp
  ../../components/espcontrol/page_media_state_runtime.cpp ../../components/espcontrol/page_forecast_runtime.cpp
  ../../components/espcontrol/page_local_sensor_runtime.cpp ../../components/espcontrol/page_local_relay_runtime.cpp
  ../../components/espcontrol/page_fan_status_runtime.cpp ../../components/espcontrol/page_percent_slider_runtime.cpp
  ../../components/espcontrol/page_number_slider_runtime.cpp ../../components/espcontrol/page_light_temperature_runtime.cpp
  ../../components/espcontrol/page_timer_runtime.cpp ../../components/espcontrol/page_cover_toggle_runtime.cpp
  ../../components/espcontrol/page_access_state_runtime.cpp ../../components/espcontrol/page_calendar_runtime.cpp
  ../../components/espcontrol/page_timezone_runtime.cpp ../../components/espcontrol/page_alarm_state_runtime.cpp
  ../../components/espcontrol/page_alarm_ack_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_native_scene_state_test PRIVATE cxx_std_17)
target_compile_options(page_native_scene_state_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_native_scene_state_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_native_scene_state_test COMMAND page_native_scene_state_test)

add_executable(page_confirmation_runtime_test page_confirmation_runtime_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp)
target_compile_features(page_confirmation_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_confirmation_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_confirmation_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_confirmation_runtime_test COMMAND page_confirmation_runtime_test)

file(GLOB PAGE_STORE_FIXTURES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../common/config/*_card_normalization_fixtures.json)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/page_store_goldens.h
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/generate_page_store_goldens.py
    --output ${CMAKE_CURRENT_BINARY_DIR}/page_store_goldens.h
  DEPENDS ${PAGE_STORE_FIXTURES}
    ../../common/config/card_normalization_fixtures.json
    ../../scripts/generate_page_store_goldens.py
    ../../scripts/generate_saved_config_parser_test.py
  VERBATIM)
add_executable(page_store_test page_store_test.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/page_store_goldens.h
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_store_test PRIVATE cxx_std_17)
target_compile_options(page_store_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_store_test PRIVATE stubs ../../components/espcontrol
  ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME page_store_test COMMAND page_store_test)

add_executable(page_store_runtime_test page_store_runtime_test.cpp
  ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp)
target_compile_features(page_store_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_store_runtime_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_store_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_store_runtime_test COMMAND page_store_runtime_test)

add_executable(page_entity_state_runtime_test page_entity_state_runtime_test.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_entity_state_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_entity_state_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_entity_state_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_entity_state_runtime_test COMMAND page_entity_state_runtime_test)

set(PAGE_CLIMATE_PARITY_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_climate_parity_test.cpp)
add_custom_command(
  OUTPUT ${PAGE_CLIMATE_PARITY_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_climate_parity_test.py
    --header ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_climate.h
    --output ${PAGE_CLIMATE_PARITY_SOURCE}
  DEPENDS generate_page_climate_parity_test.py generate_climate_update_test.py
    page_climate_parity_test.cpp.in ../../components/espcontrol/button_grid_climate.h
  VERBATIM)
add_executable(page_climate_parity_test ${PAGE_CLIMATE_PARITY_SOURCE}
  ../../components/espcontrol/page_climate_card.cpp
  ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_climate_parity_test PRIVATE cxx_std_17)
target_compile_options(page_climate_parity_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_climate_parity_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_climate_parity_test COMMAND page_climate_parity_test)

add_executable(page_modal_runtime_test page_modal_runtime_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_modal_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_modal_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_modal_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_modal_runtime_test COMMAND page_modal_runtime_test)

add_executable(page_modal_router_test page_modal_router_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_modal_router_test PRIVATE cxx_std_17)
target_compile_options(page_modal_router_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_modal_router_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_modal_router_test COMMAND page_modal_router_test)

add_executable(page_card_commands_test page_card_commands_test.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp)
target_compile_features(page_card_commands_test PRIVATE cxx_std_17)
target_compile_options(page_card_commands_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_card_commands_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_card_commands_test COMMAND page_card_commands_test)

add_executable(page_card_control_router_test page_card_control_router_test.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_media_state_runtime.cpp
  ../../components/espcontrol/page_timer_runtime.cpp ../../components/espcontrol/page_percent_slider_runtime.cpp
  ../../components/espcontrol/page_number_slider_runtime.cpp ../../components/espcontrol/page_light_temperature_runtime.cpp)
target_compile_features(page_card_control_router_test PRIVATE cxx_std_17)
target_compile_options(page_card_control_router_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_card_control_router_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_card_control_router_test COMMAND page_card_control_router_test)

add_executable(page_media_commands_test page_media_commands_test.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_media_state_runtime.cpp)
target_compile_features(page_media_commands_test PRIVATE cxx_std_17)
target_compile_options(page_media_commands_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_media_commands_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_media_commands_test COMMAND page_media_commands_test)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/media_action_helpers_test.cpp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_media_action_helpers_test.py
    --output ${CMAKE_CURRENT_BINARY_DIR}/media_action_helpers_test.cpp
  DEPENDS generate_media_action_helpers_test.py generate_climate_update_test.py media_action_helpers_test.cpp.in
    ../../components/espcontrol/button_grid_actions.h VERBATIM)
add_executable(media_action_helpers_test ${CMAKE_CURRENT_BINARY_DIR}/media_action_helpers_test.cpp)
target_compile_features(media_action_helpers_test PRIVATE cxx_std_17)
target_compile_options(media_action_helpers_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(media_action_helpers_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME media_action_helpers_test COMMAND media_action_helpers_test)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/device_action_helpers_test.cpp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_device_action_helpers_test.py
    --output ${CMAKE_CURRENT_BINARY_DIR}/device_action_helpers_test.cpp
  DEPENDS ../../components/espcontrol/page_device_command_policy.h ../../components/espcontrol/page_native_device_commands.h generate_device_action_helpers_test.py generate_climate_update_test.py device_action_helpers_test.cpp.in
    ../../components/espcontrol/button_grid_actions.h VERBATIM)
add_executable(device_action_helpers_test ${CMAKE_CURRENT_BINARY_DIR}/device_action_helpers_test.cpp)
target_compile_features(device_action_helpers_test PRIVATE cxx_std_17)
target_compile_options(device_action_helpers_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(device_action_helpers_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME device_action_helpers_test COMMAND device_action_helpers_test)

add_executable(page_timer_commands_test page_timer_commands_test.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_timer_runtime.cpp)
target_compile_features(page_timer_commands_test PRIVATE cxx_std_17)
target_compile_options(page_timer_commands_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_timer_commands_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_timer_commands_test COMMAND page_timer_commands_test)

add_executable(page_slider_commands_test page_slider_commands_test.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_percent_slider_runtime.cpp
  ../../components/espcontrol/page_number_slider_runtime.cpp
  ../../components/espcontrol/page_light_temperature_runtime.cpp)
target_compile_features(page_slider_commands_test PRIVATE cxx_std_17)
target_compile_options(page_slider_commands_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_slider_commands_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_slider_commands_test COMMAND page_slider_commands_test)

add_executable(page_entity_state_benchmark EXCLUDE_FROM_ALL page_entity_state_benchmark.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_entity_state_benchmark PRIVATE cxx_std_17)
target_compile_options(page_entity_state_benchmark PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_entity_state_benchmark PRIVATE stubs ../../components/espcontrol)

add_executable(page_entity_modal_replay_test page_entity_modal_replay_test.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_entity_modal_replay_test PRIVATE cxx_std_17)
target_compile_options(page_entity_modal_replay_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_entity_modal_replay_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_entity_modal_replay_test COMMAND page_entity_modal_replay_test)

add_executable(page_card_dependencies_test page_card_dependencies_test.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/page_store_goldens.h
  ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_card_dependencies_test PRIVATE cxx_std_17)
target_compile_options(page_card_dependencies_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_card_dependencies_test PRIVATE stubs ../../components/espcontrol
  ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME page_card_dependencies_test COMMAND page_card_dependencies_test)

add_executable(page_renderer_runtime_test page_renderer_runtime_test.cpp
  ../../components/espcontrol/page_renderer_layout.cpp
  ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_renderer_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_renderer_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_renderer_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_renderer_runtime_test COMMAND page_renderer_runtime_test)

add_executable(bounded_sensor_text_test bounded_sensor_text_test.cpp)
target_compile_features(bounded_sensor_text_test PRIVATE cxx_std_17)
target_compile_options(bounded_sensor_text_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(bounded_sensor_text_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME bounded_sensor_text_test COMMAND bounded_sensor_text_test)

set(PAGE_MEDIA_GROUP_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_media_group_test.cpp)
add_custom_command(OUTPUT ${PAGE_MEDIA_GROUP_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_media_group_test.py
    --output ${PAGE_MEDIA_GROUP_SOURCE}
  DEPENDS generate_page_media_group_test.py page_media_group_test.cpp.in
    ../../components/espcontrol/button_grid_media_group.h
  VERBATIM)
add_executable(page_media_group_test ${PAGE_MEDIA_GROUP_SOURCE})
target_compile_features(page_media_group_test PRIVATE cxx_std_17)
target_compile_options(page_media_group_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_media_group_test PRIVATE ../../components/espcontrol)
add_test(NAME page_media_group_test COMMAND page_media_group_test)

set(PAGE_FAN_MODE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_fan_mode_test.cpp)
add_custom_command(OUTPUT ${PAGE_FAN_MODE_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_fan_mode_test.py
    --output ${PAGE_FAN_MODE_SOURCE}
  DEPENDS generate_page_fan_mode_test.py page_fan_mode_test.cpp.in
    ../../components/espcontrol/button_grid_fan.h
  VERBATIM)
add_executable(page_fan_mode_test ${PAGE_FAN_MODE_SOURCE})
target_compile_features(page_fan_mode_test PRIVATE cxx_std_17)
target_compile_options(page_fan_mode_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_fan_mode_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_fan_mode_test COMMAND page_fan_mode_test)

add_executable(page_timer_runtime_test page_timer_runtime_test.cpp
  ../../components/espcontrol/page_timer_runtime.cpp ../../components/espcontrol/page_store_runtime.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_card_dependencies.cpp)
target_compile_features(page_timer_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_timer_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_timer_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_timer_runtime_test COMMAND page_timer_runtime_test)

set(PAGE_LIGHT_TEMPERATURE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_light_temperature_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_LIGHT_TEMPERATURE_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_light_temperature_test.py
    --output ${PAGE_LIGHT_TEMPERATURE_SOURCE}
  DEPENDS generate_page_light_temperature_test.py page_light_temperature_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_sliders.h
  VERBATIM)
add_executable(page_light_temperature_runtime_test ${PAGE_LIGHT_TEMPERATURE_SOURCE}
  ../../components/espcontrol/page_light_temperature_runtime.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_light_temperature_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_light_temperature_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_light_temperature_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_light_temperature_runtime_test COMMAND page_light_temperature_runtime_test)

set(PAGE_NUMBER_SLIDER_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_number_slider_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_NUMBER_SLIDER_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_number_slider_test.py
    --output ${PAGE_NUMBER_SLIDER_SOURCE}
  DEPENDS generate_page_number_slider_test.py page_number_slider_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_sliders.h
  VERBATIM)
add_executable(page_number_slider_runtime_test ${PAGE_NUMBER_SLIDER_SOURCE}
  ../../components/espcontrol/page_store_runtime.cpp
  ../../components/espcontrol/page_number_slider_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp)
target_compile_features(page_number_slider_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_number_slider_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_number_slider_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_number_slider_runtime_test COMMAND page_number_slider_runtime_test)

set(PAGE_PERCENT_SLIDER_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_percent_slider_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_PERCENT_SLIDER_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_percent_slider_test.py
    --output ${PAGE_PERCENT_SLIDER_SOURCE}
  DEPENDS generate_page_percent_slider_test.py page_percent_slider_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_sliders.h
  VERBATIM)
add_executable(page_percent_slider_runtime_test ${PAGE_PERCENT_SLIDER_SOURCE}
  ../../components/espcontrol/page_percent_slider_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_percent_slider_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_percent_slider_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_percent_slider_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_percent_slider_runtime_test COMMAND page_percent_slider_runtime_test)

set(ALARM_ACK_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_alarm_ack_runtime_test.cpp)
add_custom_command(OUTPUT ${ALARM_ACK_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_alarm_ack_test.py --output ${ALARM_ACK_SOURCE}
  DEPENDS generate_alarm_ack_test.py page_alarm_ack_runtime_test.cpp.in ../../components/espcontrol/button_grid_alarm.h
  VERBATIM)
add_executable(page_alarm_ack_runtime_test ${ALARM_ACK_SOURCE}
  ../../components/espcontrol/page_alarm_ack_runtime.cpp ../../components/espcontrol/page_alarm_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_store_runtime.cpp
  ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp)
target_compile_features(page_alarm_ack_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_alarm_ack_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_alarm_ack_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_alarm_ack_runtime_test COMMAND page_alarm_ack_runtime_test)

set(ALARM_POLICY_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_alarm_policy_test.cpp)
add_custom_command(OUTPUT ${ALARM_POLICY_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_alarm_policy_test.py --output ${ALARM_POLICY_SOURCE}
  DEPENDS generate_alarm_policy_test.py page_alarm_policy_test.cpp.in ../../components/espcontrol/button_grid_alarm.h
  VERBATIM)
add_executable(page_alarm_policy_test ${ALARM_POLICY_SOURCE})
target_compile_features(page_alarm_policy_test PRIVATE cxx_std_17)
target_compile_options(page_alarm_policy_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_alarm_policy_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_alarm_policy_test COMMAND page_alarm_policy_test)

set(ALARM_STATE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_alarm_state_runtime_test.cpp)
add_custom_command(OUTPUT ${ALARM_STATE_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_alarm_state_test.py --output ${ALARM_STATE_SOURCE}
  DEPENDS generate_page_alarm_state_test.py page_alarm_state_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_alarm.h ../../components/espcontrol/button_grid_access_cards.h
  VERBATIM)
add_executable(page_alarm_state_runtime_test ${ALARM_STATE_SOURCE}
  ../../components/espcontrol/page_alarm_state_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp)
target_compile_features(page_alarm_state_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_alarm_state_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_alarm_state_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_alarm_state_runtime_test COMMAND page_alarm_state_runtime_test)

add_executable(page_alarm_presentation_test page_alarm_presentation_test.cpp
  ../../components/espcontrol/page_alarm_presentation.cpp ../../components/espcontrol/page_modal_runtime.cpp
  ../../components/espcontrol/page_alarm_ack_runtime.cpp ../../components/espcontrol/page_alarm_state_runtime.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_alarm_presentation_test PRIVATE cxx_std_17)
target_compile_options(page_alarm_presentation_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_alarm_presentation_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_alarm_presentation_test COMMAND page_alarm_presentation_test)

set(ALARM_COMMANDS_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_alarm_commands_test.cpp)
add_custom_command(OUTPUT ${ALARM_COMMANDS_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_alarm_commands_test.py --output ${ALARM_COMMANDS_SOURCE}
  DEPENDS generate_alarm_commands_test.py page_alarm_commands_test.cpp.in ../../components/espcontrol/button_grid_alarm.h
  VERBATIM)
add_executable(page_alarm_commands_test ${ALARM_COMMANDS_SOURCE}
  ../../components/espcontrol/page_alarm_commands.cpp
  ../../components/espcontrol/page_alarm_ack_runtime.cpp
  ../../components/espcontrol/page_alarm_state_runtime.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_alarm_commands_test PRIVATE cxx_std_17)
target_compile_options(page_alarm_commands_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_alarm_commands_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_alarm_commands_test COMMAND page_alarm_commands_test)

set(ALARM_CARD_VIEW_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/alarm_card_view_test.cpp)
add_custom_command(OUTPUT ${ALARM_CARD_VIEW_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_alarm_card_view_test.py --output ${ALARM_CARD_VIEW_SOURCE}
  DEPENDS generate_alarm_card_view_test.py alarm_card_view_test.cpp.in ../../components/espcontrol/button_grid_alarm.h
  VERBATIM)
add_executable(alarm_card_view_test ${ALARM_CARD_VIEW_SOURCE})
target_compile_features(alarm_card_view_test PRIVATE cxx_std_17)
target_compile_options(alarm_card_view_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(alarm_card_view_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME alarm_card_view_test COMMAND alarm_card_view_test)

set(NUMBER_LAYOUT_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/basic_number_layout_test.cpp)
add_custom_command(OUTPUT ${NUMBER_LAYOUT_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_number_layout_test.py --output ${NUMBER_LAYOUT_SOURCE}
  DEPENDS generate_number_layout_test.py basic_number_layout_test.cpp.in
    ../../components/espcontrol/button_grid_sensor_driver.h ../../components/espcontrol/button_grid_climate_control_driver.h
  VERBATIM)
add_executable(basic_number_layout_test ${NUMBER_LAYOUT_SOURCE})
target_compile_features(basic_number_layout_test PRIVATE cxx_std_17)
target_compile_options(basic_number_layout_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(basic_number_layout_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME basic_number_layout_test COMMAND basic_number_layout_test)

set(CLOCK_LAYOUT_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/clock_card_layout_test.cpp)
add_custom_command(OUTPUT ${CLOCK_LAYOUT_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_clock_layout_test.py --output ${CLOCK_LAYOUT_SOURCE}
  DEPENDS generate_clock_layout_test.py clock_card_layout_test.cpp.in ../../components/espcontrol/button_grid_date_time_driver.h
  VERBATIM)
add_executable(clock_card_layout_test ${CLOCK_LAYOUT_SOURCE})
target_compile_features(clock_card_layout_test PRIVATE cxx_std_17)
target_compile_options(clock_card_layout_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(clock_card_layout_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME clock_card_layout_test COMMAND clock_card_layout_test)

set(PAGE_TIMEZONE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_timezone_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_TIMEZONE_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_timezone_test.py
    --output ${PAGE_TIMEZONE_SOURCE}
  DEPENDS generate_page_timezone_test.py page_timezone_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_datetime_cards.h ../../components/espcontrol/sun_calc.h
    ../../components/espcontrol/clock_bar.h
  VERBATIM)
add_executable(page_timezone_runtime_test ${PAGE_TIMEZONE_SOURCE}
  ../../components/espcontrol/page_timezone_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_timezone_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_timezone_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_timezone_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_timezone_runtime_test COMMAND page_timezone_runtime_test)

set(CALENDAR_TEXT_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/calendar_card_text_test.cpp)
add_custom_command(OUTPUT ${CALENDAR_TEXT_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_calendar_text_test.py
    --output ${CALENDAR_TEXT_SOURCE}
  DEPENDS generate_calendar_text_test.py calendar_card_text_test.cpp.in
    ../../components/espcontrol/button_grid_datetime_cards.h ../../components/espcontrol/clock_bar.h
  VERBATIM)
add_executable(calendar_card_text_test ${CALENDAR_TEXT_SOURCE})
target_compile_features(calendar_card_text_test PRIVATE cxx_std_17)
target_compile_options(calendar_card_text_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(calendar_card_text_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME calendar_card_text_test COMMAND calendar_card_text_test)

set(PAGE_CALENDAR_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_calendar_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_CALENDAR_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_calendar_test.py
    --output ${PAGE_CALENDAR_SOURCE}
  DEPENDS generate_page_calendar_test.py page_calendar_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_datetime_cards.h
  VERBATIM)
add_executable(page_calendar_runtime_test ${PAGE_CALENDAR_SOURCE}
  ../../components/espcontrol/page_calendar_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_card_dependencies.cpp)
target_compile_features(page_calendar_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_calendar_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_calendar_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_calendar_runtime_test COMMAND page_calendar_runtime_test)

set(CLEANING_POLICY_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/cleaning_card_policy_test.cpp)
add_custom_command(OUTPUT ${CLEANING_POLICY_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_cleaning_policy_test.py
    --output ${CLEANING_POLICY_SOURCE}
  DEPENDS generate_cleaning_policy_test.py cleaning_card_policy_test.cpp.in
    ../../components/espcontrol/button_grid_vacuum.h ../../components/espcontrol/button_grid_lawn_mower.h
  VERBATIM)
add_executable(cleaning_card_policy_test ${CLEANING_POLICY_SOURCE})
target_compile_features(cleaning_card_policy_test PRIVATE cxx_std_17)
target_compile_options(cleaning_card_policy_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(cleaning_card_policy_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME cleaning_card_policy_test COMMAND cleaning_card_policy_test)

set(PAGE_COVER_TOGGLE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_cover_toggle_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_COVER_TOGGLE_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_cover_toggle_test.py
    --output ${PAGE_COVER_TOGGLE_SOURCE}
  DEPENDS generate_page_cover_toggle_test.py page_cover_toggle_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_access_cards.h
    ../../components/espcontrol/button_grid_config_parser.h ../../components/espcontrol/display_utf8_case.h
  VERBATIM)
add_executable(page_cover_toggle_runtime_test ${PAGE_COVER_TOGGLE_SOURCE}
  ../../components/espcontrol/page_cover_toggle_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_cover_toggle_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_cover_toggle_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_cover_toggle_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_cover_toggle_runtime_test COMMAND page_cover_toggle_runtime_test)

set(PAGE_ACCESS_STATE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_access_state_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_ACCESS_STATE_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_access_state_test.py
    --output ${PAGE_ACCESS_STATE_SOURCE}
  DEPENDS generate_page_access_state_test.py page_access_state_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_access_cards.h
    ../../components/espcontrol/button_grid_config_parser.h ../../components/espcontrol/display_utf8_case.h
  VERBATIM)
add_executable(page_access_state_runtime_test ${PAGE_ACCESS_STATE_SOURCE}
  ../../components/espcontrol/page_access_state_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_access_state_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_access_state_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_access_state_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_access_state_runtime_test COMMAND page_access_state_runtime_test)

set(PAGE_FAN_STATUS_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_fan_status_runtime_test.cpp)
add_custom_command(OUTPUT ${PAGE_FAN_STATUS_SOURCE}
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_fan_status_test.py
    --output ${PAGE_FAN_STATUS_SOURCE}
  DEPENDS generate_page_fan_status_test.py page_fan_status_runtime_test.cpp.in
    ../../components/espcontrol/button_grid_fan.h ../../components/espcontrol/button_grid_access_cards.h
  VERBATIM)
add_executable(page_fan_status_runtime_test ${PAGE_FAN_STATUS_SOURCE}
  ../../components/espcontrol/page_fan_status_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_store_runtime.cpp)
target_compile_features(page_fan_status_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_fan_status_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_fan_status_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_fan_status_runtime_test COMMAND page_fan_status_runtime_test)

add_executable(page_resource_publication_test page_resource_publication_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp
  ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_local_relay_runtime.cpp ../../components/espcontrol/page_local_sensor_runtime.cpp
  ../../components/espcontrol/page_media_state_runtime.cpp ../../components/espcontrol/page_forecast_runtime.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp)
target_compile_features(page_resource_publication_test PRIVATE cxx_std_17)
target_compile_options(page_resource_publication_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_resource_publication_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_resource_publication_test COMMAND page_resource_publication_test)

add_executable(page_local_relay_runtime_test page_local_relay_runtime_test.cpp
  ../../components/espcontrol/page_local_relay_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_local_relay_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_local_relay_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_local_relay_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_local_relay_runtime_test COMMAND page_local_relay_runtime_test)

add_executable(page_local_sensor_runtime_test page_local_sensor_runtime_test.cpp
  ../../components/espcontrol/page_local_sensor_runtime.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_local_sensor_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_local_sensor_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_local_sensor_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_local_sensor_runtime_test COMMAND page_local_sensor_runtime_test)
foreach(variant IN ITEMS "" "_empty" "_numeric" "_text")
  add_executable(page_esphome_local_sensor_provider${variant}_test page_esphome_local_sensor_provider_test.cpp)
  target_compile_features(page_esphome_local_sensor_provider${variant}_test PRIVATE cxx_std_17)
  target_compile_options(page_esphome_local_sensor_provider${variant}_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_esphome_local_sensor_provider${variant}_test PRIVATE local_sensor_stubs stubs ../../components/espcontrol)
  if(NOT variant)
    target_compile_definitions(page_esphome_local_sensor_provider${variant}_test PRIVATE USE_SENSOR USE_TEXT_SENSOR ESPHOME_ENTITY_SENSOR_COUNT=2 ESPHOME_ENTITY_TEXT_SENSOR_COUNT=2)
  elseif(variant STREQUAL "_numeric")
    target_compile_definitions(page_esphome_local_sensor_provider${variant}_test PRIVATE USE_SENSOR ESPHOME_ENTITY_SENSOR_COUNT=2)
  elseif(variant STREQUAL "_text")
    target_compile_definitions(page_esphome_local_sensor_provider${variant}_test PRIVATE USE_TEXT_SENSOR ESPHOME_ENTITY_TEXT_SENSOR_COUNT=2)
  endif()
  add_test(NAME page_esphome_local_sensor_provider${variant}_test COMMAND page_esphome_local_sensor_provider${variant}_test)
endforeach()

add_executable(page_basic_card_test page_basic_card_test.cpp
    ../../components/espcontrol/page_number_slider_runtime.cpp
    ../../components/espcontrol/page_light_temperature_runtime.cpp
    ../../components/espcontrol/page_timer_runtime.cpp
  ../../components/espcontrol/page_cover_toggle_runtime.cpp
  ../../components/espcontrol/page_access_state_runtime.cpp
  ../../components/espcontrol/page_calendar_runtime.cpp
  ../../components/espcontrol/page_timezone_runtime.cpp
  ../../components/espcontrol/page_alarm_state_runtime.cpp
  ../../components/espcontrol/page_percent_slider_runtime.cpp ../../components/espcontrol/page_fan_status_runtime.cpp ../../components/espcontrol/page_local_relay_runtime.cpp ../../components/espcontrol/page_local_sensor_runtime.cpp ../../components/espcontrol/page_basic_card.cpp ../../components/espcontrol/page_media_state_runtime.cpp
    ../../components/espcontrol/page_climate_card.cpp
  ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_basic_card_test PRIVATE cxx_std_17)
target_compile_options(page_basic_card_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_basic_card_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_basic_card_test COMMAND page_basic_card_test)
get_target_property(PAGE_BASIC_CARD_SOURCES page_basic_card_test SOURCES)
list(REMOVE_ITEM PAGE_BASIC_CARD_SOURCES page_basic_card_test.cpp)
add_executable(page_device_commands_test page_device_commands_test.cpp ${PAGE_BASIC_CARD_SOURCES})
target_compile_features(page_device_commands_test PRIVATE cxx_std_17)
target_compile_options(page_device_commands_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_device_commands_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_device_commands_test COMMAND page_device_commands_test)

add_executable(page_basic_card_no_forecast_test page_basic_card_test.cpp
    ../../components/espcontrol/page_number_slider_runtime.cpp
    ../../components/espcontrol/page_light_temperature_runtime.cpp
    ../../components/espcontrol/page_timer_runtime.cpp
  ../../components/espcontrol/page_cover_toggle_runtime.cpp
  ../../components/espcontrol/page_access_state_runtime.cpp
  ../../components/espcontrol/page_calendar_runtime.cpp
  ../../components/espcontrol/page_timezone_runtime.cpp
  ../../components/espcontrol/page_alarm_state_runtime.cpp
  ../../components/espcontrol/page_percent_slider_runtime.cpp ../../components/espcontrol/page_fan_status_runtime.cpp ../../components/espcontrol/page_local_relay_runtime.cpp ../../components/espcontrol/page_local_sensor_runtime.cpp ../../components/espcontrol/page_basic_card.cpp ../../components/espcontrol/page_media_state_runtime.cpp
    ../../components/espcontrol/page_climate_card.cpp
  ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store.cpp)
target_compile_features(page_basic_card_no_forecast_test PRIVATE cxx_std_17)
target_compile_options(page_basic_card_no_forecast_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_basic_card_no_forecast_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_basic_card_no_forecast_test COMMAND page_basic_card_no_forecast_test)
target_compile_definitions(page_basic_card_no_forecast_test PRIVATE ESPCONTROL_DISABLE_WEATHER_FORECAST=1)


add_executable(dpi_timing_test dpi_timing_test.cpp)
target_compile_features(dpi_timing_test PRIVATE cxx_std_17)
target_compile_options(dpi_timing_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(dpi_timing_test PRIVATE ../../components/mipi_dsi)
add_test(NAME dpi_timing_test COMMAND dpi_timing_test)
add_test(NAME p4_pipeline_runtime_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/p4_pipeline_runtime_test.py)
add_test(NAME mipi_dsi_draw_failure_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mipi_dsi_draw_failure_test.py)
add_test(NAME device_ui_limits_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/device_ui_limits_test.py)

add_test(NAME gsl3680_startup_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gsl3680_startup_test.py)

add_executable(foundation_test foundation_test.cpp)
target_compile_features(foundation_test PRIVATE cxx_std_17)
target_compile_options(foundation_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME foundation_test COMMAND foundation_test)

add_executable(continuous_corners_test continuous_corners_test.cpp)
target_compile_features(continuous_corners_test PRIVATE cxx_std_17)
target_compile_options(continuous_corners_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME continuous_corners_test COMMAND continuous_corners_test)
add_test(NAME card_curve_default_boot_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/card_curve_default_boot_test.py)
add_test(NAME card_curve_button_theme_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/card_curve_button_theme_test.py)
add_test(NAME network_status_corners_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/network_status_corners_test.py)

add_test(NAME option_select_dispatch_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/option_select_dispatch_test.py)

add_executable(theme_palette_test theme_palette_test.cpp)
target_compile_features(theme_palette_test PRIVATE cxx_std_17)
target_compile_options(theme_palette_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(theme_palette_test PRIVATE ../../components/espcontrol)
add_test(NAME theme_palette_test COMMAND theme_palette_test)

add_executable(theme_settings_test theme_settings_test.cpp)
target_compile_features(theme_settings_test PRIVATE cxx_std_17)
target_compile_options(theme_settings_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(theme_settings_test PRIVATE ../../components/espcontrol)
add_test(NAME theme_settings_test COMMAND theme_settings_test)

add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/theme_modal_adapter.h
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_theme_modal_test.py
    --header ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_modal.h
    --output ${CMAKE_CURRENT_BINARY_DIR}/theme_modal_adapter.h
  DEPENDS generate_theme_modal_test.py ../../components/espcontrol/button_grid_modal.h
    ../../components/espcontrol/button_grid_subscriptions.h
    ../../components/espcontrol/button_grid_media.h
    ../../components/espcontrol/card_availability.h
  VERBATIM)
add_executable(theme_runtime_tree_test theme_runtime_tree_test.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/theme_modal_adapter.h)
target_compile_features(theme_runtime_tree_test PRIVATE cxx_std_17)
target_compile_options(theme_runtime_tree_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(theme_runtime_tree_test PRIVATE ../../components/espcontrol
  ${CMAKE_CURRENT_BINARY_DIR})
add_test(NAME theme_runtime_tree_test COMMAND theme_runtime_tree_test)
add_test(NAME option_select_live_options_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/option_select_live_options_test.py)

add_test(NAME theme_refresh_contract_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/theme_refresh_contract_test.py)

add_executable(digest_auth_policy_test digest_auth_policy_test.cpp)
target_compile_features(digest_auth_policy_test PRIVATE cxx_std_17)
target_compile_options(digest_auth_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(digest_auth_policy_test PRIVATE ../../components/web_server_idf)
add_test(NAME digest_auth_policy_test COMMAND digest_auth_policy_test)

add_test(NAME multipart_header_fragments_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/multipart_header_fragments_test.py)
add_test(NAME multipart_upload_completion_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/multipart_upload_completion_test.py)

add_test(NAME web_ota_guard_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/web_ota_guard_test.py)
add_test(NAME web_event_source_lifecycle_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/web_event_source_lifecycle_test.py)

add_test(NAME option_select_values_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/option_select_values_test.py)

add_executable(button_grid_foundations_test button_grid_foundations_test.cpp)
target_compile_features(button_grid_foundations_test PRIVATE cxx_std_17)
target_compile_options(button_grid_foundations_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(button_grid_foundations_test PRIVATE
  stubs
  ../../components/espcontrol
)
add_test(NAME button_grid_foundations_test COMMAND button_grid_foundations_test)
add_test(NAME local_sensor_lifetime_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/local_sensor_lifetime_test.py)
add_test(NAME card_colour_rendering_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/card_colour_rendering_test.py ${CMAKE_CXX_COMPILER})

add_executable(media_display_text_test media_display_text_test.cpp)
target_compile_features(media_display_text_test PRIVATE cxx_std_17)
target_compile_options(media_display_text_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_display_text_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME media_display_text_test COMMAND media_display_text_test)
add_test(NAME media_display_integration_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/media_display_integration_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME compact_landscape_controls_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/compact_landscape_controls_test.py ${CMAKE_CXX_COMPILER})

add_executable(grid_navigation_service_test
  grid_navigation_service_test.cpp
  ../../components/espcontrol/espcontrol_app_core.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(grid_navigation_service_test PRIVATE cxx_std_17)
target_compile_options(grid_navigation_service_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(grid_navigation_service_test PRIVATE ../../components/espcontrol)
add_test(NAME grid_navigation_service_test COMMAND grid_navigation_service_test)

add_executable(control_modal_service_test
  control_modal_service_test.cpp
  ../../components/espcontrol/espcontrol_app_core.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(control_modal_service_test PRIVATE cxx_std_17)
target_compile_options(control_modal_service_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(control_modal_service_test PRIVATE ../../components/espcontrol)
add_test(NAME control_modal_service_test COMMAND control_modal_service_test)

add_executable(page_guest_wifi_runtime_test page_guest_wifi_runtime_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp
  ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp
  ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_guest_wifi_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_guest_wifi_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_guest_wifi_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_guest_wifi_runtime_test COMMAND page_guest_wifi_runtime_test)

add_executable(guest_wifi_state_test guest_wifi_state_test.cpp)
target_compile_features(guest_wifi_state_test PRIVATE cxx_std_17)
target_compile_options(guest_wifi_state_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(guest_wifi_state_test PRIVATE ../../components/espcontrol)
add_test(NAME guest_wifi_state_test COMMAND guest_wifi_state_test)
add_test(NAME local_sensor_invalid_values_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/local_sensor_invalid_values_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME guest_wifi_lifecycle_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/guest_wifi_lifecycle_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME webhook_log_secrets_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/webhook_log_secrets_test.py ${CMAKE_CXX_COMPILER})

add_executable(page_qr_runtime_test page_qr_runtime_test.cpp)
target_compile_features(page_qr_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_qr_runtime_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_qr_runtime_test PRIVATE ../../components/espcontrol)
add_test(NAME page_qr_runtime_test COMMAND page_qr_runtime_test)

add_executable(page_qr_matrix_test page_qr_matrix_test.cpp)
target_compile_features(page_qr_matrix_test PRIVATE cxx_std_17)
target_compile_options(page_qr_matrix_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_qr_matrix_test PRIVATE ../../components/espcontrol)
add_test(NAME page_qr_matrix_test COMMAND page_qr_matrix_test)

add_executable(wifi_qr_codec_test wifi_qr_codec_test.cpp)
target_compile_features(wifi_qr_codec_test PRIVATE cxx_std_17)
target_compile_options(wifi_qr_codec_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(wifi_qr_codec_test PRIVATE ../../components/espcontrol)
add_test(NAME wifi_qr_codec_test COMMAND wifi_qr_codec_test)

add_executable(icons_legacy_alias_test icons_legacy_alias_test.cpp)
target_compile_features(icons_legacy_alias_test PRIVATE cxx_std_17)
target_compile_options(icons_legacy_alias_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(icons_legacy_alias_test PRIVATE ../../components/espcontrol)
add_test(NAME icons_legacy_alias_test COMMAND icons_legacy_alias_test)

add_executable(external_memory_allocator_test external_memory_allocator_test.cpp)
target_compile_features(external_memory_allocator_test PRIVATE cxx_std_17)
target_compile_options(external_memory_allocator_test PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(external_memory_allocator_test PRIVATE USE_ESP32)
target_include_directories(external_memory_allocator_test PRIVATE
  stubs
  ../../components/espcontrol
)
add_test(NAME external_memory_allocator_test COMMAND external_memory_allocator_test)


add_executable(climate_target_logic_test climate_target_logic_test.cpp)
target_compile_features(climate_target_logic_test PRIVATE cxx_std_17)
target_compile_options(climate_target_logic_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(climate_target_logic_test PRIVATE ../../components/espcontrol)
add_test(NAME climate_target_logic_test COMMAND climate_target_logic_test)

add_executable(climate_subscription_policy_test climate_subscription_policy_test.cpp)
target_compile_features(climate_subscription_policy_test PRIVATE cxx_std_17)
target_compile_options(climate_subscription_policy_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(climate_subscription_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME climate_subscription_policy_test COMMAND climate_subscription_policy_test)

add_executable(climate_state_logic_test climate_state_logic_test.cpp)
target_compile_features(climate_state_logic_test PRIVATE cxx_std_17)
target_compile_options(climate_state_logic_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(climate_state_logic_test PRIVATE ../../components/espcontrol)
add_test(NAME climate_state_logic_test COMMAND climate_state_logic_test)

add_executable(media_volume_capability_test media_volume_capability_test.cpp)
target_compile_features(media_volume_capability_test PRIVATE cxx_std_17)
target_compile_options(media_volume_capability_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_volume_capability_test PRIVATE ../../components/espcontrol)
add_test(NAME media_volume_capability_test COMMAND media_volume_capability_test)

add_test(NAME media_power_override_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/media_power_override_test.py)
add_test(NAME media_playback_pool_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/media_playback_pool_test.py)
add_test(NAME weather_forecast_queue_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/weather_forecast_queue_test.py)

add_executable(media_control_tabs_test media_control_tabs_test.cpp)
target_compile_features(media_control_tabs_test PRIVATE cxx_std_17)
target_compile_options(media_control_tabs_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_control_tabs_test PRIVATE ../../components/espcontrol)
add_test(NAME media_control_tabs_test COMMAND media_control_tabs_test)
add_test(NAME media_control_tabs_runtime_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/media_control_tabs_runtime_test.py)
add_test(NAME media_speaker_volume_event_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/media_speaker_volume_event_test.py)

add_executable(media_power_capability_test media_power_capability_test.cpp)
target_compile_features(media_power_capability_test PRIVATE cxx_std_17)
target_compile_options(media_power_capability_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_power_capability_test PRIVATE ../../components/espcontrol)
add_test(NAME media_power_capability_test COMMAND media_power_capability_test)

add_executable(media_playback_modes_test media_playback_modes_test.cpp)
target_compile_features(media_playback_modes_test PRIVATE cxx_std_17)
target_compile_options(media_playback_modes_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_playback_modes_test PRIVATE ../../components/espcontrol)
add_test(NAME media_playback_modes_test COMMAND media_playback_modes_test)

add_executable(media_metadata_policy_test media_metadata_policy_test.cpp)
target_compile_features(media_metadata_policy_test PRIVATE cxx_std_17)
target_compile_options(media_metadata_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_metadata_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME media_metadata_policy_test COMMAND media_metadata_policy_test)

set(MEDIA_LIFECYCLE_PRODUCTION ${CMAKE_CURRENT_BINARY_DIR}/media_lifecycle_production.h)
add_custom_command(
  OUTPUT ${MEDIA_LIFECYCLE_PRODUCTION}
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_media_lifecycle_integration.py
    --root ${CMAKE_CURRENT_SOURCE_DIR}/../..
    --output ${MEDIA_LIFECYCLE_PRODUCTION}
  DEPENDS
    generate_media_lifecycle_integration.py
    ../../components/espcontrol/button_grid_sliders.h
    ../../components/espcontrol/button_grid_media.h
    ../../components/espcontrol/button_grid_grid.h
    ../../components/espcontrol/button_grid_media_driver.h
  VERBATIM
)
add_executable(media_lifecycle_integration_test
  media_lifecycle_integration_test.cpp ${MEDIA_LIFECYCLE_PRODUCTION})
target_compile_features(media_lifecycle_integration_test PRIVATE cxx_std_17)
target_compile_options(media_lifecycle_integration_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_lifecycle_integration_test PRIVATE
  ${CMAKE_CURRENT_BINARY_DIR} ../../components/espcontrol)
add_test(NAME media_lifecycle_integration_test COMMAND media_lifecycle_integration_test)

add_executable(media_slider_lifecycle_test media_slider_lifecycle_test.cpp)
target_compile_features(media_slider_lifecycle_test PRIVATE cxx_std_17)
target_compile_options(media_slider_lifecycle_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_slider_lifecycle_test PRIVATE ../../components/espcontrol)
set(ESPCONTROL_REQUIRED_FLAGS_SAVED "${CMAKE_REQUIRED_FLAGS}")
set(ESPCONTROL_REQUIRED_LINK_OPTIONS_SAVED "${CMAKE_REQUIRED_LINK_OPTIONS}")
set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
set(CMAKE_REQUIRED_LINK_OPTIONS "${CMAKE_REQUIRED_LINK_OPTIONS};-fsanitize=address")
check_cxx_source_compiles("int main() { return 0; }" ESPCONTROL_HAS_ADDRESS_SANITIZER)
set(CMAKE_REQUIRED_FLAGS "${ESPCONTROL_REQUIRED_FLAGS_SAVED}")
set(CMAKE_REQUIRED_LINK_OPTIONS "${ESPCONTROL_REQUIRED_LINK_OPTIONS_SAVED}")
if(ESPCONTROL_HAS_ADDRESS_SANITIZER)
  foreach(target media_slider_lifecycle_test media_lifecycle_integration_test)
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${target} PRIVATE -fsanitize=address)
  endforeach()
endif()
add_test(NAME media_slider_lifecycle_test COMMAND media_slider_lifecycle_test)

add_executable(slider_geometry_test slider_geometry_test.cpp)
target_compile_features(slider_geometry_test PRIVATE cxx_std_17)
target_compile_options(slider_geometry_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(slider_geometry_test PRIVATE ../../components/espcontrol)
add_test(NAME slider_geometry_test COMMAND slider_geometry_test)

add_executable(number_slider_policy_test number_slider_policy_test.cpp)
target_compile_features(number_slider_policy_test PRIVATE cxx_std_17)
target_compile_options(number_slider_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(number_slider_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME number_slider_policy_test COMMAND number_slider_policy_test)

add_executable(display_mode_controller_test display_mode_controller_test.cpp)
target_compile_features(display_mode_controller_test PRIVATE cxx_std_17)
target_compile_options(display_mode_controller_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(display_mode_controller_test PRIVATE ../../components/espcontrol)
add_test(NAME display_mode_controller_test COMMAND display_mode_controller_test)

add_executable(backlight_fade_test backlight_fade_test.cpp)
target_compile_features(backlight_fade_test PRIVATE cxx_std_17)
target_compile_options(backlight_fade_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(backlight_fade_test PRIVATE ../../components/espcontrol)
add_test(NAME backlight_fade_test COMMAND backlight_fade_test)

add_executable(network_status_layout_test network_status_layout_test.cpp)
target_compile_features(network_status_layout_test PRIVATE cxx_std_17)
target_compile_options(network_status_layout_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME network_status_layout_test COMMAND network_status_layout_test)

add_executable(configuration_store_test
  configuration_store_test.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(configuration_store_test PRIVATE cxx_std_17)
target_compile_options(configuration_store_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(configuration_store_test PRIVATE ../../components/espcontrol)
add_test(NAME configuration_store_test COMMAND configuration_store_test)

add_executable(configuration_service_test
  configuration_service_test.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(configuration_service_test PRIVATE cxx_std_17)
target_compile_options(configuration_service_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(configuration_service_test PRIVATE ../../components/espcontrol)
add_test(NAME configuration_service_test COMMAND configuration_service_test)

add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/application_scene_runtime_test.cpp
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_application_scene_runtime_test.py
    --output ${CMAKE_CURRENT_BINARY_DIR}/application_scene_runtime_test.cpp
  DEPENDS generate_application_scene_runtime_test.py application_scene_runtime_test.cpp.in
    ../../components/espcontrol/espcontrol_app.cpp VERBATIM)
add_executable(application_scene_runtime_test ${CMAKE_CURRENT_BINARY_DIR}/application_scene_runtime_test.cpp
  ../../components/espcontrol/configuration_main_loop.cpp
  ../../components/espcontrol/configuration_service.cpp ../../components/espcontrol/configuration_store.cpp)
target_compile_features(application_scene_runtime_test PRIVATE cxx_std_17)
target_compile_options(application_scene_runtime_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(application_scene_runtime_test PRIVATE ../../components/espcontrol)
target_link_libraries(application_scene_runtime_test PRIVATE Threads::Threads)
add_test(NAME application_scene_runtime_test COMMAND application_scene_runtime_test)

add_executable(configuration_main_loop_test configuration_main_loop_test.cpp
  ../../components/espcontrol/configuration_main_loop.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp)
target_compile_features(configuration_main_loop_test PRIVATE cxx_std_17)
target_compile_options(configuration_main_loop_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(configuration_main_loop_test PRIVATE ../../components/espcontrol)
target_link_libraries(configuration_main_loop_test PRIVATE Threads::Threads)
add_test(NAME configuration_main_loop_test COMMAND configuration_main_loop_test)

add_executable(panel_config_document_test panel_config_document_test.cpp)
target_compile_features(panel_config_document_test PRIVATE cxx_std_17)
target_compile_options(panel_config_document_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_document_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_document_test COMMAND panel_config_document_test)

add_executable(panel_config_capabilities_test
  panel_config_capabilities_test.cpp
)
target_compile_features(panel_config_capabilities_test PRIVATE cxx_std_17)
target_compile_options(panel_config_capabilities_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_capabilities_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_capabilities_test COMMAND panel_config_capabilities_test)

add_executable(panel_config_http_etag_test panel_config_http_etag_test.cpp)
target_compile_features(panel_config_http_etag_test PRIVATE cxx_std_17)
target_compile_options(panel_config_http_etag_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_http_etag_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_http_etag_test COMMAND panel_config_http_etag_test)

add_executable(panel_config_write_status_test panel_config_write_status_test.cpp)
target_compile_features(panel_config_write_status_test PRIVATE cxx_std_17)
target_compile_options(panel_config_write_status_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_write_status_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_write_status_test COMMAND panel_config_write_status_test)

add_executable(panel_config_storage_backend_test
  panel_config_storage_backend_test.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(panel_config_storage_backend_test PRIVATE cxx_std_17)
target_compile_options(panel_config_storage_backend_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_storage_backend_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_storage_backend_test COMMAND panel_config_storage_backend_test)

add_executable(panel_config_storage_selection_test
  panel_config_storage_selection_test.cpp)
target_compile_features(panel_config_storage_selection_test PRIVATE cxx_std_17)
target_compile_options(panel_config_storage_selection_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_storage_selection_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_storage_selection_test COMMAND panel_config_storage_selection_test)

add_executable(panel_config_endpoint_policy_test
  panel_config_endpoint_policy_test.cpp)
target_compile_features(panel_config_endpoint_policy_test PRIVATE cxx_std_17)
target_compile_options(panel_config_endpoint_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_endpoint_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_endpoint_policy_test COMMAND panel_config_endpoint_policy_test)

add_executable(panel_config_legacy_adapter_test
  panel_config_legacy_adapter_test.cpp
  ../../components/espcontrol/panel_config_legacy_adapter.cpp
  ../../components/espcontrol/panel_config_runtime_adapter.cpp
  ../../components/espcontrol/panel_config_text_bindings.cpp
)
target_compile_features(panel_config_legacy_adapter_test PRIVATE cxx_std_17)
target_compile_options(panel_config_legacy_adapter_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_legacy_adapter_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_legacy_adapter_test COMMAND panel_config_legacy_adapter_test)

add_executable(espcontrol_app_core_test
  espcontrol_app_core_test.cpp
  ../../components/espcontrol/espcontrol_app_core.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(espcontrol_app_core_test PRIVATE cxx_std_17)
target_compile_options(espcontrol_app_core_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(espcontrol_app_core_test PRIVATE ../../components/espcontrol)
add_test(NAME espcontrol_app_core_test COMMAND espcontrol_app_core_test)

add_executable(ha_read_coordinator_test
  ha_read_coordinator_test.cpp
  ../../components/espcontrol/espcontrol_app_core.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(ha_read_coordinator_test PRIVATE cxx_std_17)
target_compile_options(ha_read_coordinator_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(ha_read_coordinator_test PRIVATE ../../components/espcontrol)
add_test(NAME ha_read_coordinator_test COMMAND ha_read_coordinator_test)

add_executable(owned_ha_subscriptions_test owned_ha_subscriptions_test.cpp)
target_compile_features(owned_ha_subscriptions_test PRIVATE cxx_std_17)
target_compile_options(owned_ha_subscriptions_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(owned_ha_subscriptions_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME owned_ha_subscriptions_test COMMAND owned_ha_subscriptions_test)

add_executable(home_assistant_transport_runtime_test home_assistant_transport_runtime_test.cpp
  ../../components/espcontrol/home_assistant_transport_runtime.cpp)
target_compile_features(home_assistant_transport_runtime_test PRIVATE cxx_std_17)
target_compile_definitions(home_assistant_transport_runtime_test PRIVATE USE_API_HOMEASSISTANT_STATES)
target_compile_options(home_assistant_transport_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(home_assistant_transport_runtime_test PRIVATE owned_ha_stubs stubs ../../components/espcontrol)
add_test(NAME home_assistant_transport_runtime_test COMMAND home_assistant_transport_runtime_test)

add_executable(artwork_controller_test artwork_controller_test.cpp)
target_compile_features(artwork_controller_test PRIVATE cxx_std_17)
target_compile_options(artwork_controller_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(artwork_controller_test PRIVATE ../../components/espcontrol)
add_test(NAME artwork_controller_test COMMAND artwork_controller_test)

add_executable(home_assistant_endpoint_policy_test home_assistant_endpoint_policy_test.cpp)
target_compile_features(home_assistant_endpoint_policy_test PRIVATE cxx_std_17)
target_compile_options(home_assistant_endpoint_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(home_assistant_endpoint_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME home_assistant_endpoint_policy_test COMMAND home_assistant_endpoint_policy_test)

# Compile the unchanged service implementation beside the build output so its
# quoted artwork_image.h include resolves to the hardware-free test double.
configure_file(../../components/artwork_image/image_service.cpp image_service.cpp COPYONLY)
add_executable(page_artwork_requests_test page_artwork_requests_test.cpp)
target_compile_definitions(page_artwork_requests_test PRIVATE USE_LVGL)
target_compile_features(page_artwork_requests_test PRIVATE cxx_std_17)
target_compile_options(page_artwork_requests_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_artwork_requests_test PRIVATE stubs/owned_artwork stubs
  ../../components/espcontrol ../../components/artwork_image)
add_test(NAME page_artwork_requests_test COMMAND page_artwork_requests_test)

add_executable(page_cover_art_runtime_test page_cover_art_runtime_test.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_media_state_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_cover_art_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_cover_art_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_cover_art_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_cover_art_runtime_test COMMAND page_cover_art_runtime_test)

add_executable(page_modal_image_runtime_test page_modal_image_runtime_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_modal_image_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_modal_image_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_modal_image_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_modal_image_runtime_test COMMAND page_modal_image_runtime_test)

add_executable(page_image_tile_runtime_test page_image_tile_runtime_test.cpp
  ../../components/espcontrol/page_modal_runtime.cpp
  ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
  ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_store_runtime.cpp
  ../../components/espcontrol/page_renderer_layout.cpp)
target_compile_features(page_image_tile_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_image_tile_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_image_tile_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_image_tile_runtime_test COMMAND page_image_tile_runtime_test)


add_executable(page_cover_art_policy_test page_cover_art_policy_test.cpp)
target_compile_features(page_cover_art_policy_test PRIVATE cxx_std_17)
target_compile_options(page_cover_art_policy_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_cover_art_policy_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_cover_art_policy_test COMMAND page_cover_art_policy_test)

add_executable(page_image_tile_policy_test page_image_tile_policy_test.cpp)
target_compile_features(page_image_tile_policy_test PRIVATE cxx_std_17)
target_compile_options(page_image_tile_policy_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(page_image_tile_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME page_image_tile_policy_test COMMAND page_image_tile_policy_test)

add_executable(page_image_runtime_test page_image_runtime_test.cpp)
target_compile_features(page_image_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_image_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_image_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_image_runtime_test COMMAND page_image_runtime_test)
add_test(NAME artwork_owned_result_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/artwork_owned_result_test.py ${CMAKE_CXX_COMPILER})

add_executable(image_service_test image_service_test.cpp ${CMAKE_CURRENT_BINARY_DIR}/image_service.cpp)
target_compile_features(image_service_test PRIVATE cxx_std_17)
target_compile_options(image_service_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(image_service_test PRIVATE stubs/artwork_image ../../components/artwork_image)
add_test(NAME image_service_test COMMAND image_service_test)

add_executable(bmp_stream_test bmp_stream_test.cpp)
target_compile_features(bmp_stream_test PRIVATE cxx_std_17)
target_compile_options(bmp_stream_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(bmp_stream_test PRIVATE ../../components/artwork_image)
add_test(NAME bmp_stream_test COMMAND bmp_stream_test)

add_executable(gif_player_test gif_player_test.cpp)
target_compile_features(gif_player_test PRIVATE cxx_std_17)
target_compile_options(gif_player_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(gif_player_test PRIVATE ../../components/artwork_image)
target_compile_definitions(gif_player_test PRIVATE GIF_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures/gif")
add_test(NAME gif_player_test COMMAND gif_player_test)
add_test(NAME gif_runtime_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/gif_runtime_test.py)
add_test(NAME p4_gif_transfer_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/p4_gif_transfer_test.py)

add_executable(p4_scaling_test p4_scaling_test.cpp)
target_compile_features(p4_scaling_test PRIVATE cxx_std_17)
target_compile_options(p4_scaling_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(p4_scaling_test PRIVATE ../../components/artwork_image)
add_test(NAME p4_scaling_test COMMAND p4_scaling_test)

add_executable(image_pipeline_policy_test image_pipeline_policy_test.cpp)
target_compile_features(image_pipeline_policy_test PRIVATE cxx_std_17)
target_compile_options(image_pipeline_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(image_pipeline_policy_test PRIVATE ../../components/artwork_image)
add_test(NAME image_pipeline_policy_test COMMAND image_pipeline_policy_test)

add_test(NAME climate_pending_temperature_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/climate_pending_temperature_test.py)

set(CLIMATE_SUBSCRIPTION_TEST_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/climate_subscription_runtime_test.cpp)
add_custom_command(
  OUTPUT ${CLIMATE_SUBSCRIPTION_TEST_SOURCE}
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/generate_climate_subscription_runtime_test.py
    --output ${CLIMATE_SUBSCRIPTION_TEST_SOURCE}
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/generate_climate_subscription_runtime_test.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_climate.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_climate_subscriptions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/climate_subscription_runtime_test.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/fixtures/issue_1837_climate_subpage.json
  VERBATIM
)
add_executable(climate_subscription_runtime_test ${CLIMATE_SUBSCRIPTION_TEST_SOURCE})
target_compile_features(climate_subscription_runtime_test PRIVATE cxx_std_17)
target_compile_options(climate_subscription_runtime_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(climate_subscription_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME climate_subscription_runtime_test COMMAND climate_subscription_runtime_test)

add_test(NAME display_schedule_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/display_schedule_test.py
  --compiler ${CMAKE_CXX_COMPILER})
add_test(NAME cover_art_activation_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cover_art_activation_test.py
  --compiler ${CMAKE_CXX_COMPILER})
add_test(NAME cover_art_rotation_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cover_art_rotation_test.py
  --compiler ${CMAKE_CXX_COMPILER})
add_test(NAME presence_transition_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/presence_transition_test.py)
add_test(NAME ha_reconnect_recovery_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/ha_reconnect_recovery_test.py)
add_test(NAME cover_art_metadata_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cover_art_metadata_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME cover_art_pending_reads_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cover_art_pending_reads_test.py ${CMAKE_CXX_COMPILER})
add_executable(rgb565_scaler_test rgb565_scaler_test.cpp)
target_compile_features(rgb565_scaler_test PRIVATE cxx_std_17)
target_compile_options(rgb565_scaler_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(rgb565_scaler_test PRIVATE ../../components/artwork_image)
add_test(NAME rgb565_scaler_test COMMAND rgb565_scaler_test)

add_executable(scanline_resampler_test scanline_resampler_test.cpp)
target_compile_features(scanline_resampler_test PRIVATE cxx_std_17)
target_compile_options(scanline_resampler_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(scanline_resampler_test PRIVATE ../../components/artwork_image)
add_test(NAME scanline_resampler_test COMMAND scanline_resampler_test)
add_test(NAME image_resize_integration_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/image_resize_integration_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME camera_expanded_image_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/camera_expanded_image_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME p4_jpeg_memory_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/p4_jpeg_memory_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME artwork_release_memory_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/artwork_release_memory_test.py ${CMAKE_CXX_COMPILER})

file(GLOB SAVED_CONFIG_FIXTURES
  CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../common/config/*_card_normalization_fixtures.json
)
list(APPEND SAVED_CONFIG_FIXTURES
  ${CMAKE_CURRENT_SOURCE_DIR}/../../common/config/card_normalization_fixtures.json
)
set(SAVED_CONFIG_TEST_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/saved_config_parser_test.cpp)
add_custom_command(
  OUTPUT ${SAVED_CONFIG_TEST_SOURCE}
  COMMAND
    ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/generate_saved_config_parser_test.py
    --output ${SAVED_CONFIG_TEST_SOURCE}
  DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/generate_saved_config_parser_test.py
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_config_parser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_saved_config_vacuum_generated.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_subpages.h
    ${SAVED_CONFIG_FIXTURES}
  VERBATIM
)
add_executable(saved_config_parser_test ${SAVED_CONFIG_TEST_SOURCE})
target_compile_features(saved_config_parser_test PRIVATE cxx_std_17)
target_compile_options(saved_config_parser_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(saved_config_parser_test PRIVATE
  stubs
  ../../components/espcontrol
)
add_test(NAME saved_config_parser_test COMMAND saved_config_parser_test)

add_test(NAME climate_option_values_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/climate_option_values_test.py)

set(CLIMATE_UPDATE_TEST_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/climate_update_test.cpp)
add_custom_command(
  OUTPUT ${CLIMATE_UPDATE_TEST_SOURCE}
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_climate_update_test.py
    --header ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/button_grid_climate.h
    --output ${CLIMATE_UPDATE_TEST_SOURCE}
  DEPENDS
    generate_climate_update_test.py
    climate_update_test.cpp.in
    ../../components/espcontrol/button_grid_climate.h
  VERBATIM
)
add_executable(climate_update_test ${CLIMATE_UPDATE_TEST_SOURCE})
target_compile_features(climate_update_test PRIVATE cxx_std_17)
target_compile_options(climate_update_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(climate_update_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME climate_update_test COMMAND climate_update_test)

add_executable(camera_refresh_policy_test camera_refresh_policy_test.cpp)
target_compile_features(camera_refresh_policy_test PRIVATE cxx_std_17)
target_compile_options(camera_refresh_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(camera_refresh_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME camera_refresh_policy_test COMMAND camera_refresh_policy_test)
add_test(NAME camera_refresh_runtime_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/camera_refresh_runtime_test.py)
add_executable(reset_policy_test reset_policy_test.cpp)
find_package(Threads REQUIRED)
target_link_libraries(reset_policy_test PRIVATE Threads::Threads)
target_compile_features(reset_policy_test PRIVATE cxx_std_17)
target_compile_options(reset_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(reset_policy_test PRIVATE ../../components/espcontrol)
add_test(NAME reset_policy_test COMMAND reset_policy_test)
add_test(NAME reset_ota_wrapper_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/reset_ota_wrapper_test.py)
add_test(NAME wifi_reset_override_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/wifi_reset_override_test.py)
add_test(NAME wifi_reset_boot_codegen_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/wifi_reset_boot_codegen_test.py)
add_test(NAME wifi_setup_package_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/wifi_setup_package_test.py)
add_executable(panel_identity_test panel_identity_test.cpp)
target_compile_features(panel_identity_test PRIVATE cxx_std_17)
target_compile_options(panel_identity_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_identity_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_identity_test COMMAND panel_identity_test)

add_executable(panel_identity_runtime_test panel_identity_runtime_test.cpp ../../components/espcontrol/panel_identity.cpp)
target_compile_features(panel_identity_runtime_test PRIVATE cxx_std_17)
target_compile_options(panel_identity_runtime_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_identity_runtime_test PRIVATE panel_identity_stubs ../../components/espcontrol)
add_test(NAME panel_identity_runtime_test COMMAND panel_identity_runtime_test)

add_executable(panel_identity_runtime_beta_test panel_identity_runtime_test.cpp ../../components/espcontrol/panel_identity.cpp)
target_compile_features(panel_identity_runtime_beta_test PRIVATE cxx_std_17)
target_compile_options(panel_identity_runtime_beta_test PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(panel_identity_runtime_beta_test PRIVATE TEST_ESPHOME_MINOR=10)
target_include_directories(panel_identity_runtime_beta_test PRIVATE panel_identity_stubs ../../components/espcontrol)
add_test(NAME panel_identity_runtime_beta_test COMMAND panel_identity_runtime_beta_test)

set(CLOCK_BAR_TITLE_TEST_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/clock_bar_title_test.cpp)
add_custom_command(
  OUTPUT ${CLOCK_BAR_TITLE_TEST_SOURCE}
  COMMAND ${Python3_EXECUTABLE}
    ${CMAKE_CURRENT_SOURCE_DIR}/generate_clock_bar_title_test.py
    --header ${CMAKE_CURRENT_SOURCE_DIR}/../../components/espcontrol/clock_bar.h
    --output ${CLOCK_BAR_TITLE_TEST_SOURCE}
  DEPENDS
    generate_clock_bar_title_test.py
    clock_bar_title_test.cpp.in
    ../../components/espcontrol/clock_bar.h
    ../../components/espcontrol/button_grid_modal.h
  VERBATIM
)
add_executable(clock_bar_title_test ${CLOCK_BAR_TITLE_TEST_SOURCE})
target_compile_features(clock_bar_title_test PRIVATE cxx_std_17)
target_compile_options(clock_bar_title_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME clock_bar_title_test COMMAND clock_bar_title_test)

add_executable(timer_card_test timer_card_test.cpp)
target_compile_features(timer_card_test PRIVATE cxx_std_17)
target_compile_options(timer_card_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(timer_card_test PRIVATE ../../components/espcontrol)
add_test(NAME timer_card_test COMMAND timer_card_test)

add_test(NAME timer_subpage_cleanup_test
  COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/timer_subpage_cleanup_test.py)

add_test(NAME cover_confirmation_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cover_confirmation_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME cover_stop_failure_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/cover_stop_failure_test.py ${CMAKE_CXX_COMPILER})

add_test(NAME camera_feedback_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/camera_feedback_test.py ${CMAKE_CXX_COMPILER})

add_test(NAME remote_modal_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/remote_modal_test.py)

add_test(NAME fan_control_label_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/fan_control_label_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME robot_card_icons_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/robot_card_icons_test.py ${CMAKE_CXX_COMPILER})

add_test(NAME clock_image_overlay_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/clock_image_overlay_test.py)

add_executable(home_assistant_endpoint_resolver_test
  home_assistant_endpoint_resolver_test.cpp
  ../../components/espcontrol/home_assistant_endpoint_resolver.cpp
  ../../components/espcontrol/home_assistant_endpoint_probe.cpp)
target_compile_features(home_assistant_endpoint_resolver_test PRIVATE cxx_std_17)
target_compile_options(home_assistant_endpoint_resolver_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(home_assistant_endpoint_resolver_test PRIVATE endpoint_stubs ../../components/espcontrol)
add_test(NAME home_assistant_endpoint_resolver_test COMMAND home_assistant_endpoint_resolver_test)

add_executable(home_assistant_endpoint_probe_test
  home_assistant_endpoint_probe_test.cpp ../../components/espcontrol/home_assistant_endpoint_probe.cpp)
target_compile_features(home_assistant_endpoint_probe_test PRIVATE cxx_std_17)
target_compile_options(home_assistant_endpoint_probe_test PRIVATE -Wall -Wextra -Werror)
target_compile_definitions(home_assistant_endpoint_probe_test PRIVATE USE_ESP_IDF)
target_include_directories(home_assistant_endpoint_probe_test PRIVATE endpoint_idf_stubs endpoint_stubs ../../components/espcontrol)
add_test(NAME home_assistant_endpoint_probe_test COMMAND home_assistant_endpoint_probe_test)

add_test(NAME clock_screensaver_time_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/clock_screensaver_time_test.py)

add_executable(screensaver_pin_test screensaver_pin_test.cpp
  ../../components/espcontrol/screensaver_pin.cpp)
target_compile_features(screensaver_pin_test PRIVATE cxx_std_17)
target_compile_options(screensaver_pin_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(screensaver_pin_test PRIVATE screensaver_pin_stubs ../../components/espcontrol)
target_link_libraries(screensaver_pin_test PRIVATE Threads::Threads)
add_test(NAME screensaver_pin_test COMMAND screensaver_pin_test)
add_test(NAME screensaver_pin_ui_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/screensaver_pin_ui_test.py ${CMAKE_CXX_COMPILER})
add_test(NAME screensaver_pin_cover_art_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/screensaver_pin_cover_art_test.py ${CMAKE_CXX_COMPILER})

add_executable(weather_daily_forecast_test weather_daily_forecast_test.cpp)
target_compile_features(weather_daily_forecast_test PRIVATE cxx_std_17)
target_compile_options(weather_daily_forecast_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(weather_daily_forecast_test PRIVATE ../../components/espcontrol)
add_test(NAME weather_daily_forecast_test COMMAND weather_daily_forecast_test)

add_test(NAME weather_daily_template_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/weather_daily_template_test.py)

add_executable(weather_daily_ui_test weather_daily_ui_test.cpp)
target_compile_features(weather_daily_ui_test PRIVATE cxx_std_17)
target_compile_options(weather_daily_ui_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(weather_daily_ui_test PRIVATE ../../components/espcontrol)
add_test(NAME weather_daily_ui_test COMMAND weather_daily_ui_test)

add_test(NAME weather_daily_requests_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/weather_daily_requests_test.py)

add_test(NAME camera_screensaver_dimming_test COMMAND
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/camera_screensaver_dimming_test.py)

add_test(NAME discovery_shutdown_order_test COMMAND ${Python3_EXECUTABLE}
  ${CMAKE_CURRENT_SOURCE_DIR}/discovery_shutdown_order_test.py)

add_executable(page_forecast_runtime_test page_forecast_runtime_test.cpp
  ../../components/espcontrol/page_forecast_runtime.cpp ../../components/espcontrol/page_store.cpp)
target_compile_features(page_forecast_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_forecast_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_forecast_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_forecast_runtime_test COMMAND page_forecast_runtime_test)


foreach(FORECAST_TRANSPORT_TEST page_ha_forecast_requests_test page_ha_forecast_requests_disabled_test)
  add_executable(${FORECAST_TRANSPORT_TEST} page_ha_forecast_requests_test.cpp)
  target_compile_features(${FORECAST_TRANSPORT_TEST} PRIVATE cxx_std_17)
  target_compile_options(${FORECAST_TRANSPORT_TEST} PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(${FORECAST_TRANSPORT_TEST} PRIVATE ../../components/espcontrol)
  add_test(NAME ${FORECAST_TRANSPORT_TEST} COMMAND ${FORECAST_TRANSPORT_TEST})
endforeach()
target_compile_definitions(page_ha_forecast_requests_disabled_test PRIVATE ESPCONTROL_DISABLE_WEATHER_FORECAST=1)

add_test(NAME media_playback_clock_parity_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/media_playback_clock_parity_test.py)

add_executable(media_playback_clock_test media_playback_clock_test.cpp)
target_compile_features(media_playback_clock_test PRIVATE cxx_std_17)
target_compile_options(media_playback_clock_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
target_include_directories(media_playback_clock_test PRIVATE ../../components/espcontrol)
add_test(NAME media_playback_clock_test COMMAND media_playback_clock_test)

add_executable(media_input_state_test media_input_state_test.cpp)
target_compile_features(media_input_state_test PRIVATE cxx_std_17)
target_compile_options(media_input_state_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(media_input_state_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME media_input_state_test COMMAND media_input_state_test)

add_executable(page_media_state_runtime_test page_media_state_runtime_test.cpp
  ../../components/espcontrol/page_media_state_runtime.cpp ../../components/espcontrol/page_store.cpp
  ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp)
target_compile_features(page_media_state_runtime_test PRIVATE cxx_std_17)
target_compile_options(page_media_state_runtime_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
target_include_directories(page_media_state_runtime_test PRIVATE stubs ../../components/espcontrol)
add_test(NAME page_media_state_runtime_test COMMAND page_media_state_runtime_test)

add_test(NAME media_playlist_match_test COMMAND
  ${CMAKE_COMMAND} -E env "CXX=${CMAKE_CXX_COMPILER}"
  ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/media_playlist_match_test.py)

# Optional real-LVGL integration, using the exact library from an ESPHome build.
# No network/download or machine-specific path is required by the normal suite.
set(ESPCONTROL_LVGL_SOURCE_DIR "" CACHE PATH "LVGL 9.5 source from pinned firmware build")
if(ESPCONTROL_LVGL_SOURCE_DIR)
  enable_language(C)
  set(LV_BUILD_CONF_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lvgl_native/lv_conf.h" CACHE PATH "" FORCE)
  set(CONFIG_LV_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(CONFIG_LV_BUILD_DEMOS OFF CACHE BOOL "" FORCE)
  set(CONFIG_LV_USE_THORVG_INTERNAL OFF CACHE BOOL "" FORCE)
  add_subdirectory("${ESPCONTROL_LVGL_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/lvgl")
  if(APPLE)
    # Darwin ld has no GNU --wrap. Route only the pinned label implementation;
    # the original display implementation remains available to the test shim.
    set_source_files_properties("${ESPCONTROL_LVGL_SOURCE_DIR}/src/widgets/label/lv_label.c"
      DIRECTORY "${ESPCONTROL_LVGL_SOURCE_DIR}"
      PROPERTIES COMPILE_DEFINITIONS
      "lv_display_add_event_cb=__wrap_lv_display_add_event_cb;lv_display_remove_event_cb_with_user_data=__wrap_lv_display_remove_event_cb_with_user_data")
  endif()
  add_library(page_label_refresh_test_support OBJECT ../../components/espcontrol/page_label_refresh.cpp)
  target_compile_features(page_label_refresh_test_support PRIVATE cxx_std_17)
  target_compile_options(page_label_refresh_test_support PRIVATE -Wall -Wextra -Werror)
  target_compile_definitions(page_label_refresh_test_support PRIVATE USE_LVGL)
  target_include_directories(page_label_refresh_test_support PRIVATE owned_ha_stubs ../../components/espcontrol)
  target_include_directories(page_label_refresh_test_support INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(page_label_refresh_test_support PUBLIC lvgl::lvgl)
  if(APPLE)
    target_compile_definitions(page_label_refresh_test_support PRIVATE ESPCONTROL_NATIVE_LABEL_REFRESH)
  else()
    target_link_options(page_label_refresh_test_support INTERFACE
      -Wl,--wrap=lv_display_add_event_cb -Wl,--wrap=lv_display_remove_event_cb_with_user_data)
  endif()
  # All optional LVGL executables share this library, including the modal test,
  # because the pinned label translation unit is routed through its wrappers.
  link_libraries(page_label_refresh_test_support)

  set(NATIVE_SCENE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_native_scene_test.cpp)
  set(NATIVE_SCENE_STYLE_INPUTS)
  foreach(family light_cover fan_control option_select media_volume media_control weather
      confirmation alarm expanded_image wifi_qr network_status)
    list(APPEND NATIVE_SCENE_STYLE_INPUTS ../../components/espcontrol/page_${family}_modal.h)
  endforeach()
  add_custom_command(OUTPUT ${NATIVE_SCENE_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_native_scene_test.py
      --output ${NATIVE_SCENE_SOURCE}
    DEPENDS generate_native_scene_test.py page_native_scene_test.cpp.in
      ../../components/espcontrol/page_native_scene.h
      ../../components/espcontrol/page_native_device_styles.h
      ../../components/espcontrol/button_grid_display.h
      ../../components/espcontrol/button_grid_grid.h
      ../../components/espcontrol/button_grid_layout.h
      ../../components/espcontrol/button_grid_style.h
      ../../components/espcontrol/button_grid_climate_control_driver.h
      ../../components/espcontrol/icons.h
      generate_media_lifecycle_integration.py ${NATIVE_SCENE_STYLE_INPUTS}
    VERBATIM)
  add_executable(page_native_scene_test ${NATIVE_SCENE_SOURCE}
    ${ESPCONTROL_LVGL_SOURCE_DIR}/src/libs/qrcode/qrcodegen.c
    ../../components/espcontrol/page_store.cpp ../../components/espcontrol/page_store_runtime.cpp
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_basic_card.cpp ../../components/espcontrol/page_renderer_layout.cpp
    ../../components/espcontrol/page_climate_card.cpp
    ../../components/espcontrol/page_media_state_runtime.cpp ../../components/espcontrol/page_forecast_runtime.cpp
    ../../components/espcontrol/page_local_sensor_runtime.cpp ../../components/espcontrol/page_local_relay_runtime.cpp
    ../../components/espcontrol/page_fan_status_runtime.cpp ../../components/espcontrol/page_percent_slider_runtime.cpp
    ../../components/espcontrol/page_number_slider_runtime.cpp ../../components/espcontrol/page_light_temperature_runtime.cpp
    ../../components/espcontrol/page_timer_runtime.cpp ../../components/espcontrol/page_cover_toggle_runtime.cpp
    ../../components/espcontrol/page_access_state_runtime.cpp ../../components/espcontrol/page_calendar_runtime.cpp
    ../../components/espcontrol/page_timezone_runtime.cpp ../../components/espcontrol/page_alarm_state_runtime.cpp
    ../../components/espcontrol/page_alarm_ack_runtime.cpp ../../components/espcontrol/page_alarm_commands.cpp
    ../../components/espcontrol/page_alarm_presentation.cpp)
  target_compile_features(page_native_scene_test PRIVATE cxx_std_17)
  target_compile_options(page_native_scene_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG
    $<$<COMPILE_LANGUAGE:C>:-Wno-unused-function>)
  target_compile_definitions(page_native_scene_test PRIVATE USE_LVGL LV_USE_QRCODE=1 ESPCONTROL_REAL_LVGL_TEST)
  target_include_directories(page_native_scene_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}
    stubs/owned_artwork stubs ../../components/espcontrol ../../components/artwork_image)
  target_link_libraries(page_native_scene_test PRIVATE lvgl::lvgl)
  add_test(NAME page_native_scene_test COMMAND page_native_scene_test)
  set_tests_properties(page_native_scene_test PROPERTIES TIMEOUT 180)

  add_executable(page_native_card_commands_test page_native_card_commands_test.cpp)
  target_compile_features(page_native_card_commands_test PRIVATE cxx_std_17)
  target_compile_options(page_native_card_commands_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  target_include_directories(page_native_card_commands_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_native_card_commands_test PRIVATE lvgl::lvgl)
  add_test(NAME page_native_card_commands_test COMMAND page_native_card_commands_test)

  set(NATIVE_MODAL_COMMANDS_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_native_modal_commands_test.cpp)
  add_custom_command(OUTPUT ${NATIVE_MODAL_COMMANDS_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_native_modal_commands_test.py
      --output ${NATIVE_MODAL_COMMANDS_SOURCE}
    DEPENDS generate_native_modal_commands_test.py generate_climate_update_test.py
      page_native_modal_commands_test.cpp.in ../../components/espcontrol/button_grid_actions.h
      ../../components/espcontrol/button_grid_config_parser.h
      ../../components/espcontrol/page_native_modal_bank.h
    VERBATIM)
  add_executable(page_native_modal_commands_test ${NATIVE_MODAL_COMMANDS_SOURCE}
    ../../components/espcontrol/page_alarm_commands.cpp ../../components/espcontrol/page_alarm_ack_runtime.cpp
    ../../components/espcontrol/page_alarm_state_runtime.cpp
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_store.cpp
    ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp)
  target_compile_features(page_native_modal_commands_test PRIVATE cxx_std_17)
  target_compile_options(page_native_modal_commands_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  target_include_directories(page_native_modal_commands_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_native_modal_commands_test PRIVATE lvgl::lvgl)
  add_test(NAME page_native_modal_commands_test COMMAND page_native_modal_commands_test)
  set(FORECAST_REFERENCE ${CMAKE_CURRENT_BINARY_DIR}/forecast_reference.h)
  add_custom_command(OUTPUT ${FORECAST_REFERENCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_forecast_reference.py --output ${FORECAST_REFERENCE}
    DEPENDS generate_forecast_reference.py generate_climate_update_test.py
      ../../components/espcontrol/button_grid_weather_daily_ui.h ../../components/espcontrol/button_grid_weather_daily_data.h
    VERBATIM)
  set(POSITION_REFERENCE ${CMAKE_CURRENT_BINARY_DIR}/position_reference.h)
  add_custom_command(OUTPUT ${POSITION_REFERENCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_position_reference.py --output ${POSITION_REFERENCE}
    DEPENDS generate_position_reference.py generate_climate_update_test.py ../../components/espcontrol/button_grid_sliders.h
    VERBATIM)
  set(NOW_PLAYING_REFERENCE ${CMAKE_CURRENT_BINARY_DIR}/now_playing_reference.h)
  add_custom_command(OUTPUT ${NOW_PLAYING_REFERENCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_now_playing_reference.py --output ${NOW_PLAYING_REFERENCE}
    DEPENDS generate_now_playing_reference.py generate_climate_update_test.py ../../components/espcontrol/button_grid_media.h
    VERBATIM)
  add_executable(page_basic_lvgl_port_test page_basic_lvgl_port_test.cpp ${FORECAST_REFERENCE} ${POSITION_REFERENCE} ${NOW_PLAYING_REFERENCE}
    ../../components/espcontrol/page_number_slider_runtime.cpp
    ../../components/espcontrol/page_light_temperature_runtime.cpp
    ../../components/espcontrol/page_timer_runtime.cpp
  ../../components/espcontrol/page_cover_toggle_runtime.cpp
  ../../components/espcontrol/page_access_state_runtime.cpp
  ../../components/espcontrol/page_calendar_runtime.cpp
  ../../components/espcontrol/page_timezone_runtime.cpp
  ../../components/espcontrol/page_alarm_state_runtime.cpp
    ../../components/espcontrol/page_percent_slider_runtime.cpp ../../components/espcontrol/page_fan_status_runtime.cpp ../../components/espcontrol/page_local_relay_runtime.cpp ../../components/espcontrol/page_local_sensor_runtime.cpp ../../components/espcontrol/page_basic_card.cpp ../../components/espcontrol/page_media_state_runtime.cpp
    ../../components/espcontrol/page_forecast_runtime.cpp
    ../../components/espcontrol/page_climate_card.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_renderer_layout.cpp
    ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_basic_lvgl_port_test PRIVATE cxx_std_17)
  target_compile_options(page_basic_lvgl_port_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_basic_lvgl_port_test PRIVATE stubs ../../components/espcontrol ${CMAKE_CURRENT_BINARY_DIR})
  target_link_libraries(page_basic_lvgl_port_test PRIVATE lvgl::lvgl)
  add_test(NAME page_basic_lvgl_port_test COMMAND page_basic_lvgl_port_test)
  add_executable(page_cover_art_frame_test page_cover_art_frame_test.cpp)
  target_compile_features(page_cover_art_frame_test PRIVATE cxx_std_17)
  target_compile_options(page_cover_art_frame_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  target_include_directories(page_cover_art_frame_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_cover_art_frame_test PRIVATE lvgl::lvgl)
  add_test(NAME page_cover_art_frame_test COMMAND page_cover_art_frame_test)
  add_executable(page_media_label_test page_media_label_test.cpp)
  target_compile_features(page_media_label_test PRIVATE cxx_std_17)
  target_compile_options(page_media_label_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  target_include_directories(page_media_label_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_media_label_test PRIVATE lvgl::lvgl)
  add_test(NAME page_media_label_test COMMAND page_media_label_test)
  add_executable(page_label_refresh_test page_label_refresh_test.cpp)
  target_compile_features(page_label_refresh_test PRIVATE cxx_std_17)
  target_compile_options(page_label_refresh_test PRIVATE -Wall -Wextra -Werror -UNDEBUG)
  target_include_directories(page_label_refresh_test PRIVATE ../../components/espcontrol)
  target_link_libraries(page_label_refresh_test PRIVATE lvgl::lvgl)
  add_test(NAME page_label_refresh_test COMMAND page_label_refresh_test)
  set(PAGE_QR_MATRIX_NATIVE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_qr_matrix_native_test.cpp)
  add_custom_command(OUTPUT ${PAGE_QR_MATRIX_NATIVE_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_qr_matrix_test.py --output ${PAGE_QR_MATRIX_NATIVE_SOURCE}
    DEPENDS generate_page_qr_matrix_test.py generate_climate_update_test.py page_qr_matrix_native_test.cpp.in
      ../../components/espcontrol/button_grid_wifi_qr_driver.h
    VERBATIM)
  add_executable(page_qr_matrix_native_test ${PAGE_QR_MATRIX_NATIVE_SOURCE}
    ${ESPCONTROL_LVGL_SOURCE_DIR}/src/libs/qrcode/qrcodegen.c
    ${ESPCONTROL_LVGL_SOURCE_DIR}/src/libs/qrcode/lv_qrcode.c)
  target_compile_features(page_qr_matrix_native_test PRIVATE cxx_std_17)
  target_compile_options(page_qr_matrix_native_test PRIVATE -Wall -Wextra -Werror -UNDEBUG
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
  target_compile_definitions(page_qr_matrix_native_test PRIVATE LV_USE_QRCODE=1)
  target_include_directories(page_qr_matrix_native_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_qr_matrix_native_test PRIVATE lvgl::lvgl)
  add_test(NAME page_qr_matrix_native_test COMMAND page_qr_matrix_native_test)
  set_tests_properties(page_qr_matrix_native_test PROPERTIES TIMEOUT 180)

  set(PAGE_WIFI_QR_MODAL_NATIVE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_wifi_qr_modal_test.cpp)
  add_custom_command(OUTPUT ${PAGE_WIFI_QR_MODAL_NATIVE_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_wifi_qr_modal_test.py --output ${PAGE_WIFI_QR_MODAL_NATIVE_SOURCE}
    DEPENDS generate_page_wifi_qr_modal_test.py generate_climate_update_test.py page_wifi_qr_modal_test.cpp.in
      ../../components/espcontrol/button_grid_modal.h ../../components/espcontrol/button_grid_sliders.h
      ../../components/espcontrol/button_grid_modal_layout.h
    VERBATIM)
  add_executable(page_wifi_qr_modal_test ${PAGE_WIFI_QR_MODAL_NATIVE_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp ../../components/espcontrol/page_store.cpp
    ${ESPCONTROL_LVGL_SOURCE_DIR}/src/libs/qrcode/qrcodegen.c
    ${ESPCONTROL_LVGL_SOURCE_DIR}/src/libs/qrcode/lv_qrcode.c)
  target_compile_features(page_wifi_qr_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_wifi_qr_modal_test PRIVATE -Wall -Wextra -Werror -UNDEBUG
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
  target_compile_definitions(page_wifi_qr_modal_test PRIVATE LV_USE_QRCODE=1)
  target_include_directories(page_wifi_qr_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_wifi_qr_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_wifi_qr_modal_test COMMAND page_wifi_qr_modal_test)
  set_tests_properties(page_wifi_qr_modal_test PROPERTIES TIMEOUT 180)

  set(PAGE_NETWORK_STATUS_MODAL_NATIVE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_network_status_modal_test.cpp)
  add_custom_command(OUTPUT ${PAGE_NETWORK_STATUS_MODAL_NATIVE_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_network_status_modal_test.py --output ${PAGE_NETWORK_STATUS_MODAL_NATIVE_SOURCE}
    DEPENDS generate_page_network_status_modal_test.py generate_climate_update_test.py page_network_status_modal_test.cpp.in
      ../../components/espcontrol/button_grid_subpages.h ../../components/espcontrol/button_grid_layout.h
    VERBATIM)
  add_executable(page_network_status_modal_test ${PAGE_NETWORK_STATUS_MODAL_NATIVE_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_network_status_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_network_status_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_network_status_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_network_status_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_network_status_modal_test COMMAND page_network_status_modal_test)
  set_tests_properties(page_network_status_modal_test PROPERTIES TIMEOUT 180)

  set(PAGE_CONFIRMATION_MODAL_NATIVE_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_confirmation_modal_test.cpp)
  add_custom_command(OUTPUT ${PAGE_CONFIRMATION_MODAL_NATIVE_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_confirmation_modal_test.py --output ${PAGE_CONFIRMATION_MODAL_NATIVE_SOURCE}
    DEPENDS generate_page_confirmation_modal_test.py generate_climate_update_test.py page_confirmation_modal_test.cpp.in
      ../../components/espcontrol/button_grid_modal.h ../../components/espcontrol/button_grid_confirm.h
      ../../components/espcontrol/button_grid_modal_layout.h
    VERBATIM)
  add_executable(page_confirmation_modal_test ${PAGE_CONFIRMATION_MODAL_NATIVE_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_store.cpp
    ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp)
  target_compile_features(page_confirmation_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_confirmation_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_confirmation_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_confirmation_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_confirmation_modal_test COMMAND page_confirmation_modal_test)
  set_tests_properties(page_confirmation_modal_test PROPERTIES TIMEOUT 180)

  add_executable(page_expanded_image_modal_test page_expanded_image_modal_test.cpp
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_expanded_image_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_expanded_image_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_expanded_image_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_expanded_image_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_expanded_image_modal_test COMMAND page_expanded_image_modal_test)
  set_tests_properties(page_expanded_image_modal_test PROPERTIES TIMEOUT 180)

  set(PAGE_ALARM_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_alarm_modal_test.cpp)
  add_custom_command(OUTPUT ${PAGE_ALARM_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_alarm_modal_test.py --output ${PAGE_ALARM_MODAL_SOURCE}
    DEPENDS generate_page_alarm_modal_test.py generate_climate_update_test.py page_alarm_modal_test.cpp.in
      ../../components/espcontrol/button_grid_alarm.h ../../components/espcontrol/alarm_modal_lifetime.h
      ../../components/espcontrol/page_alarm_modal.h
    VERBATIM)
  add_executable(page_alarm_modal_test ${PAGE_ALARM_MODAL_SOURCE}
    ../../components/espcontrol/page_alarm_presentation.cpp
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_alarm_commands.cpp
    ../../components/espcontrol/page_alarm_ack_runtime.cpp ../../components/espcontrol/page_alarm_state_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store_runtime.cpp ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_alarm_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_alarm_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_alarm_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_alarm_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_alarm_modal_test COMMAND page_alarm_modal_test)

  set(PAGE_CLIMATE_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_climate_modal_test.cpp)
  add_custom_command(
    OUTPUT ${PAGE_CLIMATE_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_climate_modal_test.py
      --output ${PAGE_CLIMATE_MODAL_SOURCE}
    DEPENDS generate_page_climate_modal_test.py generate_climate_update_test.py
      page_climate_modal_test.cpp.in ../../components/espcontrol/button_grid_climate.h
      ../../components/espcontrol/button_grid_climate_control_driver.h
    VERBATIM)
  add_executable(page_climate_modal_test ${PAGE_CLIMATE_MODAL_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_climate_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_climate_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_climate_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_climate_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_climate_modal_test COMMAND page_climate_modal_test)
  set(PAGE_LIGHT_COVER_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_light_cover_modal_test.cpp)
  add_custom_command(
    OUTPUT ${PAGE_LIGHT_COVER_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_light_cover_modal_test.py
      --output ${PAGE_LIGHT_COVER_MODAL_SOURCE}
    DEPENDS generate_page_light_cover_modal_test.py generate_climate_update_test.py
      page_light_cover_modal_test.cpp.in ../../components/espcontrol/button_grid_sliders.h
      ../../components/espcontrol/page_light_cover_modal.h
    VERBATIM)
  add_executable(page_light_cover_modal_test ${PAGE_LIGHT_COVER_MODAL_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_light_cover_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_light_cover_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_light_cover_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_light_cover_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_light_cover_modal_test COMMAND page_light_cover_modal_test)

  set(PAGE_MEDIA_VOLUME_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_media_volume_modal_test.cpp)
  add_custom_command(
    OUTPUT ${PAGE_MEDIA_VOLUME_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_media_volume_modal_test.py
      --output ${PAGE_MEDIA_VOLUME_MODAL_SOURCE}
    DEPENDS generate_page_media_volume_modal_test.py generate_climate_update_test.py
      page_media_volume_modal_test.cpp.in ../../components/espcontrol/button_grid_sliders.h
      ../../components/espcontrol/button_grid_media.h
      ../../components/espcontrol/page_media_volume_modal.h
    VERBATIM)
  add_executable(page_media_volume_modal_test ${PAGE_MEDIA_VOLUME_MODAL_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_media_volume_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_media_volume_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_media_volume_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_media_volume_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_media_volume_modal_test COMMAND page_media_volume_modal_test)

  set(PAGE_MEDIA_CONTROL_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_media_control_modal_test.cpp)
  add_custom_command(
    OUTPUT ${PAGE_MEDIA_CONTROL_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_media_control_modal_test.py
      --output ${PAGE_MEDIA_CONTROL_MODAL_SOURCE}
    DEPENDS generate_page_media_control_modal_test.py generate_climate_update_test.py
      page_media_control_modal_test.cpp.in ../../components/espcontrol/button_grid_sliders.h
      ../../components/espcontrol/button_grid_media.h
      ../../components/espcontrol/button_grid_media_group.h
      ../../components/espcontrol/page_media_control_modal.h
    VERBATIM)
  add_executable(page_media_control_modal_test ${PAGE_MEDIA_CONTROL_MODAL_SOURCE}
    ../../components/espcontrol/page_media_state_runtime.cpp
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_media_control_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_media_control_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_media_control_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_media_control_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_media_control_modal_test COMMAND page_media_control_modal_test)

  set(PAGE_OPTION_SELECT_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_option_select_modal_test.cpp)
  add_custom_command(OUTPUT ${PAGE_OPTION_SELECT_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_option_select_modal_test.py
      --output ${PAGE_OPTION_SELECT_MODAL_SOURCE}
    DEPENDS generate_page_option_select_modal_test.py generate_climate_update_test.py
      page_option_select_modal_test.cpp.in ../../components/espcontrol/button_grid_option_select.h
      ../../components/espcontrol/page_option_select_modal.h
    VERBATIM)
  add_executable(page_option_select_modal_test ${PAGE_OPTION_SELECT_MODAL_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_option_select_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_option_select_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_option_select_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_option_select_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_option_select_modal_test COMMAND page_option_select_modal_test)

  set(PAGE_FAN_CONTROL_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_fan_control_modal_test.cpp)
  add_custom_command(
    OUTPUT ${PAGE_FAN_CONTROL_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_fan_control_modal_test.py
      --output ${PAGE_FAN_CONTROL_MODAL_SOURCE}
    DEPENDS generate_page_fan_control_modal_test.py generate_climate_update_test.py
      page_fan_control_modal_test.cpp.in ../../components/espcontrol/button_grid_fan.h
      ../../components/espcontrol/page_fan_control_modal.h
      ../../components/espcontrol/page_fan_preset_modal.h
      ../../components/espcontrol/page_fan_modal_state.h
    VERBATIM)
  add_executable(page_fan_control_modal_test ${PAGE_FAN_CONTROL_MODAL_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp
    ../../components/espcontrol/page_card_dependencies.cpp
    ../../components/espcontrol/page_entity_state_runtime.cpp
    ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_fan_control_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_fan_control_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_fan_control_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_fan_control_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_fan_control_modal_test COMMAND page_fan_control_modal_test)

  set(PAGE_WEATHER_MODAL_SOURCE ${CMAKE_CURRENT_BINARY_DIR}/page_weather_modal_test.cpp)
  add_custom_command(OUTPUT ${PAGE_WEATHER_MODAL_SOURCE}
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate_page_weather_modal_test.py --output ${PAGE_WEATHER_MODAL_SOURCE}
    DEPENDS generate_page_weather_modal_test.py generate_climate_update_test.py page_weather_modal_test.cpp.in
      ../../components/espcontrol/button_grid_weather_daily_ui.h ../../components/espcontrol/button_grid_weather_modal.h
    VERBATIM)
  add_executable(page_weather_modal_test ${PAGE_WEATHER_MODAL_SOURCE}
    ../../components/espcontrol/page_modal_runtime.cpp ../../components/espcontrol/page_forecast_runtime.cpp ../../components/espcontrol/page_store.cpp)
  target_compile_features(page_weather_modal_test PRIVATE cxx_std_17)
  target_compile_options(page_weather_modal_test PRIVATE -Wall -Wextra -Werror -fno-rtti -UNDEBUG)
  target_include_directories(page_weather_modal_test PRIVATE stubs ../../components/espcontrol)
  target_link_libraries(page_weather_modal_test PRIVATE lvgl::lvgl)
  add_test(NAME page_weather_modal_test COMMAND page_weather_modal_test)

endif()

add_test(NAME weather_condition_parity_test COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/weather_condition_parity_test.py)
