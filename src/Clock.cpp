//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#include "Clock.hpp"

#include <thread>

namespace emulator::mos_6502 {
Clock::Clock(const std::chrono::nanoseconds period) noexcept : _period(period) {}

void Clock::wait_for_pulse() noexcept {
    if (_period.count() == 0) return;

    const auto elapsed = std::chrono::high_resolution_clock::now() - _last_pulse;
    std::this_thread::sleep_for(_period - elapsed);
    _last_pulse = std::chrono::high_resolution_clock::now();
}
} // namespace emulator::mos_6502