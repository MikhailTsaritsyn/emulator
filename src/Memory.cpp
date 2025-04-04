//
// Created by Mikhail Tsaritsyn on Apr 03, 2025.
//

#include "Memory.hpp"

#include <algorithm>

namespace emulator::mos_6502 {
Memory::Memory(const Data &data) noexcept : _data(data) {}

Memory::Memory(const Data &data, std::unordered_set<uint16_t> rom_masks) noexcept
        : _data(data),
          _rom_masks(std::move(rom_masks)) {}

Memory Memory::Commodore64(const Data &data) noexcept { return { data, { 0xA000, 0xD000 } }; }

Memory Memory::AppleII(const Data &data) noexcept { return { data, {0xC000} };
}

uint8_t Memory::operator[](const uint16_t address) const noexcept { return _data[address]; }

bool Memory::write(const uint16_t address, const uint8_t value) noexcept {
    if (within_rom(address)) return false;
    _data[address] = value;
    return true;
}

bool Memory::within_rom(const uint16_t address) const noexcept {
    return std::ranges::any_of(_rom_masks, [address](const uint16_t mask) { return (address & mask) == mask; });
}
} // namespace emulator::mos_6502