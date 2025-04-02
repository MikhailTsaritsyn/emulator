//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#ifndef EMULATOR_MOS_6502_PULSE_ON_HPP
#define EMULATOR_MOS_6502_PULSE_ON_HPP

namespace emulator::mos_6502 {
/**
 * @brief Generates a pulse that is always high
 */
struct PulseOn {
    [[nodiscard]] constexpr static bool value() noexcept { return true; }
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_PULSE_ON_HPP
