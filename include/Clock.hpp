//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#ifndef EMULATOR_MOS_6502_PERIODIC_PULSE_HPP
#define EMULATOR_MOS_6502_PERIODIC_PULSE_HPP
#include <chrono>

namespace emulator::mos_6502 {
/**
 * @brief A device generating short pulses at a predefined rate
 */
class Clock {
public:
    explicit Clock(std::chrono::nanoseconds period) noexcept;

    /**
     * @brief Sleep until it is time for the next pulse
     */
    void wait_for_pulse() noexcept;

private:
    /// @brief The time since the last pulse
    std::chrono::time_point<std::chrono::high_resolution_clock> _last_pulse = std::chrono::high_resolution_clock::now();

    /// @brief Minimal time between two consecutive pulses
    std::chrono::nanoseconds _period;
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_PERIODIC_PULSE_HPP
