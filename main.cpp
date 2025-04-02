#include "CPU.hpp"
#include "PeriodicPulse.hpp"

int main() {
    emulator::mos_6502::CPU cpu{ emulator::mos_6502::PeriodicPulse{ std::chrono::seconds(1) } };
    cpu.start();
    return 0;
}
