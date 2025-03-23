cmake_minimum_required(VERSION 3.31)
project(emulator)

add_executable(emulator main.cpp)

target_compile_options(emulator PRIVATE -Werror)
target_compile_features(emulator PUBLIC cxx_std_23)
