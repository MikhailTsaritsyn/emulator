//
// Created by Mikhail Tsaritsyn on Apr 25, 2025.
//

#ifndef EMULATOR_UTILS_PANIC_HPP
#define EMULATOR_UTILS_PANIC_HPP
#include <string_view>

namespace mtl {
/**
 * @brief Erroneously stop the program execution
 *
 * Designed for mistakes in code logic, that are not user's fault.
 *
 * @note works both in Debug and Release configurations.
 * @note Tested with GCC and Clang compilers
 *
 * @param message To be displayed before exiting
 */
[[noreturn]] void panic(std::string_view message);
}; // namespace mtl

#endif //EMULATOR_UTILS_PANIC_HPP
