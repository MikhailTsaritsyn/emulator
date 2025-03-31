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

/**
 * @brief Add two unsigned 8-bit integers with borrow
 *
 * The borrow means that a previous operation has to borrow 1 from the current value.
 * If a single-precision is performed, there is no borrow in the beginning.
 *
 * @param[in] a The value to subtract from
 * @param[in] b The value to subtract
 * @param[in, out] sr The carry, or borrow, value is taken from it.
 *                    After the subtraction is performed, some of its flags are updated.
 *
 * @return Unsigned 8-bit result modulo 256
 *
 * @post The status register is updated at the end of the operation.
 *       - The carry flag is set if the result is greater than or equal to zero,
 *       otherwise it is reset indicating a borrow.
 *       - The overflow flag is set when the result exceeds +127 or -127, otherwise it is reset.
 *       - The negative flag is set if the result has bit 7 on, otherwise it is reset.
 *       - The zero flag is set if the result is zero, otherwise it is reset.
 */
[[nodiscard]] uint8_t subtract(uint8_t a, uint8_t b, StatusRegister &sr) noexcept;

/**
 * @brief AND two unsigned 8-bit integers
 *
 * @param[in] a The first number
 * @param[in] b The second number
 * @param[out] sr Does not affect the operation. Some of its flags are set when it's finished.
 *
 * This instruction affects the Status Register
 * - The zero flag is set if the result in the Accumulator is 0, otherwise it is reset.
 * - The negative flag is set if the result in the accumulator has bit 7 on, otherwise it is reset.
 *
 * @return Unsigned 8-bit result
 *
 * @post The status register is updated at the end of the operation.
 *       - The negative flag is set if the result has bit 7 on, otherwise it is reset.
 *       - The zero flag is set if the result is zero, otherwise it is reset.
 */
[[nodiscard]] uint8_t logical_and(uint8_t a, uint8_t b, StatusRegister &sr) noexcept;

/**
 * @brief OR two unsigned 8-bit integers
 *
 * @copydoc logical_and
 */
[[nodiscard]] uint8_t logical_or(uint8_t a, uint8_t b, StatusRegister &sr) noexcept;

/**
 * @brief XOR two unsigned 8-bit integers
 *
 * @copydoc logical_and
 */
[[nodiscard]] uint8_t logical_xor(uint8_t a, uint8_t b, StatusRegister &sr) noexcept;
} // namespace emulator::mos_6502::ALU

#endif //EMULATOR_MOS_6502_ALU_HPP
