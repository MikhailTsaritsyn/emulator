//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#include "Clock.hpp"

namespace emulator::mos_6502 {
Clock::Clock(const std::chrono::nanoseconds period) noexcept : _period(period) {}

bool Clock::value() noexcept {
    if (_period.count() == 0) return true;

    if (const auto current = std::chrono::high_resolution_clock::now(); current - _last_pulse >= _period) {
        _last_pulse = current;
        return true;
    }

    return false;
}
} // namespace emulator::mos_6502