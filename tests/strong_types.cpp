//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#include "strong_types.hpp"

#include <format>
#include <gtest/gtest.h>

namespace mtl::test {
TEST(StrongInt, SafeCasts) {
    EXPECT_EQ(u16(u8(100)).to_underlying(), 100);
    EXPECT_EQ(i16(u8(100)).to_underlying(), 100);
    EXPECT_EQ(i16(i8(100)).to_underlying(), 100);
}

TEST(StrongInt, NarrowingCasts) {
    // u8(u16(1000)); // does not compile
    // u8(i16(1000)); // does not compile
    // i8(i16(1000)); // does not compile
    EXPECT_EQ(u16(1000).wrap<uint8_t>().to_underlying(), 232);
    EXPECT_EQ(i16(1000).wrap<uint8_t>().to_underlying(), 232);
    EXPECT_EQ(i16(1000).wrap<int8_t>().to_underlying(), -24);

    EXPECT_FALSE(u16(1000).narrow<uint8_t>());
    EXPECT_FALSE(i16(1000).narrow<uint8_t>());
    EXPECT_FALSE(i16(1000).narrow<int8_t>());
    EXPECT_EQ(u16(100).narrow<uint8_t>()->to_underlying(), 100);
    EXPECT_EQ(i16(100).narrow<uint8_t>()->to_underlying(), 100);
    EXPECT_EQ(i16(100).narrow<int8_t>()->to_underlying(), 100);

    EXPECT_THROW((void)u16(1000).unsafe_cast<uint8_t>(), std::overflow_error);
    EXPECT_THROW((void)i16(1000).unsafe_cast<uint8_t>(), std::overflow_error);
    EXPECT_THROW((void)i16(1000).unsafe_cast<int8_t>(), std::overflow_error);
    EXPECT_THROW((void)i16(-1000).unsafe_cast<uint8_t>(), std::underflow_error);
    EXPECT_THROW((void)i16(-1000).unsafe_cast<int8_t>(), std::underflow_error);
    EXPECT_EQ(u16(100).unsafe_cast<uint8_t>().to_underlying(), 100);
    EXPECT_EQ(i16(100).unsafe_cast<uint8_t>().to_underlying(), 100);
    EXPECT_EQ(i16(100).unsafe_cast<int8_t>().to_underlying(), 100);
}
} // namespace mtl::test
