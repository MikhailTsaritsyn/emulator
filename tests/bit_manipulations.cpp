//
// Created by Mikhail Tsaritsyn on Mar 31, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, bool, uint8_t>;

struct BitManip : testing::TestWithParam<TestParameters> {
    StatusRegister sr;

    uint8_t input    = 0;
    bool input_carry = false;
    uint8_t output   = 0;

    void SetUp() override {
        std::tie(input, input_carry, output) = GetParam();

        sr = { .negative  = false,
               .overflow  = false,
               .break_    = false,
               .decimal   = false,
               .interrupt = false,
               .zero      = false,
               .carry     = input_carry };
    }

    void TearDown() override {
        EXPECT_EQ(sr.negative, static_cast<int8_t>(output) < 0);
        EXPECT_FALSE(sr.overflow);
        EXPECT_FALSE(sr.break_);
        EXPECT_FALSE(sr.decimal);
        EXPECT_FALSE(sr.interrupt);
        EXPECT_EQ(sr.zero, output == 0);
    }
};

struct ShiftRight : BitManip {};

TEST_P(ShiftRight, Test) {
    EXPECT_EQ(ALU::shift_right(input, sr), output);
    EXPECT_EQ(sr.carry, input & 1);
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

TEST_P(ShiftLeft, Test) {
    EXPECT_EQ(ALU::shift_left(input, sr), output);
    EXPECT_EQ(sr.carry, static_cast<bool>(input & 0x80));
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

TEST_P(RotateLeft, Test) {
    EXPECT_EQ(ALU::rotate_left(input, sr), output);
    EXPECT_EQ(sr.carry, static_cast<bool>(input & 0x80));
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

struct RotateRight : BitManip {};

TEST_P(RotateRight, Test) {
    EXPECT_EQ(ALU::rotate_right(input, sr), output);
    EXPECT_EQ(sr.carry, static_cast<bool>(input & 0x01));
}

INSTANTIATE_TEST_SUITE_P(Values,
                         RotateRight,
                         ::testing::Values(TestParameters{ 0b00000000, false, 0b00000000 },
                                           TestParameters{ 0b00000000, true, 0b10000000 },
                                           TestParameters{ 0b10000000, false, 0b01000000 },
                                           TestParameters{ 0b10000000, true, 0b11000000 },
                                           TestParameters{ 0b01000000, false, 0b00100000 },
                                           TestParameters{ 0b01000000, true, 0b10100000 },
                                           TestParameters{ 0b11000011, false, 0b01100001 },
                                           TestParameters{ 0b11000011, true, 0b11100001 },
                                           TestParameters{ 0b01010101, false, 0b00101010 },
                                           TestParameters{ 0b01010101, true, 0b10101010 },
                                           TestParameters{ 0b10101010, false, 0b01010101 },
                                           TestParameters{ 0b10101010, true, 0b11010101 },
                                           TestParameters{ 0b00000001, false, 0b00000000 },
                                           TestParameters{ 0b00000001, true, 0b10000000 },
                                           TestParameters{ 0b10011000, false, 0b01001100 },
                                           TestParameters{ 0b10011000, true, 0b11001100 },
                                           TestParameters{ 0b11111111, false, 0b01111111 },
                                           TestParameters{ 0b11111111, true, 0b11111111 }));
} // namespace emulator::mos_6502::test
