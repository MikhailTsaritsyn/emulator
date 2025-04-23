//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//
#include "CPU.hpp"

#include "ALU.hpp"
#include "helpers.hpp"
#include <chrono>
#include <iostream>

namespace emulator::mos_6502 {
CPU::CPU(const std::chrono::nanoseconds clock_period, Memory memory) noexcept
        : _clock(clock_period),
          _memory(std::move(memory)) {}

void CPU::start() noexcept {
    auto prev_time = std::chrono::high_resolution_clock::now();

    reset();

    static constexpr size_t window = 100;
    while (!_terminate.test()) {
        if (_non_maskable_interrupt_requested.test()) interrupt(NMI);
        if (_interrupt_requested.test() && !SR.interrupt_disable) interrupt(IRQ);

        [[maybe_unused]] const auto opcode = read(PC++);

        if (!decode_and_execute(opcode)) {
            std::cerr << std::format("Encountered an illegal opcode {:#04x} at address {:#06x}", opcode, PC - 1)
                      << std::endl;
            _terminate.test_and_set();
        }

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

uint16_t CPU::program_counter() const noexcept { return PC; }

size_t CPU::cycle() const noexcept { return _cycle; }

void CPU::wait_for_pulse() noexcept {
    _clock.wait_for_pulse();
    _cycle++;
}

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

bool CPU::decode_and_execute(const uint8_t opcode) {
    const auto instruction = getInstruction(opcode);
    if (!instruction) return false;

    const auto addressing = getAddressing(opcode);
    if (!addressing) panic("A valid opcode must contain both instruction and addressing");

    switch (*instruction) {
    case Instruction::LDA: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address))
            AC = read(PC++);
        else if (std::holds_alternative<uint16_t>(address)) AC = read(std::get<uint16_t>(address));
        else panic("Unsupported addressing mode for LDA");

        SR.zero     = AC == 0;
        SR.negative = AC & 0x80;
    } break;

    case Instruction::STA: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address)) {
            wait_for_pulse();
            _memory.write(std::get<uint16_t>(address), AC);
        } else panic("Unsupported addressing mode for STA");
    } break;

    case Instruction::ADC: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address)) {
            AC = ALU::add(AC, read(PC++), SR);
        } else if (std::holds_alternative<uint16_t>(address)) {
            const auto arg = read(std::get<uint16_t>(address));
            AC = ALU::add(AC, arg, SR);
        } else panic("Unsupported addressing mode for ADC");
    } break;

    case Instruction::SBC: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address)) {
            AC = ALU::subtract(AC, read(PC++), SR);
        } else if (std::holds_alternative<uint16_t>(address)) {
            const auto arg = read(std::get<uint16_t>(address));
            wait_for_pulse();
            AC = ALU::subtract(AC, arg, SR);
        } else panic("Unsupported addressing mode for SBC");
    } break;

    case Instruction::AND: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address)) {
            AC = ALU::logical_and(AC, read(PC++), SR);
        } else if (std::holds_alternative<uint16_t>(address)) {
            const auto arg = read(std::get<uint16_t>(address));
            wait_for_pulse();
            AC = ALU::logical_and(AC, arg, SR);
        } else panic("Unsupported addressing mode for AND");
    } break;

    case Instruction::ORA: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address)) {
            AC = ALU::logical_or(AC, read(PC++), SR);
        } else if (std::holds_alternative<uint16_t>(address)) {
            const auto arg = read(std::get<uint16_t>(address));
            wait_for_pulse();
            AC = ALU::logical_or(AC, arg, SR);
        } else panic("Unsupported addressing mode for ORA");
    } break;

    case Instruction::EOR: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address)) {
            AC = ALU::logical_xor(AC, read(PC++), SR);
        } else if (std::holds_alternative<uint16_t>(address)) {
            const auto arg = read(std::get<uint16_t>(address));
            wait_for_pulse();
            AC = ALU::logical_xor(AC, arg, SR);
        } else panic("Unsupported addressing mode for EOR");
    } break;

    case Instruction::SEC: {
        wait_for_pulse();
        SR.carry = true;
    } break;

    case Instruction::CLC: {
        wait_for_pulse();
        SR.carry = false;
    } break;

    case Instruction::SEI: {
        wait_for_pulse();
        SR.interrupt_disable = true;
    } break;

    case Instruction::CLI: {
        wait_for_pulse();
        SR.interrupt_disable = false;
    } break;

    case Instruction::SED: {
        wait_for_pulse();
        SR.decimal = true;
    } break;

    case Instruction::CLD: {
        wait_for_pulse();
        SR.decimal = false;
    } break;

    case Instruction::CLV: {
        wait_for_pulse();
        SR.overflow = false;
    } break;

    case Instruction::JMP: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address))
            PC = std::get<uint16_t>(address);
        else panic("Unsupported addressing mode for JMP");
    } break;

    case Instruction::BMI: PC = branch(SR.negative); break;

    case Instruction::BPL: PC = branch(!SR.negative); break;

    case Instruction::BCC: PC = branch(!SR.carry); break;

    case Instruction::BCS: PC = branch(SR.carry); break;

    case Instruction::BEQ: PC = branch(SR.zero); break;

    case Instruction::BNE: PC = branch(!SR.zero); break;

    case Instruction::BVS: PC = branch(SR.overflow); break;

    case Instruction::BVC: PC = branch(!SR.overflow); break;

    case Instruction::CMP: {
        uint8_t memory;
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address))
            memory = read(PC++);
        else if (std::holds_alternative<uint16_t>(address)) memory = read(std::get<uint16_t>(address));
        else panic("Unsupported addressing mode for CMP");

        compare(AC, memory, SR);
    } break;

    case Instruction::BIT: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address)) {
            const auto result = static_cast<uint8_t>(AC & read(std::get<uint16_t>(address)));
            SR.negative       = result & 0x80;
            SR.overflow       = result & 0x40;
            SR.zero           = result == 0;
        } else panic("Unsupported addressing mode for BIT");
    } break;

    case Instruction::LDX: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address))
            X = read(PC++);
        else if (std::holds_alternative<uint16_t>(address)) X = read(std::get<uint16_t>(address));
        else panic("Unsupported addressing mode for LDX");

        SR.zero     = X == 0;
        SR.negative = X & 0x80;
    } break;

    case Instruction::LDY: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address))
            Y = read(PC++);
        else if (std::holds_alternative<uint16_t>(address)) Y = read(std::get<uint16_t>(address));
        else panic("Unsupported addressing mode for LDY");

        SR.zero     = Y == 0;
        SR.negative = Y & 0x80;
    } break;

    case Instruction::STX: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address))
            _memory.write(std::get<uint16_t>(address), X);
        else panic("Unsupported addressing mode for STX");
    } break;

    case Instruction::STY: {
        if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address))
            _memory.write(std::get<uint16_t>(address), Y);
        else panic("Unsupported addressing mode for STY");
    } break;

    case Instruction::INX: {
        wait_for_pulse();
        X++;
        SR.zero     = X == 0;
        SR.negative = X & 0x80;
    } break;

    case Instruction::INY: {
        wait_for_pulse();
        Y++;
        SR.zero     = Y == 0;
        SR.negative = Y & 0x80;
    } break;

    case Instruction::DEX: {
        wait_for_pulse();
        X--;
        SR.zero     = X == 0;
        SR.negative = X & 0x80;
    } break;

    case Instruction::DEY: {
        wait_for_pulse();
        Y--;
        SR.zero     = Y == 0;
        SR.negative = Y & 0x80;
    } break;

    case Instruction::CPX: {
        uint8_t memory;
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address))
            memory = read(PC++);
        else if (std::holds_alternative<uint16_t>(address)) memory = read(std::get<uint16_t>(address));
        else panic("Unsupported addressing mode for CPX");

        compare(X, memory, SR);
    } break;

    case Instruction::CPY: {
        uint8_t memory;
        if (const auto address = fetch_address(*addressing); std::holds_alternative<immediate_t>(address))
            memory = read(PC++);
        else if (std::holds_alternative<uint16_t>(address)) memory = read(std::get<uint16_t>(address));
        else panic("Unsupported addressing mode for CPY");

        compare(Y, memory, SR);
    } break;

    case Instruction::TAX: {
        wait_for_pulse();
        X           = AC;
        SR.zero     = X == 0;
        SR.negative = X & 0x80;
    } break;

    case Instruction::TXA: {
        wait_for_pulse();
        AC          = X;
        SR.zero     = AC == 0;
        SR.negative = AC & 0x80;
    } break;

    case Instruction::TAY: {
        wait_for_pulse();
        Y           = AC;
        SR.zero     = Y == 0;
        SR.negative = Y & 0x80;
    } break;

    case Instruction::TYA: {
        wait_for_pulse();
        AC          = Y;
        SR.zero     = AC == 0;
        SR.negative = AC & 0x80;
    } break;

    case Instruction::JSR: {
        const auto adl = read(PC++);
        wait_for_pulse();
        wait_for_pulse();
        push(high_byte(PC));
        wait_for_pulse();
        push(low_byte(PC));
        const auto adh = read(PC);
        PC             = make_word(adh, adl);
    } break;

    case Instruction::RTS: {
        read(PC++);
        wait_for_pulse();
        SP++;
        const auto pcl = read(0x0100 & SP++);
        const auto pch = read(0x0100 & SP);
        wait_for_pulse();
        PC = make_word(pch, pcl);
        PC++;
    } break;

    case Instruction::PHA: {
        read(PC); // the data is discarded
        wait_for_pulse();
        push(AC);
    } break;

    case Instruction::PLA: {
        read(PC); // the data is discarded
        wait_for_pulse();
        SP++;
        AC          = read(0x0100 & SP);
        SR.zero     = AC == 0;
        SR.negative = AC & 0x80;
    } break;

    case Instruction::TXS: {
        wait_for_pulse();
        SP = X;
    } break;

    case Instruction::TSX: {
        wait_for_pulse();
        X           = SP;
        SR.zero     = X == 0;
        SR.negative = X & 0x80;
    } break;

    case Instruction::PHP: {
        read(PC); // the data is discarded
        wait_for_pulse();
        push(static_cast<uint8_t>(SR));
    } break;

    case Instruction::PLP: {
        read(PC); // the data is discarded
        wait_for_pulse();
        SP++;
        SR = read(0x0100 & SP);
    } break;

    case Instruction::BRK:
        if (!SR.interrupt_disable) {
            wait_for_pulse();
            push(static_cast<uint8_t>(StatusRegister{ .break_ = true }));
            interrupt(IRQ);
        }
        break;

    case Instruction::RTI: return_from_interrupt(); break;

    case Instruction::LSR: shift_or_rotate(*addressing, ALU::shift_right, Instruction::LSR); break;

    case Instruction::ASL: shift_or_rotate(*addressing, ALU::shift_left, Instruction::ASL); break;

    case Instruction::ROL: shift_or_rotate(*addressing, ALU::rotate_left, Instruction::ROL); break;

    case Instruction::ROR: shift_or_rotate(*addressing, ALU::rotate_right, Instruction::ROR); break;

    case Instruction::INC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(X);
            const auto memory  = read(address);
            wait_for_pulse();
            wait_for_pulse();
            _memory.write(address, memory + 1);
        } else if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address)) {
            const auto memory = read(std::get<uint16_t>(address));
            wait_for_pulse();
            wait_for_pulse();
            _memory.write(std::get<uint16_t>(address), memory + 1);
        } else panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::DEC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(X);
            const auto memory  = read(address);
            wait_for_pulse();
            wait_for_pulse();
            _memory.write(address, memory - 1);
        } else if (const auto address = fetch_address(*addressing); std::holds_alternative<uint16_t>(address)) {
            const auto memory = read(std::get<uint16_t>(address));
            wait_for_pulse();
            wait_for_pulse();
            _memory.write(std::get<uint16_t>(address), memory - 1);
        } else panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::NOP: wait_for_pulse(); break;

    default: panic(std::format("Unhandled instruction {}", to_string(*instruction)));
    }

    return true;
}

void CPU::reset() noexcept {
    SR.interrupt_disable = true;
    read(PC++);
    read(PC++);
    SP = 0xFF;
    read(0x0100 + SP);
    read(0x0100 + SP - 1);
    read(0x0100 + SP - 2);
    const auto pcl = read(RES);
    const auto pch = read(RES + 1);
    PC             = make_word(pch, pcl);
}

uint8_t CPU::read(const uint16_t address) noexcept {
    wait_for_pulse();
    return _memory[address];
}

uint16_t CPU::branch(const bool condition) noexcept {
    // Assume PC = 0x0101
    const auto offset = std::bit_cast<int8_t>(read(PC++)); // assume -0x50
    if (!condition) return PC;

    read(PC);                                                             // from PC = 0x0102, this data is ignored
    const auto [pcl, overflow] = add_with_overflow(low_byte(PC), offset); // 0xB2
    auto pch                   = high_byte(PC);                           // 0x01

    switch (overflow) {
    case SignedOverflow::None: return make_word(pch, pcl);
    case SignedOverflow::Negative: pch--; break; // 0x00
    case SignedOverflow::Positive: pch++; break;
    }

    wait_for_pulse();
    return make_word(pch, pcl); // 0x00B2

    // Next operation reads an opcode from 0x00B2
}

void CPU::compare(const uint8_t a, const uint8_t b, StatusRegister &sr) noexcept {
    sr.negative = static_cast<uint8_t>(static_cast<int>(a) - static_cast<int>(b)) & 0x80;
    sr.carry    = a >= b;
    sr.zero     = a == b;
}

void CPU::push(const uint8_t byte) noexcept {
    if (!_memory.write(0x0100 & SP--, byte)) panic("Stack is read-only");
}

void CPU::interrupt(const uint16_t handler_address) noexcept {
    read(PC); // this data is discarded
    wait_for_pulse();
    push(high_byte(PC));
    wait_for_pulse();
    push(low_byte(PC));
    wait_for_pulse();
    push(static_cast<uint8_t>(SR));
    const auto pcl = read(handler_address);
    const auto pch = read(handler_address + 1);
    PC             = make_word(pch, pcl);
}

void CPU::return_from_interrupt() noexcept {
    read(PC++);
    wait_for_pulse();
    SP++;
    SR             = read(0x0100 + SP++);
    const auto pcl = read(0x0100 + SP++);
    const auto pch = read(0x0100 + SP++);
    PC             = make_word(pch, pcl);
}

void CPU::shift_or_rotate(const Addressing addressing,
                          uint8_t (*operation)(uint8_t, StatusRegister &),
                          const Instruction instruction) noexcept {
    if (addressing == Addressing::AbsoluteX) {
        const auto address = fetch_absolute_address_long(X);
        const auto memory  = read(address);
        wait_for_pulse();
        const auto result = operation(memory, SR);
        wait_for_pulse();
        _memory.write(address, result);
    } else if (const auto address = fetch_address(addressing); std::holds_alternative<accumulator_t>(address)) {
        wait_for_pulse();
        AC = operation(AC, SR);
    } else if (std::holds_alternative<uint16_t>(address)) {
        const auto memory = read(std::get<uint16_t>(address));
        wait_for_pulse();
        const auto result = operation(memory, SR);
        wait_for_pulse();
        _memory.write(std::get<uint16_t>(address), result);
    } else panic("Unsupported addressing mode for " + to_string(instruction));
}

uint16_t CPU::fetch_absolute_address_long(const uint8_t index) noexcept {
    const auto adl           = read(PC++);
    const auto adh           = read(PC++);
    const auto [adlx, carry] = add_with_overflow(adl, index);

    // This cycle is wasted because read/modify/write instruction should wait
    // until the carry has been added to the address high
    // to avoid writing a false memory location
    read(make_word(adh, adlx)); // this data is discarded

    return make_word(adh + carry, adlx);
}
} // namespace emulator::mos_6502