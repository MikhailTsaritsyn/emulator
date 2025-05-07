//
// Created by Mikhail Tsaritsyn on May 07, 2025.
//

#include "linker.hpp"

#include "CPU.hpp"
#include "helpers.hpp"

namespace emulator::mos_6502::test::linker {
Memory::Data assemble(const Source &main, const std::vector<Source> &others) noexcept {
    Memory::Data result{};

    const auto program_start              = main.address_start.prev();
    result[program_start.to_underlying()] = mtl::u8(0x58); // CLI, to clear the interrupt disable flag set at startup

    if (main.binary.size() + main.address_start.to_underlying() > result.size())
        mtl::panic("linker: source is too long to fit in memory");
    std::ranges::copy(main.binary, result.begin() + main.address_start.to_underlying());

    for (const auto &[binary, start]: others) {
        if (binary.size() + start.to_underlying() > result.size())
            mtl::panic("linker: source is too long to fit in memory");
        std::ranges::copy(binary, result.begin() + start.to_underlying());
    }

    result[CPU::RES.to_underlying()]        = low_byte(program_start);
    result[CPU::RES.next().to_underlying()] = high_byte(program_start);
    return result;
}
} // namespace emulator::mos_6502::test::linker