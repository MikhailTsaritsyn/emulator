//
// Created by Mikhail Tsaritsyn on Apr 20, 2025.
//

#include "helpers.hpp"

namespace emulator::mos_6502 {
std::pair<uint8_t, bool> add_with_overflow(const uint8_t a, const uint8_t b) noexcept {
    uint8_t result;
    const auto overflow = __builtin_add_overflow(a, b, &result);
    return { result, overflow };
}
} // namespace emulator::mos_6502