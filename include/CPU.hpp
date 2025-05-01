//
// Created by Mikhail Tsaritsyn on Apr 01, 2025.
//

#ifndef EMULATOR_MOS_6502_CPU_HPP
#define EMULATOR_MOS_6502_CPU_HPP
#include "ALU.hpp"
#include "Clock.hpp"
#include "Memory.hpp"
#include "Opcode.hpp"
#include "Registers.hpp"
#include <atomic>
#include <tuple>

// TODO: on_clock_pulse(Func &&func) wrapper?
// TODO: instead of passing memory + PC pass iterator to the current byte?

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

    CPU() noexcept = default;

    /**
     * @brief Start the CPU
     *
     * @param memory
     * @param clock Pulse generator for the CPU.
     * @param registers
     *
     * It enters an endless loop executing instructions one by one.
     */
    void start(Memory &memory, Clock &clock, Registers &registers) noexcept;

    /**
     * @brief Reset the CPU to its initial state
     */
    static void reset(Clock &clock, const Memory &memory, mtl::u16 &PC, mtl::u8 &SP, bool &interrupt_disable) noexcept;

    /**
     * @brief Terminate the execution of the CPU
     *
     * It is designed to be called from a thread other than that running the CPU.
     */
    void terminate() noexcept;

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
     * @brief Addressing modes that can be used to read a value from the memory
     */
    using ReadAddress = std::variant<immediate_t, mtl::u16>;

    /**
     * @brief Determine the address from which the current instruction's argument can be read
     *
     * @param[in]      addressing Addressing mode of the instruction
     * @param[in]      memory Memory used by the CPU
     * @param[in, out] PC The program counter
     * @param[in]      X Index register X
     * @param[in]      Y Index register Y
     * @param[in, out] clock Emulated CPU clock
     *
     * @retval std::nullopt If and only if the given addressing mode does not support the desired address
     */
    [[nodiscard]] static std::optional<ReadAddress> fetch_read_address(
            Addressing addressing, const Memory &memory, mtl::u16 &PC, mtl::u8 X, mtl::u8 Y, Clock &clock) noexcept;

    /**
     * @brief Determine the address of the current instruction's argument
     *
     * @param[in]      addressing Addressing mode of the instruction
     * @param[in]      memory Memory used by the CPU
     * @param[in, out] pc The program counter
     * @param[in]      x Index register X
     * @param[in]      y Index register Y
     * @param[in, out] clock Emulated CPU clock
     */
    [[nodiscard]] static Address fetch_address(
            Addressing addressing, const Memory &memory, mtl::u16 &pc, mtl::u8 x, mtl::u8 y, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16 fetch_absolute_address(const Memory &memory, mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16
    fetch_absolute_address(const Memory &memory, mtl::u16 &pc, mtl::u8 index, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16 fetch_indirect_address(const Memory &memory, mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16
    fetch_indexed_indirect_address(const Memory &memory, mtl::u16 &pc, mtl::u8 x, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16
    fetch_indirect_indexed_address(const Memory &memory, mtl::u16 &pc, mtl::u8 y, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16 fetch_zero_page_address(const Memory &memory, mtl::u16 &pc, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16
    fetch_zero_page_address(const Memory &memory, mtl::u16 &pc, mtl::u8 index, Clock &clock) noexcept;

    [[nodiscard]] static bool decode_and_execute(mtl::u8 opcode, Clock &clock, Memory &memory, Registers &registers);

    /**
     * @brief Read a byte from a specified address of the memory
     *
     * @pre Waits until the next high pulse arrives from @p clock.
     *
     * @post Increments the cycle count.
     */
    static mtl::u8 read(const Memory &memory, mtl::u16 address, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u8 read(const Memory &memory, ReadAddress address, mtl::u16 &PC, Clock &clock) noexcept;

    /**
     * @brief Jump by a signed offset
     *
     * The offset is read at the current program counter.
     *
     * @param[in]      memory Memory used by the CPU
     * @param[in]      pc The current program counter.
     * @param[in]      condition Whether to perform the jump.
     *                           If @c false, continue execution at the current program counter.
     * @param[in, out] clock Emulated CPU clock
     *
     * @return The new program counter value
     */
    [[nodiscard]] static mtl::u16 branch(const Memory &memory, mtl::u16 pc, bool condition, Clock &clock) noexcept;

    /**
     * @return {negative, carry, zero}
     */
    [[nodiscard]] static std::tuple<bool, bool, bool> compare(mtl::u8 a, mtl::u8 b) noexcept;

    [[nodiscard]] static mtl::u8 push(Memory &memory, mtl::u8 sp, mtl::u8 byte) noexcept;

    /**
     * @break Jump to the interrupt handler
     *
     * Saves the current program counter and status register on the stack.
     *
     * @param[in]      memory Memory used by the CPU
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
    [[nodiscard]] static std::pair<mtl::u16, mtl::u8> interrupt(Memory &memory,
                                                                mtl::u16 pc,
                                                                mtl::u8 sp,
                                                                StatusRegister sr,
                                                                mtl::u16 handler_address,
                                                                Clock &clock) noexcept;

    /**
     * @brief Transfers from the stack the processor status and the program counter for the instruction
     *        which was interrupted
     *
     * @param[in]      memory Memory used by the CPU
     * @param[in]      pc Current program counter
     * @param[in]      sp Current stack pointer
     * @param[in, out] clock Emulated CPU clock
     *
     * @return {PC, SP, SR}
     * @retval PC Program counter of the interrupted instruction
     * @retval SP Updated stack pointer
     * @retval SR Status register before interrupt
     */
    [[nodiscard]] static std::tuple<mtl::u16, mtl::u8, StatusRegister>
    return_from_interrupt(const Memory &memory, mtl::u16 pc, mtl::u8 sp, Clock &clock) noexcept;

    [[nodiscard]] static mtl::u16
    fetch_absolute_address_long(const Memory &memory, mtl::u16 &pc, mtl::u8 index, Clock &clock) noexcept;

    /**
     * @param src The new value
     *
     * @return {value, zero, negative}
     * @retval value The given value
     * @retval zero Is set if the value is zero, otherwise it is reset.
     * @retval negative Is set if the result has bit 7 on, otherwise it is reset.
     */
    [[nodiscard]] static std::tuple<mtl::u8, bool, bool> value_with_flags(mtl::u8 src) noexcept;

    /// @brief If @p true, the CPU must stop after completing the current operation
    std::atomic_flag _terminate = false;

    std::atomic_flag _interrupt_requested = false;

    std::atomic_flag _non_maskable_interrupt_requested = false;
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_CPU_HPP
