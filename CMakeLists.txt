cmake_minimum_required(VERSION 3.31)
project(emulator)

# Create library that will be used by both the executable and tests
add_library(emulator_core)
target_compile_options(emulator_core PRIVATE -Werror)
target_compile_features(emulator_core PUBLIC cxx_std_23)
target_sources(emulator_core
    PUBLIC
    FILE_SET emulator_core_headers
    TYPE HEADERS
    BASE_DIRS include
    FILES
    include/Opcode.hpp
    PRIVATE
    src/Opcode.cpp
)

# Create the main executable
add_executable(emulator main.cpp)
target_link_libraries(emulator PRIVATE emulator_core)

# Specify compiler options
target_compile_options(emulator PRIVATE -Werror)
target_compile_features(emulator PUBLIC cxx_std_23)

# Find GoogleTest
find_package(GTest REQUIRED)

# Create test executable
add_executable(emulator_test
    tests/Opcode.cpp
)
target_link_libraries(emulator_test PRIVATE emulator_core gtest gtest_main)

# Set up CTest
enable_testing()
include(GoogleTest)
gtest_discover_tests(emulator_test)

# Set up packaging
install(TARGETS emulator)
include(CPack)