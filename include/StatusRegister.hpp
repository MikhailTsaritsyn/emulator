//
// Created by Mikhail Tsaritsyn on Mar 28, 2025.
//

#ifndef EMULATOR_MOS_6502_STATUS_REGISTER_HPP
#define EMULATOR_MOS_6502_STATUS_REGISTER_HPP

namespace emulator::mos_6502 {
struct StatusRegister {
    /**
     * The negative flag (N) indicates the presence of a set sign bit in bit position 7.
     * This flag is always updated, whenever a value is transferred to a CPU register (AC,X,Y),
     * and as a result of any logical ALU operations.
     * The N flag is also updated by increment and decrement operations acting on a memory location.
     */
    bool negative : 1 = false;

    /**
     * The overflow flag indicates overflow with signed binary arithmetics.
     * As a signed byte represents a range of -128 to +127, an overflow can never occur
     * when the operands are of opposite sign, since the result never exceeds this range.
     * Thus, overflow may only occur if both operands are of the same sign.
     * Then, the result must be also of the same sign. Otherwise, overflow is detected and the overflow flag is set.
     * For example, both operands have a zero in the sign position at bit 7, but bit 7 of the result is 1,
     * or both operands have the sign-bit set, but the result is positive.
     */
    bool overflow : 1 = false;

    bool expansion : 1 = false; ///< ignored

    /**
     * The break flag (B) is not an actual flag implemented in a register,
     * and rather appears only when the status register is pushed onto or pulled from the stack.
     * When pushed, it is 1 when transferred by a BRK or PHP instruction, and zero otherwise,
     * for example, when pushed by a hardware interrupt.
     * When pulled into the status register (by PLP or on RTI), it is ignored.
     * In other words, the break flag is inserted whenever the status register is transferred to the stack
     * by software (BRK or PHP), and is zero when transferred by hardware.
     * Since there is no actual slot for the break flag, it is always ignored when retrieved (PLP or RTI).
     * The break flag is not accessed by the CPU at any time and there is no internal representation.
     * Its purpose is more for patching, to discern an interrupt caused by a BRK instruction from a normal
     * interrupt initiated by hardware.
     */
    bool break_ : 1 = false;

    /**
     * The decimal flag sets the ALU to Binary Coded Decimal (BCD) mode for additions and subtractions (ADC, SBC).
     */
    bool decimal : 1 = false;

    /**
     * The interrupt-inhibit flag blocks any maskable interrupt requests (IRQ).
     */
    bool interrupt : 1 = false;

    /**
     * The zero flag indicates a value of all zero bits.
     * This flag is always updated whenever a value is transferred to a CPU register (AC,X,Y)
     * and as a result of any logical ALU operations.
     * The Z flag is also updated by increment and decrement operations acting on a memory location.
     */
    bool zero : 1 = false;

    /**
     * The carry flag is used as a buffer and as a borrow in arithmetic operations.
     * Any comparison updates this additionally to the Z and N flags, as do shift and rotate operations.
     */
    bool carry : 1 = false;
};
} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_STATUS_REGISTER_HPP
