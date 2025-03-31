//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//

#include "ALU.hpp"

#include <cassert>
#include <limits>
#include <utility>

namespace emulator::mos_6502::ALU {
namespace internal {
/**
 * @brief Convert a binary-represented decimal to its value
 *
 * The binary representation of a decimal works as follows:
 * - the first four bits carry the high binary digits,
 * - the last four bits carry the low binary digit.
 * Thus, an 8-bit binary integer can represent decimals from 0 to 99 inclusively.
 *
 * For example, binary 0b01111001 represents a decimal 79, and a binary 0b00010100 represents a decimal 14.
 * Therefore, the function returns {7, 9} and {1, 4} correspondingly.
 */
[[nodiscard]] constexpr std::pair<uint8_t, uint8_t> decode_decimal(const uint8_t binary) noexcept {
    const uint8_t low_digit  = binary & 0x0f;
    const uint8_t high_digit = (binary & 0xf0) >> 4;
    return { high_digit, low_digit };
}

/**
 * @brief Encode two decimal digits into a binary 8-bit number
 *
 * @copydetails decode_decimal
 */
[[nodiscard]] constexpr uint8_t encode_decimal(const uint8_t high_digit, const uint8_t low_digit) noexcept {
    assert(high_digit <= 9);
    assert(low_digit <= 9);
    return (high_digit << 4) | low_digit;
}

/**
 * @brief Add two numbers in range [0, 9] inclusively with carry
 *
 * @param[in] a The first number
 * @param[in] b The second number
 * @param[in, out] carry Is added to the result. Is then set if the result is greater than 9 and reset otherwise.
 *
 * @return The sum of two numbers modulo 10
 */
[[nodiscard]] constexpr uint8_t add_decimal_digits(const uint8_t a, const uint8_t b, bool &carry) noexcept {
    assert(a <= 9);
    assert(b <= 9);
    const auto result = a + b + carry;
    carry             = result > 9;
    return static_cast<uint8_t>(result % 10);
}

/**
 * @brief Add two binary-coded unsigned decimal 8-bit integers with carry
 *
 * Each of the operands is considered to be a binary-codd decimal as described in @link decode_decimal @endlink.
 *
 * @param[in] a The first number
 * @param[in] b The second number
 * @param[in, out] carry Its initial value is added to the result.
 *                       If the result is greater than 99, the carry is set, and reset otherwise.
 *
 * @return Binary-coded decimal result modulo 100
 */
[[nodiscard]] constexpr uint8_t add_decimal(const uint8_t a, const uint8_t b, bool &carry) noexcept {
    const auto [high_digit_a, low_digit_a] = decode_decimal(a);
    const auto [high_digit_b, low_digit_b] = decode_decimal(b);

    const auto low_digit_sum  = add_decimal_digits(low_digit_a, low_digit_b, carry);
    const auto high_digit_sum = add_decimal_digits(high_digit_a, high_digit_b, carry);

    return encode_decimal(high_digit_sum, low_digit_sum);
}

/**
 * @brief Add two unsigned 8-bit integers with carry
 *
 * @param[in] a The first number
 * @param[in] b The second number
 * @param[in, out] carry Its initial value is added to the result.
 *                       If the result is greater than 255, the carry is set, and reset otherwise.
 *
 * @return Potentially wrapped unsigned 8-bit result
 *
 * @post If the result is greater than 255, it is wrapped around zero.
 */
[[nodiscard]] constexpr uint8_t add_binary(const uint8_t a, const uint8_t b, bool &carry) noexcept {
    const auto result = static_cast<uint16_t>(a) + static_cast<uint16_t>(b) + (carry ? uint16_t{ 1 } : uint16_t{ 0 });
    carry = result > std::numeric_limits<uint8_t>::max();
    return static_cast<uint8_t>(result);
}

/**
 * @brief Subtract two unsigned 8-bit integers with borrow
 *
 * @param[in] a The number to subtract from
 * @param[in] b The number to subtract
 * @param[in, out] borrow Its initial value is subtracted the result.
 *                        If the result is negative, the borrow is set, and reset otherwise.
 *
 * @return Unsigned 8-bit result modulo 256
 */
[[nodiscard]] constexpr uint8_t subtract_binary(const uint8_t a, const uint8_t b, bool &borrow) noexcept {
    const auto result = static_cast<int>(a) - static_cast<int>(b) - (borrow ? 1 : 0);
    borrow            = result < 0;
    return static_cast<uint8_t>(result);
}

/**
 * @brief Subtract two numbers in range [0, 9] inclusively with borrow
 *
 * @param[in] a The number to subtract from
 * @param[in] b The number to subtract
 * @param[in, out] borrow Is subtracted from the result.
 *                        Is then set if the result is negative and reset otherwise.
 *
 * @return The difference between two numbers modulo 10
 */
[[nodiscard]] constexpr uint8_t subtract_decimal_digits(const uint8_t a, const uint8_t b, bool &borrow) noexcept {
    assert(a <= 9);
    assert(b <= 9);
    const auto result = static_cast<int>(a) - static_cast<int>(b) - (borrow ? 1 : 0);
    borrow            = result < 0;
    // Not sure how C++'s modulo operation treats negative numbers, so it's made positive to be sure
    return static_cast<uint8_t>((result + 10) % 10);
}

/**
 * @brief Subtract two binary-coded unsigned decimal 8-bit integers with borrow
 *
 * Each of the operands is considered to be a binary-codd decimal as described in @link decode_decimal @endlink.
 *
 * @param[in] a The number to subtract from
 * @param[in] b The number to subtract
 * @param[in, out] borrow Its initial value is subtracted from the result.
 *                        If the result is negative, the carry is set, and reset otherwise.
 *
 * @return Binary-coded decimal result modulo 100
 */
[[nodiscard]] constexpr uint8_t subtract_decimal(const uint8_t a, const uint8_t b, bool &borrow) noexcept {
    const auto [high_digit_a, low_digit_a] = decode_decimal(a);
    const auto [high_digit_b, low_digit_b] = decode_decimal(b);

    const auto low_digit_sum  = subtract_decimal_digits(low_digit_a, low_digit_b, borrow);
    const auto high_digit_sum = subtract_decimal_digits(high_digit_a, high_digit_b, borrow);

    return encode_decimal(high_digit_sum, low_digit_sum);
}
} // namespace internal

uint8_t add(const uint8_t a, const uint8_t b, StatusRegister &sr) noexcept {
    bool carry        = sr.carry; // bit-field sr.carry cannot be used as an in-out boolean
    const auto result = sr.decimal ? internal::add_decimal(a, b, carry) : internal::add_binary(a, b, carry);

    sr.carry    = carry;
    sr.overflow = (result & 0x80) != (a & 0x80); // Compare the sign bits
    sr.negative = result & 0x80;
    sr.zero     = result == 0;

    return result;
}

uint8_t subtract(const uint8_t a, const uint8_t b, StatusRegister &sr) noexcept {
    bool borrow       = !sr.carry;
    const auto result = sr.decimal ? internal::subtract_decimal(a, b, borrow) : internal::subtract_binary(a, b, borrow);

    sr.carry    = !borrow;
    sr.overflow = (result & 0x80) != (a & 0x80); // Compare the sign bits
    sr.negative = result & 0x80;
    sr.zero     = result == 0;

    return result;
}

uint8_t logical_and(const uint8_t a, const uint8_t b, StatusRegister &sr) noexcept {
    const auto result = a & b;

    sr.negative = result & 0x80;
    sr.zero     = result == 0;
    return static_cast<uint8_t>(result);
}

uint8_t logical_or(const uint8_t a, const uint8_t b, StatusRegister &sr) noexcept {
    const auto result = a | b;

    sr.negative = result & 0x80;
    sr.zero     = result == 0;
    return static_cast<uint8_t>(result);
}

uint8_t logical_xor(const uint8_t a, const uint8_t b, StatusRegister &sr) noexcept {
    const auto result = a ^ b;

    sr.negative = result & 0x80;
    sr.zero     = result == 0;
    return static_cast<uint8_t>(result);
}

uint8_t shift_right(uint8_t a, StatusRegister &sr) noexcept {
    sr.carry = a & 1; // store the rightmost bit
    a >>= 1;

    sr.negative = false;
    sr.zero     = a == 0;
    return a;
}

uint8_t shift_left(uint8_t a, StatusRegister &sr) noexcept {
    sr.carry = a & 0x80;
    a <<= 1;

    sr.negative = a & 0x80;
    sr.zero     = a == 0;
    return a;
}
} // namespace emulator::mos_6502::ALU