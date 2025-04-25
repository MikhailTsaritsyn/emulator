//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#ifndef EMULATOR_STRONG_TYPES_HPP
#define EMULATOR_STRONG_TYPES_HPP
#include "panic.hpp"
#include <concepts>
#include <cstdint>
#include <format>
#include <ostream>

// TODO: comparison between different types? (with unsafe_cast'ing)
// TODO: comparison to built-in types?
// TODO: strong float and double

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

private:
    friend std::ostream &operator<<(std::ostream &os, const StrongInt sting) noexcept {
        if constexpr (sizeof(T) == 1) // avoid printing as characters
            return os << static_cast<int>(sting.to_underlying());
        return os << sting._value;
    }

    // TODO: all operators from https://en.cppreference.com/w/cpp/language/operator_incdec

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

    T _value = 0;
};
} // namespace mtl

template <std::integral T> struct std::formatter<mtl::StrongInt<T>> : std::formatter<T> {
    constexpr auto format(const mtl::StrongInt<T> sting, std::format_context &ctx) const noexcept {
        return std::formatter<T>::format(sting.to_underlying(), ctx);
    }
};

#endif //EMULATOR_STRONG_TYPES_HPP
