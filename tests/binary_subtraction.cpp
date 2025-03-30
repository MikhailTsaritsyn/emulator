//
// Created by Mikhail Tsaritsyn on Mar 30, 2025.
//
#include "ALU.hpp"
#include <gtest/gtest.h>
#include <random>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, uint8_t, bool, uint8_t>;

struct BinarySubtraction : testing::TestWithParam<TestParameters> {
    StatusRegister sr;

    void SetUp() override {
        sr = { .negative  = false,
               .overflow  = false,
               .break_    = false,
               .decimal   = false,
               .interrupt = false,
               .zero      = false,
               .carry     = true };
    }
};

TEST_P(BinarySubtraction, ValueAndFlags) {
    auto [a, b, borrow, expected_result] = GetParam();
    sr.carry                    = !borrow;
    EXPECT_EQ(ALU::subtract(a, b, sr), expected_result);

    EXPECT_EQ(sr.negative, static_cast<int8_t>(expected_result) < 0)
            << "result = " << static_cast<int>(expected_result);
    EXPECT_EQ(sr.overflow, (static_cast<int8_t>(a) < 0) != (static_cast<int8_t>(expected_result) < 0))
            << static_cast<int>(a) << " -> " << static_cast<int>(expected_result);
    EXPECT_FALSE(sr.break_);
    EXPECT_FALSE(sr.decimal);
    EXPECT_FALSE(sr.interrupt);
    EXPECT_EQ(sr.zero, expected_result == 0);

    const auto result_unwrapped = static_cast<int>(a) - static_cast<int>(b) - (borrow ? 1 : 0);
    EXPECT_EQ(sr.carry, result_unwrapped >= 0)
            << "result = " << static_cast<int>(expected_result) << '(' << result_unwrapped << ')';
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

static std::mt19937 gen(0);
static std::uniform_int_distribution<int> distr(0, 255);
constexpr size_t N_TESTS = 100;

[[nodiscard]] static constexpr auto generate_random_parameters() noexcept {
    std::vector<TestParameters> result{};
    result.reserve(N_TESTS);
    while (result.size() < N_TESTS) {
        const int a = distr(gen);
        const int b = distr(gen);
        result.emplace_back(a, b, false, a - b);
        result.emplace_back(a, b, true, a - b - 1);
    }
    return result;
}

INSTANTIATE_TEST_SUITE_P(RandomValues, BinarySubtraction, ::testing::ValuesIn(generate_random_parameters()));
} // namespace emulator::mos_6502::test