//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#ifndef EMULATOR_STRONG_TYPES_HPP
#define EMULATOR_STRONG_TYPES_HPP
#include "helpers.hpp"
#include <concepts>
#include <cstdint>
#include <format>
#include <ostream>

// TODO: split signed and unsigned?
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

    // TODO: Narrowing conversions

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
     */
    // TODO: optimize
    template <std::integral U>
        requires Contains<T, U>
    [[nodiscard]] constexpr StrongInt<U> wrap() const noexcept {
        if constexpr (std::is_unsigned_v<T> && std::is_unsigned_v<U>) {
            return StrongInt<U>(static_cast<U>(_value));
        } else { // narrowing casts for signed integers are undefined, have to reimplement it
            T value       = _value;
            const T range = static_cast<T>(std::numeric_limits<U>::max())
                            - static_cast<T>(std::numeric_limits<U>::min()) + T{ 1 };
            while (value < std::numeric_limits<U>::min()) value += range;
            while (value > std::numeric_limits<U>::max()) value -= range;
            return StrongInt<U>(static_cast<U>(value));
        }
    }

private:
    // TODO: Implement formatter for std::format
    // TODO: stream operator
    // TODO: all operators from https://en.cppreference.com/w/cpp/language/operator_incdec

    T _value = 0;
};

/**
 * @brief 8-bit unsigned integer
 */
using u8  = StrongInt<uint8_t>;

/**
 * @brief 16-bit unsigned integer
 */
using u16 = StrongInt<uint16_t>;

/**
 * @brief 32-bit unsigned integer
 */
using u32 = StrongInt<uint32_t>;

/**
 * @brief 64-bit unsigned integer
 */
using u64 = StrongInt<uint64_t>;

/**
 * @brief 8-bit signed integer
 */
using i8  = StrongInt<int8_t>;

/**
 * @brief 16-bit signed integer
 */
using i16 = StrongInt<int16_t>;

/**
 * @brief 32-bit signed integer
 */
using i32 = StrongInt<int32_t>;

/**
 * @brief 64-bit signed integer
 */
using i64 = StrongInt<int64_t>;

} // namespace mtl

#endif //EMULATOR_STRONG_TYPES_HPP
