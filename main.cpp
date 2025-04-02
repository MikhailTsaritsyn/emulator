#include "CPU.hpp"
#include "PeriodicPulse.hpp"
#include "PulseOn.hpp"
#include <iostream>
#include <thread>

template <typename ClockT> constexpr void emulate(ClockT clock, const std::chrono::nanoseconds time) {
    emulator::mos_6502::CPU cpu{ std::move(clock) }; // executes as fast as it can

    std::jthread thread{ [&cpu] { cpu.start(); } };
    std::this_thread::sleep_for(time);

    std::cout << "Terminating..." << std::endl;
    cpu.terminate();

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // To be sure that the CPU is terminated
    std::cout << std::format("Final frequency = {} Hz", cpu.frequency()) << std::endl;
}

int main() {
    std::cout << "--- Fastest possible CPU ---" << std::endl;
    emulate(emulator::mos_6502::PulseOn{}, std::chrono::seconds(1));

    std::cout << "\n--- 10 Hz CPU ---" << std::endl;
    emulate(emulator::mos_6502::PeriodicPulse{ std::chrono::milliseconds(100) }, std::chrono::seconds(1));
    return 0;
}
