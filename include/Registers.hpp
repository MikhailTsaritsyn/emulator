//
// Created by Mikhail Tsaritsyn on Apr 30, 2025.
//

#ifndef EMULATOR_MOS_6502_REGISTERS_HPP
#define EMULATOR_MOS_6502_REGISTERS_HPP
#include "mtl/core.hpp"
#include "StatusRegister.hpp"

namespace emulator::mos_6502 {

struct Registers {
    /**
     * @brief Program counter
     *
     * The program counter keeps track of the memory location holding the current instruction code.
     * Its content is automatically stepped up as the program is executed and is modified by branch and jump operations.
     * As it must be able to address the full 16-bit address range of 64K bytes, it's the only 16-bit register of the
     * 6502.
     */
    mtl::u16 PC;

    /**
     * @brief Accumulator
     *
     * The accumulator is the main register of the 6502.
     * Its content is typically used by the Arithmetic Logic Unit (ALU) for the first operand,
     * and results are deposited in the accumulator again.
     * Thus, its name, as results accumulate in this register.
     * Most arithmetic and logical operations interact with this register.
     */
    mtl::u8 AC{ 0 };

    /**
     * @brief Index register X
     *
     * The X and Y registers are auxiliary registers.
     * Like the accumulator, they can be loaded directly with values,
     * both immediately, as literal constants, or from memory.
     * Additionally, they can be incremented and decremented,
     * and their contents may be transferred to and from the accumulator.
     * Their main purpose is the use as index registers, where their contents are added to a base memory location,
     * before any values are either stored to or retrieved from the resulting address,
     * which is known as the effective address.
     * This is commonly used for loops and table lookups at a given index, hence the name.
     */
    mtl::u8 X{ 0 };

    /**
     * @brief Index register Y
     *
     * The X and Y registers are auxiliary registers.
     * Like the accumulator, they can be loaded directly with values,
     * both immediately, as literal constants, or from memory.
     * Additionally, they can be incremented and decremented,
     * and their contents may be transferred to and from the accumulator.
     * Their main purpose is the use as index registers, where their contents are added to a base memory location,
     * before any values are either stored to or retrieved from the resulting address,
     * which is known as the effective address.
     * This is commonly used for loops and table lookups at a given index, hence the name.
     */
    mtl::u8 Y{ 0 };

    /**
     * @brief Stack pointer
     *
     * The stack pointer points to the current top of stack, or rather, to its bottom, as the stack grows top-down.
     * The processor stack is located on memory page #1 ($0100–$01FF), 256 bytes Last-In-First-Out (LIFO) stack,
     * which enables subroutines and also serves as a quick intermediate storage.
     * As an 8-bit register, the stack pointer holds just the low-byte of this address (the offset from $0100.)
     * Be aware that this just wraps around in case that the stack underflows.
     */
    mtl::u8 SP{ 0 };

    /**
     * @brief Status register
     *
     * The status register holds the status of the processor, consisting of flags reflecting results of previous
     * operations, configuration flags, like disabling interrupts or setting up Binary Coded Decimal mode (BCD),
     * and the carry flag, which enables multibyte arithmetics.
     *
     * @note All arithmetic operations update the Z, N, C and V flags.
     */
    StatusRegister SR{};
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_REGISTERS_HPP
