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

[[nodiscard]] mtl::u8 low_byte(mtl::u16 word) noexcept;

[[nodiscard]] mtl::u8 high_byte(mtl::u16 word) noexcept;

/**
 * @brief Construct a 16-bit unsigned integer from two 8-bit unsigned integers
 *
 * @param high High byte of the result
 * @param low Low byte of the result
 */
[[nodiscard]] mtl::u16 make_word(mtl::u8 high, mtl::u8 low) noexcept;
} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_HELPERS_HPP
