//
// Created by Mikhail Tsaritsyn on Apr 20, 2025.
//

#include "helpers.hpp"

#include <format>
#include <iostream>

namespace emulator::mos_6502 {
std::pair<uint8_t, bool> add_with_overflow(const uint8_t a, const uint8_t b) noexcept {
    uint8_t result;
    const auto overflow = __builtin_add_overflow(a, b, &result);
    return { result, overflow };
}

std::pair<uint8_t, SignedOverflow> add_with_overflow(uint8_t u, int8_t i) noexcept {
    const auto result   = static_cast<int16_t>(u) + static_cast<int16_t>(i);
    const auto overflow = [result] {
        if (result < 0) return SignedOverflow::Negative;
        if (result > std::numeric_limits<uint8_t>::max()) return SignedOverflow::Positive;
        return SignedOverflow::None;
    }();
    return { result, overflow };
}

void panic(const std::string_view message) {
    std::cerr << message << std::endl;
    __builtin_trap();
}

std::string to_string(const Instruction instruction) noexcept {
    switch (instruction) {
    case Instruction::ADC: return "ADC";
    case Instruction::AND: return "AND";
    case Instruction::ASL: return "ASL";
    case Instruction::BCC: return "BCC";
    case Instruction::BCS: return "BCS";
    case Instruction::BEQ: return "BEQ";
    case Instruction::BIT: return "BIT";
    case Instruction::BMI: return "BMI";
    case Instruction::BNE: return "BNE";
    case Instruction::BPL: return "BPL";
    case Instruction::BRK: return "BRK";
    case Instruction::BVC: return "BVC";
    case Instruction::BVS: return "BVS";
    case Instruction::CLC: return "CLC";
    case Instruction::CLD: return "CLD";
    case Instruction::CLI: return "CLI";
    case Instruction::CLV: return "CLV";
    case Instruction::CMP: return "CMP";
    case Instruction::CPX: return "CPX";
    case Instruction::CPY: return "CPY";
    case Instruction::DEC: return "DEC";
    case Instruction::DEX: return "DEX";
    case Instruction::DEY: return "DEY";
    case Instruction::EOR: return "EOR";
    case Instruction::INC: return "INC";
    case Instruction::INX: return "INX";
    case Instruction::INY: return "INY";
    case Instruction::JMP: return "JMP";
    case Instruction::JSR: return "JSR";
    case Instruction::LDA: return "LDA";
    case Instruction::LDX: return "LDX";
    case Instruction::LDY: return "LDY";
    case Instruction::LSR: return "LSR";
    case Instruction::NOP: return "NOP";
    case Instruction::ORA: return "ORA";
    case Instruction::PHA: return "PHA";
    case Instruction::PHP: return "PHP";
    case Instruction::PLA: return "PLA";
    case Instruction::PLP: return "PLP";
    case Instruction::ROL: return "ROL";
    case Instruction::ROR: return "ROR";
    case Instruction::RTI: return "RTI";
    case Instruction::RTS: return "RTS";
    case Instruction::SBC: return "SBC";
    case Instruction::SEC: return "SEC";
    case Instruction::SED: return "SED";
    case Instruction::SEI: return "SEI";
    case Instruction::STA: return "STA";
    case Instruction::STX: return "STX";
    case Instruction::STY: return "STY";
    case Instruction::TAX: return "TAX";
    case Instruction::TAY: return "TAY";
    case Instruction::TSX: return "TSX";
    case Instruction::TXA: return "TXA";
    case Instruction::TXS: return "TXS";
    case Instruction::TYA: return "TYA";
    }

    return std::format("{:d}", std::to_underlying(instruction));
}
} // namespace emulator::mos_6502