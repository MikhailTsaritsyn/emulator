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

enum struct SignedOverflow : uint8_t { None, Positive, Negative };

[[nodiscard]] std::pair<uint8_t, SignedOverflow> add_with_overflow(uint8_t u, int8_t i) noexcept;

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

[[nodiscard]] uint8_t low_byte(uint16_t word) noexcept;

[[nodiscard]] uint8_t high_byte(uint16_t word) noexcept;

/**
 * @brief Construct a 16-bit unsigned integer from two 8-bit unsigned integers
 *
 * @param high High byte of the result
 * @param low Low byte of the result
 */
[[nodiscard]] uint16_t make_word(uint8_t high, uint8_t low) noexcept;
} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_HELPERS_HPP
