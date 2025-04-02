//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//

#include "PulseGenerator.hpp"

namespace emulator::mos_6502 {
PulseGenerator::PulseGenerator(const PulseGenerator &other) : _pimpl(other._pimpl->clone()) {}

PulseGenerator &PulseGenerator::operator=(const PulseGenerator &other) {
    other._pimpl->clone().swap(_pimpl);
    return *this;
}

PulseGenerator &PulseGenerator::operator=(PulseGenerator &&other) noexcept {
    _pimpl.swap(other._pimpl);
    return *this;
}
} // namespace emulator::mos_6502