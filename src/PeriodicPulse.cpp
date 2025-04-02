//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#include "PeriodicPulse.hpp"

namespace emulator::mos_6502 {
PeriodicPulse::PeriodicPulse(const std::chrono::nanoseconds period) noexcept : _period(period) {}

bool PeriodicPulse::value() noexcept {
    if (const auto current = std::chrono::high_resolution_clock::now(); current - _last_pulse >= _period) {
        _last_pulse = current;
        return true;
    }

    return false;
}
} // namespace emulator::mos_6502