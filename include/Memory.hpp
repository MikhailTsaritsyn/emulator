//
// Created by Mikhail Tsaritsyn on Apr 03, 2025.
//

#ifndef EMULATOR_MOS_6502_MEMORY_HPP
#define EMULATOR_MOS_6502_MEMORY_HPP
#include <array>
#include <cstdint>
#include <limits>
#include <unordered_set>

// TODO: Implement Atari 2600 for a 6507 CPU with 8 KB of memory

namespace emulator::mos_6502 {
/**
 * @brief A device used as a memory for MOS 6502 CPU.
 *
 * Such devices are quite simple. They guarantee to have a value for any 16-bit address.
 * However, they might be partitioned into read-write (RAM) and read-only (ROM) parts.
 * Therefore, the value at any address can be read, but an attempt to write into a read-only cell has no effect.
 * Such values must be set at the object construction.
 *
 * Any memory must have the first two pages as RAM for the following purposes:
 * - 0x0000 to 0x00FF - Zero Page
 * - 0x0100 to 0x01FF - Stack
 *
 * Similarly, any memory must have the following addresses as ROM for the system vectors:
 * - 0xFFFA to 0xFFFB - Address of non-maskable interrupt handler
 * - 0xFFFC to 0xFFFD - Initial value of the program counter
 * - 0xFFFE to 0xFFFF - Address of maskable interrupt handler
 */
class Memory {
public:
    /**
     * @brief Underlying container storing the data
     */
    using Data = std::array<uint8_t, static_cast<size_t>(std::numeric_limits<uint16_t>::max()) + 1>;

    /**
     * @brief Initialize from existing data with minimal partitioning
     *
     * Only the system vectors belong to the ROM, all other memory is read-write.
     */
    explicit Memory(const Data &data) noexcept;

    /**
     * @brief Initialize from existing data with a custom partition.
     *
     * Partition is defined by a set of masks applied to an address.
     * If address satisfies any of them, it belongs to the ROM, otherwise to the RAM.
     * An address satisfies a mask if and only if it contains one at every position where the mask contains one.
     *
     * Example: set of masks {0xFFFA, 0xFFFC} only selects the system vectors as ROM.
     * The fist mask can be written in binary as 0b1111111111111010, so any address smaller than 0b1111111111111000
     * has at least one zero in a bit where the mask is set.
     * Addresses between 0xFFF8 and 0xFFFA have a zero where the last set bit of the mask is.
     *
     * The second mask is even simpler.
     * In binary, it is 0b1111111111111100, so any smaller address does not have enough initial ones.
     */
    Memory(const Data &data, std::unordered_set<uint16_t> rom_masks) noexcept;

    /**
     * @brief Partitioning preset for Commodore64 machines.
     *
     * @see https://archive.org/details/The_Master_Memory_Map_for_the_Commodore_64
     *
     * - 0x0000 to 0x09FF - RAM:
     *   - 0x0200 to 0x03FF - Used for storing system-related data like screen line links.
     *   - 0x0400 to 0x07FF - Video Memory to be accessed by VIC-II graphics chip to display text or graphics.
     *   - 0x0800 to 0x9FFF - General-purpose RAM.
     * - 0xA000 to 0xBFFF - ROM to store the BASIC interpreter.
     * - 0xC000 to 0xCFFF - Additional RAM for various purposes.
     * - 0xD000 to 0xFFFF - ROM:
     *   - 0xD000 to 0xDFFF - Memory-mapped I/O registers for accessing hardware like the VIC-II (graphics chip),
     *     SID (sound chip), and CIA (interface adapters).
     *     Part of this space could also contain the character ROM, which stored font data.
     *   - 0xE000 to 0xFFFF - The kernel ROM contained essential routines for low-level operations like I/O handling,
     *     screen display, and interrupt management.
     */
    [[nodiscard]] static Memory Commodore64(const Data &data) noexcept;

    /**
     * @brief Partitioning preset for Apple II machines.
     *
     * @see https://archive.org/details/understanding_the_apple_ii
     *
     * - 0x0000 to 0xBFFF - RAM:
     *   - 0x0200 to 0x02FF - Keyboard buffer to store characters typed on the keyboard before processing.
     *   - 0x0300 to 0x03FF - User area available for short machine language programs or temporary storage.
     *   - 0x0400 to 0x07FF - Text screen memory used for displaying text on the screen.
     *   - 0x0800 to 0x1FFF - Applesoft BASIC containing the Applesoft BASIC interpreter,
     *     which allows users to write and execute BASIC programs.
     *   - 0x2000 to 0x3FFF - High-resolution graphics page 1.
     *   - 0x4000 to 0x5FFF - High-resolution graphics page 2.
     *   - 0x6000 to 0xBFFF - General-purpose RAM for user programs and data.
     * - 0xC000 to 0xFFFF - ROM:
     *   - 0xC000 to 0xCFFF - Memory-mapped I/O for interfacing with hardware like disk drives, printers,
     *     and expansion cards.
     *   - 0xD000 to 0xFFFF - Contains the system firmware, including Integer BASIC, the monitor program,
     *     and other essential routines.
     */
    [[nodiscard]] static Memory AppleII(const Data &data) noexcept;

    /**
     * @brief Read a value at a given address
     */
    [[nodiscard]] uint8_t operator[](uint16_t address) const noexcept;

    /**
     * @brief Write a value to a given address.
     *
     * If the address belongs to ROM, the operation has no effect.
     *
     * @retval true If the write operation succeeded.
     * @retval false If and only if the address lies within the ROM.
     */
    bool write(uint16_t address, uint8_t value) noexcept;

private:
    [[nodiscard]] bool within_rom(uint16_t address) const noexcept;

    Data _data; ///< Encapsulated data

    std::unordered_set<uint16_t> _rom_masks = { 0xFFFA, 0xFFFC }; ///< Masks defining read-only addresses
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_MEMORY_HPP
