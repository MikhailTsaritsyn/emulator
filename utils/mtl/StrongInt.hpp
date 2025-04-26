//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#ifndef EMULATOR_STRONG_TYPES_HPP
#define EMULATOR_STRONG_TYPES_HPP
#include "panic.hpp"
#include <concepts>
#include <cstdint>
#include <expected>
#include <format>
#include <ostream>
#include <utility>

// TODO: comparison between different types? (with unsafe_cast'ing)
// TODO: comparison to built-in types?
// TODO: strong float and double
// TODO: div and mod like in Python?
// TODO: an option to remove the sign bit in right shift

// TODO: fixed point

namespace mtl {
/**
 * @brief Checks if an integral type U contains all values of an integral type T.
 *
 * A signed type contains any smaller type.
 * An unsigned type only contains smaller unsigned types.
 */
template <typename U, typename T>
concept Contains = std::is_integral_v<T> && std::is_integral_v<U> && sizeof(U) > sizeof(T)
                   && (std::is_signed_v<U> || std::is_unsigned_v<T> && std::is_unsigned_v<U>);

template <std::integral T> class StrongInt {
public:
    /**
     * @brief Zero-initialization
     */
    constexpr StrongInt() noexcept = default;

    /**
     * @brief Conversion from the underlying type
     */
    constexpr explicit StrongInt(T value) noexcept : _value{ value } {}

    /**
     * @brief Conversion to the underlying type
     */
    [[nodiscard]] constexpr explicit operator T() const noexcept { return _value; }

    /**
     * @brief Conversion to the underlying type
     */
    [[nodiscard]] constexpr T to_underlying() const noexcept { return _value; }

    /**
     * @brief Safe casts
     *
     * An integer can be cast to any type that contains all of its values.
     * Such conversions are only possible for:
     * - an unsigned to a wider unsigned
     * - an unsigned to a wider signed
     * - a signed to a wider signed
     *
     * Examples:
     * - @p uint8_t to @p uint16_t
     * - @p uint8_t to @p int16_t
     * - @p int8_t to @p int16_t
     */
    template <Contains<T> U> constexpr explicit operator StrongInt<U>() const noexcept {
        return StrongInt<U>(static_cast<U>(_value));
    }

    /**
     * @brief Narrowing cast that wraps around borders
     *
     * An integer can be narrowed to any type all of which values it contains.
     * These are the opposite of safe conversions.
     * Such conversions are only possible for:
     * - an unsigned to a narrower unsigned
     * - a signed to a narrower unsigned
     * - a signed to a narrower signed
     *
     * Examples:
     * - @p uint16_t to @p uint8_t
     * - @p int16_t to @p uint8_t
     * - @p int16_t to @p int8_t
     *
     * @note It is only defined for platforms implementing two's complement
     *
     * @example ../tests/StrongInt.cpp
     */
    template <std::integral U>
        requires Contains<T, U> && (std::bit_cast<uint8_t>(int8_t{ -1 }) == uint8_t{ 0xff })
    [[nodiscard]] constexpr StrongInt<U> wrap() const noexcept {
        if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<U>) {
            return StrongInt<U>(static_cast<U>(_value));
        }
        // narrowing casts for signed integers are undefined, have to reimplement it
        // by discarding the most significant bits of the wider type so it fits into the narrower type
        else if constexpr (std::is_unsigned_v<U>) {
            const auto mask = static_cast<T>(std::numeric_limits<U>::max()); // mask filled with ones for the size of U
            return StrongInt<U>(static_cast<U>(_value & mask));
        } else {
            const auto mask = static_cast<T>(U{ -1 }); // mask filled with ones for the size of U
            return StrongInt<U>(static_cast<U>(_value & mask));
        }
    }

    /**
     * @brief Narrowing cast that discards value out of range
     *
     * @copydetails wrap
     *
     * @retval std::nullopt If the value does not fit into the resulting type
     *
     * @example ../tests/StrongInt.cpp
     */
    template <std::integral U>
        requires Contains<T, U>
    [[nodiscard]] constexpr std::optional<StrongInt<U>> narrow() const noexcept {
        if (_value >= static_cast<T>(std::numeric_limits<U>::min())
            && _value <= static_cast<T>(std::numeric_limits<U>::max()))
            return StrongInt<U>(static_cast<U>(_value));
        else return std::nullopt;
    }

    /**
     * @copydoc narrow
     *
     * @throw std::underflow_error If the value is too small for the resulting type
     * @throw std::overflow_error If the value is too big for the resulting type
     *
     * @example ../tests/StrongInt.cpp
     */
    template <std::integral U>
        requires Contains<T, U>
    [[nodiscard]] constexpr StrongInt<U> unsafe_cast() const noexcept(false) {
        if (_value < static_cast<T>(std::numeric_limits<U>::min()))
            throw std::underflow_error("Cannot cast strong integer: underflow");

        if (_value > static_cast<T>(std::numeric_limits<U>::max()))
            throw std::overflow_error("Cannot cast strong integer: overflow");

        return StrongInt<U>(static_cast<U>(_value));
    }

    /**
     * @brief Overloads for the cases when the safe cast is possible
     */
    template <std::integral U>
        requires Contains<U, T>
    [[nodiscard]] constexpr StrongInt<U> unsafe_cast() const noexcept {
        return StrongInt<U>(*this);
    }

    /**
     * @brief Overload for the same type
     */
    template <std::integral U>
        requires(std::is_same_v<U, T>)
    [[nodiscard]] constexpr StrongInt<U> unsafe_cast() const noexcept {
        return *this;
    }

    /**
     * @brief Non-narrowing conversions.
     *
     * Such casts are:
     * - a signed to a non-narrower unsigned
     * - an unsigned to a non-wider signed
     *
     * Examples:
     * - int16_t to uint16_t
     * - int16_t to uint32_t
     * - uint16_t to int8_t
     * - uint16_t to int16_t
     *
     * @throw std::underflow_error If the value is too small for the resulting type
     * @throw std::overflow_error If the value is too big for the resulting type
     */
    template <std::integral U>
        requires(!Contains<U, T> && !Contains<T, U> && !std::is_same_v<T, U>)
    constexpr StrongInt<U> unsafe_cast() const noexcept(false) {
        if constexpr (std::is_signed_v<T> && std::is_unsigned_v<U>) {
            static_assert(sizeof(T) <= sizeof(U), "another overload must have been chosen");

            if (_value < 0) throw std::underflow_error("Cannot cast strong integer: underflow");

            return StrongInt<U>(static_cast<U>(_value));
        } else {
            static_assert(std::is_unsigned_v<T> && std::is_signed_v<U>, "another overload must have been chosen");
            static_assert(sizeof(T) >= sizeof(U), "another overload must have been chosen");

            if (_value > static_cast<T>(std::numeric_limits<U>::max()))
                throw std::overflow_error("Cannot cast strong integer: overflow");

            return StrongInt<U>(static_cast<U>(_value));
        }
    }

    /**
     * @brief Increment the value of an integer
     *
     * Overflow if the integer has the maximum value of the type.
     *
     * @retval std::nullopt Only in case of overflow
     */
    friend constexpr std::optional<StrongInt> inc(StrongInt sting) noexcept {
        if (sting._value == std::numeric_limits<T>::max()) return std::nullopt;
        return StrongInt{ static_cast<T>(sting._value + 1) };
    }

    /**
     * @copybrief inc(StrongInt)
     *
     * @copydetails inc(StrongInt)
     *
     * Panics in case of overflow.
     *
     * @return The non-incremented value
     */
    constexpr StrongInt operator++(int) noexcept {
        const auto new_value = inc(*this);
        if (!new_value) panic("StrongInt: overflow in increment");
        return std::exchange(*this, *new_value);
    }

    /**
     * @copybrief inc(StrongInt)
     *
     * @copydetails inc(StrongInt)
     *
     * Panics in case of overflow.
     *
     * @return Reference to the incremented value
     */
    constexpr StrongInt &operator++() noexcept {
        const auto new_value = inc(*this);
        if (!new_value) panic("StrongInt: overflow in increment");
        return *this = *new_value;
    }

    /**
     * @brief Decrement the value of an integer
     *
     * Overflow if the integer has the minimum value of the type.
     *
     * @retval std::nullopt Only in case of overflow
     */
    friend constexpr std::optional<StrongInt> dec(StrongInt sting) noexcept {
        if (sting._value == std::numeric_limits<T>::min()) return std::nullopt;
        return StrongInt{ static_cast<T>(sting._value - 1) };
    }

    /**
     * @copybrief dec(StrongInt)
     *
     * @copydetails dec(StrongInt)
     *
     * Panics in case of overflow.
     *
     * @return The non-decremented value
     */
    constexpr StrongInt operator--(int) noexcept {
        const auto new_value = dec(*this);
        if (!new_value) panic("StrongInt: overflow in decrement");
        return std::exchange(*this, *new_value);
    }

    /**
     * @copybrief dec(StrongInt)
     *
     * @copydetails dec(StrongInt)
     *
     * Panics in case of overflow.
     *
     * @return Reference to the incremented value
     */
    constexpr StrongInt &operator--() noexcept {
        const auto new_value = dec(*this);
        if (!new_value) panic("StrongInt: overflow in decrement");
        return *this = *new_value;
    }

private:
    friend std::ostream &operator<<(std::ostream &os, const StrongInt sting) noexcept {
        if constexpr (sizeof(T) == 1) // avoid printing as characters
            return os << static_cast<int>(sting.to_underlying());
        return os << sting._value;
    }

    friend constexpr auto operator<=>(StrongInt lhs, StrongInt rhs) noexcept { return lhs._value <=> rhs._value; }

    friend constexpr bool operator==(StrongInt lhs, StrongInt rhs) noexcept = default;

    /**
     * @brief Add two integers
     *
     * If the overflow happens, the sum is wrapped.
     * The overflow itself is returned as the second value.
     *
     * @retval first The (wrapped) result
     * @retval second Whether the overflow happened
     */
    friend constexpr std::pair<StrongInt, bool> add_with_overflow(StrongInt lhs, StrongInt rhs) noexcept {
        T result;
        const auto overflow = __builtin_add_overflow(lhs._value, rhs._value, &result);
        return { StrongInt(result), overflow };
    }

    /**
     * @brief Add two integers with overflow detection
     *
     * If the overflow happens, the function traps.
     * To get a wrapped sum, use @link add_with_overflow(StrongInt, StrongInt) @endlink.
     */
    friend constexpr StrongInt operator+(StrongInt lhs, StrongInt rhs) noexcept {
        const auto [result, overflow] = add_with_overflow(lhs, rhs);
        if (overflow) panic("StrongInt: overflow in addition");
        return result;
    }

    /**
     * @brief Add an integer in-place
     *
     * If the overflow happens, the function traps.
     * To get a wrapped sum, use @link add_with_overflow(StrongInt, StrongInt) @endlink.
     *
     * @param[in, out] lhs This value will be updated with the resulting sum
     * @param[in]      rhs The value to add to the @p lhs
     *
     * @return The resulting sum
     */
    friend constexpr StrongInt &operator+=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs + rhs;
        return lhs;
    }

    /**
     * @brief Subtract two integers
     *
     * If the overflow happens, the result is wrapped.
     * The overflow itself is returned as the second value.
     *
     * @retval first The (wrapped) result
     * @retval second Whether the overflow happened
     */
    friend constexpr std::pair<StrongInt, bool> sub_with_overflow(StrongInt lhs, StrongInt rhs) noexcept {
        T result;
        const auto overflow = __builtin_sub_overflow(lhs._value, rhs._value, &result);
        return { StrongInt(result), overflow };
    }

    /**
     * @brief Subtract two integers with overflow detection
     *
     * If the overflow happens, the function traps.
     * To get a wrapped result, use @link sub_with_overflow(StrongInt, StrongInt) @endlink.
     */
    friend constexpr StrongInt operator-(StrongInt lhs, StrongInt rhs) noexcept {
        const auto [result, overflow] = sub_with_overflow(lhs, rhs);
        if (overflow) panic("StrongInt: overflow in subtraction");
        return result;
    }

    /**
     * @brief Subtract an integer in-place
     *
     * If the overflow happens, the function traps.
     * To get a wrapped result, use @link sub_with_overflow(StrongInt, StrongInt) @endlink.
     *
     * @param[in, out] lhs This value will be updated with the subtraction result
     * @param[in]      rhs The value to subtract from the @p lhs
     */
    friend constexpr StrongInt &operator-=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs - rhs;
        return lhs;
    }

    /**
     * @brief Multiply two integers
     *
     * If the overflow happens, the result is wrapped.
     * The overflow itself is returned as the second value.
     *
     * @retval first The (wrapped) result
     * @retval second Whether the overflow happened
     */
    friend constexpr std::pair<StrongInt, bool> mul_with_overflow(StrongInt lhs, StrongInt rhs) noexcept {
        T result;
        const auto overflow = __builtin_mul_overflow(lhs._value, rhs._value, &result);
        return { StrongInt(result), overflow };
    }

    /**
     * @brief Multiply two integers with overflow detection
     *
     * If the overflow happens, the function traps.
     * To get a wrapped result, use @link mul_with_overflow(StrongInt, StrongInt) @endlink.
     */
    friend constexpr StrongInt operator*(StrongInt lhs, StrongInt rhs) noexcept {
        const auto [result, overflow] = mul_with_overflow(lhs, rhs);
        if (overflow) panic("StrongInt: overflow in multiplication");
        return result;
    }

    /**
     * @brief Multiply an integer in-place
     *
     * If the overflow happens, the function traps.
     * To get a wrapped result, use @link mul_with_overflow(StrongInt, StrongInt) @endlink.
     *
     * @param[in, out] lhs This value will be updated with the multiplication result
     * @param[in]      rhs The value to multiply by the @p lhs
     */
    friend constexpr StrongInt &operator*=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs * rhs;
        return lhs;
    }

    enum struct IntegerDivisionError { ZeroDivision, Overflow };

    /**
     * @brief Integer division
     *
     * @retval std::nullopt If the @p rhs is zero
     * @retval std::nullopt If the result cannot fit into the type.
     *                      It only happens in one case: when the type is signed, the @p lhs is its minimal value,
     *                      and the rhs is -1.
     *
     * @return The result of integer division
     */
    friend constexpr std::expected<StrongInt, IntegerDivisionError> div(StrongInt lhs, StrongInt rhs) noexcept {
        if (rhs._value == 0) return std::unexpected(IntegerDivisionError::ZeroDivision);
        if constexpr (std::is_signed_v<T>)
            if (rhs._value == -1 && lhs._value == std::numeric_limits<T>::min())
                return std::unexpected(IntegerDivisionError::Overflow);
        return StrongInt{ static_cast<T>(lhs._value / rhs._value) };
    }

    /**
     * @brief Integer division
     *
     * It panics in two cases:
     * - division by zero
     * - overflow (for signed types only).
     *
     * See @link div(StrongInt, StrongInt) @endlink for details.
     */
    friend constexpr StrongInt operator/(StrongInt lhs, StrongInt rhs) noexcept {
        const auto result = div(lhs, rhs);
        if (!result) {
            switch (result.error()) {
            case IntegerDivisionError::ZeroDivision: panic("StrongInt: division by zero");
            case IntegerDivisionError::Overflow: panic("StrongInt: overflow in division");
            }
            std::unreachable();
        }
        return *result;
    }

    /**
     * @brief In-place integer division
     *
     * It panics in two cases:
     * - division by zero
     * - overflow (for signed types only).
     *
     * See @link div(StrongInt, StrongInt) @endlink for details.
     */
    friend constexpr StrongInt &operator/=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs / rhs;
        return lhs;
    }

    /**
     * @brief The remainder of integer division
     *
     * @retval std::nullopt If the @p rhs is zero
     */
    friend constexpr std::optional<StrongInt> mod(StrongInt lhs, StrongInt rhs) noexcept {
        if (rhs._value == 0) return std::nullopt;
        return StrongInt{ static_cast<T>(lhs._value % rhs._value) };
    }

    /**
     * @brief The remainder of integer division
     *
     * If the divisor is zero, it panics.
     */
    friend constexpr StrongInt operator%(StrongInt lhs, StrongInt rhs) noexcept {
        const auto result = mod(lhs, rhs);
        if (!result) panic("StrongInt: division by zero");
        return *result;
    }

    /**
     * @brief In-place remainder of integer division
     *
     * If the divisor is zero, it panics.
     */
    friend constexpr StrongInt &operator%=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs % rhs;
        return lhs;
    }

    friend constexpr StrongInt operator&(StrongInt lhs, StrongInt rhs) noexcept {
        return StrongInt{ static_cast<T>(lhs._value & rhs._value) };
    }

    friend constexpr StrongInt &operator&=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs & rhs;
        return lhs;
    }

    friend constexpr StrongInt operator|(StrongInt lhs, StrongInt rhs) noexcept {
        return StrongInt{ static_cast<T>(lhs._value | rhs._value) };
    }

    friend constexpr StrongInt &operator|=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs | rhs;
        return lhs;
    }

    friend constexpr StrongInt operator^(StrongInt lhs, StrongInt rhs) noexcept {
        return StrongInt{ static_cast<T>(lhs._value ^ rhs._value) };
    }

    friend constexpr StrongInt &operator^=(StrongInt &lhs, StrongInt rhs) noexcept {
        lhs = lhs ^ rhs;
        return lhs;
    }

    friend constexpr StrongInt operator~(StrongInt sting) noexcept {
        return StrongInt{ static_cast<T>(~sting._value) };
    }

    /**
     * @brief Shift an integer left by a specified number of bits
     *
     * New bits appearing on the right are set to zero.
     * Bits shifted away to the left are discarded.
     *
     * @retval first The result of the shift
     * @retval second Overflow, which is only true if any discarded bit was set
     */
    friend constexpr std::pair<StrongInt, bool> shift_left(StrongInt sting, size_t shift) noexcept {
        constexpr size_t N_BITS = 8 * sizeof(T);
        if (shift > N_BITS) return { StrongInt(0), sting._value != 0 };
        T mask = 0;
        for (size_t i = N_BITS - shift; i < N_BITS; ++i) mask |= static_cast<T>(T{ 1 } << i);
        return { StrongInt(static_cast<T>(sting._value << shift)), sting._value & mask };
    }

    /**
     * @brief Shift an integer left by a specified number of bits
     *
     * Panics in case of an overflow.
     * For details, see @link shift_left(StrongInt, size_t) @endlink.
     */
    friend constexpr StrongInt operator<<(StrongInt sting, size_t shift) noexcept {
        const auto [result, overflow] = shift_left(sting, shift);
        if (overflow) panic("StrongInt: overflow in left shift");
        return result;
    }

    /**
     * @brief Shift an integer left in-place by a specified number of bits
     *
     * Panics in case of an overflow.
     * For details, see @link shift_left(StrongInt, size_t) @endlink.
     */
    friend constexpr StrongInt &operator<<=(StrongInt &sting, size_t shift) noexcept {
        sting = sting << shift;
        return sting;
    }

    /**
     * @brief Shift an integer right by a specified number of bits
     *
     * New bits appearing on the left are set to zero.
     * Bits shifted away to the right are discarded.
     * If the value is signed, the sign bit remains intact.
     */
    friend constexpr StrongInt operator>>(StrongInt sting, size_t shift) noexcept {
        return StrongInt{ static_cast<T>(sting._value >> shift) };
    }

    /**
     * @brief Shift an integer right in-place by a specified number of bits
     *
     * New bits appearing on the left are set to zero.
     * Bits shifted away to the right are discarded.
     * If the value is signed, the sign bit remains intact.
     */
    friend constexpr StrongInt &operator>>=(StrongInt &sting, size_t shift) noexcept {
        sting = sting >> shift;
        return sting;
    }

    /**
     * Has no effect
     */
    friend constexpr StrongInt operator+(StrongInt sting) noexcept { return sting; }

    /**
     * @brief Change the sign of an integer
     *
     * May overflow if the integer is the minimal possible value, for example, -128 for @c int8_t.
     *
     * @retval std::nullopt Only in case of overflow.
     */
    friend constexpr std::optional<StrongInt> negate(StrongInt sting) noexcept
        requires std::is_signed_v<T>
    {
        if (sting._value == std::numeric_limits<T>::min()) return std::nullopt;
        return StrongInt{ static_cast<T>(-sting._value) };
    }

    /**
     * @copybrief negate
     *
     * @copydetails negate
     *
     * Panics in case of overflow
     */
    friend constexpr StrongInt operator-(StrongInt sting) noexcept
        requires std::is_signed_v<T>
    {
        const auto result = negate(sting);
        if (!result) panic("StrongInt: overflow in negation");
        return *result;
    }

    T _value = 0;
};
} // namespace mtl

template <std::integral T> struct std::formatter<mtl::StrongInt<T>> : std::formatter<T> {
    constexpr auto format(const mtl::StrongInt<T> sting, std::format_context &ctx) const noexcept {
        return std::formatter<T>::format(sting.to_underlying(), ctx);
    }
};

#endif //EMULATOR_STRONG_TYPES_HPP
