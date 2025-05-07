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
void CPU::start(Memory &memory, Clock &clock, Registers &registers) noexcept {
    reset(clock, memory, registers.PC, registers.SP, registers.SR.interrupt_disable);

    while (!_terminate.test()) {
        if (_non_maskable_interrupt_requested.test())
            std::tie(registers.PC, registers.SP) =
                    interrupt(memory, registers.PC, registers.SP, registers.SR, NMI, clock);
        if (_interrupt_requested.test() && !registers.SR.interrupt_disable)
            std::tie(registers.PC, registers.SP) =
                    interrupt(memory, registers.PC, registers.SP, registers.SR, IRQ, clock);

        if (const auto opcode = read(memory, registers.PC++, clock);
            !decode_and_execute(opcode, clock, memory, registers)) {
            std::cerr << std::format(
                    "Encountered an illegal opcode {:#04x} at address {:#06x}", opcode, registers.PC.prev())
                      << std::endl;
            _terminate.test_and_set();
        }
    }
}

void CPU::terminate() noexcept { _terminate.test_and_set(); }

mtl::u16 CPU::fetch_address(const MemoryAddressing addressing,
                            const Memory &memory,
                            mtl::u16 &pc,
                            const mtl::u8 x,
                            const mtl::u8 y,
                            Clock &clock) noexcept {
    switch (addressing) {
    case MemoryAddressing::Absolute: return fetch_absolute_address(memory, pc, clock);
    case MemoryAddressing::AbsoluteX: return fetch_absolute_address(memory, pc, x, clock);
    case MemoryAddressing::AbsoluteY: return fetch_absolute_address(memory, pc, y, clock);
    case MemoryAddressing::IndexedIndirect: return fetch_indexed_indirect_address(memory, pc, x, clock);
    case MemoryAddressing::IndirectIndexed: return fetch_indirect_indexed_address(memory, pc, y, clock);
    case MemoryAddressing::ZeroPage: return fetch_zero_page_address(memory, pc, clock);
    case MemoryAddressing::ZeroPageX: return fetch_zero_page_address(memory, pc, x, clock);
    case MemoryAddressing::ZeroPageY: return fetch_zero_page_address(memory, pc, y, clock);
    case MemoryAddressing::Indirect: return fetch_indirect_address(memory, pc, clock);
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

bool CPU::decode_and_execute(const mtl::u8 opcode, Clock &clock, Memory &memory, Registers &registers) {
    const auto instruction = getInstruction(opcode);
    if (!instruction) return false;

    const auto addressing = getAddressing(opcode);
    if (!addressing) mtl::panic("A valid opcode must contain both instruction and addressing");

    switch (*instruction) {
    case Instruction::LDA: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(arg);
    } break;

    case Instruction::STA: {
        if (!std::holds_alternative<MemoryAddressing>(*addressing)) mtl::panic("Unsupported addressing mode for STA");
        const auto address = fetch_address(
                std::get<MemoryAddressing>(*addressing), memory, registers.PC, registers.X, registers.Y, clock);
        write(memory, address, registers.AC, clock);
    } break;

    case Instruction::ADC: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);

        mtl::u8 result;
        if (registers.SR.decimal)
            std::tie(result, registers.SR.carry, registers.SR.overflow) =
                    ALU::add_decimal(registers.AC, arg, registers.SR.carry);
        else
            std::tie(result, registers.SR.carry, registers.SR.overflow) =
                    ALU::add_binary(registers.AC, arg, registers.SR.carry);
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(result);
    } break;

    case Instruction::SBC: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);

        mtl::u8 result;
        if (registers.SR.decimal)
            std::tie(result, registers.SR.carry, registers.SR.overflow) =
                    ALU::subtract_decimal(registers.AC, arg, registers.SR.carry);
        else
            std::tie(result, registers.SR.carry, registers.SR.overflow) =
                    ALU::subtract_binary(registers.AC, arg, registers.SR.carry);
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(result);
    } break;

    case Instruction::AND: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.AC & arg);
    } break;

    case Instruction::ORA: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.AC | arg);
    } break;

    case Instruction::EOR: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.AC ^ arg);
    } break;

    case Instruction::SEC: {
        clock.wait_for_pulse();
        registers.SR.carry = true;
    } break;

    case Instruction::CLC: {
        clock.wait_for_pulse();
        registers.SR.carry = false;
    } break;

    case Instruction::SEI: {
        clock.wait_for_pulse();
        registers.SR.interrupt_disable = true;
    } break;

    case Instruction::CLI: {
        clock.wait_for_pulse();
        registers.SR.interrupt_disable = false;
    } break;

    case Instruction::SED: {
        clock.wait_for_pulse();
        registers.SR.decimal = true;
    } break;

    case Instruction::CLD: {
        clock.wait_for_pulse();
        registers.SR.decimal = false;
    } break;

    case Instruction::CLV: {
        clock.wait_for_pulse();
        registers.SR.overflow = false;
    } break;

    case Instruction::JMP:
        if (!std::holds_alternative<MemoryAddressing>(*addressing)) mtl::panic("Unsupported addressing mode for JMP");
        registers.PC = fetch_address(
                std::get<MemoryAddressing>(*addressing), memory, registers.PC, registers.X, registers.Y, clock);
        break;

    case Instruction::BMI: registers.PC = branch(memory, registers.PC, registers.SR.negative, clock); break;

    case Instruction::BPL: registers.PC = branch(memory, registers.PC, !registers.SR.negative, clock); break;

    case Instruction::BCC: registers.PC = branch(memory, registers.PC, !registers.SR.carry, clock); break;

    case Instruction::BCS: registers.PC = branch(memory, registers.PC, registers.SR.carry, clock); break;

    case Instruction::BEQ: registers.PC = branch(memory, registers.PC, registers.SR.zero, clock); break;

    case Instruction::BNE: registers.PC = branch(memory, registers.PC, !registers.SR.zero, clock); break;

    case Instruction::BVS: registers.PC = branch(memory, registers.PC, registers.SR.overflow, clock); break;

    case Instruction::BVC: registers.PC = branch(memory, registers.PC, !registers.SR.overflow, clock); break;

    case Instruction::CMP: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.SR.negative, registers.SR.carry, registers.SR.zero) = compare(registers.AC, arg);
    } break;

    case Instruction::BIT: {
        const auto arg        = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        const auto result     = registers.AC & arg;
        registers.SR.negative = (result & mtl::u8(0x80)) != 0;
        registers.SR.overflow = (result & mtl::u8(0x40)) != 0;
        registers.SR.zero     = result == 0;
    } break;

    case Instruction::LDX: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(arg);
    } break;

    case Instruction::LDY: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.Y, registers.SR.zero, registers.SR.negative) = value_with_flags(arg);
    } break;

    case Instruction::STX: {
        if (!std::holds_alternative<MemoryAddressing>(*addressing)) mtl::panic("Unsupported addressing mode for STX");
        const auto address = fetch_address(
                std::get<MemoryAddressing>(*addressing), memory, registers.PC, registers.X, registers.Y, clock);
        write(memory, address, registers.X, clock);
    } break;

    case Instruction::STY: {
        if (!std::holds_alternative<MemoryAddressing>(*addressing)) mtl::panic("Unsupported addressing mode for STY");
        const auto address = fetch_address(
                std::get<MemoryAddressing>(*addressing), memory, registers.PC, registers.X, registers.Y, clock);
        write(memory, address, registers.Y, clock);
    } break;

    case Instruction::INX: {
        clock.wait_for_pulse();
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.X.next());
    } break;

    case Instruction::INY: {
        clock.wait_for_pulse();
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.Y.next());
    } break;

    case Instruction::DEX: {
        clock.wait_for_pulse();
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.X.prev());
    } break;

    case Instruction::DEY: {
        clock.wait_for_pulse();
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.Y.prev());
    } break;

    case Instruction::CPX: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.SR.negative, registers.SR.carry, registers.SR.zero) = compare(registers.X, arg);
    } break;

    case Instruction::CPY: {
        const auto arg = read(*addressing, memory, registers.PC, registers.X, registers.Y, clock);
        std::tie(registers.SR.negative, registers.SR.carry, registers.SR.zero) = compare(registers.Y, arg);
    } break;

    case Instruction::TAX: {
        clock.wait_for_pulse();
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.AC);
    } break;

    case Instruction::TXA: {
        clock.wait_for_pulse();
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.X);
    } break;

    case Instruction::TAY: {
        clock.wait_for_pulse();
        std::tie(registers.Y, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.AC);
    } break;

    case Instruction::TYA: {
        clock.wait_for_pulse();
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.Y);
    } break;

    case Instruction::JSR: {
        const auto adl = read(memory, registers.PC++, clock);
        clock.wait_for_pulse();
        registers.SP             = push(memory, registers.SP, high_byte(registers.PC), clock);
        registers.SP             = push(memory, registers.SP, low_byte(registers.PC), clock);
        const auto adh = read(memory, registers.PC, clock);
        registers.PC             = make_word(adh, adl);
    } break;

    case Instruction::RTS: {
        read(memory, registers.PC++, clock);
        clock.wait_for_pulse();
        registers.SP++;
        const auto pcl = read(memory, make_word(mtl::u8(0x01), registers.SP++), clock);
        const auto pch = read(memory, make_word(mtl::u8(0x01), registers.SP), clock);
        clock.wait_for_pulse();
        registers.PC = make_word(pch, pcl);
        registers.PC++;
    } break;

    case Instruction::PHA: {
        read(memory, registers.PC, clock); // the data is discarded
        registers.SP = push(memory, registers.SP, registers.AC, clock);
    } break;

    case Instruction::PLA: {
        read(memory, registers.PC, clock); // the data is discarded
        clock.wait_for_pulse();
        registers.SP++;
        std::tie(registers.AC, registers.SR.zero, registers.SR.negative) =
                value_with_flags(read(memory, make_word(mtl::u8(0x01), registers.SP), clock));
    } break;

    case Instruction::TXS: {
        clock.wait_for_pulse();
        registers.SP = registers.X;
    } break;

    case Instruction::TSX: {
        clock.wait_for_pulse();
        std::tie(registers.X, registers.SR.zero, registers.SR.negative) = value_with_flags(registers.SP);
    } break;

    case Instruction::PHP: {
        read(memory, registers.PC, clock); // the data is discarded
        registers.SP = push(memory, registers.SP, static_cast<mtl::u8>(registers.SR), clock);
    } break;

    case Instruction::PLP: {
        read(memory, registers.PC, clock); // the data is discarded
        clock.wait_for_pulse();
        registers.SP++;
        registers.SR = read(memory, make_word(mtl::u8(0x01), registers.SP), clock);
    } break;

    case Instruction::BRK:
        if (!registers.SR.interrupt_disable) {
            registers.SP = push(memory, registers.SP, static_cast<mtl::u8>(StatusRegister{ .break_ = true }), clock);
            std::tie(registers.PC, registers.SP) =
                    interrupt(memory, registers.PC, registers.SP, registers.SR, IRQ, clock);
        }
        break;

    case Instruction::RTI:
        std::tie(registers.PC, registers.SP, registers.SR) =
                return_from_interrupt(memory, registers.PC, registers.SP, clock);
        break;

    case Instruction::LSR:
        bit_manip(*addressing,
                  memory,
                  registers.PC,
                  registers.X,
                  registers.Y,
                  registers.AC,
                  registers.SR,
                  clock,
                  [](const mtl::u8 arg) { return ALU::shift_right(arg); });
        break;

    case Instruction::ASL:
        bit_manip(*addressing,
                  memory,
                  registers.PC,
                  registers.X,
                  registers.Y,
                  registers.AC,
                  registers.SR,
                  clock,
                  [](const mtl::u8 arg) { return shift_left(arg, 1); });
        break;

    case Instruction::ROL:
        bit_manip(*addressing,
                  memory,
                  registers.PC,
                  registers.X,
                  registers.Y,
                  registers.AC,
                  registers.SR,
                  clock,
                  [carry = registers.SR.carry](const mtl::u8 arg) { return ALU::rotate_left(arg, carry); });
        break;

    case Instruction::ROR:
        bit_manip(*addressing,
                  memory,
                  registers.PC,
                  registers.X,
                  registers.Y,
                  registers.AC,
                  registers.SR,
                  clock,
                  [carry = registers.SR.carry](const mtl::u8 arg) { return ALU::rotate_right(arg, carry); });
        break;

    case Instruction::INC: {
        if (!std::holds_alternative<MemoryAddressing>(*addressing)) mtl::panic("Unsupported addressing mode for INC");
        const auto memory_addressing = std::get<MemoryAddressing>(*addressing);
        // FIXME: for increment and decrement, all address fetches take up one more cycle
        const auto address =
                memory_addressing == MemoryAddressing::AbsoluteX
                        ? fetch_absolute_address_long(memory, registers.PC, registers.X, clock)
                        : fetch_address(memory_addressing, memory, registers.PC, registers.X, registers.Y, clock);
        const auto arg = read(memory, address, clock);
        write(memory, address, arg.next(), clock);
    } break;

    case Instruction::DEC: {
        if (!std::holds_alternative<MemoryAddressing>(*addressing)) mtl::panic("Unsupported addressing mode for DEC");
        const auto memory_addressing = std::get<MemoryAddressing>(*addressing);
        const auto address =
                memory_addressing == MemoryAddressing::AbsoluteX
                        ? fetch_absolute_address_long(memory, registers.PC, registers.X, clock)
                        : fetch_address(memory_addressing, memory, registers.PC, registers.X, registers.Y, clock);
        const auto arg = read(memory, address, clock);
        write(memory, address, arg.prev(), clock);
    } break;

    case Instruction::NOP: clock.wait_for_pulse(); break;

    default: mtl::panic(std::format("Unhandled instruction {}", to_string(*instruction)));
    }

    return true;
}

void CPU::reset(Clock &clock, const Memory &memory, mtl::u16 &PC, mtl::u8 &SP, bool &interrupt_disable) noexcept {
    interrupt_disable = true;
    read(memory, PC++, clock);
    read(memory, PC++, clock);
    SP = mtl::u8(0xFF);
    read(memory, make_word(mtl::u8(0x01), SP), clock);
    read(memory, make_word(mtl::u8(0x01), SP - mtl::u8(1)), clock);
    read(memory, make_word(mtl::u8(0x01), SP - mtl::u8(2)), clock);
    const auto pcl = read(memory, RES, clock);
    const auto pch = read(memory, RES.next(), clock);
    PC  = make_word(pch, pcl);
}

mtl::u8 CPU::read(const Memory &memory, const mtl::u16 address, Clock &clock) noexcept {
    clock.wait_for_pulse();
    return memory[address];
}

mtl::u8 CPU::read(const Addressing addressing,
                  const Memory &memory,
                  mtl::u16 &PC,
                  const mtl::u8 X,
                  const mtl::u8 Y,
                  Clock &clock) noexcept {
    if (std::holds_alternative<immediate_t>(addressing)) return read(memory, PC++, clock);

    if (!std::holds_alternative<MemoryAddressing>(addressing))
        mtl::panic("Unsupported addressing mode for reading from memory");
    const auto address = fetch_address(std::get<MemoryAddressing>(addressing), memory, PC, X, Y, clock);
    return read(memory, address, clock);
}

void CPU::write(Memory &memory, const mtl::u16 address, const mtl::u8 value, Clock &clock) noexcept {
    clock.wait_for_pulse();
    if (!memory.write(address, value)) mtl::panic("Writing to read-only memory");
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

mtl::u8 CPU::push(Memory &memory, const mtl::u8 sp, const mtl::u8 byte, Clock &clock) noexcept {
    write(memory, make_word(mtl::u8(0x01), sp), byte, clock);
    return sp.prev();
}

std::pair<mtl::u16, mtl::u8> CPU::interrupt(Memory &memory,
                                            const mtl::u16 pc,
                                            mtl::u8 sp,
                                            const StatusRegister sr,
                                            const mtl::u16 handler_address,
                                            Clock &clock) noexcept {
    read(memory, pc, clock); // this data is discarded
    sp             = push(memory, sp, high_byte(pc), clock);
    sp             = push(memory, sp, low_byte(pc), clock);
    sp             = push(memory, sp, static_cast<mtl::u8>(sr), clock);
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