//
// Created by Mikhail Tsaritsyn on Mar 31, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, bool, uint8_t>;

// TODO: Move all initialization to the SetUp()
// TODO: Move the operation itself to the SetUp() and store the result in a field?

struct BitManip : testing::TestWithParam<TestParameters> {
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

struct ShiftRight : BitManip {};

TEST_P(ShiftRight, ValueAndFlags) {
    const auto [a, carry, result] = GetParam();
    sr.carry                      = carry;
    EXPECT_EQ(ALU::shift_right(a, sr), result);

    EXPECT_EQ(sr.negative, static_cast<int8_t>(result) < 0);
    EXPECT_FALSE(sr.overflow);
    EXPECT_FALSE(sr.break_);
    EXPECT_FALSE(sr.decimal);
    EXPECT_FALSE(sr.interrupt);
    EXPECT_EQ(sr.zero, result == 0);
    EXPECT_EQ(sr.carry, a & 1);
}

INSTANTIATE_TEST_SUITE_P(Values,
                         ShiftRight,
                         ::testing::Values(TestParameters{ 0b00000000, true, 0b00000000 },
                                           TestParameters{ 0b00000001, true, 0b00000000 },
                                           TestParameters{ 0b00000010, true, 0b00000001 },
                                           TestParameters{ 0b00000011, true, 0b00000001 },
                                           TestParameters{ 0b01010101, true, 0b00101010 },
                                           TestParameters{ 0b10101010, true, 0b01010101 },
                                           TestParameters{ 0b10000000, true, 0b01000000 },
                                           TestParameters{ 0b11001101, true, 0b01100110 },
                                           TestParameters{ 0b11111111, true, 0b01111111 }));

struct ShiftLeft : BitManip {};

TEST_P(ShiftLeft, ValueAndFlags) {
    const auto [a, carry, result] = GetParam();
    sr.carry = carry;
    EXPECT_EQ(ALU::shift_left(a, sr), result);

    EXPECT_EQ(sr.negative, static_cast<int8_t>(result) < 0);
    EXPECT_FALSE(sr.overflow);
    EXPECT_FALSE(sr.break_);
    EXPECT_FALSE(sr.decimal);
    EXPECT_FALSE(sr.interrupt);
    EXPECT_EQ(sr.zero, result == 0);
    EXPECT_EQ(sr.carry, static_cast<bool>(a & 0x80));
}

INSTANTIATE_TEST_SUITE_P(Values,
                         ShiftLeft,
                         ::testing::Values(TestParameters{ 0b00000000, true, 0b00000000 },
                                           TestParameters{ 0b10000000, true, 0b00000000 },
                                           TestParameters{ 0b01000000, true, 0b10000000 },
                                           TestParameters{ 0b11000011, true, 0b10000110 },
                                           TestParameters{ 0b01010101, true, 0b10101010 },
                                           TestParameters{ 0b10101010, true, 0b01010100 },
                                           TestParameters{ 0b00000001, true, 0b00000010 },
                                           TestParameters{ 0b10011000, true, 0b00110000 },
                                           TestParameters{ 0b11111111, true, 0b11111110 }));

struct RotateLeft : BitManip {};

TEST_P(RotateLeft, ValueAndFlags) {
    const auto [a, carry, result] = GetParam();
    sr.carry                      = carry;
    EXPECT_EQ(ALU::rotate_left(a, sr), result);

    EXPECT_EQ(sr.negative, static_cast<int8_t>(result) < 0);
    EXPECT_FALSE(sr.overflow);
    EXPECT_FALSE(sr.break_);
    EXPECT_FALSE(sr.decimal);
    EXPECT_FALSE(sr.interrupt);
    EXPECT_EQ(sr.zero, result == 0);
    EXPECT_EQ(sr.carry, static_cast<bool>(a & 0x80));
}

INSTANTIATE_TEST_SUITE_P(Values,
                         RotateLeft,
                         ::testing::Values(TestParameters{ 0b00000000, false, 0b00000000 },
                                           TestParameters{ 0b00000000, true, 0b00000001 },
                                           TestParameters{ 0b10000000, false, 0b00000000 },
                                           TestParameters{ 0b10000000, true, 0b00000001 },
                                           TestParameters{ 0b01000000, false, 0b10000000 },
                                           TestParameters{ 0b01000000, true, 0b10000001 },
                                           TestParameters{ 0b11000011, false, 0b10000110 },
                                           TestParameters{ 0b11000011, true, 0b10000111 },
                                           TestParameters{ 0b01010101, false, 0b10101010 },
                                           TestParameters{ 0b01010101, true, 0b10101011 },
                                           TestParameters{ 0b10101010, false, 0b01010100 },
                                           TestParameters{ 0b10101010, true, 0b01010101 },
                                           TestParameters{ 0b00000001, false, 0b00000010 },
                                           TestParameters{ 0b00000001, true, 0b00000011 },
                                           TestParameters{ 0b10011000, false, 0b00110000 },
                                           TestParameters{ 0b10011000, true, 0b00110001 },
                                           TestParameters{ 0b11111111, false, 0b11111110 },
                                           TestParameters{ 0b11111111, true , 0b11111111}));
} // namespace emulator::mos_6502::test
