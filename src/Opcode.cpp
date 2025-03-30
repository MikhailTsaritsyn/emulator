//
// Created by Mikhail Tsaritsyn on Mar 25, 2025.
//

#include "Opcode.hpp"

#include <utility>

namespace emulator::mos_6502 {
std::optional<Addressing> getAddressing(const uint8_t opcode) noexcept {
    const uint8_t a = (opcode & 0xE0) >> 5;
    const uint8_t b = (opcode & 0x1C) >> 2;
    const uint8_t c = opcode & 0x03;

    switch (b) {
    case 0:
        switch (c) {
        case 0:
            switch (a) {
            case 0: return Addressing::Implicit;
            case 1: return Addressing::Absolute;
            case 2: return Addressing::Implicit;
            case 3: return Addressing::Implicit;
            case 5: return Addressing::Immediate;
            case 6: return Addressing::Immediate;
            case 7: return Addressing::Immediate;
            default: return std::nullopt;
            }
        case 1: return Addressing::IndexedIndirect;
        case 2: return a == 5 ? std::make_optional(Addressing::Immediate) : std::nullopt;
        default: return std::nullopt;
        }
    case 1:
        switch (c) {
        case 0: return a == 0 || a == 2 || a == 3 ? std::nullopt : std::make_optional(Addressing::ZeroPage);
        case 1: return Addressing::ZeroPage;
        case 2: return Addressing::ZeroPage;
        default: return std::nullopt;
        }
    case 2:
        switch (c) {
        case 0: return Addressing::Implicit;
        case 1: return a == 4 ? std::nullopt : std::make_optional(Addressing::Immediate);
        case 2: return a < 4 ? Addressing::Accumulator : Addressing::Implicit;
        default: return std::nullopt;
        }
    case 3:
        switch (c) {
        case 0:
            switch (a) {
            case 0: return std::nullopt;
            case 3: return Addressing::Indirect;
            default: return Addressing::Absolute;
            }
        case 1: return Addressing::Absolute;
        case 2: return Addressing::Absolute;
        default: return std::nullopt;
        }
    case 4:
        switch (c) {
        case 0: return Addressing::Relative;
        case 1: return Addressing::IndirectIndexed;
        default: return std::nullopt;
        }
    case 5:
        switch (c) {
        case 0: return a == 4 || a == 5 ? std::make_optional(Addressing::ZeroPageX) : std::nullopt;
        case 1: return Addressing::ZeroPageX;
        case 2: return a == 4 || a == 5 ? Addressing::ZeroPageY : Addressing::ZeroPageX;
        default: return std::nullopt;
        }
    case 6:
        switch (c) {
        case 0: return Addressing::Implicit;
        case 1: return Addressing::AbsoluteY;
        case 2: return a == 4 || a == 5 ? std::make_optional(Addressing::Implicit) : std::nullopt;
        default: return std::nullopt;
        }
    case 7:
        switch (c) {
        case 0: return a == 5 ? std::make_optional(Addressing::AbsoluteX) : std::nullopt;
        case 1: return Addressing::AbsoluteX;
        case 2:
            switch (a) {
            case 4: return std::nullopt;
            case 5: return Addressing::AbsoluteY;
            default: return Addressing::AbsoluteX;
            }
        default: return std::nullopt;
        }
    default: std::unreachable();
    }
}

std::optional<Instruction> getInstruction(const uint8_t opcode) noexcept {
    const uint8_t a = (opcode & 0xE0) >> 5;
    const uint8_t b = (opcode & 0x1C) >> 2;
    const uint8_t c = opcode & 0x03;

    switch (c) {
    case 0:
        switch (a) {
        case 0:
            switch (b) {
            case 0: return Instruction::BRK;
            case 2: return Instruction::PHP;
            case 4: return Instruction::BPL;
            case 6: return Instruction::CLC;
            default: return std::nullopt;
            }
        case 1:
            switch (b) {
            case 0: return Instruction::JSR;
            case 1: return Instruction::BIT;
            case 2: return Instruction::PLP;
            case 3: return Instruction::BIT;
            case 4: return Instruction::BMI;
            case 6: return Instruction::SEC;
            default: return std::nullopt;
            }
        case 2:
            switch (b) {
            case 0: return Instruction::RTI;
            case 2: return Instruction::PHA;
            case 3: return Instruction::JMP;
            case 4: return Instruction::BVC;
            case 6: return Instruction::CLI;
            default: return std::nullopt;
            }
        case 3:
            switch (b) {
            case 0: return Instruction::RTS;
            case 2: return Instruction::PLA;
            case 3: return Instruction::JMP;
            case 4: return Instruction::BVS;
            case 6: return Instruction::SEI;
            default: return std::nullopt;
            }
        case 4:
            switch (b) {
            case 1: return Instruction::STY;
            case 2: return Instruction::DEY;
            case 3: return Instruction::STY;
            case 4: return Instruction::BCC;
            case 5: return Instruction::STY;
            case 6: return Instruction::TYA;
            default: return std::nullopt;
            }
        case 5:
            switch (b) {
            case 0: return Instruction::LDY;
            case 1: return Instruction::LDY;
            case 2: return Instruction::TAY;
            case 3: return Instruction::LDY;
            case 4: return Instruction::BCS;
            case 5: return Instruction::LDY;
            case 6: return Instruction::CLV;
            case 7: return Instruction::LDY;
            default: std::unreachable();
            }
        case 6:
            switch (b) {
            case 0: return Instruction::CPY;
            case 1: return Instruction::CPY;
            case 2: return Instruction::INY;
            case 3: return Instruction::CPY;
            case 4: return Instruction::BNE;
            case 6: return Instruction::CLD;
            default: return std::nullopt;
            }
        case 7:
            switch (b) {
            case 0: return Instruction::CPX;
            case 1: return Instruction::CPX;
            case 2: return Instruction::INX;
            case 3: return Instruction::CPX;
            case 4: return Instruction::BEQ;
            case 6: return Instruction::SED;
            default: return std::nullopt;
            }
        default: std::unreachable();
        }
    case 1:
        switch (a) {
        case 0: return Instruction::ORA;
        case 1: return Instruction::AND;
        case 2: return Instruction::EOR;
        case 3: return Instruction::ADC;
        case 4: return b == 2 ? std::nullopt : std::make_optional(Instruction::STA);
        case 5: return Instruction::LDA;
        case 6: return Instruction::CMP;
        case 7: return Instruction::SBC;
        default: std::unreachable();
        }
    case 2:
        switch (a) {
        case 0:
            if (b == 0 || b == 4 || b == 6) return std::nullopt;
            else return Instruction::ASL;
        case 1:
            if (b == 0 || b == 4 || b == 6) return std::nullopt;
            else return Instruction::ROL;
        case 2:
            if (b == 0 || b == 4 || b == 6) return std::nullopt;
            else return Instruction::LSR;
        case 3:
            if (b == 0 || b == 4 || b == 6) return std::nullopt;
            else return Instruction::ROR;
        case 4:
            switch (b) {
            case 1: return Instruction::STX;
            case 2: return Instruction::TXA;
            case 3: return Instruction::STX;
            case 5: return Instruction::STX;
            case 6: return Instruction::TXS;
            default: return std::nullopt;
            }
        case 5:
            switch (b) {
            case 0: return Instruction::LDX;
            case 1: return Instruction::LDX;
            case 2: return Instruction::TAX;
            case 3: return Instruction::LDX;
            case 5: return Instruction::LDX;
            case 6: return Instruction::TSX;
            case 7: return Instruction::LDX;
            default: return std::nullopt;
            }
        case 6:
            if (b == 0 || b == 4 || b == 6) return std::nullopt;
            else return b == 2 ? Instruction::DEX : Instruction::DEC;
        case 7:
            if (b == 0 || b == 4 || b == 6) return std::nullopt;
            else return b == 2 ? Instruction::NOP : Instruction::INC;
        default: std::unreachable();
        }
    default: return std::nullopt;
    }
}
} // namespace mos6502
