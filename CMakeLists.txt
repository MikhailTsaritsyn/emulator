cmake_minimum_required(VERSION 3.31)
project(emulator)

# Create the main executable
add_executable(emulator main.cpp)

# Specify compiler options
target_compile_options(emulator PRIVATE -Werror)
target_compile_features(emulator PUBLIC cxx_std_23)

# Find GoogleTest
find_package(GTest REQUIRED)

# Create test executable
add_executable(emulator_test
    tests/test.cpp
)
target_link_libraries(emulator_test PRIVATE gtest gtest_main)

# Set up CTest
enable_testing()
include(GoogleTest)
gtest_discover_tests(emulator_test)

# Set up packaging
install(
    TARGETS emulator
    EXPORT emulatorTargets
    FILE_SET emulator_headers
)
install(
    EXPORT emulatorTargets
    FILE emulatorTargets.cmake
    DESTINATION share/cmake/emulator
    NAMESPACE emulator::
)
install(
    FILES cmake/emulatorConfig.cmake
    DESTINATION share/cmake/emulator
)
include(CPack)