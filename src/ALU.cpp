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
 * Decimal encoding is described in @link add_decimal @endlink.
 * The function returns {7, 9} and {1, 4} for 0x79 and 0x14 correspondingly.
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

[[nodiscard]] constexpr bool is_sign_bit_different(const mtl::u8 a, const mtl::u8 b) noexcept {
    return (a & mtl::u8(0x80)) != (b & mtl::u8(0x80));
}
} // namespace internal

std::tuple<mtl::u8, bool, bool> add_binary(const mtl::u8 lhs, const mtl::u8 rhs, const bool carry) noexcept {
    const auto [tmp, overflow1]    = add_with_overflow(lhs, rhs);
    const auto [result, overflow2] = add_with_overflow(tmp, carry ? mtl::u8(1) : mtl::u8(0));
    return { result, overflow1 || overflow2, internal::is_sign_bit_different(result, lhs) };
}

std::tuple<mtl::u8, bool, bool> add_decimal(const mtl::u8 lhs, const mtl::u8 rhs, bool carry) noexcept {
    const auto [high_digit_lhs, low_digit_lhs] = internal::decode_decimal(lhs);
    const auto [high_digit_rhs, low_digit_rhs] = internal::decode_decimal(rhs);

    const auto low_digit_sum  = internal::add_decimal_digits(low_digit_lhs, low_digit_rhs, carry);
    const auto high_digit_sum = internal::add_decimal_digits(high_digit_lhs, high_digit_rhs, carry);

    const auto result = internal::encode_decimal(high_digit_sum, low_digit_sum);
    return { result, carry, internal::is_sign_bit_different(result, lhs) };
}

std::tuple<mtl::u8, bool, bool> subtract_binary(const mtl::u8 lhs, const mtl::u8 rhs, const bool carry) noexcept {
    const auto [tmp, overflow1]    = sub_with_overflow(lhs, rhs);
    const auto [result, overflow2] = sub_with_overflow(tmp, !carry ? mtl::u8(1) : mtl::u8(0));
    return { result, !(overflow1 || overflow2), internal::is_sign_bit_different(result, lhs) };
}

std::tuple<mtl::u8, bool, bool> subtract_decimal(const mtl::u8 lhs, const mtl::u8 rhs, const bool carry) noexcept {
    bool borrow = !carry;
    const auto [high_digit_lhs, low_digit_lhs] = internal::decode_decimal(lhs);
    const auto [high_digit_rhs, low_digit_rhs] = internal::decode_decimal(rhs);

    const auto low_digit_sum  = internal::subtract_decimal_digits(low_digit_lhs, low_digit_rhs, borrow);
    const auto high_digit_sum = internal::subtract_decimal_digits(high_digit_lhs, high_digit_rhs, borrow);

    const auto result = internal::encode_decimal(high_digit_sum, low_digit_sum);
    return { result, !borrow, internal::is_sign_bit_different(result, lhs) };
}

std::pair<mtl::u8, bool> shift_right(const mtl::u8 byte) noexcept { return { byte >> 1, (byte & mtl::u8(1)) != 0 }; }

std::pair<mtl::u8, bool> rotate_left(const mtl::u8 a, const bool carry) noexcept {
    auto [shifted, overflow] = shift_left(a, 1);
    if (carry) shifted |= mtl::u8(1); // set the rightmost bit
    return { shifted, overflow };
}

std::pair<mtl::u8, bool> rotate_right(mtl::u8 a, const bool carry) noexcept {
    const bool output_carry = (a & mtl::u8(1)) != 0; // store the rightmost bit
    a >>= 1;
    if (carry) a |= mtl::u8(0x80); // set the leftmost bit

    return { a, output_carry };
}
} // namespace emulator::mos_6502::ALU