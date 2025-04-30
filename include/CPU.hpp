//
// Created by Mikhail Tsaritsyn on Apr 01, 2025.
//

#ifndef EMULATOR_MOS_6502_CPU_HPP
#define EMULATOR_MOS_6502_CPU_HPP
#include "ALU.hpp"
#include "Clock.hpp"
#include "Memory.hpp"
#include "Opcode.hpp"
#include "StatusRegister.hpp"
#include <atomic>
#include <tuple>

namespace emulator::mos_6502 {
class CPU {
public:
    /**
     * @brief Non-Maskable Interrupt vector
     */
    static constexpr mtl::u16 NMI{ 0xFFFA };

    /**
     * @brief Reset vector
     */
    static constexpr mtl::u16 RES{ 0xFFFC };

    /**
     * @brief Interrupt Request vector
     */
    static constexpr mtl::u16 IRQ{ 0xFFFE };

    explicit CPU(std::chrono::nanoseconds clock_period, Memory memory) noexcept;

    /**
     * @brief Start the CPU
     *
     * It enters an endless loop executing instructions one by one.
     */
    void start() noexcept;

    /**
     * @brief Reset the CPU to its initial state
     */
    void reset(Clock &clock) noexcept;

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

    [[nodiscard]] mtl::u16 program_counter() const noexcept;

    [[nodiscard]] size_t cycle() const noexcept;

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
    // TODO: specialize reading addresses
    // TODO: specialize writing addresses
    using Address = std::variant<accumulator_t, implicit_t, immediate_t, relative_t, mtl::u16>;

    /**
     * @brief Wait for the next clock tick and increment the cycle counter
     */
    void wait_for_pulse(Clock &clock) noexcept;

    /**
     * @brief Determine the address of the current instruction's argument
     *
     * @param[in]      addressing Addressing mode of the instruction
     * @param[in, out] pc The program counter
     * @param[in]      x Index register X
     * @param[in]      y Index register Y
     * @param[in, out] clock Emulated CPU clock
     */
    [[nodiscard]] Address
    fetch_address(Addressing addressing, mtl::u16 &pc, mtl::u8 x, mtl::u8 y, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_absolute_address(mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_absolute_address(mtl::u16 &pc, mtl::u8 index, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_indirect_address(mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_indexed_indirect_address(mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_indirect_indexed_address(mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_zero_page_address(mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_zero_page_address(mtl::u16 &pc, mtl::u8 index, Clock &clock) noexcept;

    [[nodiscard]] bool decode_and_execute(mtl::u8 opcode, Clock &clock);

    /**
     * @brief Read a byte from a specified address of the memory
     *
     * @pre Waits until the next high pulse arrives from @link _clock @endlink.
     *
     * @post Increments the cycle count.
     */
    mtl::u8 read(mtl::u16 address, Clock &clock) noexcept;

    /**
     * @brief Jump by a signed offset
     *
     * The offset is read at the current program counter.
     *
     * @param[in]      pc The current program counter.
     * @param[in]      condition Whether to perform the jump.
     *                           If @c false, continue execution at the current program counter.
     * @param[in, out] clock Emulated CPU clock
     *
     * @return The new program counter value
     */
    [[nodiscard]] mtl::u16 branch(mtl::u16 pc, bool condition, Clock &clock) noexcept;

    /**
     * @return {negative, carry, zero}
     */
    [[nodiscard]] static std::tuple<bool, bool, bool> compare(mtl::u8 a, mtl::u8 b) noexcept;

    [[nodiscard]] mtl::u8 push(mtl::u8 sp, mtl::u8 byte) noexcept;

    /**
     * @break Jump to the interrupt handler
     *
     * Saves the current program counter and status register on the stack.
     *
     * @param[in]      pc Current program counter
     * @param[in]      sp Current stack pointer
     * @param[in]      sr Current status register
     * @param[in]      handler_address Address where the low byte of the interrupt handler's address is stored.
     * @param[in, out] clock Emulated CPU clock
     *
     * @return {PC, SP}
     * @retvap PC New program counter pointing at the start of the interrupt handling routine
     * @retval SP Updated stack pointer
     */
    [[nodiscard]] std::pair<mtl::u16, mtl::u8>
    interrupt(mtl::u16 pc, mtl::u8 sp, StatusRegister sr, mtl::u16 handler_address, Clock &clock) noexcept;

    /**
     * @brief Transfers from the stack the processor status and the program counter for the instruction
     *        which was interrupted
     *
     * @param[in]      pc Current program counter
     * @param[in]      sp Current stack pointer
     * @param[in, out] clock Emulated CPU clock
     *
     * @return {PC, SP, SR}
     * @retval PC Program counter of the interrupted instruction
     * @retval SP Updated stack pointer
     * @retval SR Status register before interrupt
     */
    [[nodiscard]] std::tuple<mtl::u16, mtl::u8, StatusRegister>
    return_from_interrupt(mtl::u16 pc, mtl::u8 sp, Clock &clock) noexcept;

    [[nodiscard]] mtl::u16 fetch_absolute_address_long(mtl::u16 &pc, mtl::u8 index, Clock &clock) noexcept;

    /**
     * @param src The new value
     *
     * @return {value, zero, negative}
     * @retval value The given value
     * @retval zero Is set if the value is zero, otherwise it is reset.
     * @retval negative Is set if the result has bit 7 on, otherwise it is reset.
     */
    [[nodiscard]] static std::tuple<mtl::u8, bool, bool> value_with_flags(mtl::u8 src) noexcept;

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
     * @brief Time spent executing all instructions up to this moment
     */
    std::chrono::duration<double> _elapsed{ 0 };

    /**
     * @brief The number of clock pulses generated by the moment
     */
    size_t _cycle = 0;

    std::atomic_flag _interrupt_requested = false;

    std::atomic_flag _non_maskable_interrupt_requested = false;
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_CPU_HPP
