#include "Clock.hpp"
#include "CPU.hpp"
#include <iostream>
#include <thread>

void emulate(const std::chrono::nanoseconds clock_period, const std::chrono::nanoseconds time) {
    emulator::mos_6502::CPU::ROM rom{};
    std::ranges::fill(rom, 0);
    emulator::mos_6502::CPU cpu{ clock_period, rom }; // executes as fast as it can

    std::jthread thread{ [&cpu] { cpu.start(); } };
    std::this_thread::sleep_for(time);

    std::cout << "Terminating..." << std::endl;
    cpu.terminate();

    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // To be sure that the CPU is terminated

    std::cout << "Final frequency = ";
    if (const auto frequency = cpu.frequency(); frequency < 1e3)
        std::cout << std::format("{} Hz", frequency) << std::endl;
    else if (frequency < 1e6) std::cout << std::format("{} kHz", frequency / 1e3) << std::endl;
    else if (frequency < 1e9) std::cout << std::format("{} MHz", frequency / 1e6) << std::endl;
    else std::cout << std::format("{} GHz", frequency / 1e9) << std::endl;
}

int main() {
    std::cout << "--- Fastest possible CPU ---" << std::endl;
    emulate(std::chrono::seconds(0), std::chrono::seconds(1));

    std::cout << "\n--- 10 Hz CPU ---" << std::endl;
    emulate(std::chrono::milliseconds(100), std::chrono::seconds(1));
    return 0;
}
