//
// Created by Mikhail Tsaritsyn on Apr 20, 2025.
//

#ifndef EMULATOR_MOS_6502_HELPERS_HPP
#define EMULATOR_MOS_6502_HELPERS_HPP
#include "Opcode.hpp"
#include <cstdint>
#include <string_view>
#include <utility>

namespace emulator::mos_6502 {
std::pair<uint8_t, bool> add_with_overflow(uint8_t a, uint8_t b) noexcept;

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

[[nodiscard]] std::string to_string(Instruction instruction) noexcept;
} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_HELPERS_HPP
