//
// Created by Mikhail Tsaritsyn on Mar 30, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, uint8_t, bool, uint8_t, bool>;

struct DecimalSubtraction : testing::TestWithParam<TestParameters> {
    StatusRegister sr;

    void SetUp() override {
        sr = { .negative  = false,
               .overflow  = false,
               .break_    = false,
               .decimal   = true,
               .interrupt = false,
               .zero      = false,
               .carry     = true };
    }
};

TEST_P(DecimalSubtraction, TestNegative) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    EXPECT_EQ(sr.negative, static_cast<int8_t>(result) < 0) << "result = " << static_cast<int>(result);
}

TEST_P(DecimalSubtraction, TestOverflow) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    EXPECT_EQ(sr.overflow, (static_cast<int8_t>(a) < 0) != (static_cast<int8_t>(result) < 0))
            << static_cast<int>(a) << " -> " << static_cast<int>(result);
}

TEST_P(DecimalSubtraction, TestBreak) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    EXPECT_FALSE(sr.break_);
}

TEST_P(DecimalSubtraction, TestDecimal) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    EXPECT_TRUE(sr.decimal);
}

TEST_P(DecimalSubtraction, TestInterrupt) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    EXPECT_FALSE(sr.interrupt);
}

TEST_P(DecimalSubtraction, TestZero) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    EXPECT_EQ(sr.zero, result == 0);
}

TEST_P(DecimalSubtraction, TestBorrow) {
    auto [a, b, borrow, result, borrow_after] = GetParam();
    sr.carry                                  = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), result);
    const auto result_unwrapped = static_cast<int>(a) - static_cast<int>(b) - (borrow ? 1 : 0);
    EXPECT_EQ(sr.carry, !borrow_after) << "result = " << std::hex << static_cast<int>(result) << '(' << result_unwrapped
                                       << ')' << std::dec;
}

INSTANTIATE_TEST_SUITE_P(Zero,
                         DecimalSubtraction,
                         ::testing::Values(TestParameters{ 0, 0, false, 0, false },
                                           TestParameters{ 0x01, 0x01, false, 0, false },
                                           TestParameters{ 0x01, 0x00, true, 0, false },
                                           TestParameters{ 0x50, 0x50, false, 0, false },
                                           TestParameters{ 0x99, 0x99, false, 0, false },
                                           TestParameters{ 0x99, 0x98, true, 0, false },
                                           TestParameters{ 0x0, 0x99, true, 0, true }));

INSTANTIATE_TEST_SUITE_P(Overflow,
                         DecimalSubtraction,
                         ::testing::Values(TestParameters{ 0x00, 0x01, false, 0x99, true },
                                           TestParameters{ 0x00, 0x00, true, 0x99, true },
                                           TestParameters{ 0x01, 0x02, false, 0x99, true },
                                           TestParameters{ 0x01, 0x01, true, 0x99, true },
                                           TestParameters{ 0x98, 0x99, false, 0x99, true },
                                           TestParameters{ 0x99, 0x99, true, 0x99, true },
                                           TestParameters{ 0x10, 0x20, false, 0x90, true },
                                           TestParameters{ 0x10, 0x90, false, 0x20, true }));

INSTANTIATE_TEST_SUITE_P(NoOverflow,
                         DecimalSubtraction,
                         ::testing::Values(TestParameters{ 0x02, 0x01, false, 0x01, false },
                                           TestParameters{ 0x01, 0x00, false, 0x01, false },
                                           TestParameters{ 0x10, 0x01, false, 0x09, false },
                                           TestParameters{ 0x10, 0x00, true, 0x09, false },
                                           TestParameters{ 0x99, 0x90, false, 0x09, false },
                                           TestParameters{ 0x11, 0x02, false, 0x09, false },
                                           TestParameters{ 0x21, 0x12, true, 0x08, false },
                                           TestParameters{ 0x99, 0x98, false, 0x01, false }));
} // namespace emulator::mos_6502::test
