//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>
#include <random>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, uint8_t, bool, uint8_t>;

struct BinaryAddition : testing::TestWithParam<TestParameters> {
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

TEST_P(BinaryAddition, ValueAndFlags) {
    auto [a, b, carry, expected_result] = GetParam();
    sr.carry                   = carry;
    EXPECT_EQ(ALU::add(a, b, sr), expected_result);

    EXPECT_EQ(sr.negative, static_cast<int8_t>(expected_result) < 0)
            << "result = " << static_cast<int>(expected_result);
    EXPECT_EQ(sr.overflow, (static_cast<int8_t>(a) < 0) != (static_cast<int8_t>(expected_result) < 0))
            << static_cast<int>(a) << " -> " << static_cast<int>(expected_result);
    EXPECT_FALSE(sr.break_);
    EXPECT_FALSE(sr.decimal);
    EXPECT_FALSE(sr.interrupt);
    EXPECT_EQ(sr.zero, expected_result == 0);

    const auto result_unwrapped = static_cast<int>(a) + static_cast<int>(b) + (carry ? 1 : 0);
    EXPECT_EQ(sr.carry, result_unwrapped > 255)
            << "result = " << static_cast<int>(expected_result) << '(' << result_unwrapped << ')';
}

// Use predefined seed for repeatability
static std::mt19937 gen(0);

[[nodiscard]] static constexpr auto generate_sum_zero_no_overflow(size_t n) noexcept {
    std::vector<TestParameters> result{};
    result.reserve(n);
    while (result.size() < n) {

    }
};

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


static std::uniform_int_distribution<int> distr(0, 255);
constexpr size_t N_TESTS = 100;

[[nodiscard]] static constexpr auto generate_random_parameters() noexcept {
    std::vector<TestParameters> result{};
    result.reserve(N_TESTS);
    while (result.size() < N_TESTS) {
        const int a = distr(gen);
        const int b = distr(gen);
        result.emplace_back(a, b, false, a + b);
        result.emplace_back(a, b, true, a + b + 1);
    }
    return result;
}

INSTANTIATE_TEST_SUITE_P(RandomValues, BinaryAddition, ::testing::ValuesIn(generate_random_parameters()));
} // namespace emulator::mos_6502::test
