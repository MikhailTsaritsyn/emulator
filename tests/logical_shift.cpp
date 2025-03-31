//
// Created by Mikhail Tsaritsyn on Mar 31, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, uint8_t, bool>;

struct LogicalShift : testing::TestWithParam<TestParameters> {
    StatusRegister sr;

    void SetUp() override {
        sr = { .negative  = false,
               .overflow  = false,
               .break_    = false,
               .decimal   = false,
               .interrupt = false,
               .zero      = false,
               .carry     = false };
    }
};

TEST_P(LogicalShift, ValueAndFlags) {
    const auto [a, result, carry] = GetParam();
    EXPECT_EQ(ALU::shift_right(a, sr), result);

    EXPECT_EQ(sr.negative, static_cast<int8_t>(result) < 0);
    EXPECT_FALSE(sr.overflow);
    EXPECT_FALSE(sr.break_);
    EXPECT_FALSE(sr.decimal);
    EXPECT_FALSE(sr.interrupt);
    EXPECT_EQ(sr.zero, result == 0);
    EXPECT_EQ(sr.carry, a & 1);
}

INSTANTIATE_TEST_SUITE_P(Right,
                         LogicalShift,
                         ::testing::Values(TestParameters{ 0b00000000, 0b00000000, false },
                                           TestParameters{ 0b00000001, 0b00000000, true },
                                           TestParameters{ 0b00000010, 0b00000001, false },
                                           TestParameters{ 0b00000011, 0b00000001, true },
                                           TestParameters{ 0b01010101, 0b00101010, true },
                                           TestParameters{ 0b10101010, 0b01010101, false },
                                           TestParameters{ 0b10000000, 0b01000000, false },
                                           TestParameters{ 0b11001101, 0b01100110, true },
                                           TestParameters{ 0b11111111, 0b01111111, true }));
} // namespace emulator::mos_6502::test
