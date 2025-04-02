//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//
#include "CPU.hpp"

#include <iostream>

namespace emulator::mos_6502 {

void CPU::start() noexcept {
    while (true) {
        wait_for_clock();
        std::cout << "Tic\n";
    }
}

void CPU::wait_for_clock() const noexcept { while (!_pulse_gen.value()); }
} // namespace emulator::mos_6502