//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//
#include "CPU.hpp"

#include <chrono>

namespace emulator::mos_6502 {

CPU::CPU(const std::chrono::nanoseconds clock_period, const ROM &rom) noexcept : _clock(clock_period), _rom(rom) {}

void CPU::start() noexcept {
    reset();

    auto prev_time = std::chrono::high_resolution_clock::now();
    static constexpr size_t window = 100;
    while (!_terminate.test()) {
        read(PC++);

        // Estimate the clock frequency using the last 100 pulses
        // TODO: Move to a separate function?
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

const CPU::ROM &CPU::rom() const & noexcept { return _rom; }

CPU::ROM CPU::rom() const && noexcept { return _rom; }

uint16_t CPU::make_word(const uint8_t high, const uint8_t low) noexcept {
    return static_cast<uint16_t>(high) << 8 | static_cast<uint16_t>(low);
}

void CPU::reset() noexcept {
    read(PC++);
    read(PC++);
    read(0x0100 + SP);
    read(0x0100 + SP - 1);
    read(0x0100 + SP - 2);
    const auto pcl = read(RES);
    const auto pch = read(RES + 1);
    PC             = make_word(pch, pcl);
}

uint8_t CPU::read(const uint16_t address) noexcept {
    while (!_clock.value()) {} // wait for the next clock pulse
    _cycle++;
    return _rom[address];
}
} // namespace emulator::mos_6502