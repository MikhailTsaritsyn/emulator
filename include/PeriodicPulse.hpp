//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#ifndef EMULATOR_MOS_6502_PERIODIC_PULSE_HPP
#define EMULATOR_MOS_6502_PERIODIC_PULSE_HPP
#include <chrono>

// TODO(optimization): Pass current time to the value().
//                     When no CPU operations are performed, call to std::chrono::...::now() consumes most of the time.
//                     It is done twice: in the CPU loop and in the clock

namespace emulator::mos_6502 {
/**
 * @brief A device generating short pulses at a predefined rate
 *
 * This class emulates a hardware clock generating pulses at a constant rate.
 * However, it does not mimic the hardware behavior exactly, but adapts it to the needs of the emulator.
 *
 * The following pulse is to be emulated:
 * @code{text}
 * ┌┐         ┌┐         ┌┐         ┌┐         ┌┐
 * ┘└─────────┘└─────────┘└─────────┘└─────────┘└────
 * ──────────────────────────────────────────────────> time
 * <─────────><─────────><─────────><─────────><─────
 *   period
 * @endcode
 *
 * Instead, the high pulse can only be generated when @link PeriodicPulse::value @endlink is called.
 * Therefore, the generator only guarantees that the distance between two consecutive high pulses is not less
 * than the period, but it is allowed to be greater:
 * @code{text}
 * value() queries
 * ↓  ↓    ↓  ↓   ↓ ↓   ↓   ↓  ↓ ↓  ↓   ↓  ↓    ↓
 * ┌┐         ┌┐            ┌┐          ┌┐
 * ┘└─────────┘└────────────┘└──────────┘└─────────
 * ──────────────────────────────────────────────────> time
 * <─────────><─────────>   <─────────> <─────────><─
 *   period              ↑
 *                       high pulse ready
 * @endcode
 * We see that pulses 3 and 4 are generated with delays, because queries did not happen at the correct times.
 */
class PeriodicPulse {
public:
    explicit PeriodicPulse(std::chrono::nanoseconds period) noexcept;

    /**
     * @return The value of the pulse at the current moment
     */
    [[nodiscard]] bool value() noexcept;

private:
    /// @brief The time since the last query resulted in a high pulse
    std::chrono::time_point<std::chrono::high_resolution_clock> _last_pulse = std::chrono::high_resolution_clock::now();

    /// @brief Minimal time between two consecutive high pulses
    std::chrono::nanoseconds _period;
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_PERIODIC_PULSE_HPP
