add_library(emulator_utils)
target_compile_options(emulator_utils PRIVATE -Werror)
target_compile_features(emulator_utils PUBLIC cxx_std_23)
target_sources(emulator_utils
    PUBLIC
    FILE_SET emulator_core_headers
    TYPE HEADERS
    BASE_DIRS .

    FILES
    mtl/core.hpp
    mtl/panic.hpp
    mtl/StrongInt.hpp

    PRIVATE
    src/panic.cpp
)

# Specify compiler options
target_compile_options(emulator_utils PRIVATE -Werror)
target_compile_features(emulator_utils PUBLIC cxx_std_23)

# Find GoogleTest
find_package(GTest REQUIRED)

# Create test executable
add_executable(emulator_utils_test
    tests/StrongInt.cpp
)
target_link_libraries(emulator_utils_test PRIVATE emulator_utils gtest gtest_main)

# Set up CTest
enable_testing()
include(GoogleTest)
gtest_discover_tests(emulator_utils_test)