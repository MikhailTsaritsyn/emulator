//
// Created by Mikhail Tsaritsyn on Apr 20, 2025.
//

#ifndef EMULATOR_MOS_6502_HELPERS_HPP
#define EMULATOR_MOS_6502_HELPERS_HPP
#include <cstdint>
#include <utility>

namespace emulator::mos_6502 {
std::pair<uint8_t, bool> add_with_overflow(uint8_t a, uint8_t b) noexcept;
}

#endif //EMULATOR_MOS_6502_HELPERS_HPP
