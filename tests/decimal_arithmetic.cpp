//
// Created by Mikhail Tsaritsyn on Apr 01, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<mtl::u8, mtl::u8, bool, mtl::u8, bool>;

struct DecimalArithmetic : testing::TestWithParam<TestParameters> {
    mtl::u8 input_first{0};
    mtl::u8 input_second{0};
    mtl::u8 output{0};
};

struct DecimalAddition : DecimalArithmetic {
    bool input_carry  = false;
    bool output_carry = false;

    void SetUp() override {
        std::tie(input_first, input_second, input_carry, output, output_carry) = GetParam();
    }
};

TEST_P(DecimalAddition, Test) {
    const auto [sum, carry, overflow] = ALU::add(input_first, input_second, input_carry, true);
    EXPECT_EQ(sum, output);

    const auto result_raw = static_cast<int>(input_first.to_underlying()) + static_cast<int>(input_second.to_underlying()) + (input_carry ? 1 : 0);
    EXPECT_EQ(carry, output_carry) << "result = " << std::hex << output << '(' << result_raw << ')' << std::dec;
    EXPECT_EQ(overflow, (std::bit_cast<mtl::i8>(input_first) < mtl::i8(0)) != (std::bit_cast<mtl::i8>(output) < mtl::i8(0)))
            << input_first << " -> " << output;
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

struct DecimalSubtraction : DecimalArithmetic {
    bool input_borrow  = false;
    bool output_borrow = false;

    void SetUp() override {
        std::tie(input_first, input_second, input_borrow, output, output_borrow) = GetParam();
    }
};

TEST_P(DecimalSubtraction, Test) {
    const auto [sum, carry, overflow] = ALU::subtract(input_first, input_second, !input_borrow, true);
    EXPECT_EQ(sum, output);

    const auto result_raw = static_cast<int>(input_first.to_underlying()) - static_cast<int>(input_second.to_underlying()) - (input_borrow ? 1 : 0);
    EXPECT_EQ(carry, !output_borrow) << "result = " << std::hex << output << '(' << result_raw << ')' << std::dec;
    EXPECT_EQ(overflow, (std::bit_cast<mtl::i8>(input_first) < mtl::i8(0)) != (std::bit_cast<mtl::i8>(output) < mtl::i8(0)))
            << input_first << " -> " << output;
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