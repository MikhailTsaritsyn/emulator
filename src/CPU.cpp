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
    reset(clock, _memory);

    while (!_terminate.test()) {
        if (_non_maskable_interrupt_requested.test()) std::tie(PC, SP) = interrupt(_memory, PC, SP, SR, NMI, clock);
        if (_interrupt_requested.test() && !SR.interrupt_disable)
            std::tie(PC, SP) = interrupt(_memory, PC, SP, SR, IRQ, clock);

        [[maybe_unused]] const auto opcode = read(_memory, PC++, clock);

        if (!decode_and_execute(opcode, clock, _memory)) {
            std::cerr << std::format("Encountered an illegal opcode {:#04x} at address {:#06x}", opcode, PC.prev())
                      << std::endl;
            _terminate.test_and_set();
        }
    }
}

void CPU::terminate() noexcept { _terminate.test_and_set(); }

const Memory &CPU::memory() const & noexcept { return _memory; }

Memory &&CPU::memory() && noexcept { return std::move(_memory); }

mtl::u16 CPU::program_counter() const noexcept { return PC; }

CPU::Address CPU::fetch_address(const Addressing addressing,
                                const Memory &memory,
                                mtl::u16 &pc,
                                const mtl::u8 x,
                                const mtl::u8 y,
                                Clock &clock) noexcept {
    switch (addressing) {
    case Addressing::Accumulator: return accumulator_t{};
    case Addressing::Absolute: return fetch_absolute_address(memory, pc, clock);
    case Addressing::AbsoluteX: return fetch_absolute_address(memory, pc, x, clock);
    case Addressing::AbsoluteY: return fetch_absolute_address(memory, pc, y, clock);
    case Addressing::Implicit: return implicit_t{};
    case Addressing::Immediate: return immediate_t{};
    case Addressing::IndexedIndirect: return fetch_indexed_indirect_address(memory, pc, clock);
    case Addressing::IndirectIndexed: return fetch_indirect_indexed_address(memory, pc, clock);
    case Addressing::Relative: return relative_t{};
    case Addressing::ZeroPage: return fetch_zero_page_address(memory, pc, clock);
    case Addressing::ZeroPageX: return fetch_zero_page_address(memory, pc, x, clock);
    case Addressing::ZeroPageY: return fetch_zero_page_address(memory, pc, y, clock);
    case Addressing::Indirect: return fetch_indirect_address(memory, pc, clock);
    }
    std::unreachable();
}

mtl::u16 CPU::fetch_absolute_address(const Memory &memory, mtl::u16 &pc, Clock &clock) noexcept {
    const auto address_low  = read(memory, pc++, clock);
    const auto address_high = read(memory, pc++, clock);
    return make_word(address_high, address_low);
}

mtl::u16 CPU::fetch_absolute_address(const Memory &memory, mtl::u16 &pc, const mtl::u8 index, Clock &clock) noexcept {
    const auto bal                     = read(memory, pc++, clock);
    const auto bah                     = read(memory, pc++, clock);
    const auto [bal_updated, overflow] = add_with_overflow(bal, index);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(memory, address, clock);
    return make_word(bah.next(), bal_updated);
}

mtl::u16 CPU::fetch_indirect_address(const Memory &memory, mtl::u16 &pc, Clock &clock) noexcept {
    const auto ial = read(memory, pc++, clock);
    const auto iah = read(memory, pc++, clock);
    pc             = make_word(iah, ial);
    const auto adl = read(memory, pc++, clock);
    const auto adh = read(memory, pc, clock);
    return make_word(adh, adl);
}

mtl::u16 CPU::fetch_indexed_indirect_address(const Memory &memory, mtl::u16 &pc, Clock &clock) const noexcept {
    const auto bal = read(memory, pc++, clock);
    read(memory, mtl::u16(bal), clock);
    const auto adl = read(memory, make_word(mtl::u8(0), bal + X), clock);
    const auto adh = read(memory, make_word(mtl::u8(0), (bal + X).next()), clock);
    return make_word(adh, adl);
}

mtl::u16 CPU::fetch_indirect_indexed_address(const Memory &memory, mtl::u16 &pc, Clock &clock) const noexcept {
    const auto ial                     = read(memory, pc++, clock);
    const auto bal                     = read(memory, mtl::u16(ial), clock);
    const auto bah                     = read(memory, make_word(mtl::u8(0), ial.next()), clock);
    const auto [bal_updated, overflow] = add_with_overflow(bal, Y);
    const auto address                 = make_word(bah, bal_updated);
    if (!overflow) return address;
    read(memory, address, clock);
    return make_word(bah.next(), bal_updated);
}

mtl::u16 CPU::fetch_zero_page_address(const Memory &memory, mtl::u16 &pc, Clock &clock) noexcept {
    return mtl::u16(read(memory, pc++, clock));
}

mtl::u16 CPU::fetch_zero_page_address(const Memory &memory, mtl::u16 &pc, const mtl::u8 index, Clock &clock) noexcept {
    const auto adl = read(memory, pc++, clock);
    read(memory, mtl::u16(adl), clock); // This data is ignored
    return make_word(mtl::u8(0), adl + index);
}

bool CPU::decode_and_execute(const mtl::u8 opcode, Clock &clock, Memory &memory) {
    const auto instruction = getInstruction(opcode);
    if (!instruction) return false;

    const auto addressing = getAddressing(opcode);
    if (!addressing) mtl::panic("A valid opcode must contain both instruction and addressing");

    switch (*instruction) {
    case Instruction::LDA: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(memory, PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(memory, std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDA");
    } break;

    case Instruction::STA: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<mtl::u16>(address)) {
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), AC);
        } else mtl::panic("Unsupported addressing mode for STA");
    } break;

    case Instruction::ADC: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (SR.decimal)
                std::tie(result, SR.carry, SR.overflow) = ALU::add_decimal(AC, read(memory, PC++, clock), SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::add_binary(AC, read(memory, PC++, clock), SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            mtl::u8 result;
            if (SR.decimal) std::tie(result, SR.carry, SR.overflow) = ALU::add_decimal(AC, arg, SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::add_binary(AC, arg, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for ADC");
    } break;

    case Instruction::SBC: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (SR.decimal)
                std::tie(result, SR.carry, SR.overflow) =
                        ALU::subtract_decimal(AC, read(memory, PC++, clock), SR.carry);
            else
                std::tie(result, SR.carry, SR.overflow) = ALU::subtract_binary(AC, read(memory, PC++, clock), SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            mtl::u8 result;
            if (SR.decimal) std::tie(result, SR.carry, SR.overflow) = ALU::subtract_decimal(AC, arg, SR.carry);
            else std::tie(result, SR.carry, SR.overflow) = ALU::subtract_binary(AC, arg, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for SBC");
    } break;

    case Instruction::AND: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC & read(memory, PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg                     = read(memory, std::get<mtl::u16>(address), clock);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC & arg);
        } else mtl::panic("Unsupported addressing mode for AND");
    } break;

    case Instruction::ORA: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC | read(memory, PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC | arg);
        } else mtl::panic("Unsupported addressing mode for ORA");
    } break;

    case Instruction::EOR: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC ^ read(memory, PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC ^ arg);
        } else mtl::panic("Unsupported addressing mode for EOR");
    } break;

    case Instruction::SEC: {
        clock.wait_for_pulse();
        SR.carry = true;
    } break;

    case Instruction::CLC: {
        clock.wait_for_pulse();
        SR.carry = false;
    } break;

    case Instruction::SEI: {
        clock.wait_for_pulse();
        SR.interrupt_disable = true;
    } break;

    case Instruction::CLI: {
        clock.wait_for_pulse();
        SR.interrupt_disable = false;
    } break;

    case Instruction::SED: {
        clock.wait_for_pulse();
        SR.decimal = true;
    } break;

    case Instruction::CLD: {
        clock.wait_for_pulse();
        SR.decimal = false;
    } break;

    case Instruction::CLV: {
        clock.wait_for_pulse();
        SR.overflow = false;
    } break;

    case Instruction::JMP: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock); std::holds_alternative<mtl::u16>(address))
            PC = std::get<mtl::u16>(address);
        else mtl::panic("Unsupported addressing mode for JMP");
    } break;

    case Instruction::BMI: PC = branch(memory, PC, SR.negative, clock); break;

    case Instruction::BPL: PC = branch(memory, PC, !SR.negative, clock); break;

    case Instruction::BCC: PC = branch(memory, PC, !SR.carry, clock); break;

    case Instruction::BCS: PC = branch(memory, PC, SR.carry, clock); break;

    case Instruction::BEQ: PC = branch(memory, PC, SR.zero, clock); break;

    case Instruction::BNE: PC = branch(memory, PC, !SR.zero, clock); break;

    case Instruction::BVS: PC = branch(memory, PC, SR.overflow, clock); break;

    case Instruction::BVC: PC = branch(memory, PC, !SR.overflow, clock); break;

    case Instruction::CMP: {
        mtl::u8 arg;
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            arg = read(memory, PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) arg = read(memory, std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CMP");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(AC, arg);
    } break;

    case Instruction::BIT: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<mtl::u16>(address)) {
            const auto result = static_cast<uint8_t>(AC & read(memory, std::get<mtl::u16>(address), clock));
            SR.negative       = result & 0x80;
            SR.overflow       = result & 0x40;
            SR.zero           = result == 0;
        } else mtl::panic("Unsupported addressing mode for BIT");
    } break;

    case Instruction::LDX: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(memory, PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(memory, std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDX");
    } break;

    case Instruction::LDY: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(memory, PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(X, SR.zero, SR.negative) = value_with_flags(read(memory, std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDY");
    } break;

    case Instruction::STX: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock); std::holds_alternative<mtl::u16>(address))
            _memory.write(std::get<mtl::u16>(address), X);
        else mtl::panic("Unsupported addressing mode for STX");
    } break;

    case Instruction::STY: {
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock); std::holds_alternative<mtl::u16>(address))
            _memory.write(std::get<mtl::u16>(address), Y);
        else mtl::panic("Unsupported addressing mode for STY");
    } break;

    case Instruction::INX: {
        clock.wait_for_pulse();
        std::tie(X, SR.zero, SR.negative) = value_with_flags(X.next());
    } break;

    case Instruction::INY: {
        clock.wait_for_pulse();
        std::tie(X, SR.zero, SR.negative) = value_with_flags(Y.next());
    } break;

    case Instruction::DEX: {
        clock.wait_for_pulse();
        std::tie(X, SR.zero, SR.negative) = value_with_flags(X.prev());
    } break;

    case Instruction::DEY: {
        clock.wait_for_pulse();
        std::tie(X, SR.zero, SR.negative) = value_with_flags(Y.prev());
    } break;

    case Instruction::CPX: {
        mtl::u8 arg;
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            arg = read(memory, PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) arg = read(memory, std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CPX");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(X, arg);
    } break;

    case Instruction::CPY: {
        mtl::u8 arg;
        if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
            std::holds_alternative<immediate_t>(address))
            arg = read(memory, PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) arg = read(memory, std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CPY");

        std::tie(SR.negative, SR.carry, SR.zero) = compare(Y, arg);
    } break;

    case Instruction::TAX: {
        clock.wait_for_pulse();
        std::tie(X, SR.zero, SR.negative) = value_with_flags(AC);
    } break;

    case Instruction::TXA: {
        clock.wait_for_pulse();
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(X);
    } break;

    case Instruction::TAY: {
        clock.wait_for_pulse();
        std::tie(Y, SR.zero, SR.negative) = value_with_flags(AC);
    } break;

    case Instruction::TYA: {
        clock.wait_for_pulse();
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(Y);
    } break;

    case Instruction::JSR: {
        const auto adl = read(memory, PC++, clock);
        clock.wait_for_pulse();
        clock.wait_for_pulse();
        SP = push(SP, high_byte(PC));
        clock.wait_for_pulse();
        SP             = push(SP, low_byte(PC));
        const auto adh = read(memory, PC, clock);
        PC             = make_word(adh, adl);
    } break;

    case Instruction::RTS: {
        read(memory, PC++, clock);
        clock.wait_for_pulse();
        SP++;
        const auto pcl = read(memory, make_word(mtl::u8(0x01), SP++), clock);
        const auto pch = read(memory, make_word(mtl::u8(0x01), SP), clock);
        clock.wait_for_pulse();
        PC = make_word(pch, pcl);
        PC++;
    } break;

    case Instruction::PHA: {
        read(memory, PC, clock); // the data is discarded
        clock.wait_for_pulse();
        SP = push(SP, AC);
    } break;

    case Instruction::PLA: {
        read(memory, PC, clock); // the data is discarded
        clock.wait_for_pulse();
        SP++;
        std::tie(AC, SR.zero, SR.negative) = value_with_flags(read(memory, make_word(mtl::u8(0x01), SP), clock));
    } break;

    case Instruction::TXS: {
        clock.wait_for_pulse();
        SP = X;
    } break;

    case Instruction::TSX: {
        clock.wait_for_pulse();
        std::tie(X, SR.zero, SR.negative) = value_with_flags(SP);
    } break;

    case Instruction::PHP: {
        read(memory, PC, clock); // the data is discarded
        clock.wait_for_pulse();
        SP = push(SP, static_cast<mtl::u8>(SR));
    } break;

    case Instruction::PLP: {
        read(memory, PC, clock); // the data is discarded
        clock.wait_for_pulse();
        SP++;
        SR = read(memory, make_word(mtl::u8(0x01), SP), clock);
    } break;

    case Instruction::BRK:
        if (!SR.interrupt_disable) {
            clock.wait_for_pulse();
            SP               = push(SP, static_cast<mtl::u8>(StatusRegister{ .break_ = true }));
            std::tie(PC, SP) = interrupt(memory, PC, SP, SR, IRQ, clock);
        }
        break;

    case Instruction::RTI: std::tie(PC, SP, SR) = return_from_interrupt(memory, PC, SP, clock); break;

    case Instruction::LSR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, PC, X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::shift_right(arg);
            clock.wait_for_pulse();
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(AC, SR.carry)             = ALU::shift_right(AC);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::shift_right(arg);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for LSR");
    } break;

    case Instruction::ASL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, PC, X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry) = shift_left(arg, 1);
            clock.wait_for_pulse();
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(AC, SR.carry)             = shift_left(AC, 1);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry)             = shift_left(arg, 1);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ASL");
    } break;

    case Instruction::ROL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, PC, X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::rotate_left(arg, SR.carry);
            clock.wait_for_pulse();
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(AC, SR.carry)             = ALU::rotate_left(AC, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::rotate_left(arg, SR.carry);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROL");
    } break;

    case Instruction::ROR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, PC, X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry) = ALU::rotate_right(arg, SR.carry);
            clock.wait_for_pulse();
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            _memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(AC, SR.carry)             = ALU::rotate_right(AC, SR.carry);
            std::tie(AC, SR.zero, SR.negative) = value_with_flags(AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, SR.carry)             = ALU::rotate_right(arg, SR.carry);
            std::tie(result, SR.zero, SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROR");
    } break;

    case Instruction::INC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, PC, X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            _memory.write(address, arg.next());
        } else if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
                   std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), arg.next());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::DEC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, PC, X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            _memory.write(address, arg.prev());
        } else if (const auto address = fetch_address(*addressing, memory, PC, X, Y, clock);
                   std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            _memory.write(std::get<mtl::u16>(address), arg.prev());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::NOP: clock.wait_for_pulse(); break;

    default: mtl::panic(std::format("Unhandled instruction {}", to_string(*instruction)));
    }

    return true;
}

void CPU::reset(Clock &clock, const Memory &memory) noexcept {
    SR.interrupt_disable = true;
    read(memory, PC++, clock);
    read(memory, PC++, clock);
    SP = mtl::u8(0xFF);
    read(memory, make_word(mtl::u8(0x01), SP), clock);
    read(memory, make_word(mtl::u8(0x01), SP - mtl::u8(1)), clock);
    read(memory, make_word(mtl::u8(0x01), SP - mtl::u8(2)), clock);
    const auto pcl = read(memory, RES, clock);
    const auto pch = read(memory, RES.next(), clock);
    PC             = make_word(pch, pcl);
}

mtl::u8 CPU::read(const Memory &memory, const mtl::u16 address, Clock &clock) noexcept {
    clock.wait_for_pulse();
    return memory[address];
}

mtl::u16 CPU::branch(const Memory &memory, mtl::u16 pc, const bool condition, Clock &clock) noexcept {
    // Assume PC = 0x0101
    const auto offset = std::bit_cast<mtl::i8>(read(memory, pc++, clock)); // assume -0x50
    if (!condition) return pc;

    read(memory, pc, clock);                                              // from PC = 0x0102, this data is ignored
    const auto [pcl, overflow] = add_with_overflow(low_byte(pc), offset); // 0xB2
    auto pch                   = high_byte(pc);                           // 0x01

    switch (overflow) {
    case SignedOverflow::None: return make_word(pch, pcl);
    case SignedOverflow::Negative: pch--; break; // 0x00
    case SignedOverflow::Positive: pch++; break;
    }

    clock.wait_for_pulse();
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

std::pair<mtl::u16, mtl::u8> CPU::interrupt(const Memory &memory,
                                            const mtl::u16 pc,
                                            mtl::u8 sp,
                                            const StatusRegister sr,
                                            const mtl::u16 handler_address,
                                            Clock &clock) noexcept {
    read(memory, pc, clock); // this data is discarded
    clock.wait_for_pulse();
    sp = push(sp, high_byte(pc));
    clock.wait_for_pulse();
    sp = push(sp, low_byte(pc));
    clock.wait_for_pulse();
    sp             = push(sp, static_cast<mtl::u8>(sr));
    const auto pcl = read(memory, handler_address, clock);
    const auto pch = read(memory, handler_address.next(), clock);
    return { make_word(pch, pcl), sp };
}

std::tuple<mtl::u16, mtl::u8, StatusRegister>
CPU::return_from_interrupt(const Memory &memory, mtl::u16 pc, mtl::u8 sp, Clock &clock) noexcept {
    read(memory, pc++, clock);
    clock.wait_for_pulse();
    ++sp;
    StatusRegister sr;
    sr             = read(memory, make_word(mtl::u8(0x01), sp++), clock);
    const auto pcl = read(memory, make_word(mtl::u8(0x01), sp++), clock);
    const auto pch = read(memory, make_word(mtl::u8(0x01), sp++), clock);
    return { make_word(pch, pcl), sp, sr };
}

mtl::u16
CPU::fetch_absolute_address_long(const Memory &memory, mtl::u16 &pc, const mtl::u8 index, Clock &clock) noexcept {
    const auto adl           = read(memory, pc++, clock);
    const auto adh           = read(memory, pc++, clock);
    const auto [adlx, carry] = add_with_overflow(adl, index);

    // This cycle is wasted because read/modify/write instruction should wait
    // until the carry has been added to the address high
    // to avoid writing a false memory location
    read(memory, make_word(adh, adlx), clock); // this data is discarded

    return make_word(carry ? adh.next() : adh, adlx);
}

std::tuple<mtl::u8, bool, bool> CPU::value_with_flags(const mtl::u8 src) noexcept {
    return { src, src == 0, (src & mtl::u8(0x80)) != 0 };
}
} // namespace emulator::mos_6502