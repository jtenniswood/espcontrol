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

add_executable(display_mode_controller_test display_mode_controller_test.cpp)
target_compile_features(display_mode_controller_test PRIVATE cxx_std_17)
target_compile_options(display_mode_controller_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(display_mode_controller_test PRIVATE ../../components/espcontrol)
add_test(NAME display_mode_controller_test COMMAND display_mode_controller_test)

add_executable(ha_read_coordinator_test ha_read_coordinator_test.cpp)
target_compile_features(ha_read_coordinator_test PRIVATE cxx_std_17)
target_compile_options(ha_read_coordinator_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(ha_read_coordinator_test PRIVATE ../../components/espcontrol)
add_test(NAME ha_read_coordinator_test COMMAND ha_read_coordinator_test)

add_executable(artwork_controller_test artwork_controller_test.cpp)
target_compile_features(artwork_controller_test PRIVATE cxx_std_17)
target_compile_options(artwork_controller_test PRIVATE -Wall -Wextra -Werror)
target_include_directories(artwork_controller_test PRIVATE ../../components/espcontrol)
add_test(NAME artwork_controller_test COMMAND artwork_controller_test)

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
