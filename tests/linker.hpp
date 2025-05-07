//
// Created by Mikhail Tsaritsyn on May 07, 2025.
//

#ifndef EMULATOR_MOS_6502_TESTS_LINKER_HPP
#define EMULATOR_MOS_6502_TESTS_LINKER_HPP
#include "Memory.hpp"
#include <vector>

namespace emulator::mos_6502::test::linker {
struct Source {
    std::vector<mtl::u8> binary; ///< The source code
    mtl::u16 address_start;      ///< The address where to put the code to
};

// TODO: check for overwriting stack and zero page?

/**
 * @brief Assemble given binary sources to a chunk of memory ready for the CPU
 *
 * Each binary source is inserted at the given address.
 * The main binary is prepended with @p CLI instruction to enable interrupts.
 *
 * The CPU reset vector is filled with the address of the prepended main binary start.
 *
 * @param main The entry point of the program. The execution starts here.
 * @param others Optional pieces of code that the main may jump to.
 *
 * @return Chunk of memory ready for the CPU to run on it
 */
[[nodiscard]] Memory::Data assemble(const Source &main, const std::vector<Source> &others = {}) noexcept;
} // namespace emulator::mos_6502::test::linker

#endif //EMULATOR_MOS_6502_TESTS_LINKER_HPP
