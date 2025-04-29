//
// Created by Mikhail Tsaritsyn on Apr 29, 2025.
//

#include "StatusRegister.hpp"

namespace emulator::mos_6502 {
StatusRegister::operator mtl::u8() const noexcept {
    mtl::u8 result{ 0 };
    if (negative) result |= mtl::u8{ 0b00000001 };
    if (overflow) result |= mtl::u8{ 0b00000010 };
    if (expansion) result |= mtl::u8{ 0b0000100 };
    if (break_) result |= mtl::u8{ 0b00001000 };
    if (decimal) result |= mtl::u8{ 0b00010000 };
    if (interrupt_disable) result |= mtl::u8{ 0b00100000 };
    if (zero) result |= mtl::u8{ 0b01000000 };
    if (carry) result |= mtl::u8{ 0b10000000 };
    return result;
}

StatusRegister &StatusRegister::operator=(const mtl::u8 value) noexcept {
    negative          = (value & mtl::u8{ 0b00000001 }) != 0;
    overflow          = (value & mtl::u8{ 0b00000010 }) != 0;
    expansion         = (value & mtl::u8{ 0b00000100 }) != 0;
    break_            = (value & mtl::u8{ 0b00001000 }) != 0;
    decimal           = (value & mtl::u8{ 0b00010000 }) != 0;
    interrupt_disable = (value & mtl::u8{ 0b00100000 }) != 0;
    zero              = (value & mtl::u8{ 0b01000000 }) != 0;
    carry             = (value & mtl::u8{ 0b10000000 }) != 0;
    return *this;
}
} // namespace emulator::mos_6502
