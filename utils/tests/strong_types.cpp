//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#include "mtl/strong_types.hpp"

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
}

TEST(StrongInt, UnsafeCastNarrowing) {
    EXPECT_THROW((void)u16(1000).unsafe_cast<uint8_t>(), std::overflow_error);
    EXPECT_EQ(u16(100).unsafe_cast<uint8_t>().to_underlying(), 100);

    EXPECT_THROW((void)i16(1000).unsafe_cast<uint8_t>(), std::overflow_error);
    EXPECT_THROW((void)i16(-100).unsafe_cast<uint8_t>(), std::underflow_error);
    EXPECT_EQ(i16(100).unsafe_cast<uint8_t>().to_underlying(), 100);

    EXPECT_THROW((void)i16(1000).unsafe_cast<int8_t>(), std::overflow_error);
    EXPECT_THROW((void)i16(-1000).unsafe_cast<int8_t>(), std::underflow_error);
    EXPECT_EQ(i16(100).unsafe_cast<int8_t>().to_underlying(), 100);
    EXPECT_EQ(i16(-100).unsafe_cast<int8_t>().to_underlying(), -100);
}

TEST(StrongInt, UnsafeCastTrivial) {
    EXPECT_EQ(u16(100).unsafe_cast<uint16_t>().to_underlying(), 100);
    EXPECT_EQ(u16(100).unsafe_cast<uint32_t>().to_underlying(), 100);

    EXPECT_EQ(u16(100).unsafe_cast<int32_t>().to_underlying(), 100);

    EXPECT_EQ(i16(100).unsafe_cast<int16_t>().to_underlying(), 100);
    EXPECT_EQ(i16(100).unsafe_cast<int32_t>().to_underlying(), 100);
}

TEST(StrongInt, UnsafeCastOther) {
    EXPECT_THROW((void)u16(1000).unsafe_cast<int8_t>(), std::overflow_error);
    EXPECT_EQ(u16(100).unsafe_cast<int8_t>().to_underlying(), 100);

    EXPECT_THROW((void)u16(0x9000).unsafe_cast<int8_t>(), std::overflow_error);
    EXPECT_EQ(u16(100).unsafe_cast<int16_t>().to_underlying(), 100);

    EXPECT_THROW((void)i16(-100).unsafe_cast<uint16_t>(), std::underflow_error);
    EXPECT_EQ(i16(100).unsafe_cast<uint16_t>().to_underlying(), 100);

    EXPECT_THROW((void)i16(-100).unsafe_cast<uint32_t>(), std::underflow_error);
    EXPECT_EQ(i16(100).unsafe_cast<uint32_t>().to_underlying(), 100);
}

TEST(StrongInt, Print) {
    std::stringstream ss;
    ss << i16(100);
    EXPECT_EQ(ss.str(), "100");

    EXPECT_EQ(std::format("{}", i16(100)), "100");
    EXPECT_EQ(std::format("{:#04x}", u16(100)), "0x64");
}

TEST(StrongInt, Comparison) {
    EXPECT_LT(u16(100), u16(200));
    EXPECT_GT(u16(200), u16(100));
    EXPECT_EQ(u16(100), u16(100));

    EXPECT_LT(i16(100), i16(200));
    EXPECT_GT(i16(200), i16(100));
    EXPECT_EQ(i16(100), i16(100));
    EXPECT_LT(i16(-200), i16(-100));
    EXPECT_GT(i16(-100), i16(-200));
    EXPECT_EQ(i16(-100), i16(-100));
    EXPECT_LT(i16(-200), i16(0));
    EXPECT_GT(i16(0), i16(-200));
    EXPECT_EQ(i16(0), i16(0));
}

TEST(StrongInt, Addition) {
    EXPECT_EQ((u16(150) + u16(230)).to_underlying(), 380);

    EXPECT_DEATH(i8(100) + i8(100), "");

    {
        const auto [result, overflow] = add_with_overflow(i8(100), i8(100));
        EXPECT_TRUE(overflow);
        EXPECT_EQ(result.to_underlying(), -56);
    }

    {
        const auto [result, overflow] = add_with_overflow(u8(200), u8(100));
        EXPECT_TRUE(overflow);
        EXPECT_EQ(result.to_underlying(), 44);
    }
}
} // namespace mtl::test
