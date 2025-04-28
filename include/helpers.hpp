//
// Created by Mikhail Tsaritsyn on Apr 20, 2025.
//

#ifndef EMULATOR_MOS_6502_HELPERS_HPP
#define EMULATOR_MOS_6502_HELPERS_HPP
#include "mtl/core.hpp"
#include "Opcode.hpp"
#include <cstdint>
#include <utility>

namespace emulator::mos_6502 {
enum struct SignedOverflow : uint8_t { None, Positive, Negative };

[[nodiscard]] std::pair<mtl::u8, SignedOverflow> add_with_overflow(mtl::u8 u, mtl::i8 i) noexcept;

[[nodiscard]] std::string to_string(Instruction instruction) noexcept;

[[nodiscard]] mtl::u8 low_byte(uint16_t word) noexcept;

[[nodiscard]] mtl::u8 high_byte(uint16_t word) noexcept;

/**
 * @brief Construct a 16-bit unsigned integer from two 8-bit unsigned integers
 *
 * @param high High byte of the result
 * @param low Low byte of the result
 */
[[nodiscard]] uint16_t make_word(mtl::u8 high, mtl::u8 low) noexcept;
} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_HELPERS_HPP
