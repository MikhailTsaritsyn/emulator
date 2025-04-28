//
// Created by Mikhail Tsaritsyn on Apr 03, 2025.
//

#include "Memory.hpp"

#include <algorithm>

namespace emulator::mos_6502 {
Memory::Memory(const Data &data) noexcept : _data(data) {}

Memory::Memory(const Data &data, std::unordered_set<mtl::u16> rom_masks) noexcept
        : _data(data),
          _rom_masks(std::move(rom_masks)) {}

Memory Memory::Commodore64(const Data &data) noexcept {
    return {
        data, { mtl::u16(0xA000), mtl::u16(0xD000) }
    };
}

Memory Memory::AppleII(const Data &data) noexcept { return { data, { mtl::u16{ 0xC000 } } }; }

mtl::u8 Memory::operator[](const mtl::u16 address) const noexcept { return _data[address.to_underlying()]; }

bool Memory::write(const mtl::u16 address, const mtl::u8 value) noexcept {
    if (within_rom(address)) return false;
    _data[address.to_underlying()] = value;
    return true;
}

bool Memory::within_rom(const mtl::u16 address) const noexcept {
    return std::ranges::any_of(_rom_masks, [address](const mtl::u16 mask) { return (address & mask) == mask; });
}
} // namespace emulator::mos_6502