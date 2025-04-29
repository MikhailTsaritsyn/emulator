//
// Created by Mikhail Tsaritsyn on Apr 01, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<mtl::u8, mtl::u8, bool, mtl::u8>;

struct BinaryArithmetic : testing::TestWithParam<TestParameters> {
    mtl::u8 input_first{0};
    mtl::u8 input_second{0};
    mtl::u8 output{0};
};

struct BinaryAddition : BinaryArithmetic {
    bool input_carry = false;

    void SetUp() override {
        std::tie(input_first, input_second, input_carry, output) = GetParam();
    }
};

TEST_P(BinaryAddition, Test) {
    const auto [sum, carry, overflow] = ALU::add(input_first, input_second, input_carry, false);
    EXPECT_EQ(sum, output);

    const auto result_raw = static_cast<int>(input_first.to_underlying()) + static_cast<int>(input_second.to_underlying()) + (input_carry ? 1 : 0);
    EXPECT_EQ(carry, result_raw > 255) << "result = " << output << '(' << result_raw << ')';
    EXPECT_EQ(overflow,
              (std::bit_cast<mtl::i8>(input_first) < mtl::i8(0)) != (std::bit_cast<mtl::i8>(output) < mtl::i8(0)))
            << input_first << " -> " << output;
}

INSTANTIATE_TEST_SUITE_P(Zero,
                         BinaryAddition,
                         ::testing::Values(TestParameters{ 0, 0, false, 0 },
                                           TestParameters{ 128, 128, false, 0 },
                                           TestParameters{ 127, 128, true, 0 },
                                           TestParameters{ 128, 127, true, 0 }));

INSTANTIATE_TEST_SUITE_P(Carry,
                         BinaryAddition,
                         ::testing::Values(TestParameters{ 0, 255, true, 0 },
                                           TestParameters{ 255, 0, true, 0 },
                                           TestParameters{ 1, 255, false, 0 },
                                           TestParameters{ 255, 1, false, 0 },
                                           TestParameters{ 1, 255, true, 1 },
                                           TestParameters{ 255, 1, true, 1 },
                                           TestParameters{ 255, 255, true, 255 },
                                           TestParameters{ 200, 200, false, 144 }));

INSTANTIATE_TEST_SUITE_P(Overflow,
                         BinaryAddition,
                         ::testing::Values(TestParameters{ 100, 100, false, 200 },
                                           TestParameters{ 100, 100, true, 201 },
                                           TestParameters{ 1, 127, false, 128 },
                                           TestParameters{ 127, 1, false, 128 },
                                           TestParameters{ 0, 127, true, 128 },
                                           TestParameters{ 127, 0, true, 128 },
                                           TestParameters{ 10, 120, true, 131 }));

INSTANTIATE_TEST_SUITE_P(NoOverflow,
                         BinaryAddition,
                         ::testing::Values(TestParameters{ 10, 10, false, 20 },
                                           TestParameters{ 10, 10, true, 21 },
                                           TestParameters{ 1, 126, false, 127 },
                                           TestParameters{ 126, 1, false, 127 },
                                           TestParameters{ 0, 126, true, 127 },
                                           TestParameters{ 126, 0, true, 127 },
                                           TestParameters{ 10, 100, true, 111 }));

struct BinarySubtraction : BinaryArithmetic {
    bool input_borrow = false;

    void SetUp() override {
        std::tie(input_first, input_second, input_borrow, output) = GetParam();
    }
};

TEST_P(BinarySubtraction, Test) {
    const auto [sum, carry, overflow] = ALU::subtract(input_first, input_second, !input_borrow, false);
    EXPECT_EQ(sum, output);

    const auto result_raw = static_cast<int>(input_first.to_underlying()) - static_cast<int>(input_second.to_underlying()) - (input_borrow ? 1 : 0);
    EXPECT_EQ(carry, result_raw >= 0) << "result = " << output << '(' << result_raw << ')';
    EXPECT_EQ(overflow,
              (std::bit_cast<mtl::i8>(input_first) < mtl::i8(0)) != (std::bit_cast<mtl::i8>(output) < mtl::i8(0)))
            << input_first << " -> " << output;
}

INSTANTIATE_TEST_SUITE_P(Zero,
                         BinarySubtraction,
                         ::testing::Values(TestParameters{ 0, 0, false, 0 },
                                           TestParameters{ 1, 0, true, 0 },
                                           TestParameters{ 128, 128, false, 0 },
                                           TestParameters{ 128, 127, true, 0 },
                                           TestParameters{ 255, 255, false, 0 },
                                           TestParameters{ 255, 254, true, 0 },
                                           TestParameters{ 0, 255, true, 0 }));

INSTANTIATE_TEST_SUITE_P(Borrow,
                         BinarySubtraction,
                         ::testing::Values(TestParameters{ 0, 1, false, 255 }, TestParameters{ 1, 1, true, 255 }));

INSTANTIATE_TEST_SUITE_P(Overflow,
                         BinarySubtraction,
                         ::testing::Values(TestParameters{ 128, 1, false, 127 },
                                           TestParameters{ 128, 0, true, 127 },
                                           TestParameters{ 0, 129, false, 127 },
                                           TestParameters{ 0, 128, true, 127 }));

INSTANTIATE_TEST_SUITE_P(NoOverflow,
                         BinarySubtraction,
                         ::testing::Values(TestParameters{ 1, 0, false, 1 },
                                           TestParameters{ 1, 0, true, 0 },
                                           TestParameters{ 127, 0, false, 127 },
                                           TestParameters{ 93, 45, false, 48 }));
} // namespace emulator::mos_6502::test