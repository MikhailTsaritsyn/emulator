#include "CPU.hpp"
#include "PeriodicPulse.hpp"
#include "PulseOn.hpp"
#include <iostream>
#include <thread>

int main() {
    {
        std::cout << "--- 1 Hz CPU ---" << std::endl;
        emulator::mos_6502::CPU cpu{ emulator::mos_6502::PeriodicPulse{ std::chrono::seconds(1) } };

        std::jthread thread{ [&cpu] { cpu.start(); } };
        std::this_thread::sleep_for(std::chrono::seconds(10));

        std::cout << "Terminating..." << std::endl;
        cpu.terminate();
    }

    {
        std::cout << "--- Fastest possible CPU ---" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        emulator::mos_6502::CPU cpu{ emulator::mos_6502::PulseOn{} }; // executes as fast as it can

        std::jthread thread{ [&cpu] { cpu.start(); } };
        std::this_thread::sleep_for(std::chrono::microseconds(100));

        std::cout << "Terminating..." << std::endl;
        cpu.terminate();
    }
    return 0;
}
