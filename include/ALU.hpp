//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//

#ifndef EMULATOR_MOS_6502_ALU_HPP
#define EMULATOR_MOS_6502_ALU_HPP
#include "StatusRegister.hpp"
#include <cstdint>

namespace emulator::mos_6502::ALU {
/**
 * @brief Add two unsigned 8-bit integers with carry
 *
 * @param[in] a The first value to add
 * @param[in] b The second value to add
 * @param[in, out] sr The carry value is taken from it.
 *                    After the addition is performed, some of its flags are updated.
 *
 * @return Unsigned 8-bit result modulo 256
 *
 * @post The status register is updated at the end of the operation.
 *       - The carry flag is set when the sum of a binary addition exceeds 255
 *         or when the sum of a decimal addition exceeds 99, otherwise it is reset.
 *       - The overflow flag is set whe the sign or bit 7 differs from that of the first value
 *         due to result exceeding +127 or -128, otherwise it is reset.
 *       - The negative flag is set if the result contains bit 7 on, otherwise it is reset.
 *       - The zero flag is set if the result is zero, otherwise it is reset.
 */
[[nodiscard]] uint8_t add(uint8_t a, uint8_t b, StatusRegister &sr) noexcept;
} // namespace emulator::mos_6502::ALU

#endif //EMULATOR_MOS_6502_ALU_HPP
