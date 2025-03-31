//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, uint8_t, bool, uint8_t, bool>;

struct DecimalAddition : testing::TestWithParam<TestParameters> {
    StatusRegister sr;

    void SetUp() override {
        sr = { .negative  = false,
               .overflow  = false,
               .break_    = false,
               .decimal   = true,
               .interrupt = false,
               .zero      = false,
               .carry     = false };
    }
};

TEST_P(DecimalAddition, TestNegative) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    EXPECT_EQ(sr.negative, static_cast<int8_t>(result) < 0) << "result = " << static_cast<int>(result);
}

TEST_P(DecimalAddition, TestOverflow) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    EXPECT_EQ(sr.overflow, (static_cast<int8_t>(a) < 0) != (static_cast<int8_t>(result) < 0))
            << static_cast<int>(a) << " -> " << static_cast<int>(result);
}

TEST_P(DecimalAddition, TestBreak) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    EXPECT_FALSE(sr.break_);
}

TEST_P(DecimalAddition, TestDecimal) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    EXPECT_TRUE(sr.decimal);
}

TEST_P(DecimalAddition, TestInterrupt) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    EXPECT_FALSE(sr.interrupt);
}

TEST_P(DecimalAddition, TestZero) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    EXPECT_EQ(sr.zero, result == 0);
}

TEST_P(DecimalAddition, TestCarry) {
    auto [a, b, carry, result, carry_after] = GetParam();
    sr.carry                                = carry;
    EXPECT_EQ(ALU::add(a, b, sr), result);
    const auto result_unwrapped = static_cast<int>(a) + static_cast<int>(b) + (carry ? 1 : 0);
    EXPECT_EQ(sr.carry, carry_after) << "result = " << std::hex << static_cast<int>(result) << '(' << result_unwrapped
                                     << ')' << std::dec;
}

INSTANTIATE_TEST_SUITE_P(Zero,
                         DecimalAddition,
                         ::testing::Values(TestParameters{ 0, 0, false, 0, false },
                                           TestParameters{ 0x99, 0x01, false, 0, true },
                                           TestParameters{ 0x01, 0x99, false, 0, true },
                                           TestParameters{ 0x50, 0x50, false, 0, true },
                                           TestParameters{ 0x99, 0x00, true, 0, true },
                                           TestParameters{ 0x00, 0x99, true, 0, true },
                                           TestParameters{ 0x19, 0x80, true, 0, true }));

INSTANTIATE_TEST_SUITE_P(Overflow,
                         DecimalAddition,
                         ::testing::Values(TestParameters{ 0x99, 0x02, false, 0x1, true },
                                           TestParameters{ 0x99, 0x01, true, 0x1, true },
                                           TestParameters{ 0x02, 0x99, false, 0x1, true },
                                           TestParameters{ 0x01, 0x99, true, 0x1, true },
                                           TestParameters{ 0x99, 0x99, false, 0x98, true },
                                           TestParameters{ 0x99, 0x99, true, 0x99, true },
                                           TestParameters{ 0x60, 0x60, false, 0x20, true },
                                           TestParameters{ 0x60, 0x60, true, 0x21, true }));

INSTANTIATE_TEST_SUITE_P(NoOverflow,
                         DecimalAddition,
                         ::testing::Values(TestParameters{ 0x00, 0x00, true, 0x01, false },
                                           TestParameters{ 0x01, 0x00, false, 0x01, false },
                                           TestParameters{ 0x00, 0x01, false, 0x01, false },
                                           TestParameters{ 0x09, 0x01, false, 0x10, false },
                                           TestParameters{ 0x09, 0x00, true, 0x10, false },
                                           TestParameters{ 0x01, 0x09, false, 0x10, false },
                                           TestParameters{ 0x00, 0x09, true, 0x10, false },
                                           TestParameters{ 0x10, 0x20, true, 0x31, false }));
} // namespace emulator::mos_6502::test