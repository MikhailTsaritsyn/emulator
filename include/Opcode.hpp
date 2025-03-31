//
// Created by Mikhail Tsaritsyn on Mar 25, 2025.
//

#ifndef EMULATOR_MOS_6502_OPCODE_HPP
#define EMULATOR_MOS_6502_OPCODE_HPP
#include <cstdint>
#include <optional>

namespace emulator::mos_6502 {
/**
 * The 6502 processors provide several ways in which memory locations can be addressed.
 * Some instructions support several different modes while others may only support one.
 * In addition, the two index registers cannot always be used interchangeably.
 * This lack of orthogonality in the instruction set is one of the features that make the 6502 trickier to program well.
 */
enum struct Addressing : uint8_t {
    /**
     * Some instructions can operate directly upon the accumulator.
     * The programmer specifies this by using a special operand value, 'A'.
     */
    Accumulator,

    /**
     * Instructions using absolute addressing contain a full 16-bit address to identify the target location.
     */
    Absolute,

    /**
     * The address to be accessed by an instruction using X register indexed absolute addressing is computed by taking
     * the 16-bit address from the instruction and added the contents of the X register.
     * For example, if X contains $92 then an STA $2000,X instruction stores the accumulator at $2092,
     * for example, $2000 + $92.
     */
    AbsoluteX,

    /**
     * The address to be accessed by an instruction using Y register indexed absolute addressing is computed by taking
     * the 16-bit address from the instruction and added the contents of the Y register.
     * For example, if Y contains $92 then an STA $2000,Y instruction stores the accumulator at $2092,
     * for example, $2000 + $92.
     */
    AbsoluteY,

    /**
     * For many 6502 instructions the source and destination of the information to be manipulated is implied directly
     * by the function of the instruction itself, and no further operand needs to be specified.
     * Operations like Clear Carry Flag (CLC) and Return from Subroutine (RTS) are implicit.
     */
    Implicit,

    /**
     * Immediate addressing allows the programmer to directly specify an 8-bit constant within the instruction.
     * It is indicated by a '#' symbol followed by a numeric expression.
     */
    Immediate,

    /**
    * JMP is the only 6502 instruction to support indirection.
    * The instruction contains a 16-bit address which identifies the location of the least significant byte of another
    * 16-bit memory address which is the real target of the instruction.
    *
    * For example, if location $0120 contains $FC and location $0121 contains $BA then the instruction JMP ($0120)
    * causes the next instruction execution to occur at $BAFC, that is the contents of $0120 and $0121.
     */
    Indirect,

    /**
     * Indexed indirect addressing is normally used in conjunction with a table of address held on zero-page.
     * The address of the table is taken from the instruction, and the X register added to it
     * with zero-page wrap around to give the location of the least significant byte of the target address.
     */
    IndexedIndirect,

    /**
     * Indirect indexed addressing is the most common indirection mode used on the 6502.
     * In instruction contains the zero-page location of the least significant byte of 16-bit address.
     * The Y register is dynamically added to this value to generate the actual target address for operation.
     */
    IndirectIndexed,

    /**
     * Relative addressing mode is used by branch instructions, for example, BEQ, BNE, etc., which contain a
     * signed 8-bit relative offset, for example, -128 to +127, which is added to program counter if the condition is true.
     * As the program counter itself is incremented during instruction execution by two, the effective address range
     * for the target instruction must be with -126 to +129 bytes of the branch.
     */
    Relative,

    /**
     * An instruction using zero page addressing mode has only an 8-bit address operand.
     * This limits it to addressing only the first 256 bytes of memory, for example, $0000 to $00FF, where the most
     * significant byte of the address is always zero.
     * In zero-page mode, only the least significant byte of the address is held in the instruction, shortening it
     * by one byte, which is important for space-saving, and one less memory fetch during execution,
     * which is important for speed.
     */
    ZeroPage,

    /**
     * The address to be accessed by an instruction using indexed zero page addressing is calculated by taking
     * the 8-bit zero-page address from the instruction and adding the current value of the X register to it.
     * For example, if the X register contains $0 F and the instruction LDA $80,X is executed, then the accumulator
     * is loaded from $008 F, for example, $80 + $0 F => $8 F.
     *
     * @note The address calculation wraps around if the sum of the base address and the register exceed $FF.
     *       If we repeat the last example but with $FF in the X register, then the accumulator is loaded
     *       from $007 F, for example, $80 + $FF => $7 F, and not $017F.
     */
    ZeroPageX,

    /**
     * The address to be accessed by an instruction using indexed zero page addressing is calculated by taking
     * the 8-bit zero-page address from the instruction and adding the current value of the Y register to it.
     * This mode can only be used with the LDX and STX instructions.
     */
    ZeroPageY
};

enum class Instruction : uint8_t {
    /**
     * @brief Add Memory to Accumulator with Carry
     *
     * This instruction adds the value of memory and carry from the previous operation to the value of the accumulator
     * and stores the result in the accumulator.
     *
     * This instruction affects the Status Register.
     * - The carry flag is set when the sum of a binary addition exceeds 255
     *   or when the sum of a decimal addition exceeds 99, otherwise it is reset.
     * - The overflow flag is set whe the sign or bit 7 is changed due to result exceeding +127 or -128,
     *   otherwise it is reset.
     * - The negative flag is set if the accumulator result contains bit 7 on, otherwise it is reset.
     * - The zero flag is set if the accumulator result is zero, otherwise it is reset.
     */
    ADC,

    /**
     * @brief AND Memory with Accumulator
     *
     * The AND instruction transfers the Accumulator and Memory to the adder, which performs a bit-by-bit AND operation
     * and stores the result back in the accumulator.
     *
     * This instruction affects the Status Register.
     * - The Negative flag is set if the result in the accumulator has bit 7 on, otherwise it is reset.
     * - The Zero flag is set if the result in the Accumulator is 0, otherwise it is reset.
     */
    AND,

    /**
     * @brief Shift left one bit, Memory or Accumulator
     */
    ASL,

    /**
     * @brief Branch on Carry Clear
     */
    BCC,

    /**
     * @brief Branch on Carry Set
     */
    BCS,

    /**
     * @brief Branch on Result Zero
     */
    BEQ,

    /**
     * @brief Test bits in Memory with Accumulator
     */
    BIT,

    /**
     * @brief Branch on Result Minus
     */
    BMI,

    /**
     * @brief Branch on Result Not Zero
     */
    BNE,

    /**
     * @brief Branch on Result Plus
     */
    BPL,

    /**
     * @brief Force Break
     */
    BRK,

    /**
     * @brief Branch on Overflow Clear
     */
    BVC,

    /**
     * @brief Branch on Overflow Set
     */
    BVS,

    /**
     * @brief Clear Carry flag
     */
    CLC,

    /**
     * @brief Clear Decimal mode
     */
    CLD,

    /**
     * @brief Clear Interrupt Disable bit
     */
    CLI,

    /**
     * @brief Clear Overflow flag
     */
    CLV,

    /**
     * @brief Compare Memory and Accumulator
     */
    CMP,

    /**
     * @brief Compare Memory and Index X
     */
    CPX,

    /**
     * @brief Compare Memory and Index Y
     */
    CPY,

    /**
     * @brief Decrement Memory by one
     */
    DEC,

    /**
     * @brief Decrement Index X by one
     */
    DEX,

    /**
     * @brief Decrement Index Y by one
     */
    DEY,

    /**
     * @brief Exclusive OR Memory with Accumulator
     *
     * The EOR instruction transfers the memory and the accumulator to the adder,
     * which performs a binary exclusive OR on a bit-by-bit basis and stores the result in the accumulator.
     *
     * This instruction affects the Status Register.
     * - The Negative flag is set if the result in the accumulator has bit 7 on, otherwise it is reset.
     * - The Zero flag is set if the result in the Accumulator is 0, otherwise it is reset.
     */
    EOR,

    /**
     * @brief Increment Memory by one
     */
    INC,

    /**
     * @brief Increment Index X by one
     */
    INX,

    /**
     * @brief Increment Index Y by one
     */
    INY,

    /**
     * @brief Jump to new location
     */
    JMP,

    /**
     * @brief Jump to new location, saving return address
     */
    JSR,

    /**
     * @brief Load Accumulator with Memory
     */
    LDA,

    /**
     * @brief Load Index X with Memory
     */
    LDX,

    /**
     * @brief Load Index Y with Memory
     */
    LDY,

    /**
     * @brief Shift right one bit, Memory or Accumulator
     */
    LSR,

    /**
     * @brief No operation
     */
    NOP,

    /**
     * @brief OR Memory with Accumulator
     *
     * The ORA instruction transfers the Memory and the Accumulator to the adder,
     * which performs a binary OR on a bit-by-bit basis and stores the result in the Accumulator.
     *
     * This instruction affects the Status Register.
     * - The Negative flag is set if the result in the accumulator has bit 7 on, otherwise it is reset.
     * - The Zero flag is set if the result in the Accumulator is 0, otherwise it is reset.
     */
    ORA,

    /**
     * @brief Push Accumulator on Stack
     */
    PHA,

    /**
     * @brief Push Processor Status on Stack
     */
    PHP,

    /**
     * @brief Pull Accumulator from Stack
     */
    PLA,

    /**
     * @brief Pull Processor Status from Stack
     */
    PLP,

    /**
     * @brief Rotate one bit left, Memory or Accumulator
     */
    ROL,

    /**
     * @brief Rotate one bit right, Memory or Accumulator
     */
    ROR,

    /**
     * @brief Return from interrupt
     */
    RTI,

    /**
     * @brief Return from subroutine
     */
    RTS,

    /**
     * @brief Subtract Memory from Accumulator with borrow
     *
     * This instruction subtracts the value of memory and borrow from the value of the Accumulator,
     * using two's complement arithmetic, and stores the result in the Accumulator.
     * Borrow is defined as the carry flag complemented, therefore, a resultant carry flag indicates that
     * a borrow has not occurred.
     *
     * This instruction affects the Status Register.
     * - The Carry flag is set if the result is greater than or equal to zero,
     *   otherwise it is reset indicating a borrow.
     * - The Overflow flag is set when the result exceeds +127 or -127, otherwise it is reset.
     * - The Negative flag is set if the result in the Accumulator has bit 7 on, otherwise it is reset.
     * - The Zero flag is set if the result in the accumulator is zero, otherwise it is reset.
     *
     * @note The programmer must set the Carry flag by using the SEC instruction before using the SBC instruction.
     */
    SBC,

    /**
     * @brief Set Carry flag
     */
    SEC,

    /**
     * @brief Set Decimal mode
     */
    SED,

    /**
     * @brief Set Interrupt Disable status
     */
    SEI,

    /**
     * @brief Store Accumulator in Memory
     */
    STA,

    /**
     * @brief Store Index X in Memory
     */
    STX,

    /**
     * @brief Store Index Y in Memory
     */
    STY,

    /**
     * @brief Transfer Accumulator to Index X
     */
    TAX,

    /**
     * @brief Transfer Accumulator to Index Y
     */
    TAY,

    /**
     * @brief Transfer Stack Pointer to Index X
     */
    TSX,

    /**
     * @brief Transfer Index X to Accumulator
     */
    TXA,

    /**
     * @brief Transfer Index X to Stack Pointer
     */
    TXS,

    /**
     * @brief Transfer Index Y to Accumulator
     */
    TYA
};

/**
 * @brief Determine the addressing mode encoded in an opcode
 *
 * @see https://www.masswerk.at/6502/6502_instruction_set.html
 *
 * @retval std::nullopt If and only if the opcode is illegal
 */
[[nodiscard]] std::optional<Addressing> getAddressing(uint8_t opcode) noexcept;

/**
 * @brief Determine the instruction encoded in an opcode
 *
 * @see https://www.masswerk.at/6502/6502_instruction_set.html
 *
 * @retval std::nullopt If and only if the opcode is illegal
 */
[[nodiscard]] std::optional<Instruction> getInstruction(uint8_t opcode) noexcept;
} // namespace mos6502

#endif //EMULATOR_MOS_6502_OPCODE_HPP
