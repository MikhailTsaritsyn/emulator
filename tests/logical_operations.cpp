//
// Created by Mikhail Tsaritsyn on Apr 01, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, uint8_t, uint8_t>;

struct LogicalOperations : testing::TestWithParam<TestParameters> {
    StatusRegister sr;

    uint8_t input_first  = 0;
    uint8_t input_second = 0;
    uint8_t output       = 0;

    void SetUp() override {
        std::tie(input_first, input_second, output) = GetParam();

        sr = { .negative  = false,
               .overflow  = false,
               .break_    = false,
               .decimal   = false,
               .interrupt = false,
               .zero      = false,
               .carry     = false };
    }

    void TearDown() override {
        EXPECT_EQ(sr.negative, static_cast<int8_t>(output) < 0) << "output = " << static_cast<int>(output);
        EXPECT_FALSE(sr.overflow);
        EXPECT_FALSE(sr.break_);
        EXPECT_FALSE(sr.decimal);
        EXPECT_FALSE(sr.interrupt);
        EXPECT_EQ(sr.zero, output == 0) << "output = " << static_cast<int>(output);
        EXPECT_FALSE(sr.carry);
    }
};

struct LogicalAnd : LogicalOperations {};

TEST_P(LogicalAnd, Test) {
    EXPECT_EQ(ALU::logical_and(input_first, input_second, sr), output);
    EXPECT_EQ(ALU::logical_and(input_second, input_first, sr), output);
}

INSTANTIATE_TEST_SUITE_P(Values,
                         LogicalAnd,
                         ::testing::Values(TestParameters{ 0b00000000, 0b00000000, 0b00000000 },
                                           TestParameters{ 0b00000000, 0b01010101, 0b00000000 },
                                           TestParameters{ 0b00000000, 0b10101010, 0b00000000 },

                                           TestParameters{ 0b11001100, 0b10101010, 0b10001000 },
                                           TestParameters{ 0b00110011, 0b10101010, 0b00100010 },
                                           TestParameters{ 0b11001100, 0b01010101, 0b01000100 },
                                           TestParameters{ 0b00110011, 0b01010101, 0b00010001 },

                                           TestParameters{ 0b11110000, 0b10101010, 0b10100000 },
                                           TestParameters{ 0b00001111, 0b10101010, 0b00001010 },
                                           TestParameters{ 0b11110000, 0b01010101, 0b01010000 },
                                           TestParameters{ 0b00001111, 0b01010101, 0b00000101 },

                                           TestParameters{ 0b11110000, 0b11001100, 0b11000000 },
                                           TestParameters{ 0b00001111, 0b11001100, 0b00001100 },
                                           TestParameters{ 0b11110000, 0b00110011, 0b00110000 },
                                           TestParameters{ 0b00001111, 0b00110011, 0b00000011 },

                                           TestParameters{ 0b11111111, 0b00000000, 0b00000000 },
                                           TestParameters{ 0b11111111, 0b01010101, 0b01010101 },
                                           TestParameters{ 0b11111111, 0b10101010, 0b10101010 }));

struct LogicalOr : LogicalOperations {};

TEST_P(LogicalOr, Test) {
    EXPECT_EQ(ALU::logical_or(input_first, input_second, sr), output);
    EXPECT_EQ(ALU::logical_or(input_second, input_first, sr), output);
}

INSTANTIATE_TEST_SUITE_P(Values,
                         LogicalOr,
                         ::testing::Values(TestParameters{ 0b00000000, 0b00000000, 0b00000000 },
                                           TestParameters{ 0b01010101, 0b00000000, 0b01010101 },
                                           TestParameters{ 0b00000000, 0b10101010, 0b10101010 },

                                           TestParameters{ 0b11001100, 0b10101010, 0b11101110 },
                                           TestParameters{ 0b00110011, 0b10101010, 0b10111011 },
                                           TestParameters{ 0b11001100, 0b01010101, 0b11011101 },
                                           TestParameters{ 0b00110011, 0b01010101, 0b01110111 },

                                           TestParameters{ 0b11110000, 0b10101010, 0b11111010 },
                                           TestParameters{ 0b00001111, 0b10101010, 0b10101111 },
                                           TestParameters{ 0b11110000, 0b01010101, 0b11110101 },
                                           TestParameters{ 0b00001111, 0b01010101, 0b01011111 },

                                           TestParameters{ 0b11110000, 0b11001100, 0b11111100 },
                                           TestParameters{ 0b00001111, 0b11001100, 0b11001111 },
                                           TestParameters{ 0b11110000, 0b00110011, 0b11110011 },
                                           TestParameters{ 0b00001111, 0b00110011, 0b00111111 },

                                           TestParameters{ 0b11111111, 0b00000000, 0b11111111 },
                                           TestParameters{ 0b11111111, 0b01010101, 0b11111111 },
                                           TestParameters{ 0b11111111, 0b10101010, 0b11111111 }));

struct LogicalXor : LogicalOperations {};

TEST_P(LogicalXor, Test) {
    EXPECT_EQ(ALU::logical_xor(input_first, input_second, sr), output);
    EXPECT_EQ(ALU::logical_xor(input_second, input_first, sr), output);
}

INSTANTIATE_TEST_SUITE_P(Values,
                         LogicalXor,
                         ::testing::Values(TestParameters{ 0b00000000, 0b00000000, 0b00000000 },
                                           TestParameters{ 0b01010101, 0b00000000, 0b01010101 },
                                           TestParameters{ 0b00000000, 0b10101010, 0b10101010 },

                                           TestParameters{ 0b11001100, 0b10101010, 0b01100110 },
                                           TestParameters{ 0b00110011, 0b10101010, 0b10011001 },
                                           TestParameters{ 0b11001100, 0b01010101, 0b10011001 },
                                           TestParameters{ 0b00110011, 0b01010101, 0b01100110 },

                                           TestParameters{ 0b11110000, 0b10101010, 0b01011010 },
                                           TestParameters{ 0b00001111, 0b10101010, 0b10100101 },
                                           TestParameters{ 0b11110000, 0b01010101, 0b10100101 },
                                           TestParameters{ 0b00001111, 0b01010101, 0b01011010 },

                                           TestParameters{ 0b11110000, 0b11001100, 0b00111100 },
                                           TestParameters{ 0b00001111, 0b11001100, 0b11000011 },
                                           TestParameters{ 0b11110000, 0b00110011, 0b11000011 },
                                           TestParameters{ 0b00001111, 0b00110011, 0b00111100 },

                                           TestParameters{ 0b11111111, 0b00000000, 0b11111111 },
                                           TestParameters{ 0b11111111, 0b01010101, 0b10101010 },
                                           TestParameters{ 0b11111111, 0b10101010, 0b01010101 }));
} // namespace emulator::mos_6502::test