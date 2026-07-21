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

add_executable(card_reconciler_test card_reconciler_test.cpp)
target_compile_features(card_reconciler_test PRIVATE cxx_std_17)
target_compile_options(card_reconciler_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(card_reconciler_test PRIVATE ../../components/espcontrol)
add_test(NAME card_reconciler_test COMMAND card_reconciler_test)

add_executable(card_instance_test card_instance_test.cpp)
target_compile_features(card_instance_test PRIVATE cxx_std_17)
target_compile_options(card_instance_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(card_instance_test PRIVATE ../../components/espcontrol)
add_test(NAME card_instance_test COMMAND card_instance_test)

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

add_executable(configuration_document_api_test
  configuration_document_api_test.cpp
  ../../components/espcontrol/configuration_service.cpp
  ../../components/espcontrol/configuration_store.cpp
)
target_compile_features(configuration_document_api_test PRIVATE cxx_std_17)
target_compile_options(configuration_document_api_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(configuration_document_api_test PRIVATE ../../components/espcontrol)
add_test(NAME configuration_document_api_test COMMAND configuration_document_api_test)

add_executable(espcontrol_app_core_test
  espcontrol_app_core_test.cpp
  ../../components/espcontrol/espcontrol_app_core.cpp
)
target_compile_features(espcontrol_app_core_test PRIVATE cxx_std_17)
target_compile_options(espcontrol_app_core_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(espcontrol_app_core_test PRIVATE ../../components/espcontrol)
add_test(NAME espcontrol_app_core_test COMMAND espcontrol_app_core_test)

add_executable(ha_state_broker_test ha_state_broker_test.cpp)
target_compile_features(ha_state_broker_test PRIVATE cxx_std_17)
target_compile_options(ha_state_broker_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(ha_state_broker_test PRIVATE ../../components/espcontrol)
add_test(NAME ha_state_broker_test COMMAND ha_state_broker_test)

add_executable(web_event_stream_policy_test web_event_stream_policy_test.cpp)
target_compile_features(web_event_stream_policy_test PRIVATE cxx_std_17)
target_compile_options(web_event_stream_policy_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(web_event_stream_policy_test PRIVATE ../../components/web_server_idf)
add_test(NAME web_event_stream_policy_test COMMAND web_event_stream_policy_test)

add_executable(artwork_controller_test artwork_controller_test.cpp)
target_compile_features(artwork_controller_test PRIVATE cxx_std_17)
target_compile_options(artwork_controller_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(artwork_controller_test PRIVATE ../../components/espcontrol)
add_test(NAME artwork_controller_test COMMAND artwork_controller_test)

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
