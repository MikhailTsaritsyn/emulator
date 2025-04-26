//
// Created by Mikhail Tsaritsyn on Apr 23, 2025.
//

#include "mtl/core.hpp"
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
    EXPECT_EQ(u16(150) + u16(230), u16(380));

    EXPECT_DEATH(i8(100) + i8(100), "");

    {
        const auto [result, overflow] = add_with_overflow(i8(100), i8(100));
        EXPECT_TRUE(overflow);
        EXPECT_EQ(result, i8(-56));
    }

    {
        const auto [result, overflow] = add_with_overflow(u8(200), u8(100));
        EXPECT_TRUE(overflow);
        EXPECT_EQ(result, u8(44));
    }
}

TEST(StrongInt, Subtraction) {
    EXPECT_EQ(u16(300) - u16(120), u16(180));
    EXPECT_DEATH(u16(300) - u16(400), "");

    {
        const auto [result, overflow] = sub_with_overflow(i8(100), i8(100));
        EXPECT_FALSE(overflow);
        EXPECT_EQ(result, i8(0));
    }

    {
        const auto [result, overflow] = sub_with_overflow(i8(50), i8(100));
        EXPECT_FALSE(overflow);
        EXPECT_EQ(result, i8(-50));
    }

    {
        const auto [result, overflow] = sub_with_overflow(i8(-50), i8(100));
        EXPECT_TRUE(overflow);
        EXPECT_EQ(result, i8(106));
    }

    {
        const auto [result, overflow] = sub_with_overflow(u8(100), u8(200));
        EXPECT_TRUE(overflow);
        EXPECT_EQ(result, u8(156));
    }
}

TEST(StrongInt, Multiplication) {
    EXPECT_EQ(u8(10) * u8(10), u8(100));
    EXPECT_DEATH(u8(100) * u8(100), "");

    EXPECT_EQ(u16(100) * u16(100), u16(10000));

    EXPECT_EQ(i8(10) * i8(-10), i8(-100));
    EXPECT_EQ(i8(127) * i8(-1), i8(-127));
    EXPECT_DEATH(i8(-128) * i8(-1), "");
}

TEST(StrongInt, Division) {
    EXPECT_FALSE(div(u8(100), u8(0)));
    EXPECT_EQ(div(u8(100), u8(25)), u8(4));
    EXPECT_EQ(div(u8(100), u8(30)), u8(3));

    EXPECT_FALSE(div(i8(100), i8(0)));
    EXPECT_FALSE(div(i8(-128), i8(-1)));
    EXPECT_EQ(div(i8(10), i8(-1)), i8(-10));
    EXPECT_EQ(div(i8(-10), i8(-1)), i8(10));
    EXPECT_EQ(div(i8(127), i8(-1)), i8(-127));
    EXPECT_EQ(div(i8(-127), i8(-1)), i8(127));

    EXPECT_DEATH(u8(100) / u8(0), "");
    EXPECT_DEATH(u16(100) / u16(0), "");
    EXPECT_DEATH(i16(100) / i16(0), "");
    EXPECT_DEATH(i16(std::numeric_limits<int16_t>::min()) / i16(-1), "");

    EXPECT_EQ(u16(2000) / u16(1000), u16(2));
    EXPECT_EQ(u16(1000) / u16(1000), u16(1));
    EXPECT_EQ(u16(500) / u16(1000), u16(0));
    EXPECT_EQ(u16(500) / u16(200), u16(2));

    EXPECT_EQ(i16(1000) / i16(-10), i16(-100));
    EXPECT_EQ(i16(-1000) / i16(10), i16(-100));
    EXPECT_EQ(i16(100) / i16(-100), i16(-1));
    EXPECT_EQ(i16(-100) / i16(100), i16(-1));
    EXPECT_EQ(i16(100) / i16(-1000), i16(0));
    EXPECT_EQ(i16(-100) / i16(1000), i16(0));

    EXPECT_EQ(i16(100) / i16(30), i16(3));
    EXPECT_EQ(i16(100) / i16(-30), i16(-3));
    EXPECT_EQ(i16(-100) / i16(30), i16(-3));
    EXPECT_EQ(i16(-100) / i16(-30), i16(3));

    EXPECT_EQ(u64(0xffffffffffffffffULL) / u64(2), u64(0x7fffffffffffffffULL));
}

TEST(StrongInt, Remainder) {
    EXPECT_FALSE(mod(u8(100), u8(0)));
    EXPECT_EQ(mod(u8(100), u8(25)), u8(0));
    EXPECT_EQ(mod(u8(100), u8(30)), u8(10));

    EXPECT_FALSE(mod(i8(100), i8(0)));
    EXPECT_EQ(mod(i8(-128), i8(-1)), i8(0));
    EXPECT_EQ(mod(i8(10), i8(-1)), i8(0));
    EXPECT_EQ(mod(i8(-10), i8(-1)), i8(0));
    EXPECT_EQ(mod(i8(127), i8(-1)), i8(0));
    EXPECT_EQ(mod(i8(-127), i8(-1)), i8(0));

    EXPECT_DEATH(u8(100) % u8(0), "");
    EXPECT_DEATH(u16(100) % u16(0), "");
    EXPECT_DEATH(i16(100) % i16(0), "");
    EXPECT_EQ(i16(std::numeric_limits<int16_t>::min()) % i16(-1), i16(0));

    EXPECT_EQ(u16(2000) % u16(1000), u16(0));
    EXPECT_EQ(u16(1000) % u16(1000), u16(0));
    EXPECT_EQ(u16(500) % u16(1000), u16(500));
    EXPECT_EQ(u16(500) % u16(200), u16(100));

    EXPECT_EQ(i16(999) % i16(-10), i16(9));
    EXPECT_EQ(i16(-999) % i16(10), i16(-9));
    EXPECT_EQ(i16(100) % i16(-100), i16(0));
    EXPECT_EQ(i16(-100) % i16(100), i16(0));
    EXPECT_EQ(i16(100) % i16(-1000), i16(100));
    EXPECT_EQ(i16(-100) % i16(1000), i16(-100));

    EXPECT_EQ(i16(100) % i16(30), i16(10));
    EXPECT_EQ(i16(100) % i16(-30), i16(10));
    EXPECT_EQ(i16(-100) % i16(30), i16(-10));
    EXPECT_EQ(i16(-100) % i16(-30), i16(-10));

    EXPECT_EQ(u64(0xffffffffffffffffULL) % u64(2), u64(1));
}

TEST(StrongInt, Logical) {
    EXPECT_EQ(u8(0b10101010) & u8(0b11001100), u8(0b10001000));
    EXPECT_EQ(i8(-86) & i8(-52), i8(-120)); // the same, but in decimal to avoid conversions

    EXPECT_EQ(u8(0b10101010) | u8(0b11001100), u8(0b11101110));
    EXPECT_EQ(i8(-86) | i8(-52), i8(-18)); // the same, but in decimal to avoid conversions

    EXPECT_EQ(u8(0b10101010) ^ u8(0b11001100), u8(0b01100110));
    EXPECT_EQ(i8(-86) ^ i8(-52), i8(102)); // the same, but in decimal to avoid conversions

    EXPECT_EQ(~u8(0b01001110), u8(0b10110001));
    EXPECT_EQ(~i8(78), i8(-79)); // the same, but in decimal to avoid conversions
}

TEST(StrongInt, LeftShift) {
    EXPECT_EQ(u8(0b00000001) << 0, u8(0b00000001));
    EXPECT_EQ(u8(0b00000001) << 1, u8(0b00000010));
    EXPECT_EQ(u8(0b00000001) << 2, u8(0b00000100));
    EXPECT_EQ(u8(0b00000001) << 3, u8(0b00001000));
    EXPECT_EQ(u8(0b00000001) << 4, u8(0b00010000));
    EXPECT_EQ(u8(0b00000001) << 5, u8(0b00100000));
    EXPECT_EQ(u8(0b00000001) << 6, u8(0b01000000));
    EXPECT_EQ(u8(0b00000001) << 7, u8(0b10000000));
    EXPECT_DEATH(u8(0b00000001) << 8, "");
    EXPECT_EQ(u8(0) << 8, u8(0));

    EXPECT_EQ(i8(0b00000001) << 0, i8(0b00000001));
    EXPECT_EQ(i8(0b00000001) << 1, i8(0b00000010));
    EXPECT_EQ(i8(0b00000001) << 2, i8(0b00000100));
    EXPECT_EQ(i8(0b00000001) << 3, i8(0b00001000));
    EXPECT_EQ(i8(0b00000001) << 4, i8(0b00010000));
    EXPECT_EQ(i8(0b00000001) << 5, i8(0b00100000));
    EXPECT_EQ(i8(0b00000001) << 6, i8(0b01000000));
    EXPECT_EQ(i8(0b00000001) << 7, i8(-128));
    EXPECT_DEATH(i8(0b00000001) << 8, "");
    EXPECT_EQ(i8(0) << 8, i8(0));

    // 0b10000001 << 1 = 0b0000010, sign bit goes away
    EXPECT_EQ(shift_left(i8(-127), 1).first, i8(0b00000010));

    EXPECT_EQ(u64(0x4000000000000000ULL) << 1, u64(0x8000000000000000ULL));
}

TEST(StrongInt, RightShift) {
    EXPECT_EQ(u8(0b10000000) >> 0, u8(0b10000000));
    EXPECT_EQ(u8(0b10000000) >> 1, u8(0b01000000));
    EXPECT_EQ(u8(0b10000000) >> 2, u8(0b00100000));
    EXPECT_EQ(u8(0b10000000) >> 3, u8(0b00010000));
    EXPECT_EQ(u8(0b10000000) >> 4, u8(0b00001000));
    EXPECT_EQ(u8(0b10000000) >> 5, u8(0b00000100));
    EXPECT_EQ(u8(0b10000000) >> 6, u8(0b00000010));
    EXPECT_EQ(u8(0b10000000) >> 7, u8(0b00000001));
    EXPECT_EQ(u8(0b10000000) >> 8, u8(0b00000000));

    EXPECT_EQ(i8(0b01000000) >> 0, i8(0b01000000));
    EXPECT_EQ(i8(0b01000000) >> 1, i8(0b00100000));
    EXPECT_EQ(i8(0b01000000) >> 2, i8(0b00010000));
    EXPECT_EQ(i8(0b01000000) >> 3, i8(0b00001000));
    EXPECT_EQ(i8(0b01000000) >> 4, i8(0b00000100));
    EXPECT_EQ(i8(0b01000000) >> 5, i8(0b00000010));
    EXPECT_EQ(i8(0b01000000) >> 6, i8(0b00000001));
    EXPECT_EQ(i8(0b01000000) >> 7, i8(0b00000000));

    EXPECT_EQ(i8(-128) >> 0, i8(-128));
    EXPECT_EQ(i8(-128) >> 1, i8(-64));
    EXPECT_EQ(i8(-128) >> 2, i8(-32));
    EXPECT_EQ(i8(-128) >> 3, i8(-16));
    EXPECT_EQ(i8(-128) >> 4, i8(-8));
    EXPECT_EQ(i8(-128) >> 5, i8(-4));
    EXPECT_EQ(i8(-128) >> 6, i8(-2));
    EXPECT_EQ(i8(-128) >> 7, i8(-1));
    EXPECT_EQ(i8(-128) >> 8, i8(-1));
    EXPECT_EQ(i8(-128) >> 9, i8(-1));
}

TEST(StrongInt, Negation) {
    EXPECT_FALSE(negate(i8(-128)));
    EXPECT_DEATH(-i8(-128), "");

    EXPECT_EQ(*negate(i8(127)), i8(-127));
    EXPECT_EQ(-i8(127), i8(-127));

    EXPECT_EQ(-i64(std::numeric_limits<int64_t>::max()), i64(std::numeric_limits<int64_t>::min() + 1));

    // -u8(100); // does not compile
}

TEST(StrongInt, Increment) {
    EXPECT_FALSE(inc(u8(255)));
    EXPECT_EQ(inc(u8(0)), u8(1));
    {
        u8 n(255);
        EXPECT_DEATH(n++, "");
    }
    {
        u16 n(0);
        n++;
        EXPECT_EQ(n, u16(1));
    }
    {
        u8 n(255);
        EXPECT_DEATH(++n, "");
    }
    {
        u16 n(0);
        EXPECT_EQ(++n, u16(1));
    }

    EXPECT_FALSE(inc(i8(127)));
    EXPECT_EQ(inc(i8(0)), i8(1));
    {
        i8 n(127);
        EXPECT_DEATH(n++, "");
    }
    {
        i16 n(0);
        n++;
        EXPECT_EQ(n, i16(1));
    }
    {
        i8 n(127);
        EXPECT_DEATH(++n, "");
    }
    {
        i16 n(0);
        EXPECT_EQ(++n, i16(1));
    }

    {
        StrongInt n(std::numeric_limits<size_t>::max());
        EXPECT_DEATH(n++, "");
    }
    {
        StrongInt n(std::numeric_limits<size_t>::max());
        EXPECT_DEATH(++n, "");
    }
    {
        StrongInt<size_t> n(std::numeric_limits<size_t>::max() - 1);
        EXPECT_EQ(++n, StrongInt(std::numeric_limits<size_t>::max()));
    }
}
} // namespace mtl::test
