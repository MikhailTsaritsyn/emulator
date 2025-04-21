//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//
#include "CPU.hpp"

#include "helpers.hpp"
#include <chrono>
#include <utility>

namespace emulator::mos_6502 {

CPU::CPU(const std::chrono::nanoseconds clock_period, const Memory &memory) noexcept
        : _clock(clock_period),
          _memory(memory) {}

void CPU::start() noexcept {
    // reset();

    auto prev_time = std::chrono::high_resolution_clock::now();
    static constexpr size_t window = 100;
    while (!_terminate.test()) {
        [[maybe_unused]] const auto opcode = read(PC++);

        // const auto instruction = getInstruction(opcode);
        // const auto addressing  = getAddressing(opcode);
        //
        // if (!instruction || !addressing) {
        //     std::cerr << std::format("Encountered an illegal opcode {:#02x} at address {:#04x}", opcode, PC - 1)
        //               << std::endl;
        //     _terminate.test_and_set();
        // }

        // TODO:
        // execute(*instruction, fetch_address(*addressing));

        // Update the elapsed time every 100 pulses to reduce the overhead
        if (_cycle % window == 0) {
            const auto current_time                     = std::chrono::high_resolution_clock::now();
            _elapsed += current_time - prev_time;
            prev_time                                   = current_time;
        }
    }

    // Add the remaining cycles after the last window
    _elapsed += std::chrono::high_resolution_clock::now() - prev_time;
}

void CPU::terminate() noexcept { _terminate.test_and_set(); }

double CPU::frequency() const noexcept { return static_cast<double>(_cycle) / _elapsed.count(); }

const Memory &CPU::memory() const & noexcept { return _memory; }

Memory &&CPU::memory() && noexcept { return std::move(_memory); }

CPU::Address CPU::fetch_address(const Addressing addressing) noexcept {
    switch (addressing) {
    case Addressing::Accumulator: return accumulator_t{};
    case Addressing::Absolute: return fetch_absolute_address();
    case Addressing::AbsoluteX: return fetch_absolute_address(X);
    case Addressing::AbsoluteY: return fetch_absolute_address(Y);
    case Addressing::Implicit: return implicit_t{};
    case Addressing::Immediate: return immediate_t{};
    case Addressing::IndexedIndirect: return fetch_indexed_indirect_address();
    case Addressing::IndirectIndexed: return fetch_indirect_indexed_address();
    case Addressing::Relative: return relative_t{};
    case Addressing::ZeroPage: return fetch_zero_page_address();
    case Addressing::ZeroPageX: return fetch_zero_page_address(X);
    case Addressing::ZeroPageY: return fetch_zero_page_address(Y);
    case Addressing::Indirect: return fetch_indirect_address();
    }
    std::unreachable();
}

uint16_t CPU::fetch_absolute_address() noexcept {
    const auto address_low  = read(PC++);
    const auto address_high = read(PC++);
    return make_word(address_high, address_low);
}

uint16_t CPU::fetch_absolute_address(const uint8_t index) noexcept {
    const auto bal                     = read(PC++);
    const auto bah                     = read(PC++);
    const auto [bal_updated, overflow] = add_with_overflow(bal, index);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(address);
    return make_word(bah + 1, bal_updated);
}

uint16_t CPU::fetch_indirect_address() noexcept {
    const auto ial = read(PC++);
    const auto iah = read(PC++);
    PC             = make_word(iah, ial);
    const auto adl = read(PC++);
    const auto adh = read(PC);
    return make_word(adh, adl);
}

uint16_t CPU::fetch_indexed_indirect_address() noexcept {
    const auto bal = read(PC++);
    read(bal);
    const auto adl = read(make_word(0, bal + X));
    const auto adh = read(make_word(0, static_cast<uint8_t>(bal + X + 1)));
    return make_word(adh, adl);
}

uint16_t CPU::fetch_indirect_indexed_address() noexcept {
    const auto ial                     = read(PC++);
    const auto bal                     = read(ial);
    const auto bah                     = read(make_word(0, ial + 1));
    const auto [bal_updated, overflow] = add_with_overflow(bal, Y);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(address);
    return make_word(bah + 1, bal_updated);
}

uint16_t CPU::fetch_zero_page_address() noexcept { return read(PC++); }

uint16_t CPU::fetch_zero_page_address(const uint8_t index) noexcept {
    const auto adl = read(PC++);
    read(adl); // This data is ignored
    return make_word(0, adl + index);
}

uint16_t CPU::make_word(const uint8_t high, const uint8_t low) noexcept {
    return static_cast<uint16_t>(high) << 8 | static_cast<uint16_t>(low);
}

void CPU::reset() noexcept {
    read(PC++);
    read(PC++);
    read(0x0100 + SP);
    read(0x0100 + SP - 1);
    read(0x0100 + SP - 2);
    const auto pcl = read(RES);
    const auto pch = read(RES + 1);
    PC             = make_word(pch, pcl);
}

uint8_t CPU::read(const uint16_t address) noexcept {
    _clock.wait_for_pulse();
    _cycle++;
    return _memory[address];
}
} // namespace emulator::mos_6502