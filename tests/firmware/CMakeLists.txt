cmake_minimum_required(VERSION 3.20)
project(espcontrol_firmware_tests LANGUAGES CXX)

enable_testing()

add_executable(foundation_test foundation_test.cpp)
target_compile_features(foundation_test PRIVATE cxx_std_17)
target_compile_options(foundation_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME foundation_test COMMAND foundation_test)

add_executable(button_grid_foundations_test button_grid_foundations_test.cpp)
target_compile_features(button_grid_foundations_test PRIVATE cxx_std_17)
target_compile_options(button_grid_foundations_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(button_grid_foundations_test PRIVATE
  stubs
  ../../components/espcontrol
)
add_test(NAME button_grid_foundations_test COMMAND button_grid_foundations_test)

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

add_executable(alarm_delay_audio_test alarm_delay_audio_test.cpp)
target_compile_features(alarm_delay_audio_test PRIVATE cxx_std_17)
target_compile_options(alarm_delay_audio_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(alarm_delay_audio_test PRIVATE ../../components/espcontrol)
add_test(NAME alarm_delay_audio_test COMMAND alarm_delay_audio_test)

add_executable(climate_target_logic_test climate_target_logic_test.cpp)
target_compile_features(climate_target_logic_test PRIVATE cxx_std_17)
target_compile_options(climate_target_logic_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(climate_target_logic_test PRIVATE ../../components/espcontrol)
add_test(NAME climate_target_logic_test COMMAND climate_target_logic_test)

add_executable(media_volume_capability_test media_volume_capability_test.cpp)
target_compile_features(media_volume_capability_test PRIVATE cxx_std_17)
target_compile_options(media_volume_capability_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(media_volume_capability_test PRIVATE ../../components/espcontrol)
add_test(NAME media_volume_capability_test COMMAND media_volume_capability_test)

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

add_executable(slider_geometry_test slider_geometry_test.cpp)
target_compile_features(slider_geometry_test PRIVATE cxx_std_17)
target_compile_options(slider_geometry_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(slider_geometry_test PRIVATE ../../components/espcontrol)
add_test(NAME slider_geometry_test COMMAND slider_geometry_test)

add_executable(display_mode_controller_test display_mode_controller_test.cpp)
target_compile_features(display_mode_controller_test PRIVATE cxx_std_17)
target_compile_options(display_mode_controller_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(display_mode_controller_test PRIVATE ../../components/espcontrol)
add_test(NAME display_mode_controller_test COMMAND display_mode_controller_test)

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

add_executable(panel_config_event_filter_test panel_config_event_filter_test.cpp)
target_compile_features(panel_config_event_filter_test PRIVATE cxx_std_17)
target_compile_options(panel_config_event_filter_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_event_filter_test PRIVATE ../../components/web_server_idf)
add_test(NAME panel_config_event_filter_test COMMAND panel_config_event_filter_test)

add_executable(panel_config_legacy_entity_guard_test
  panel_config_legacy_entity_guard_test.cpp)
target_compile_features(panel_config_legacy_entity_guard_test PRIVATE cxx_std_17)
target_compile_options(panel_config_legacy_entity_guard_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(panel_config_legacy_entity_guard_test PRIVATE ../../components/espcontrol)
add_test(NAME panel_config_legacy_entity_guard_test COMMAND panel_config_legacy_entity_guard_test)

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

add_executable(image_service_test image_service_test.cpp)
target_compile_features(image_service_test PRIVATE cxx_std_17)
target_compile_options(image_service_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(image_service_test PRIVATE ../../components/artwork_image)
add_test(NAME image_service_test COMMAND image_service_test)

add_executable(bmp_stream_test bmp_stream_test.cpp)
target_compile_features(bmp_stream_test PRIVATE cxx_std_17)
target_compile_options(bmp_stream_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(bmp_stream_test PRIVATE ../../components/artwork_image)
add_test(NAME bmp_stream_test COMMAND bmp_stream_test)

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

find_package(Python3 REQUIRED COMPONENTS Interpreter)
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
