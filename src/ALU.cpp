//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//

#include "ALU.hpp"

#include <cassert>
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
[[nodiscard]] constexpr std::pair<mtl::u8, mtl::u8> decode_decimal(const mtl::u8 binary) noexcept {
    const auto low_digit  = binary & mtl::u8(0x0f);
    const auto high_digit = (binary & mtl::u8(0xf0)) >> 4;
    return { high_digit, low_digit };
}

/**
 * @brief Encode two decimal digits into a binary 8-bit number
 *
 * @copydetails decode_decimal
 */
[[nodiscard]] constexpr mtl::u8 encode_decimal(const mtl::u8 high_digit, const mtl::u8 low_digit) noexcept {
    assert(high_digit <= mtl::u8(9));
    assert(low_digit <= mtl::u8(9));
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
[[nodiscard]] constexpr mtl::u8 add_decimal_digits(const mtl::u8 a, const mtl::u8 b, bool &carry) noexcept {
    assert(a <= mtl::u8(9));
    assert(b <= mtl::u8(9));
    const auto result = a + b + (carry ? mtl::u8(1) : mtl::u8(0));
    carry             = result > mtl::u8(9);
    return result % mtl::u8(10);
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
[[nodiscard]] constexpr mtl::u8 add_decimal(const mtl::u8 a, const mtl::u8 b, bool &carry) noexcept {
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
[[nodiscard]] constexpr mtl::u8 add_binary(const mtl::u8 a, const mtl::u8 b, bool &carry) noexcept {
    const auto [tmp, overflow1]    = add_with_overflow(a, b);
    const auto [result, overflow2] = add_with_overflow(tmp, carry ? mtl::u8(1) : mtl::u8(0));
    carry                          = overflow1 || overflow2;
    return result;
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
[[nodiscard]] constexpr mtl::u8 subtract_binary(const mtl::u8 a, const mtl::u8 b, bool &borrow) noexcept {
    const auto [tmp, overflow1]    = sub_with_overflow(a, b);
    const auto [result, overflow2] = sub_with_overflow(tmp, borrow ? mtl::u8(1) : mtl::u8(0));
    borrow                         = overflow1 || overflow2;
    return result;
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
[[nodiscard]] constexpr mtl::u8 subtract_decimal_digits(const mtl::u8 a, const mtl::u8 b, bool &borrow) noexcept {
    assert(a <= mtl::u8(9));
    assert(b <= mtl::u8(9));
    const auto result = std::bit_cast<mtl::i8>(a) - std::bit_cast<mtl::i8>(b) - (borrow ? mtl::i8(1) : mtl::i8(0));

    borrow            = result < mtl::i8(0);
    // Not sure how C++'s modulo operation treats negative numbers, so it's made positive to be sure
    return ((result + mtl::i8(10)) % mtl::i8(10)).unsafe_cast<uint8_t>();
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
[[nodiscard]] constexpr mtl::u8 subtract_decimal(const mtl::u8 a, const mtl::u8 b, bool &borrow) noexcept {
    const auto [high_digit_a, low_digit_a] = decode_decimal(a);
    const auto [high_digit_b, low_digit_b] = decode_decimal(b);

    const auto low_digit_sum  = subtract_decimal_digits(low_digit_a, low_digit_b, borrow);
    const auto high_digit_sum = subtract_decimal_digits(high_digit_a, high_digit_b, borrow);

    return encode_decimal(high_digit_sum, low_digit_sum);
}
} // namespace internal

std::tuple<mtl::u8, bool, bool> add(const mtl::u8 lhs, const mtl::u8 rhs, bool carry, const bool decimal) noexcept {
    const auto result = decimal ? internal::add_decimal(lhs, rhs, carry) : internal::add_binary(lhs, rhs, carry);
    return { result, carry, (result & mtl::u8(0x80)) != (lhs & mtl::u8(0x80)) };
}

std::tuple<mtl::u8, bool, bool>
subtract(const mtl::u8 lhs, const mtl::u8 rhs, const bool carry, const bool decimal) noexcept {
    bool borrow = !carry;
    const auto result =
            decimal ? internal::subtract_decimal(lhs, rhs, borrow) : internal::subtract_binary(lhs, rhs, borrow);
    return { result, !borrow, (result & mtl::u8(0x80)) != (lhs & mtl::u8(0x80)) };
}

mtl::u8 shift_right(mtl::u8 a, StatusRegister &sr) noexcept {
    sr.carry = (a & mtl::u8(1)) != 0; // store the rightmost bit
    a >>= 1;

    sr.negative = false;
    sr.zero     = a == 0;
    return a;
}

mtl::u8 shift_left(const mtl::u8 a, StatusRegister &sr) noexcept {
    const auto [shifted, overflow] = shift_left(a, 1);
    sr.carry                       = overflow;

    sr.negative = (shifted & mtl::u8(0x80)) != 0;
    sr.zero     = shifted == 0;
    return shifted;
}

mtl::u8 rotate_left(const mtl::u8 a, StatusRegister &sr) noexcept {
    auto [shifted, overflow] = shift_left(a, 1);
    if (sr.carry) shifted |= mtl::u8(1); // set the rightmost bit

    sr.carry    = overflow;
    sr.negative = (shifted & mtl::u8(0x80)) != 0;
    sr.zero     = shifted == 0;
    return shifted;
}

mtl::u8 rotate_right(mtl::u8 a, StatusRegister &sr) noexcept {
    const bool output_carry = (a & mtl::u8(1)) != 0; // store the rightmost bit
    a >>= 1;
    if (sr.carry) a |= mtl::u8(0x80); // set the leftmost bit

    sr.carry    = output_carry;
    sr.negative = (a & mtl::u8(0x80)) != 0;
    sr.zero     = a == 0;
    return a;
}
} // namespace emulator::mos_6502::ALU