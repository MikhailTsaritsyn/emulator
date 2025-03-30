//
// Created by Mikhail Tsaritsyn on Mar 27, 2025.
//
#include "Opcode.hpp"

#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<uint8_t, std::optional<Instruction>, std::optional<Addressing>>;

struct Opcode : ::testing::TestWithParam<TestParameters> {};

TEST_P(Opcode, Decoding) {
    const auto [opcode, instruction, addressing] = GetParam();
    EXPECT_EQ(getInstruction(opcode), instruction);
    EXPECT_EQ(getAddressing(opcode), addressing);
}

INSTANTIATE_TEST_SUITE_P(Valid,
                         Opcode,
                         ::testing::Values(TestParameters{ 0x69, Instruction::ADC, Addressing::Immediate },
                                           TestParameters{ 0x65, Instruction::ADC, Addressing::ZeroPage },
                                           TestParameters{ 0x75, Instruction::ADC, Addressing::ZeroPageX },
                                           TestParameters{ 0x6D, Instruction::ADC, Addressing::Absolute },
                                           TestParameters{ 0x7D, Instruction::ADC, Addressing::AbsoluteX },
                                           TestParameters{ 0x79, Instruction::ADC, Addressing::AbsoluteY },
                                           TestParameters{ 0x61, Instruction::ADC, Addressing::IndexedIndirect },
                                           TestParameters{ 0x71, Instruction::ADC, Addressing::IndirectIndexed },

                                           TestParameters{ 0x29, Instruction::AND, Addressing::Immediate },
                                           TestParameters{ 0x25, Instruction::AND, Addressing::ZeroPage },
                                           TestParameters{ 0x35, Instruction::AND, Addressing::ZeroPageX },
                                           TestParameters{ 0x2D, Instruction::AND, Addressing::Absolute },
                                           TestParameters{ 0x3D, Instruction::AND, Addressing::AbsoluteX },
                                           TestParameters{ 0x39, Instruction::AND, Addressing::AbsoluteY },
                                           TestParameters{ 0x21, Instruction::AND, Addressing::IndexedIndirect },
                                           TestParameters{ 0x31, Instruction::AND, Addressing::IndirectIndexed },

                                           TestParameters{ 0x0A, Instruction::ASL, Addressing::Accumulator },
                                           TestParameters{ 0x06, Instruction::ASL, Addressing::ZeroPage },
                                           TestParameters{ 0x16, Instruction::ASL, Addressing::ZeroPageX },
                                           TestParameters{ 0x0E, Instruction::ASL, Addressing::Absolute },
                                           TestParameters{ 0x1E, Instruction::ASL, Addressing::AbsoluteX },

                                           TestParameters{ 0x90, Instruction::BCC, Addressing::Relative },
                                           TestParameters{ 0xB0, Instruction::BCS, Addressing::Relative },
                                           TestParameters{ 0xF0, Instruction::BEQ, Addressing::Relative },
                                           TestParameters{ 0x30, Instruction::BMI, Addressing::Relative },
                                           TestParameters{ 0xD0, Instruction::BNE, Addressing::Relative },
                                           TestParameters{ 0x10, Instruction::BPL, Addressing::Relative },
                                           TestParameters{ 0x50, Instruction::BVC, Addressing::Relative },
                                           TestParameters{ 0x70, Instruction::BVS, Addressing::Relative },

                                           TestParameters{ 0x24, Instruction::BIT, Addressing::ZeroPage },
                                           TestParameters{ 0x2C, Instruction::BIT, Addressing::Absolute },

                                           TestParameters{ 0x00, Instruction::BRK, Addressing::Implicit },

                                           TestParameters{ 0x18, Instruction::CLC, Addressing::Implicit },
                                           TestParameters{ 0xD8, Instruction::CLD, Addressing::Implicit },
                                           TestParameters{ 0x58, Instruction::CLI, Addressing::Implicit },
                                           TestParameters{ 0xB8, Instruction::CLV, Addressing::Implicit },

                                           TestParameters{ 0xC9, Instruction::CMP, Addressing::Immediate },
                                           TestParameters{ 0xC5, Instruction::CMP, Addressing::ZeroPage },
                                           TestParameters{ 0xD5, Instruction::CMP, Addressing::ZeroPageX },
                                           TestParameters{ 0xCD, Instruction::CMP, Addressing::Absolute },
                                           TestParameters{ 0xDD, Instruction::CMP, Addressing::AbsoluteX },
                                           TestParameters{ 0xD9, Instruction::CMP, Addressing::AbsoluteY },
                                           TestParameters{ 0xC1, Instruction::CMP, Addressing::IndexedIndirect },
                                           TestParameters{ 0xD1, Instruction::CMP, Addressing::IndirectIndexed },

                                           TestParameters{ 0xE0, Instruction::CPX, Addressing::Immediate },
                                           TestParameters{ 0xE4, Instruction::CPX, Addressing::ZeroPage },
                                           TestParameters{ 0xEC, Instruction::CPX, Addressing::Absolute },

                                           TestParameters{ 0xC0, Instruction::CPY, Addressing::Immediate },
                                           TestParameters{ 0xC4, Instruction::CPY, Addressing::ZeroPage },
                                           TestParameters{ 0xCC, Instruction::CPY, Addressing::Absolute },

                                           TestParameters{ 0xC6, Instruction::DEC, Addressing::ZeroPage },
                                           TestParameters{ 0xD6, Instruction::DEC, Addressing::ZeroPageX },
                                           TestParameters{ 0xCE, Instruction::DEC, Addressing::Absolute },
                                           TestParameters{ 0xDE, Instruction::DEC, Addressing::AbsoluteX },

                                           TestParameters{ 0xCA, Instruction::DEX, Addressing::Implicit },
                                           TestParameters{ 0x88, Instruction::DEY, Addressing::Implicit },

                                           TestParameters{ 0x49, Instruction::EOR, Addressing::Immediate },
                                           TestParameters{ 0x45, Instruction::EOR, Addressing::ZeroPage },
                                           TestParameters{ 0x55, Instruction::EOR, Addressing::ZeroPageX },
                                           TestParameters{ 0x4D, Instruction::EOR, Addressing::Absolute },
                                           TestParameters{ 0x5D, Instruction::EOR, Addressing::AbsoluteX },
                                           TestParameters{ 0x59, Instruction::EOR, Addressing::AbsoluteY },
                                           TestParameters{ 0x41, Instruction::EOR, Addressing::IndexedIndirect },
                                           TestParameters{ 0x51, Instruction::EOR, Addressing::IndirectIndexed },

                                           TestParameters{ 0xE6, Instruction::INC, Addressing::ZeroPage },
                                           TestParameters{ 0xF6, Instruction::INC, Addressing::ZeroPageX },
                                           TestParameters{ 0xEE, Instruction::INC, Addressing::Absolute },
                                           TestParameters{ 0xFE, Instruction::INC, Addressing::AbsoluteX },

                                           TestParameters{ 0xE8, Instruction::INX, Addressing::Implicit },
                                           TestParameters{ 0xC8, Instruction::INY, Addressing::Implicit },

                                           TestParameters{ 0x4C, Instruction::JMP, Addressing::Absolute },
                                           TestParameters{ 0x6C, Instruction::JMP, Addressing::Indirect },

                                           TestParameters{ 0x20, Instruction::JSR, Addressing::Absolute },

                                           TestParameters{ 0xA9, Instruction::LDA, Addressing::Immediate },
                                           TestParameters{ 0xA5, Instruction::LDA, Addressing::ZeroPage },
                                           TestParameters{ 0xB5, Instruction::LDA, Addressing::ZeroPageX },
                                           TestParameters{ 0xAD, Instruction::LDA, Addressing::Absolute },
                                           TestParameters{ 0xBD, Instruction::LDA, Addressing::AbsoluteX },
                                           TestParameters{ 0xB9, Instruction::LDA, Addressing::AbsoluteY },
                                           TestParameters{ 0xA1, Instruction::LDA, Addressing::IndexedIndirect },
                                           TestParameters{ 0xB1, Instruction::LDA, Addressing::IndirectIndexed },

                                           TestParameters{ 0xA2, Instruction::LDX, Addressing::Immediate },
                                           TestParameters{ 0xA6, Instruction::LDX, Addressing::ZeroPage },
                                           TestParameters{ 0xB6, Instruction::LDX, Addressing::ZeroPageY },
                                           TestParameters{ 0xAE, Instruction::LDX, Addressing::Absolute },
                                           TestParameters{ 0xBE, Instruction::LDX, Addressing::AbsoluteY },

                                           TestParameters{ 0xA0, Instruction::LDY, Addressing::Immediate },
                                           TestParameters{ 0xA4, Instruction::LDY, Addressing::ZeroPage },
                                           TestParameters{ 0xB4, Instruction::LDY, Addressing::ZeroPageX },
                                           TestParameters{ 0xAC, Instruction::LDY, Addressing::Absolute },
                                           TestParameters{ 0xBC, Instruction::LDY, Addressing::AbsoluteX },

                                           TestParameters{ 0x4A, Instruction::LSR, Addressing::Accumulator },
                                           TestParameters{ 0x46, Instruction::LSR, Addressing::ZeroPage },
                                           TestParameters{ 0x56, Instruction::LSR, Addressing::ZeroPageX },
                                           TestParameters{ 0x4E, Instruction::LSR, Addressing::Absolute },
                                           TestParameters{ 0x5E, Instruction::LSR, Addressing::AbsoluteX },

                                           TestParameters{ 0xEA, Instruction::NOP, Addressing::Implicit },

                                           TestParameters{ 0x09, Instruction::ORA, Addressing::Immediate },
                                           TestParameters{ 0x05, Instruction::ORA, Addressing::ZeroPage },
                                           TestParameters{ 0x15, Instruction::ORA, Addressing::ZeroPageX },
                                           TestParameters{ 0x0D, Instruction::ORA, Addressing::Absolute },
                                           TestParameters{ 0x1D, Instruction::ORA, Addressing::AbsoluteX },
                                           TestParameters{ 0x19, Instruction::ORA, Addressing::AbsoluteY },
                                           TestParameters{ 0x01, Instruction::ORA, Addressing::IndexedIndirect },
                                           TestParameters{ 0x11, Instruction::ORA, Addressing::IndirectIndexed },

                                           TestParameters{ 0x48, Instruction::PHA, Addressing::Implicit },
                                           TestParameters{ 0x08, Instruction::PHP, Addressing::Implicit },

                                           TestParameters{ 0x68, Instruction::PLA, Addressing::Implicit },
                                           TestParameters{ 0x28, Instruction::PLP, Addressing::Implicit },

                                           TestParameters{ 0x2A, Instruction::ROL, Addressing::Accumulator },
                                           TestParameters{ 0x26, Instruction::ROL, Addressing::ZeroPage },
                                           TestParameters{ 0x36, Instruction::ROL, Addressing::ZeroPageX },
                                           TestParameters{ 0x2E, Instruction::ROL, Addressing::Absolute },
                                           TestParameters{ 0x3E, Instruction::ROL, Addressing::AbsoluteX },

                                           TestParameters{ 0x6A, Instruction::ROR, Addressing::Accumulator },
                                           TestParameters{ 0x66, Instruction::ROR, Addressing::ZeroPage },
                                           TestParameters{ 0x76, Instruction::ROR, Addressing::ZeroPageX },
                                           TestParameters{ 0x6E, Instruction::ROR, Addressing::Absolute },
                                           TestParameters{ 0x7E, Instruction::ROR, Addressing::AbsoluteX },

                                           TestParameters{ 0x40, Instruction::RTI, Addressing::Implicit },
                                           TestParameters{ 0x60, Instruction::RTS, Addressing::Implicit },

                                           TestParameters{ 0xE9, Instruction::SBC, Addressing::Immediate },
                                           TestParameters{ 0xE5, Instruction::SBC, Addressing::ZeroPage },
                                           TestParameters{ 0xF5, Instruction::SBC, Addressing::ZeroPageX },
                                           TestParameters{ 0xED, Instruction::SBC, Addressing::Absolute },
                                           TestParameters{ 0xFD, Instruction::SBC, Addressing::AbsoluteX },
                                           TestParameters{ 0xF9, Instruction::SBC, Addressing::AbsoluteY },
                                           TestParameters{ 0xE1, Instruction::SBC, Addressing::IndexedIndirect },
                                           TestParameters{ 0xF1, Instruction::SBC, Addressing::IndirectIndexed },

                                           TestParameters{ 0x38, Instruction::SEC, Addressing::Implicit },
                                           TestParameters{ 0xF8, Instruction::SED, Addressing::Implicit },
                                           TestParameters{ 0x78, Instruction::SEI, Addressing::Implicit },

                                           TestParameters{ 0x85, Instruction::STA, Addressing::ZeroPage },
                                           TestParameters{ 0x95, Instruction::STA, Addressing::ZeroPageX },
                                           TestParameters{ 0x8D, Instruction::STA, Addressing::Absolute },
                                           TestParameters{ 0x9D, Instruction::STA, Addressing::AbsoluteX },
                                           TestParameters{ 0x99, Instruction::STA, Addressing::AbsoluteY },
                                           TestParameters{ 0x81, Instruction::STA, Addressing::IndexedIndirect },
                                           TestParameters{ 0x91, Instruction::STA, Addressing::IndirectIndexed },

                                           TestParameters{ 0x86, Instruction::STX, Addressing::ZeroPage },
                                           TestParameters{ 0x96, Instruction::STX, Addressing::ZeroPageY },
                                           TestParameters{ 0x8E, Instruction::STX, Addressing::Absolute },

                                           TestParameters{ 0x84, Instruction::STY, Addressing::ZeroPage },
                                           TestParameters{ 0x94, Instruction::STY, Addressing::ZeroPageX },
                                           TestParameters{ 0x8C, Instruction::STY, Addressing::Absolute },

                                           TestParameters{ 0xAA, Instruction::TAX, Addressing::Implicit },
                                           TestParameters{ 0xA8, Instruction::TAY, Addressing::Implicit },
                                           TestParameters{ 0xBA, Instruction::TSX, Addressing::Implicit },
                                           TestParameters{ 0x8A, Instruction::TXA, Addressing::Implicit },
                                           TestParameters{ 0x9A, Instruction::TXS, Addressing::Implicit },
                                           TestParameters{ 0x98, Instruction::TYA, Addressing::Implicit }));

INSTANTIATE_TEST_SUITE_P(Invalid,
                         Opcode,
                         ::testing::Values(TestParameters{ 0x02, std::nullopt, std::nullopt },
                                           TestParameters{ 0x03, std::nullopt, std::nullopt },
                                           TestParameters{ 0x04, std::nullopt, std::nullopt },
                                           TestParameters{ 0x07, std::nullopt, std::nullopt },
                                           TestParameters{ 0x0B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x0C, std::nullopt, std::nullopt },
                                           TestParameters{ 0x0F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x12, std::nullopt, std::nullopt },
                                           TestParameters{ 0x13, std::nullopt, std::nullopt },
                                           TestParameters{ 0x14, std::nullopt, std::nullopt },
                                           TestParameters{ 0x17, std::nullopt, std::nullopt },
                                           TestParameters{ 0x1A, std::nullopt, std::nullopt },
                                           TestParameters{ 0x1B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x1C, std::nullopt, std::nullopt },
                                           TestParameters{ 0x1F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x22, std::nullopt, std::nullopt },
                                           TestParameters{ 0x23, std::nullopt, std::nullopt },
                                           TestParameters{ 0x27, std::nullopt, std::nullopt },
                                           TestParameters{ 0x2B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x2F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x32, std::nullopt, std::nullopt },
                                           TestParameters{ 0x33, std::nullopt, std::nullopt },
                                           TestParameters{ 0x34, std::nullopt, std::nullopt },
                                           TestParameters{ 0x37, std::nullopt, std::nullopt },
                                           TestParameters{ 0x3A, std::nullopt, std::nullopt },
                                           TestParameters{ 0x3B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x3C, std::nullopt, std::nullopt },
                                           TestParameters{ 0x3F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x42, std::nullopt, std::nullopt },
                                           TestParameters{ 0x43, std::nullopt, std::nullopt },
                                           TestParameters{ 0x44, std::nullopt, std::nullopt },
                                           TestParameters{ 0x47, std::nullopt, std::nullopt },
                                           TestParameters{ 0x4B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x4F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x52, std::nullopt, std::nullopt },
                                           TestParameters{ 0x53, std::nullopt, std::nullopt },
                                           TestParameters{ 0x54, std::nullopt, std::nullopt },
                                           TestParameters{ 0x57, std::nullopt, std::nullopt },
                                           TestParameters{ 0x5A, std::nullopt, std::nullopt },
                                           TestParameters{ 0x5B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x5C, std::nullopt, std::nullopt },
                                           TestParameters{ 0x5F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x62, std::nullopt, std::nullopt },
                                           TestParameters{ 0x63, std::nullopt, std::nullopt },
                                           TestParameters{ 0x64, std::nullopt, std::nullopt },
                                           TestParameters{ 0x67, std::nullopt, std::nullopt },
                                           TestParameters{ 0x6B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x6F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x72, std::nullopt, std::nullopt },
                                           TestParameters{ 0x73, std::nullopt, std::nullopt },
                                           TestParameters{ 0x74, std::nullopt, std::nullopt },
                                           TestParameters{ 0x77, std::nullopt, std::nullopt },
                                           TestParameters{ 0x7A, std::nullopt, std::nullopt },
                                           TestParameters{ 0x7B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x7C, std::nullopt, std::nullopt },
                                           TestParameters{ 0x7F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x80, std::nullopt, std::nullopt },
                                           TestParameters{ 0x82, std::nullopt, std::nullopt },
                                           TestParameters{ 0x83, std::nullopt, std::nullopt },
                                           TestParameters{ 0x87, std::nullopt, std::nullopt },
                                           TestParameters{ 0x89, std::nullopt, std::nullopt },
                                           TestParameters{ 0x8B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x8F, std::nullopt, std::nullopt },

                                           TestParameters{ 0x92, std::nullopt, std::nullopt },
                                           TestParameters{ 0x93, std::nullopt, std::nullopt },
                                           TestParameters{ 0x97, std::nullopt, std::nullopt },
                                           TestParameters{ 0x9B, std::nullopt, std::nullopt },
                                           TestParameters{ 0x9C, std::nullopt, std::nullopt },
                                           TestParameters{ 0x9E, std::nullopt, std::nullopt },
                                           TestParameters{ 0x9F, std::nullopt, std::nullopt },

                                           TestParameters{ 0xA3, std::nullopt, std::nullopt },
                                           TestParameters{ 0xA7, std::nullopt, std::nullopt },
                                           TestParameters{ 0xAB, std::nullopt, std::nullopt },
                                           TestParameters{ 0xAF, std::nullopt, std::nullopt },

                                           TestParameters{ 0xB2, std::nullopt, std::nullopt },
                                           TestParameters{ 0xB3, std::nullopt, std::nullopt },
                                           TestParameters{ 0xB7, std::nullopt, std::nullopt },
                                           TestParameters{ 0xBB, std::nullopt, std::nullopt },
                                           TestParameters{ 0xBF, std::nullopt, std::nullopt },

                                           TestParameters{ 0xC2, std::nullopt, std::nullopt },
                                           TestParameters{ 0xC3, std::nullopt, std::nullopt },
                                           TestParameters{ 0xC7, std::nullopt, std::nullopt },
                                           TestParameters{ 0xCB, std::nullopt, std::nullopt },
                                           TestParameters{ 0xCF, std::nullopt, std::nullopt },

                                           TestParameters{ 0xD2, std::nullopt, std::nullopt },
                                           TestParameters{ 0xD3, std::nullopt, std::nullopt },
                                           TestParameters{ 0xD4, std::nullopt, std::nullopt },
                                           TestParameters{ 0xD7, std::nullopt, std::nullopt },
                                           TestParameters{ 0xDA, std::nullopt, std::nullopt },
                                           TestParameters{ 0xDB, std::nullopt, std::nullopt },
                                           TestParameters{ 0xDC, std::nullopt, std::nullopt },
                                           TestParameters{ 0xDF, std::nullopt, std::nullopt },

                                           TestParameters{ 0xE2, std::nullopt, std::nullopt },
                                           TestParameters{ 0xE3, std::nullopt, std::nullopt },
                                           TestParameters{ 0xE7, std::nullopt, std::nullopt },
                                           TestParameters{ 0xEB, std::nullopt, std::nullopt },
                                           TestParameters{ 0xEF, std::nullopt, std::nullopt },

                                           TestParameters{ 0xF2, std::nullopt, std::nullopt },
                                           TestParameters{ 0xF3, std::nullopt, std::nullopt },
                                           TestParameters{ 0xF4, std::nullopt, std::nullopt },
                                           TestParameters{ 0xF7, std::nullopt, std::nullopt },
                                           TestParameters{ 0xFA, std::nullopt, std::nullopt },
                                           TestParameters{ 0xFB, std::nullopt, std::nullopt },
                                           TestParameters{ 0xFC, std::nullopt, std::nullopt },
                                           TestParameters{ 0xFF, std::nullopt, std::nullopt }));
} // namespace emulator::mos6502::test
