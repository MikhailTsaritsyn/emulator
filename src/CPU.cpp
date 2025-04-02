//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//
#include "CPU.hpp"

#include <chrono>

namespace emulator::mos_6502 {

// TODO: Frequency with units?
// TODO(optimization): Resolving clock polymorphism takes a significant amount of time.
//                     Use something simpler?

void CPU::start() noexcept {
    auto prev_time = std::chrono::high_resolution_clock::now();
    static constexpr size_t window = 100;
    while (!_terminate.test()) {
        wait_for_clock();
        _cycle++;

        // Estimate the clock frequency using the last 100 pulses
        if (_cycle % window == 0) {
            const auto current_time                     = std::chrono::high_resolution_clock::now();
            const std::chrono::duration<double> delta_t = current_time - prev_time;
            _frequency                                  = static_cast<double>(window) / delta_t.count();
            prev_time                                   = current_time;
        }
    }

    // If not enough cycles passed to compute the frequency in the window, use them all
    if (_cycle < window) {
        const std::chrono::duration<double> delta_t = std::chrono::high_resolution_clock::now() - prev_time;
        _frequency                                  = static_cast<double>(_cycle) / delta_t.count();
    }
}

void CPU::terminate() noexcept { _terminate.test_and_set(); }

double CPU::frequency() const noexcept { return _frequency; }

void CPU::wait_for_clock() const noexcept { while (!_pulse_gen.value()); }
} // namespace emulator::mos_6502