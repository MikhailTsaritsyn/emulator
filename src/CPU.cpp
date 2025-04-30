//
// Created by Mikhail Tsaritsyn on Apr 02, 2025.
//
#include "CPU.hpp"

#include "ALU.hpp"
#include "helpers.hpp"
#include <chrono>
#include <iostream>
#include <mtl/panic.hpp>

namespace emulator::mos_6502 {
CPU::CPU(Memory memory) noexcept : _memory(std::move(memory)) {}

void CPU::start(Clock &clock) noexcept {
    auto prev_time = std::chrono::high_resolution_clock::now();

    reset(clock);

    static constexpr size_t window = 100;
    while (!_terminate.test()) {
        if (_non_maskable_interrupt_requested.test()) std::tie(PC, SP) = interrupt(PC, SP, SR, NMI, clock);
        if (_interrupt_requested.test() && !SR.interrupt_disable) std::tie(PC, SP) = interrupt(PC, SP, SR, IRQ, clock);

        [[maybe_unused]] const auto opcode = read(PC++, clock);

        if (!decode_and_execute(opcode, clock)) {
            std::cerr << std::format("Encountered an illegal opcode {:#04x} at address {:#06x}", opcode, PC.prev())
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

mtl::u16 CPU::program_counter() const noexcept { return PC; }

size_t CPU::cycle() const noexcept { return _cycle; }

void CPU::wait_for_pulse(Clock &clock) noexcept {
    clock.wait_for_pulse();
    _cycle++;
}

CPU::Address
CPU::fetch_address(const Addressing addressing, mtl::u16 &pc, const mtl::u8 x, const mtl::u8 y, Clock &clock) noexcept {
    switch (addressing) {
    case Addressing::Accumulator: return accumulator_t{};
    case Addressing::Absolute: return fetch_absolute_address(pc, clock);
    case Addressing::AbsoluteX: return fetch_absolute_address(pc, x, clock);
    case Addressing::AbsoluteY: return fetch_absolute_address(pc, y, clock);
    case Addressing::Implicit: return implicit_t{};
    case Addressing::Immediate: return immediate_t{};
    case Addressing::IndexedIndirect: return fetch_indexed_indirect_address(pc, clock);
    case Addressing::IndirectIndexed: return fetch_indirect_indexed_address(pc, clock);
    case Addressing::Relative: return relative_t{};
    case Addressing::ZeroPage: return fetch_zero_page_address(pc, clock);
    case Addressing::ZeroPageX: return fetch_zero_page_address(pc, x, clock);
    case Addressing::ZeroPageY: return fetch_zero_page_address(pc, y, clock);
    case Addressing::Indirect: return fetch_indirect_address(pc, clock);
    }
    std::unreachable();
}

mtl::u16 CPU::fetch_absolute_address(mtl::u16 &pc, Clock &clock) noexcept {
    const auto address_low  = read(pc++, clock);
    const auto address_high = read(pc++, clock);
    return make_word(address_high, address_low);
}

mtl::u16 CPU::fetch_absolute_address(mtl::u16 &pc, const mtl::u8 index, Clock &clock) noexcept {
    const auto bal                     = read(pc++, clock);
    const auto bah                     = read(pc++, clock);
    const auto [bal_updated, overflow] = add_with_overflow(bal, index);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(address, clock);
    return make_word(bah.next(), bal_updated);
}

mtl::u16 CPU::fetch_indirect_address(mtl::u16 &pc, Clock &clock) noexcept {
    const auto ial = read(pc++, clock);
    const auto iah = read(pc++, clock);
    pc             = make_word(iah, ial);
    const auto adl = read(pc++, clock);
    const auto adh = read(pc, clock);
    return make_word(adh, adl);
}

mtl::u16 CPU::fetch_indexed_indirect_address(mtl::u16 &pc, Clock &clock) noexcept {
    const auto bal = read(pc++, clock);
    read(mtl::u16(bal), clock);
    const auto adl = read(make_word(mtl::u8(0), bal + X), clock);
    const auto adh = read(make_word(mtl::u8(0), (bal + X).next()), clock);
    return make_word(adh, adl);
}

mtl::u16 CPU::fetch_indirect_indexed_address(mtl::u16 &pc, Clock &clock) noexcept {
    const auto ial                     = read(pc++, clock);
    const auto bal                     = read(mtl::u16(ial), clock);
    const auto bah                     = read(make_word(mtl::u8(0), ial.next()), clock);
    const auto [bal_updated, overflow] = add_with_overflow(bal, Y);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(address, clock);
    return make_word(bah.next(), bal_updated);
}

mtl::u16 CPU::fetch_zero_page_address(mtl::u16 &pc, Clock &clock) noexcept { return mtl::u16(read(pc++, clock)); }

mtl::u16 CPU::fetch_zero_page_address(mtl::u16 &pc, const mtl::u8 index, Clock &clock) noexcept {
    const auto adl = read(pc++, clock);
    read(mtl::u16(adl), clock); // This data is ignored
    return make_word(mtl::u8(0), adl + index);
}

bool CPU::decode_and_execute(const mtl::u8 opcode, Clock &clock) {
    const auto instruction = getInstruction(opcode);
    if (!instruction) return false;

    const auto addressing = getAddressing(opcode);
    if (!addressing) mtl::panic("A valid opcode must contain both instruction and addressing");

    switch (*instruction) {
    case Instruction::LDA: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDA");
    } break;

    case Instruction::STA: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<mtl::u16>(address)) {
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), AC);
        } else mtl::panic("Unsupported addressing mode for STA");
    } break;

    case Instruction::ADC: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (SR.decimal) std::tie(result, SR.carry, SR.overflow) = ALU::add_decimal(AC, read(PC++, clock), SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::add_binary(AC, read(PC++, clock), SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            mtl::u8 result;
            if (SR.decimal) std::tie(result, SR.carry, SR.overflow) = ALU::add_decimal(AC, memory, SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::add_binary(AC, memory, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for ADC");
    } break;

    case Instruction::SBC: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (SR.decimal)
                std::tie(result, SR.carry, SR.overflow) = ALU::subtract_decimal(AC, read(PC++, clock), SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::subtract_binary(AC, read(PC++, clock), SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            mtl::u8 result;
            if (SR.decimal)
                std::tie(result, SR.carry, SR.overflow) = ALU::subtract_decimal(AC, memory, SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::subtract_binary(AC, memory, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for SBC");
    } break;

    case Instruction::AND: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC & read(PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(std::get<mtl::u16>(address), clock);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC & arg);
        } else mtl::panic("Unsupported addressing mode for AND");
    } break;

    case Instruction::ORA: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC | read(PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(std::get<mtl::u16>(address), clock);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC | arg);
        } else mtl::panic("Unsupported addressing mode for ORA");
    } break;

    case Instruction::EOR: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC ^ read(PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(std::get<mtl::u16>(address), clock);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC ^ arg);
        } else mtl::panic("Unsupported addressing mode for EOR");
    } break;

    case Instruction::SEC: {
        wait_for_pulse(clock);
        SR.carry = true;
    } break;

    case Instruction::CLC: {
        wait_for_pulse(clock);
        SR.carry = false;
    } break;

    case Instruction::SEI: {
        wait_for_pulse(clock);
        SR.interrupt_disable = true;
    } break;

    case Instruction::CLI: {
        wait_for_pulse(clock);
        SR.interrupt_disable = false;
    } break;

    case Instruction::SED: {
        wait_for_pulse(clock);
        SR.decimal = true;
    } break;

    case Instruction::CLD: {
        wait_for_pulse(clock);
        SR.decimal = false;
    } break;

    case Instruction::CLV: {
        wait_for_pulse(clock);
        SR.overflow = false;
    } break;

    case Instruction::JMP: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock); std::holds_alternative<mtl::u16>(address))
            PC = std::get<mtl::u16>(address);
        else mtl::panic("Unsupported addressing mode for JMP");
    } break;

    case Instruction::BMI: PC = branch(PC, SR.negative, clock); break;

    case Instruction::BPL: PC = branch(PC, !SR.negative, clock); break;

    case Instruction::BCC: PC = branch(PC, !SR.carry, clock); break;

    case Instruction::BCS: PC = branch(PC, SR.carry, clock); break;

    case Instruction::BEQ: PC = branch(PC, SR.zero, clock); break;

    case Instruction::BNE: PC = branch(PC, !SR.zero, clock); break;

    case Instruction::BVS: PC = branch(PC, SR.overflow, clock); break;

    case Instruction::BVC: PC = branch(PC, !SR.overflow, clock); break;

    case Instruction::CMP: {
        mtl::u8 memory;
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            memory = read(PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) memory = read(std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CMP");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(AC, memory);
    } break;

    case Instruction::BIT: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<mtl::u16>(address)) {
            const auto result = static_cast<uint8_t>(AC & read(std::get<mtl::u16>(address), clock));
            SR.negative       = result & 0x80;
            SR.overflow       = result & 0x40;
            SR.zero           = result == 0;
        } else mtl::panic("Unsupported addressing mode for BIT");
    } break;

    case Instruction::LDX: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDX");
    } break;

    case Instruction::LDY: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDY");
    } break;

    case Instruction::STX: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock); std::holds_alternative<mtl::u16>(address))
            _memory.write(std::get<mtl::u16>(address), X);
        else mtl::panic("Unsupported addressing mode for STX");
    } break;

    case Instruction::STY: {
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock); std::holds_alternative<mtl::u16>(address))
            _memory.write(std::get<mtl::u16>(address), Y);
        else mtl::panic("Unsupported addressing mode for STY");
    } break;

    case Instruction::INX: {
        wait_for_pulse(clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(X.next());
    } break;

    case Instruction::INY: {
        wait_for_pulse(clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(Y.next());
    } break;

    case Instruction::DEX: {
        wait_for_pulse(clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(X.prev());
    } break;

    case Instruction::DEY: {
        wait_for_pulse(clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(Y.prev());
    } break;

    case Instruction::CPX: {
        mtl::u8 memory;
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            memory = read(PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) memory = read(std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CPX");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(X, memory);
    } break;

    case Instruction::CPY: {
        mtl::u8 memory;
        if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            memory = read(PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) memory = read(std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CPY");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(Y, memory);
    } break;

    case Instruction::TAX: {
        wait_for_pulse(clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(AC);
    } break;

    case Instruction::TXA: {
        wait_for_pulse(clock);
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(X);
    } break;

    case Instruction::TAY: {
        wait_for_pulse(clock);
        std::tie(Y, SR.zero, SR.negative) = value_with_flags(AC);
    } break;

    case Instruction::TYA: {
        wait_for_pulse(clock);
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(Y);
    } break;

    case Instruction::JSR: {
        const auto adl = read(PC++, clock);
        wait_for_pulse(clock);
        wait_for_pulse(clock);
        SP = push(SP, high_byte(PC));
        wait_for_pulse(clock);
        SP             = push(SP, low_byte(PC));
        const auto adh = read(PC, clock);
        PC             = make_word(adh, adl);
    } break;

    case Instruction::RTS: {
        read(PC++, clock);
        wait_for_pulse(clock);
        SP++;
        const auto pcl = read(make_word(mtl::u8(0x01), SP++), clock);
        const auto pch = read(make_word(mtl::u8(0x01), SP), clock);
        wait_for_pulse(clock);
        PC = make_word(pch, pcl);
        PC++;
    } break;

    case Instruction::PHA: {
        read(PC, clock); // the data is discarded
        wait_for_pulse(clock);
        SP = push(SP, AC);
    } break;

    case Instruction::PLA: {
        read(PC, clock); // the data is discarded
        wait_for_pulse(clock);
        SP++;
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(make_word(mtl::u8(0x01), SP), clock));
    } break;

    case Instruction::TXS: {
        wait_for_pulse(clock);
        SP = X;
    } break;

    case Instruction::TSX: {
        wait_for_pulse(clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(SP);
    } break;

    case Instruction::PHP: {
        read(PC, clock); // the data is discarded
        wait_for_pulse(clock);
        SP = push(SP, static_cast<mtl::u8>(SR));
    } break;

    case Instruction::PLP: {
        read(PC, clock); // the data is discarded
        wait_for_pulse(clock);
        SP++;
        SR = read(make_word(mtl::u8(0x01), SP), clock);
    } break;

    case Instruction::BRK:
        if (!SR.interrupt_disable) {
            wait_for_pulse(clock);
            SP               = push(SP, static_cast<mtl::u8>(StatusRegister{ .break_ = true }));
            std::tie(PC, SP) = interrupt(PC, SP, SR, IRQ, clock);
        }
        break;

    case Instruction::RTI: std::tie(PC, SP, SR) = return_from_interrupt(PC, SP, clock); break;

    case Instruction::LSR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X, clock);
            const auto memory  = read(address, clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::shift_right(memory);
            wait_for_pulse(clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(clock);
            std::tie(AC, SR.carry)             = ALU::shift_right(AC);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::shift_right(memory);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for LSR");
    } break;

    case Instruction::ASL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X, clock);
            const auto memory  = read(address, clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = shift_left(memory, 1);
            wait_for_pulse(clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(clock);
            std::tie(AC, SR.carry)             = shift_left(AC, 1);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = shift_left(memory, 1);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ASL");
    } break;

    case Instruction::ROL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X, clock);
            const auto memory  = read(address, clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::rotate_left(memory, SR.carry);
            wait_for_pulse(clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(clock);
            std::tie(AC, SR.carry)             = ALU::rotate_left(AC, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::rotate_left(memory, SR.carry);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROL");
    } break;

    case Instruction::ROR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X, clock);
            const auto memory  = read(address, clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::rotate_right(memory, SR.carry);
            wait_for_pulse(clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(clock);
            std::tie(AC, SR.carry)             = ALU::rotate_right(AC, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            wait_for_pulse(clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::rotate_right(memory, SR.carry);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROR");
    } break;

    case Instruction::INC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X, clock);
            const auto memory  = read(address, clock);
            wait_for_pulse(clock);
            wait_for_pulse(clock);
            _memory.write(address, memory.next());
        } else if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
                   std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            wait_for_pulse(clock);
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), memory.next());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::DEC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X, clock);
            const auto memory  = read(address, clock);
            wait_for_pulse(clock);
            wait_for_pulse(clock);
            _memory.write(address, memory.prev());
        } else if (const auto address = fetch_address(*addressing, PC, X, Y, clock);
                   std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address), clock);
            wait_for_pulse(clock);
            wait_for_pulse(clock);
            _memory.write(std::get<mtl::u16>(address), memory.prev());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::NOP: wait_for_pulse(clock); break;

    default: mtl::panic(std::format("Unhandled instruction {}", to_string(*instruction)));
    }

    return true;
}

void CPU::reset(Clock &clock) noexcept {
    SR.interrupt_disable = true;
    read(PC++, clock);
    read(PC++, clock);
    SP = mtl::u8(0xFF);
    read(make_word(mtl::u8(0x01), SP), clock);
    read(make_word(mtl::u8(0x01), SP - mtl::u8(1)), clock);
    read(make_word(mtl::u8(0x01), SP - mtl::u8(2)), clock);
    const auto pcl = read(RES, clock);
    const auto pch = read(RES.next(), clock);
    PC             = make_word(pch, pcl);
}

mtl::u8 CPU::read(const mtl::u16 address, Clock &clock) noexcept {
    wait_for_pulse(clock);
    return _memory[address];
}

mtl::u16 CPU::branch(mtl::u16 pc, const bool condition, Clock &clock) noexcept {
    // Assume PC = 0x0101
    const auto offset = std::bit_cast<mtl::i8>(read(pc++, clock)); // assume -0x50
    if (!condition) return pc;

    read(pc, clock);                                                             // from PC = 0x0102, this data is ignored
    const auto [pcl, overflow] = add_with_overflow(low_byte(pc), offset); // 0xB2
    auto pch                   = high_byte(pc);                           // 0x01

    switch (overflow) {
    case SignedOverflow::None: return make_word(pch, pcl);
    case SignedOverflow::Negative: pch--; break; // 0x00
    case SignedOverflow::Positive: pch++; break;
    }

    wait_for_pulse(clock);
    return make_word(pch, pcl); // 0x00B2

    // Next operation reads an opcode from 0x00B2
}

std::tuple<bool, bool, bool> CPU::compare(const mtl::u8 a, const mtl::u8 b) noexcept {
    bool negative = (sub_with_overflow(a, b).first & mtl::u8(0x80)) != 0;
    bool carry    = a >= b;
    bool zero     = a == b;
    return { negative, carry, zero };
}

mtl::u8 CPU::push(const mtl::u8 sp, const mtl::u8 byte) noexcept {
    if (!_memory.write(make_word(mtl::u8(0x01), sp), byte)) mtl::panic("Stack is read-only");
    return sp.prev();
}

std::pair<mtl::u16, mtl::u8>
CPU::interrupt(
        const mtl::u16 pc, mtl::u8 sp, const StatusRegister sr, const mtl::u16 handler_address, Clock &clock) noexcept {
    read(pc, clock); // this data is discarded
    wait_for_pulse(clock);
    sp = push(sp, high_byte(pc));
    wait_for_pulse(clock);
    sp = push(sp, low_byte(pc));
    wait_for_pulse(clock);
    sp             = push(sp, static_cast<mtl::u8>(sr));
    const auto pcl = read(handler_address, clock);
    const auto pch = read(handler_address.next(), clock);
    return { make_word(pch, pcl), sp };
}

std::tuple<mtl::u16, mtl::u8, StatusRegister>
CPU::return_from_interrupt(mtl::u16 pc, mtl::u8 sp, Clock &clock) noexcept {
    read(pc++, clock);
    wait_for_pulse(clock);
    ++sp;
    StatusRegister sr;
    sr             = read(make_word(mtl::u8(0x01), sp++), clock);
    const auto pcl = read(make_word(mtl::u8(0x01), sp++), clock);
    const auto pch = read(make_word(mtl::u8(0x01), sp++), clock);
    return { make_word(pch, pcl), sp, sr };
}

mtl::u16 CPU::fetch_absolute_address_long(mtl::u16 &pc, const mtl::u8 index, Clock &clock) noexcept {
    const auto adl           = read(pc++, clock);
    const auto adh           = read(pc++, clock);
    const auto [adlx, carry] = add_with_overflow(adl, index);

    // This cycle is wasted because read/modify/write instruction should wait
    // until the carry has been added to the address high
    // to avoid writing a false memory location
    read(make_word(adh, adlx), clock); // this data is discarded

    return make_word(carry ? adh.next() : adh, adlx);
}

std::tuple<mtl::u8, bool, bool> CPU::value_with_flags(const mtl::u8 src) noexcept {
    return { src, src == 0, (src & mtl::u8(0x80)) != 0 };
}
} // namespace emulator::mos_6502