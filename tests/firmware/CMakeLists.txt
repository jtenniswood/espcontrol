cmake_minimum_required(VERSION 3.20)
project(espcontrol_firmware_tests LANGUAGES CXX)

enable_testing()

add_executable(foundation_test foundation_test.cpp)
target_compile_features(foundation_test PRIVATE cxx_std_17)
target_compile_options(foundation_test PRIVATE -Wall -Wextra -Werror)
add_test(NAME foundation_test COMMAND foundation_test)
