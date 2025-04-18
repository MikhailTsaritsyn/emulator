//
// Created by Mikhail Tsaritsyn on Apr 01, 2025.
//

#ifndef EMULATOR_MOS_6502_CPU_HPP
#define EMULATOR_MOS_6502_CPU_HPP
#include "Clock.hpp"
#include "Memory.hpp"
#include "Opcode.hpp"
#include "StatusRegister.hpp"
#include <atomic>

namespace emulator::mos_6502 {
class CPU {
public:
    /**
     * @brief Reset vector
     *
     * At this address in ROM lies the initial value of the PC register.
     */
    static constexpr uint16_t RES = 0xFFFC;

    explicit CPU(std::chrono::nanoseconds clock_period, const Memory &memory) noexcept;

    /**
     * @brief Start the CPU
     *
     * It enters an endless loop executing instructions one by one.
     */
    void start() noexcept;

    /**
     * @brief Reset the CPU to its initial state
     */
    void reset() noexcept;

    /**
     * @brief Terminate the execution of the CPU
     *
     * It is designed to be called from a thread other than that running the CPU.
     */
    void terminate() noexcept;

    /**
     * @brief Estimated clock frequency
     */
    [[nodiscard]] double frequency() const noexcept;

    /**
     * @brief Get a view of the CPU's memory
     */
    [[nodiscard]] const Memory &memory() const & noexcept;

    /**
     * @brief Get a copy of the CPU's memory
     */
    [[nodiscard]] Memory &&memory() && noexcept;

private:
    /**
     * @brief The argument of the current operation os the accumulator
     */
    struct accumulator_t {};

    /**
     * @brief The current operation has no arguments
     */
    struct implicit_t {};

    /**
     * @brief The argument of the current operation is written right after the opcode
     */
    struct immediate_t {};

    /**
     * @brief The branching offset is written right after the opcode
     */
    struct relative_t {};

    /**
     * @broef Address where to fetch the argument of the current operation
     */
    using Address = std::variant<accumulator_t, implicit_t, immediate_t, relative_t, uint16_t>;

    /**
     * @brief Determine the address of the argument of the current instruction
     *
     * @param addressing Addressing mode of the instruction
     */
    [[nodiscard]] Address fetch_address(Addressing addressing) noexcept;

    [[nodiscard]] uint16_t fetch_absolute_address() noexcept;

    [[nodiscard]] uint16_t fetch_absolute_address(uint8_t index) noexcept;

    [[nodiscard]] uint16_t fetch_indirect_address() noexcept;

    [[nodiscard]] uint16_t fetch_indexed_indirect_address() noexcept;

    [[nodiscard]] uint16_t fetch_indirect_indexed_address() noexcept;

    [[nodiscard]] uint16_t fetch_zero_page_address() noexcept;

    [[nodiscard]] uint16_t fetch_zero_page_address(uint8_t index) noexcept;

    /**
     * @brief Execute the current instruction
     *
     * @param instruction The instruction to execute
     * @param address The address where to find the instruction argument
     */
    void execute(Instruction instruction, Address address) noexcept;

    /**
     * @brief Construct a 16-bit unsigned integer from two 8-bit unsigned integers
     *
     * @param high High byte of the result
     * @param low Low byte of the result
     */
    [[nodiscard]] static uint16_t make_word(uint8_t high, uint8_t low) noexcept;

    /**
     * @brief Read a byte from a specified address of the memory
     *
     * @pre Waits until the next high pulse arrives from @link _clock @endlink.
     *
     * @post Increments the cycle count.
     */
    uint8_t read(uint16_t address) noexcept;

    /**
     * @brief Program counter
     *
     * The program counter keeps track of the memory location holding the current instruction code.
     * Its content is automatically stepped up as the program is executed and is modified by branch and jump operations.
     * As it must be able to address the full 16-bit address range of 64K bytes, it's the only 16-bit register of the
     * 6502.
     */
    uint16_t PC = 0;

    /**
     * @brief Accumulator
     *
     * The accumulator is the main register of the 6502.
     * Its content is typically used by the Arithmetic Logic Unit (ALU) for the first operand,
     * and results are deposited in the accumulator again.
     * Thus, its name, as results accumulate in this register.
     * Most arithmetic and logical operations interact with this register.
     */
    uint8_t AC = 0;

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
    uint8_t X = 0;

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
    uint8_t Y = 0;

    /**
     * @brief Stack pointer
     *
     * The stack pointer points to the current top of stack, or rather, to its bottom, as the stack grows top-down.
     * The processor stack is located on memory page #1 ($0100–$01FF), 256 bytes Last-In-First-Out (LIFO) stack,
     * which enables subroutines and also serves as a quick intermediate storage.
     * As an 8-bit register, the stack pointer holds just the low-byte of this address (the offset from $0100.)
     * Be aware that this just wraps around in case that the stack underflows.
     */
    uint8_t SP = 0;

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

    /**
     * @brief Pulse generator of the CPU.
     *
     * Execution of the next instruction can only start when the pulse is high.
     */
    Clock _clock;

    /// @brief Memory used by the CPU
    Memory _memory;

    /// @brief If @p true, the CPU must stop after completing the current operation
    std::atomic_flag _terminate = false;

    /**
     * @brief Estimated clock frequency
     */
    double _frequency = 0.;

    /**
     * @brief The number of clock pulses generated by the moment
     */
    size_t _cycle = 0;
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_CPU_HPP
