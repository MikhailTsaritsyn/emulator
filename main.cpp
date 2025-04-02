#include "CPU.hpp"
#include "PeriodicPulse.hpp"
#include "PulseOff.hpp"
#include "PulseOn.hpp"

int main() {
    emulator::mos_6502::CPU cpu{ emulator::mos_6502::PeriodicPulse{ std::chrono::seconds(1) } };
    // emulator::mos_6502::CPU cpu{ emulator::mos_6502::PulseOn{ } }; // executes as fast as it can
    // emulator::mos_6502::CPU cpu{ emulator::mos_6502::PulseOff{ } }; // never executes, stays "frozen"
    cpu.start();
    return 0;
}
