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
void CPU::start(Memory &memory, Clock &clock) noexcept {
    reset(clock, memory);

    while (!_terminate.test()) {
        if (_non_maskable_interrupt_requested.test())
            std::tie(_registers.PC, _registers.SP) =
                    interrupt(memory, _registers.PC, _registers.SP, _registers.SR, NMI, clock);
        if (_interrupt_requested.test() && !_registers.SR.interrupt_disable)
            std::tie(_registers.PC, _registers.SP) =
                    interrupt(memory, _registers.PC, _registers.SP, _registers.SR, IRQ, clock);

        [[maybe_unused]] const auto opcode = read(memory, _registers.PC++, clock);

        if (!decode_and_execute(opcode, clock, memory)) {
            std::cerr << std::format(
                    "Encountered an illegal opcode {:#04x} at address {:#06x}", opcode, _registers.PC.prev())
                      << std::endl;
            _terminate.test_and_set();
        }
    }
}

void CPU::terminate() noexcept { _terminate.test_and_set(); }

mtl::u16 CPU::program_counter() const noexcept { return _registers.PC; }

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
    case Addressing::IndexedIndirect: return fetch_indexed_indirect_address(memory, pc, x, clock);
    case Addressing::IndirectIndexed: return fetch_indirect_indexed_address(memory, pc, y, clock);
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

mtl::u16
CPU::fetch_indexed_indirect_address(const Memory &memory, mtl::u16 &pc, const mtl::u8 x, Clock &clock) noexcept {
    const auto bal = read(memory, pc++, clock);
    read(memory, mtl::u16(bal), clock);
    const auto adl = read(memory, make_word(mtl::u8(0), bal + x), clock);
    const auto adh = read(memory, make_word(mtl::u8(0), (bal + x).next()), clock);
    return make_word(adh, adl);
}

mtl::u16
CPU::fetch_indirect_indexed_address(const Memory &memory, mtl::u16 &pc, const mtl::u8 y, Clock &clock) noexcept {
    const auto ial                     = read(memory, pc++, clock);
    const auto bal                     = read(memory, mtl::u16(ial), clock);
    const auto bah                     = read(memory, make_word(mtl::u8(0), ial.next()), clock);
    const auto [bal_updated, overflow] = add_with_overflow(bal, y);
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
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(read(memory, _registers.PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(read(memory, std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDA");
    } break;

    case Instruction::STA: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<mtl::u16>(address)) {
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), _registers.AC);
        } else mtl::panic("Unsupported addressing mode for STA");
    } break;

    case Instruction::ADC: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (_registers.SR.decimal)
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::add_decimal(_registers.AC, read(memory, _registers.PC++, clock), _registers.SR.carry);
            else
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::add_binary(_registers.AC, read(memory, _registers.PC++, clock), _registers.SR.carry);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            mtl::u8 result;
            if (_registers.SR.decimal)
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::add_decimal(_registers.AC, arg, _registers.SR.carry);
            else
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::add_binary(_registers.AC, arg, _registers.SR.carry);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for ADC");
    } break;

    case Instruction::SBC: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            mtl::u8 result;
            if (_registers.SR.decimal)
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::subtract_decimal(_registers.AC, read(memory, _registers.PC++, clock), _registers.SR.carry);
            else
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) = ALU::subtract_binary(_registers.AC, read(memory, _registers.PC++, clock), _registers.SR.carry);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            mtl::u8 result;
            if (_registers.SR.decimal)
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::subtract_decimal(_registers.AC, arg, _registers.SR.carry);
            else
                std::tie(result, _registers.SR.carry, _registers.SR.overflow) =
                        ALU::subtract_binary(_registers.AC, arg, _registers.SR.carry);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
        } else mtl::panic("Unsupported addressing mode for SBC");
    } break;

    case Instruction::AND: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(_registers.AC & read(memory, _registers.PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC & arg);
        } else mtl::panic("Unsupported addressing mode for AND");
    } break;

    case Instruction::ORA: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(_registers.AC | read(memory, _registers.PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC | arg);
        } else mtl::panic("Unsupported addressing mode for ORA");
    } break;

    case Instruction::EOR: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address)) {
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(_registers.AC ^ read(memory, _registers.PC++, clock));
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC ^ arg);
        } else mtl::panic("Unsupported addressing mode for EOR");
    } break;

    case Instruction::SEC: {
        clock.wait_for_pulse();
        _registers.SR.carry = true;
    } break;

    case Instruction::CLC: {
        clock.wait_for_pulse();
        _registers.SR.carry = false;
    } break;

    case Instruction::SEI: {
        clock.wait_for_pulse();
        _registers.SR.interrupt_disable = true;
    } break;

    case Instruction::CLI: {
        clock.wait_for_pulse();
        _registers.SR.interrupt_disable = false;
    } break;

    case Instruction::SED: {
        clock.wait_for_pulse();
        _registers.SR.decimal = true;
    } break;

    case Instruction::CLD: {
        clock.wait_for_pulse();
        _registers.SR.decimal = false;
    } break;

    case Instruction::CLV: {
        clock.wait_for_pulse();
        _registers.SR.overflow = false;
    } break;

    case Instruction::JMP: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<mtl::u16>(address))
            _registers.PC = std::get<mtl::u16>(address);
        else mtl::panic("Unsupported addressing mode for JMP");
    } break;

    case Instruction::BMI: _registers.PC = branch(memory, _registers.PC, _registers.SR.negative, clock); break;

    case Instruction::BPL: _registers.PC = branch(memory, _registers.PC, !_registers.SR.negative, clock); break;

    case Instruction::BCC: _registers.PC = branch(memory, _registers.PC, !_registers.SR.carry, clock); break;

    case Instruction::BCS: _registers.PC = branch(memory, _registers.PC, _registers.SR.carry, clock); break;

    case Instruction::BEQ: _registers.PC = branch(memory, _registers.PC, _registers.SR.zero, clock); break;

    case Instruction::BNE: _registers.PC = branch(memory, _registers.PC, !_registers.SR.zero, clock); break;

    case Instruction::BVS: _registers.PC = branch(memory, _registers.PC, _registers.SR.overflow, clock); break;

    case Instruction::BVC: _registers.PC = branch(memory, _registers.PC, !_registers.SR.overflow, clock); break;

    case Instruction::CMP: {
        mtl::u8 arg;
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address))
            arg = read(memory, _registers.PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) arg = read(memory, std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CMP");

        std::tie(_registers.SR.negative, _registers.SR.carry, _registers.SR.zero) = compare(_registers.AC, arg);
    } break;

    case Instruction::BIT: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<mtl::u16>(address)) {
            const auto result = static_cast<uint8_t>(_registers.AC & read(memory, std::get<mtl::u16>(address), clock));
            _registers.SR.negative       = result & 0x80;
            _registers.SR.overflow       = result & 0x40;
            _registers.SR.zero           = result == 0;
        } else mtl::panic("Unsupported addressing mode for BIT");
    } break;

    case Instruction::LDX: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(read(memory, _registers.PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(read(memory, std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDX");
    } break;

    case Instruction::LDY: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address))
            std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(read(memory, _registers.PC++, clock));
        else if (std::holds_alternative<mtl::u16>(address))
            std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) =
                    value_with_flags(read(memory, std::get<mtl::u16>(address), clock));
        else mtl::panic("Unsupported addressing mode for LDY");
    } break;

    case Instruction::STX: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<mtl::u16>(address))
            memory.write(std::get<mtl::u16>(address), _registers.X);
        else mtl::panic("Unsupported addressing mode for STX");
    } break;

    case Instruction::STY: {
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock); std::holds_alternative<mtl::u16>(address))
            memory.write(std::get<mtl::u16>(address), _registers.Y);
        else mtl::panic("Unsupported addressing mode for STY");
    } break;

    case Instruction::INX: {
        clock.wait_for_pulse();
        std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.X.next());
    } break;

    case Instruction::INY: {
        clock.wait_for_pulse();
        std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.Y.next());
    } break;

    case Instruction::DEX: {
        clock.wait_for_pulse();
        std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.X.prev());
    } break;

    case Instruction::DEY: {
        clock.wait_for_pulse();
        std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.Y.prev());
    } break;

    case Instruction::CPX: {
        mtl::u8 arg;
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address))
            arg = read(memory, _registers.PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) arg = read(memory, std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CPX");

        std::tie(_registers.SR.negative, _registers.SR.carry, _registers.SR.zero) = compare(_registers.X, arg);
    } break;

    case Instruction::CPY: {
        mtl::u8 arg;
        if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
            std::holds_alternative<immediate_t>(address))
            arg = read(memory, _registers.PC++, clock);
        else if (std::holds_alternative<mtl::u16>(address)) arg = read(memory, std::get<mtl::u16>(address), clock);
        else mtl::panic("Unsupported addressing mode for CPY");

        std::tie(_registers.SR.negative, _registers.SR.carry, _registers.SR.zero) = compare(_registers.Y, arg);
    } break;

    case Instruction::TAX: {
        clock.wait_for_pulse();
        std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC);
    } break;

    case Instruction::TXA: {
        clock.wait_for_pulse();
        std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.X);
    } break;

    case Instruction::TAY: {
        clock.wait_for_pulse();
        std::tie(_registers.Y, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC);
    } break;

    case Instruction::TYA: {
        clock.wait_for_pulse();
        std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.Y);
    } break;

    case Instruction::JSR: {
        const auto adl = read(memory, _registers.PC++, clock);
        clock.wait_for_pulse();
        clock.wait_for_pulse();
        _registers.SP = push(memory, _registers.SP, high_byte(_registers.PC));
        clock.wait_for_pulse();
        _registers.SP             = push(memory, _registers.SP, low_byte(_registers.PC));
        const auto adh = read(memory, _registers.PC, clock);
        _registers.PC             = make_word(adh, adl);
    } break;

    case Instruction::RTS: {
        read(memory, _registers.PC++, clock);
        clock.wait_for_pulse();
        _registers.SP++;
        const auto pcl = read(memory, make_word(mtl::u8(0x01), _registers.SP++), clock);
        const auto pch = read(memory, make_word(mtl::u8(0x01), _registers.SP), clock);
        clock.wait_for_pulse();
        _registers.PC = make_word(pch, pcl);
        _registers.PC++;
    } break;

    case Instruction::PHA: {
        read(memory, _registers.PC, clock); // the data is discarded
        clock.wait_for_pulse();
        _registers.SP = push(memory, _registers.SP, _registers.AC);
    } break;

    case Instruction::PLA: {
        read(memory, _registers.PC, clock); // the data is discarded
        clock.wait_for_pulse();
        _registers.SP++;
        std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) =
                value_with_flags(read(memory, make_word(mtl::u8(0x01), _registers.SP), clock));
    } break;

    case Instruction::TXS: {
        clock.wait_for_pulse();
        _registers.SP = _registers.X;
    } break;

    case Instruction::TSX: {
        clock.wait_for_pulse();
        std::tie(_registers.X, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.SP);
    } break;

    case Instruction::PHP: {
        read(memory, _registers.PC, clock); // the data is discarded
        clock.wait_for_pulse();
        _registers.SP = push(memory, _registers.SP, static_cast<mtl::u8>(_registers.SR));
    } break;

    case Instruction::PLP: {
        read(memory, _registers.PC, clock); // the data is discarded
        clock.wait_for_pulse();
        _registers.SP++;
        _registers.SR = read(memory, make_word(mtl::u8(0x01), _registers.SP), clock);
    } break;

    case Instruction::BRK:
        if (!_registers.SR.interrupt_disable) {
            clock.wait_for_pulse();
            _registers.SP               = push(memory, _registers.SP, static_cast<mtl::u8>(StatusRegister{ .break_ = true }));
            std::tie(_registers.PC, _registers.SP) =
                    interrupt(memory, _registers.PC, _registers.SP, _registers.SR, IRQ, clock);
        }
        break;

    case Instruction::RTI:
        std::tie(_registers.PC, _registers.SP, _registers.SR) =
                return_from_interrupt(memory, _registers.PC, _registers.SP, clock);
        break;

    case Instruction::LSR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, _registers.PC, _registers.X, clock);
            const auto arg     = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry) = ALU::shift_right(arg);
            clock.wait_for_pulse();
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(_registers.AC, _registers.SR.carry)                        = ALU::shift_right(_registers.AC);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry)                        = ALU::shift_right(arg);
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for LSR");
    } break;

    case Instruction::ASL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, _registers.PC, _registers.X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry) = shift_left(arg, 1);
            clock.wait_for_pulse();
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(_registers.AC, _registers.SR.carry)                        = shift_left(_registers.AC, 1);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry)             = shift_left(arg, 1);
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ASL");
    } break;

    case Instruction::ROL: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, _registers.PC, _registers.X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry) = ALU::rotate_left(arg, _registers.SR.carry);
            clock.wait_for_pulse();
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(_registers.AC, _registers.SR.carry) = ALU::rotate_left(_registers.AC, _registers.SR.carry);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry)                        = ALU::rotate_left(arg, _registers.SR.carry);
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROL");
    } break;

    case Instruction::ROR: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, _registers.PC, _registers.X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry) = ALU::rotate_right(arg, _registers.SR.carry);
            clock.wait_for_pulse();
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            memory.write(address, result);
        } else if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
                   std::holds_alternative<accumulator_t>(address)) {
            clock.wait_for_pulse();
            std::tie(_registers.AC, _registers.SR.carry) = ALU::rotate_right(_registers.AC, _registers.SR.carry);
            std::tie(_registers.AC, _registers.SR.zero, _registers.SR.negative) = value_with_flags(_registers.AC);
        } else if (std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            mtl::u8 result;
            std::tie(result, _registers.SR.carry)                        = ALU::rotate_right(arg, _registers.SR.carry);
            std::tie(result, _registers.SR.zero, _registers.SR.negative) = value_with_flags(result);
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), result);
        } else mtl::panic("Unsupported addressing mode for ROR");
    } break;

    case Instruction::INC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, _registers.PC, _registers.X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            memory.write(address, arg.next());
        } else if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
                   std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), arg.next());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::DEC: {
        if (*addressing == Addressing::AbsoluteX) {
            const auto address = fetch_absolute_address_long(memory, _registers.PC, _registers.X, clock);
            const auto arg  = read(memory, address, clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            memory.write(address, arg.prev());
        } else if (const auto address = fetch_address(*addressing, memory, _registers.PC, _registers.X, _registers.Y, clock);
                   std::holds_alternative<mtl::u16>(address)) {
            const auto arg = read(memory, std::get<mtl::u16>(address), clock);
            clock.wait_for_pulse();
            clock.wait_for_pulse();
            memory.write(std::get<mtl::u16>(address), arg.prev());
        } else mtl::panic("Unsupported addressing mode for INC");
    } break;

    case Instruction::NOP: clock.wait_for_pulse(); break;

    default: mtl::panic(std::format("Unhandled instruction {}", to_string(*instruction)));
    }

    return true;
}

void CPU::reset(Clock &clock, const Memory &memory) noexcept {
    _registers.SR.interrupt_disable = true;
    read(memory, _registers.PC++, clock);
    read(memory, _registers.PC++, clock);
    _registers.SP = mtl::u8(0xFF);
    read(memory, make_word(mtl::u8(0x01), _registers.SP), clock);
    read(memory, make_word(mtl::u8(0x01), _registers.SP - mtl::u8(1)), clock);
    read(memory, make_word(mtl::u8(0x01), _registers.SP - mtl::u8(2)), clock);
    const auto pcl = read(memory, RES, clock);
    const auto pch = read(memory, RES.next(), clock);
    _registers.PC  = make_word(pch, pcl);
}

mtl::u8 CPU::read(const Memory &memory, const mtl::u16 address, Clock &clock) noexcept {
    clock.wait_for_pulse();
    return memory[address];
}

mtl::u16 CPU::branch(const Memory &memory, mtl::u16 pc, const bool condition, Clock &clock) noexcept {
    // Assume _registers.PC = 0x0101
    const auto offset = std::bit_cast<mtl::i8>(read(memory, pc++, clock)); // assume -0x50
    if (!condition) return pc;

    read(memory, pc, clock);                                              // from _registers.PC = 0x0102, this data is ignored
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

mtl::u8 CPU::push(Memory &memory, const mtl::u8 sp, const mtl::u8 byte) noexcept {
    if (!memory.write(make_word(mtl::u8(0x01), sp), byte)) mtl::panic("Stack is read-only");
    return sp.prev();
}

std::pair<mtl::u16, mtl::u8> CPU::interrupt(Memory &memory,
                                            const mtl::u16 pc,
                                            mtl::u8 sp,
                                            const StatusRegister sr,
                                            const mtl::u16 handler_address,
                                            Clock &clock) noexcept {
    read(memory, pc, clock); // this data is discarded
    clock.wait_for_pulse();
    sp = push(memory, sp, high_byte(pc));
    clock.wait_for_pulse();
    sp = push(memory, sp, low_byte(pc));
    clock.wait_for_pulse();
    sp             = push(memory, sp, static_cast<mtl::u8>(sr));
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