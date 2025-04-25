//
// Created by Mikhail Tsaritsyn on Apr 25, 2025.
//

#ifndef TSARITSYN_STING_CORE_HPP
#define TSARITSYN_STING_CORE_HPP
#include "StrongInt.hpp"

namespace mtl {
/**
 * @brief 8-bit unsigned integer
 */
using u8 = StrongInt<uint8_t>;

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
using i8 = StrongInt<int8_t>;

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

#endif //TSARITSYN_STING_CORE_HPP
