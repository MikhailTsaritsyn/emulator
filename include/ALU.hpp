//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//

#ifndef EMULATOR_MOS_6502_ALU_HPP
#define EMULATOR_MOS_6502_ALU_HPP
#include "mtl/core.hpp"
#include "StatusRegister.hpp"

namespace emulator::mos_6502::ALU {
/**
 * @brief Add two unsigned 8-bit integers with carry
 *
 * @param lhs The first value to add
 * @param rhs The second value to add
 * @param carry Value of the carry to be added
 *
 * @return {sum, carry, overflow}
 * @retval sum Unsigned 8-bit result modulo 256
 * @retval carry Is set when the sum of a binary addition exceeds 255, otherwise it is reset.
 * @retval overflow Is set whe the sign or bit 7 differs from that of the first value
 *                  due to result exceeding +127 or -128, otherwise it is reset.
 */
[[nodiscard]] std::tuple<mtl::u8, bool, bool> add_binary(mtl::u8 lhs, mtl::u8 rhs, bool carry) noexcept;

/**
 * @brief Add two binary-represented unsigned 8-bit decimal integers with carry
 *
 * The binary representation of a decimal works as follows:
 * - the first four bits carry the high binary digits,
 * - the last four bits carry the low binary digit.
 * Thus, an 8-bit binary integer can represent decimals from 0 to 99 inclusively.
 *
 * For example, 0x79 represents a decimal 79, and 0x14 represents a decimal 14.
 *
 * @param lhs The first value to add
 * @param rhs The second value to add
 * @param carry Value of the carry to be added
 *
 * @return {sum, carry, overflow}
 * @retval sum Unsigned binary-represented 8-bit decimal result modulo 100
 * @retval carry Is set when the sum of an addition exceeds 99, otherwise it is reset.
 * @retval overflow Is set whe the sign or bit 7 differs from that of the first value
 *                  due to result exceeding +127 or -128, otherwise it is reset.
 */
[[nodiscard]] std::tuple<mtl::u8, bool, bool> add_decimal(mtl::u8 lhs, mtl::u8 rhs, bool carry) noexcept;

/**
 * @brief Subtract two unsigned 8-bit integers with borrow
 *
 * The borrow means that a previous operation has to borrow 1 from the current value.
 * If a single-precision is performed, there is no borrow in the beginning.
 *
 * @param lhs The value to subtract from
 * @param rhs The value to subtract
 * @param carry Its negated value is subtracted
 *
 * @return {result, carry, overflow}
 * @retval result Unsigned 8-bit result modulo 256
 * @retval carry Is set if the result is greater than or equal to zero, otherwise it is reset indicating a borrow.
 * @retval overflow Is set when the result exceeds +127 or -127, otherwise it is reset.
 */
[[nodiscard]] std::tuple<mtl::u8, bool, bool> subtract_binary(mtl::u8 lhs, mtl::u8 rhs, bool carry) noexcept;

/**
 * @brief Subtract two binary-represented unsigned 8-bit decimal integers with borrow
 *
 * The borrow means that a previous operation has to borrow 1 from the current value.
 * If a single-precision is performed, there is no borrow in the beginning.
 *
 * Binary encoding of decimal values is described in @link add_decimal @endlink.
 *
 * @param lhs The value to subtract from
 * @param rhs The value to subtract
 * @param carry Its negated value is subtracted
 *
 * @return {result, carry, overflow}
 * @retval result Unsigned 8-bit result modulo 100
 * @retval carry Is set if the result is greater than or equal to zero, otherwise it is reset indicating a borrow.
 * @retval overflow Is set when the result exceeds +127 or -127, otherwise it is reset.
 */
[[nodiscard]] std::tuple<mtl::u8, bool, bool> subtract_decimal(mtl::u8 lhs, mtl::u8 rhs, bool carry) noexcept;

/**
 * @brief Shift an unsigned 8-bit integer right one bit
 *
 * @param[in] a The number to be shifted.
 * @param[out] sr Does not affect the operation. Some of its flags are set as a result.
 *
 * @post The status register is updated at the end of the operation.
 *       - The carry flag is set equal to bit 0 of the input.
 *       - The negative flag is always reset.
 *       - The zero flag is set if the result is zero, otherwise it is reset.
 */
[[nodiscard]] mtl::u8 shift_right(mtl::u8 a, StatusRegister &sr) noexcept;

/**
 * @brief Shift an unsigned 8-bit integer left one bit
 *
 * @param[in] a The number to be shifted.
 * @param[out] sr Does not affect the operation. Some of its flags are set as a result.
 *
 * @post The status register is updated at the end of the operation.
 *       - The carry flag is set equal to bit 7 of the input.
 *       - The negative flag is set to result bit 7 (input bit 6).
 *       - The zero flag is set if the result of the shift is zero and reset otherwise.
 */
[[nodiscard]] mtl::u8 shift_left(mtl::u8 a, StatusRegister &sr) noexcept;

/**
 * @brief Rotate an unsigned 8-bit integer left one bit
 *
 * The input carry goes into the rightmost bit.
 * The leftmost bit goes into the output carry.
 *
 * @param[in] a Value to be rotated
 * @param[in, out] sr Its carry is used as an input and is put to the rightmost bit of the result.
 *                    The output carry is equal to the leftmost bit of the input value.
 *                    Some other flags are set as well.
 *
 * @post The status register is updated at the end of the operation.
 *       - The carry flag is set equal to input bit 7.
 *       - The negative flag is set equal to input bit 6.
 *       - The zero flag is set if the result is zero, otherwise it is reset.
 */
[[nodiscard]] mtl::u8 rotate_left(mtl::u8 a, StatusRegister &sr) noexcept;

/**
 * @brief Rotate an unsigned 8-bit integer right one bit
 *
 * The input carry goes into the leftmost bit.
 * The rightmost bit goes into the output carry.
 *
 * @param[in] a Value to be rotated
 * @param[in, out] sr Its carry is used as an input and is put to the leftmost bit of the result.
 *                    The output carry is equal to the rightmost bit of the input value.
 *                    Some other flags are set as well.
 *
 * @post The status register is updated at the end of the operation.
 *       - The carry flag is set equal to input bit 0.
 *       - The negative flag is set equal to input carry.
 *       - The zero flag is set if the result is zero, otherwise it is reset.
 */
[[nodiscard]] mtl::u8 rotate_right(mtl::u8 a, StatusRegister &sr) noexcept;
} // namespace emulator::mos_6502::ALU

#endif //EMULATOR_MOS_6502_ALU_HPP
