//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#include "Clock.hpp"

#include <thread>

namespace emulator::mos_6502 {
Clock::Clock(const std::chrono::nanoseconds period) noexcept : _period(period) {}

void Clock::wait_for_pulse() noexcept {
    _cycle++;
    if (_period.count() != 0) {
        const auto elapsed = std::chrono::high_resolution_clock::now() - _last_pulse;
        std::this_thread::sleep_for(_period - elapsed);
    }
    const auto current_time = std::chrono::high_resolution_clock::now();
    _elapsed += current_time - _last_pulse;
    _last_pulse = current_time;
}

double Clock::frequency() const noexcept { return static_cast<double>(_cycle) / _elapsed.count(); }

size_t Clock::cycle() const noexcept { return _cycle; }
} // namespace emulator::mos_6502