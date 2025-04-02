//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#ifndef EMULATOR_MOS_6502_PULSE_GENERATOR_HPP
#define EMULATOR_MOS_6502_PULSE_GENERATOR_HPP
#include <memory>
#include <utility>

namespace emulator::mos_6502 {
/**
 * @brief Class generating short pulses
 *
 * The value of a pulse can be high or low. It can be queried using @link PulseGenerator::value @endlink.
 *
 * @note This class does not define a move constructor, but falls back to the copy constructor instead.
 *       This is done because neither a default nor move-after state can be defined.
 *       Therefore, its move is not @p noexcept.
 *
 * @see https://youtu.be/7MNyAHp0h7A?si=NhIvd8Zh5x1_yoYb
 */
// TODO: Implement a default "turned off" generator that always gives low value?
// TODO: Implement a "turned on" generator that always gives high value for maximum possible performance?
// TODO: Implement a manual generator only activated by some user input?
class PulseGenerator {
public:
    /**
     * @brief Wrap a given pulse generator
     */
    template <typename GenT>
    explicit PulseGenerator(GenT generator) : _pimpl(std::make_unique<Model<GenT>>(std::move(generator))) {}

    PulseGenerator(const PulseGenerator &other);

    PulseGenerator &operator=(const PulseGenerator &other);

    PulseGenerator &operator=(PulseGenerator &&other) noexcept;

    /**
     * @return The pulse value at the current moment
     */
    [[nodiscard]] bool value() const noexcept { return _pimpl->value(); }

private:
    struct Concept {
        virtual ~Concept() noexcept = default;

        [[nodiscard]] virtual bool value() noexcept = 0;

        [[nodiscard]] virtual std::unique_ptr<Concept> clone() = 0;
    };

    template <typename GenT> struct Model final : Concept {
        explicit Model(GenT gen) noexcept(std::is_nothrow_move_assignable_v<GenT>) : _gen(std::move(gen)) {}

        [[nodiscard]] bool value() noexcept override { return _gen.value(); }

        [[nodiscard]] std::unique_ptr<Concept> clone() override { return std::make_unique<Model>(*this); }

        GenT _gen;
    };

    /**
     * @brief Polymorphic wrapper of the current generator
     */
    std::unique_ptr<Concept> _pimpl;
};

} // namespace emulator::mos_6502

#endif //EMULATOR_MOS_6502_PULSE_GENERATOR_HPP
