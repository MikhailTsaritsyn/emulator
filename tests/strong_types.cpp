//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#include "strong_types.hpp"

#include <format>
#include <gtest/gtest.h>

namespace mtl::test {
TEST(StrongTypes, Test) {
    constexpr u8 x{ 10 };
    std::cout << u16(x).to_underlying() << std::endl;
    std::cout << i16(x).to_underlying() << std::endl;

    constexpr i8 y{ -10 };
    std::cout << i16(y).to_underlying() << std::endl;
    // std::cout << u16(y).to_underlying() << std::endl; // does not compile

    // u8(u16(1000)); // does not compile
    // u8(i16(1000)); // does not compile
    // i8(i16(1000)); // does not compile
    EXPECT_EQ(u16(1000).wrap<uint8_t>().to_underlying(), 232);
    EXPECT_EQ(i16(1000).wrap<uint8_t>().to_underlying(), 232);
    EXPECT_EQ(i16(1000).wrap<int8_t>().to_underlying(), -24);
}
} // namespace mtl::test
