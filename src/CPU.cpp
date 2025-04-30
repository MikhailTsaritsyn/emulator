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
CPU::CPU(const std::chrono::nanoseconds clock_period, Memory memory) noexcept
        : _clock(clock_period),
          _memory(std::move(memory)) {}

void CPU::start() noexcept {
    auto prev_time = std::chrono::high_resolution_clock::now();

    reset();

    static constexpr size_t window = 100;
    while (!_terminate.test()) {
        if (_non_maskable_interrupt_requested.test()) std::tie(PC, SP) = interrupt(PC, SP, SR, NMI);
        if (_interrupt_requested.test() && !SR.interrupt_disable) std::tie(PC, SP) = interrupt(PC, SP, SR, IRQ);

        [[maybe_unused]] const auto opcode = read(PC++);

        if (!decode_and_execute(opcode)) {
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

CPU::Address CPU::fetch_address(const Addressing addressing, mtl::u16 &pc, const mtl::u8 x, const mtl::u8 y) noexcept {
    switch (addressing) {
    case Addressing::Accumulator: return accumulator_t{};
    case Addressing::Absolute: return fetch_absolute_address(pc);
    case Addressing::AbsoluteX: return fetch_absolute_address(pc, x);
    case Addressing::AbsoluteY: return fetch_absolute_address(pc, y);
    case Addressing::Implicit: return implicit_t{};
    case Addressing::Immediate: return immediate_t{};
    case Addressing::IndexedIndirect: return fetch_indexed_indirect_address(pc);
    case Addressing::IndirectIndexed: return fetch_indirect_indexed_address(pc);
    case Addressing::Relative: return relative_t{};
    case Addressing::ZeroPage: return fetch_zero_page_address(pc);
    case Addressing::ZeroPageX: return fetch_zero_page_address(pc, x);
    case Addressing::ZeroPageY: return fetch_zero_page_address(pc, y);
    case Addressing::Indirect: return fetch_indirect_address(pc);
    }
    std::unreachable();
}

mtl::u16 CPU::fetch_absolute_address(mtl::u16 &pc) noexcept {
    const auto address_low  = read(pc++);
    const auto address_high = read(pc++);
    return make_word(address_high, address_low);
}

mtl::u16 CPU::fetch_absolute_address(mtl::u16 &pc, const mtl::u8 index) noexcept {
    const auto bal                     = read(pc++);
    const auto bah                     = read(pc++);
    const auto [bal_updated, overflow] = add_with_overflow(bal, index);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(address);
    return make_word(bah.next(), bal_updated);
}

mtl::u16 CPU::fetch_indirect_address(mtl::u16 &pc) noexcept {
    const auto ial = read(pc++);
    const auto iah = read(pc++);
    pc             = make_word(iah, ial);
    const auto adl = read(pc++);
    const auto adh = read(pc);
    return make_word(adh, adl);
}

mtl::u16 CPU::fetch_indexed_indirect_address(mtl::u16 &pc) noexcept {
    const auto bal = read(pc++);
    read(mtl::u16(bal));
    const auto adl = read(make_word(mtl::u8(0), bal + X));
    const auto adh = read(make_word(mtl::u8(0), (bal + X).next()));
    return make_word(adh, adl);
}

mtl::u16 CPU::fetch_indirect_indexed_address(mtl::u16 &pc) noexcept {
    const auto ial                     = read(pc++);
    const auto bal                     = read(mtl::u16(ial));
    const auto bah                     = read(make_word(mtl::u8(0), ial.next()));
    const auto [bal_updated, overflow] = add_with_overflow(bal, Y);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(address);
    return make_word(bah.next(), bal_updated);
}

mtl::u16 CPU::fetch_zero_page_address(mtl::u16 &pc) noexcept { return mtl::u16(read(pc++)); }

mtl::u16 CPU::fetch_zero_page_address(mtl::u16 &pc, const mtl::u8 index) noexcept {
    const auto adl = read(pc++);
    read(mtl::u16(adl)); // This data is ignored
    return make_word(mtl::u8(0), adl + index);
}

bool CPU::decode_and_execute(const mtl::u8 opcode) {
    const auto instruction = getInstruction(opcode);
    if (!instruction) return false;

    const auto addressing = getAddressing(opcode);
    if (!addressing) mtl::panic("A valid opcode must contain both instruction and addressing");

    switch (*instruction) {
    case Instruction::LDA: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address))
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(PC++));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(std::get<mtl::u16>(address)));
        else mtl::panic("Unsupported addressing mode for LDA");
    } break;

    case Instruction::STA: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address)) {
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), AC);
        } else mtl::panic("Unsupported addressing mode for STA");
    } break;

    case Instruction::ADC: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (SR.decimal) std::tie(result, SR.carry, SR.overflow) = ALU::add_decimal(AC, read(PC++), SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::add_binary(AC, read(PC++), SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            mtl::u8 result;
            if (SR.decimal) std::tie(result, SR.carry, SR.overflow) = ALU::add_decimal(AC, memory, SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::add_binary(AC, memory, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for ADC");
    } break;

    case Instruction::SBC: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (SR.decimal)
                std::tie(result, SR.carry, SR.overflow) = ALU::subtract_decimal(AC, read(PC++), SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::subtract_binary(AC, read(PC++), SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            mtl::u8 result;
            if (SR.decimal)
                std::tie(result, SR.carry, SR.overflow) = ALU::subtract_decimal(AC, memory, SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::subtract_binary(AC, memory, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for SBC");
    } break;

    case Instruction::AND: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC & read(PC++));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(std::get<mtl::u16>(address));
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC & arg);
        } else mtl::panic("Unsupported addressing mode for AND");
    } break;

    case Instruction::ORA: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC | read(PC++));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(std::get<mtl::u16>(address));
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC | arg);
        } else mtl::panic("Unsupported addressing mode for ORA");
    } break;

    case Instruction::EOR: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC ^ read(PC++));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(std::get<mtl::u16>(address));
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC ^ arg);
        } else mtl::panic("Unsupported addressing mode for EOR");
    } break;

    case Instruction::SEC: {
        wait_for_pulse(_clock);
        SR.carry = true;
    } break;

    case Instruction::CLC: {
        wait_for_pulse(_clock);
        SR.carry = false;
    } break;

    case Instruction::SEI: {
        wait_for_pulse(_clock);
        SR.interrupt_disable = true;
    } break;

    case Instruction::CLI: {
        wait_for_pulse(_clock);
        SR.interrupt_disable = false;
    } break;

    case Instruction::SED: {
        wait_for_pulse(_clock);
        SR.decimal = true;
    } break;

    case Instruction::CLD: {
        wait_for_pulse(_clock);
        SR.decimal = false;
    } break;

    case Instruction::CLV: {
        wait_for_pulse(_clock);
        SR.overflow = false;
    } break;

    case Instruction::JMP: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address))
            PC = std::get<mtl::u16>(address);
        else mtl::panic("Unsupported addressing mode for JMP");
    } break;

    case Instruction::BMI: PC = branch(PC, SR.negative); break;

    case Instruction::BPL: PC = branch(PC, !SR.negative); break;

    case Instruction::BCC: PC = branch(PC, !SR.carry); break;

    case Instruction::BCS: PC = branch(PC, SR.carry); break;

    case Instruction::BEQ: PC = branch(PC, SR.zero); break;

    case Instruction::BNE: PC = branch(PC, !SR.zero); break;

    case Instruction::BVS: PC = branch(PC, SR.overflow); break;

    case Instruction::BVC: PC = branch(PC, !SR.overflow); break;

    case Instruction::CMP: {
        mtl::u8 memory;
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address))
            memory = read(PC++);
        else if (std::holds_alternative<mtl::u16>(address)) memory = read(std::get<mtl::u16>(address));
        else mtl::panic("Unsupported addressing mode for CMP");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(AC, memory);
    } break;

    case Instruction::BIT: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address)) {
            const auto result = static_cast<uint8_t>(AC & read(std::get<mtl::u16>(address)));
            SR.negative       = result & 0x80;
            SR.overflow       = result & 0x40;
            SR.zero           = result == 0;
        } else mtl::panic("Unsupported addressing mode for BIT");
    } break;

    case Instruction::LDX: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(PC++));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(std::get<mtl::u16>(address)));
        else mtl::panic("Unsupported addressing mode for LDX");
    } break;

    case Instruction::LDY: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(PC++));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(std::get<mtl::u16>(address)));
        else mtl::panic("Unsupported addressing mode for LDY");
    } break;

    case Instruction::STX: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address))
            _memory.write(std::get<mtl::u16>(address), X);
        else mtl::panic("Unsupported addressing mode for STX");
    } break;

    case Instruction::STY: {
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address))
            _memory.write(std::get<mtl::u16>(address), Y);
        else mtl::panic("Unsupported addressing mode for STY");
    } break;

    case Instruction::INX: {
        wait_for_pulse(_clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(X.next());
    } break;

    case Instruction::INY: {
        wait_for_pulse(_clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(Y.next());
    } break;

    case Instruction::DEX: {
        wait_for_pulse(_clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(X.prev());
    } break;

    case Instruction::DEY: {
        wait_for_pulse(_clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(Y.prev());
    } break;

    case Instruction::CPX: {
        mtl::u8 memory;
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address))
            memory = read(PC++);
        else if (std::holds_alternative<mtl::u16>(address)) memory = read(std::get<mtl::u16>(address));
        else mtl::panic("Unsupported addressing mode for CPX");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(X, memory);
    } break;

    case Instruction::CPY: {
        mtl::u8 memory;
        if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<immediate_t>(address))
            memory = read(PC++);
        else if (std::holds_alternative<mtl::u16>(address)) memory = read(std::get<mtl::u16>(address));
        else mtl::panic("Unsupported addressing mode for CPY");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(Y, memory);
    } break;

    case Instruction::TAX: {
        wait_for_pulse(_clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(AC);
    } break;

    case Instruction::TXA: {
        wait_for_pulse(_clock);
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(X);
    } break;

    case Instruction::TAY: {
        wait_for_pulse(_clock);
        std::tie(Y, SR.zero, SR.negative) = value_with_flags(AC);
    } break;

    case Instruction::TYA: {
        wait_for_pulse(_clock);
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(Y);
    } break;

    case Instruction::JSR: {
        const auto adl = read(PC++);
        wait_for_pulse(_clock);
        wait_for_pulse(_clock);
        SP = push(SP, high_byte(PC));
        wait_for_pulse(_clock);
        SP             = push(SP, low_byte(PC));
        const auto adh = read(PC);
        PC             = make_word(adh, adl);
    } break;

    case Instruction::RTS: {
        read(PC++);
        wait_for_pulse(_clock);
        SP++;
        const auto pcl = read(make_word(mtl::u8(0x01), SP++));
        const auto pch = read(make_word(mtl::u8(0x01), SP));
        wait_for_pulse(_clock);
        PC = make_word(pch, pcl);
        PC++;
    } break;

    case Instruction::PHA: {
        read(PC); // the data is discarded
        wait_for_pulse(_clock);
        SP = push(SP, AC);
    } break;

    case Instruction::PLA: {
        read(PC); // the data is discarded
        wait_for_pulse(_clock);
        SP++;
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(make_word(mtl::u8(0x01), SP)));
    } break;

    case Instruction::TXS: {
        wait_for_pulse(_clock);
        SP = X;
    } break;

    case Instruction::TSX: {
        wait_for_pulse(_clock);
        std::tie(X, SR.zero, SR.negative) = value_with_flags(SP);
    } break;

    case Instruction::PHP: {
        read(PC); // the data is discarded
        wait_for_pulse(_clock);
        SP = push(SP, static_cast<mtl::u8>(SR));
    } break;

    case Instruction::PLP: {
        read(PC); // the data is discarded
        wait_for_pulse(_clock);
        SP++;
        SR = read(make_word(mtl::u8(0x01), SP));
    } break;

    case Instruction::BRK:
        if (!SR.interrupt_disable) {
            wait_for_pulse(_clock);
            SP               = push(SP, static_cast<mtl::u8>(StatusRegister{ .break_ = true }));
            std::tie(PC, SP) = interrupt(PC, SP, SR, IRQ);
        }
        break;

    case Instruction::RTI: std::tie(PC, SP, SR) = return_from_interrupt(PC, SP); break;

    case Instruction::LSR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X);
            const auto memory  = read(address);
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::shift_right(memory);
            wait_for_pulse(_clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(_clock);
            std::tie(AC, SR.carry)             = ALU::shift_right(AC);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::shift_right(memory);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for LSR");
    } break;

    case Instruction::ASL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X);
            const auto memory  = read(address);
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = shift_left(memory, 1);
            wait_for_pulse(_clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(_clock);
            std::tie(AC, SR.carry)             = shift_left(AC, 1);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = shift_left(memory, 1);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ASL");
    } break;

    case Instruction::ROL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X);
            const auto memory  = read(address);
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::rotate_left(memory, SR.carry);
            wait_for_pulse(_clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(_clock);
            std::tie(AC, SR.carry)             = ALU::rotate_left(AC, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::rotate_left(memory, SR.carry);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROL");
    } break;

    case Instruction::ROR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X);
            const auto memory  = read(address);
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::rotate_right(memory, SR.carry);
            wait_for_pulse(_clock);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, PC, X, Y);
                   std::holds_alternative<accumulator_t>(address)) {
            wait_for_pulse(_clock);
            std::tie(AC, SR.carry)             = ALU::rotate_right(AC, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            wait_for_pulse(_clock);
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::rotate_right(memory, SR.carry);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROR");
    } break;

    case Instruction::INC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X);
            const auto memory  = read(address);
            wait_for_pulse(_clock);
            wait_for_pulse(_clock);
            _memory.write(address, memory.next());
        } else if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            wait_for_pulse(_clock);
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), memory.next());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::DEC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(PC, X);
            const auto memory  = read(address);
            wait_for_pulse(_clock);
            wait_for_pulse(_clock);
            _memory.write(address, memory.prev());
        } else if (const auto address = fetch_address(*addressing, PC, X, Y); std::holds_alternative<mtl::u16>(address)) {
            const auto memory = read(std::get<mtl::u16>(address));
            wait_for_pulse(_clock);
            wait_for_pulse(_clock);
            _memory.write(std::get<mtl::u16>(address), memory.prev());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::NOP: wait_for_pulse(_clock); break;

    default: mtl::panic(std::format("Unhandled instruction {}", to_string(*instruction)));
    }

    return true;
}

void CPU::reset() noexcept {
    SR.interrupt_disable = true;
    read(PC++);
    read(PC++);
    SP = mtl::u8(0xFF);
    read(make_word(mtl::u8(0x01), SP));
    read(make_word(mtl::u8(0x01), SP - mtl::u8(1)));
    read(make_word(mtl::u8(0x01), SP - mtl::u8(2)));
    const auto pcl = read(RES);
    const auto pch = read(RES.next());
    PC             = make_word(pch, pcl);
}

mtl::u8 CPU::read(const mtl::u16 address) noexcept {
    wait_for_pulse(_clock);
    return _memory[address];
}

mtl::u16 CPU::branch(mtl::u16 pc, const bool condition) noexcept {
    // Assume PC = 0x0101
    const auto offset = std::bit_cast<mtl::i8>(read(pc++)); // assume -0x50
    if (!condition) return pc;

    read(pc);                                                             // from PC = 0x0102, this data is ignored
    const auto [pcl, overflow] = add_with_overflow(low_byte(pc), offset); // 0xB2
    auto pch                   = high_byte(pc);                           // 0x01

    switch (overflow) {
    case SignedOverflow::None: return make_word(pch, pcl);
    case SignedOverflow::Negative: pch--; break; // 0x00
    case SignedOverflow::Positive: pch++; break;
    }

    wait_for_pulse(_clock);
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
CPU::interrupt(const mtl::u16 pc, mtl::u8 sp, const StatusRegister sr, const mtl::u16 handler_address) noexcept {
    read(pc); // this data is discarded
    wait_for_pulse(_clock);
    sp = push(sp, high_byte(pc));
    wait_for_pulse(_clock);
    sp = push(sp, low_byte(pc));
    wait_for_pulse(_clock);
    sp             = push(sp, static_cast<mtl::u8>(sr));
    const auto pcl = read(handler_address);
    const auto pch = read(handler_address.next());
    return { make_word(pch, pcl), sp };
}

std::tuple<mtl::u16, mtl::u8, StatusRegister> CPU::return_from_interrupt(mtl::u16 pc, mtl::u8 sp) noexcept {
    read(pc++);
    wait_for_pulse(_clock);
    ++sp;
    StatusRegister sr;
    sr             = read(make_word(mtl::u8(0x01), sp++));
    const auto pcl = read(make_word(mtl::u8(0x01), sp++));
    const auto pch = read(make_word(mtl::u8(0x01), sp++));
    return { make_word(pch, pcl), sp, sr };
}

mtl::u16 CPU::fetch_absolute_address_long(mtl::u16 &pc, const mtl::u8 index) noexcept {
    const auto adl           = read(pc++);
    const auto adh           = read(pc++);
    const auto [adlx, carry] = add_with_overflow(adl, index);

    // This cycle is wasted because read/modify/write instruction should wait
    // until the carry has been added to the address high
    // to avoid writing a false memory location
    read(make_word(adh, adlx)); // this data is discarded

    return make_word(carry ? adh.next() : adh, adlx);
}

std::tuple<mtl::u8, bool, bool> CPU::value_with_flags(const mtl::u8 src) noexcept {
    return { src, src == 0, (src & mtl::u8(0x80)) != 0 };
}
} // namespace emulator::mos_6502