//
// Created by Mikhail Tsaritsyn on Mar 27, 2025.
//
#include "Opcode.hpp"

#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
using TestParameters = std::tuple<mtl::u8, std::optional<Instruction>, std::optional<Addressing>>;

struct Opcode : ::testing::TestWithParam<TestParameters> {};

TEST_P(Opcode, Decoding) {
    const auto [opcode, instruction, addressing] = GetParam();
    EXPECT_EQ(getInstruction(opcode), instruction);

    const auto decodedAddressing = getAddressing(opcode);
    if (!addressing) {
        EXPECT_FALSE(decodedAddressing.has_value());
    } else {
        ASSERT_TRUE(decodedAddressing.has_value());
        if (std::holds_alternative<accumulator_t>(*addressing)) {
            EXPECT_TRUE(std::holds_alternative<accumulator_t>(*decodedAddressing));
        }
        if (std::holds_alternative<implicit_t>(*addressing)) {
            EXPECT_TRUE(std::holds_alternative<implicit_t>(*decodedAddressing));
        }
        if (std::holds_alternative<immediate_t>(*addressing)) {
            EXPECT_TRUE(std::holds_alternative<immediate_t>(*decodedAddressing));
        }
        if (std::holds_alternative<relative_t>(*addressing)) {
            EXPECT_TRUE(std::holds_alternative<relative_t>(*decodedAddressing));
        }
        if (std::holds_alternative<MemoryAddressing>(*addressing)) {
            ASSERT_TRUE(std::holds_alternative<MemoryAddressing>(*decodedAddressing));
            EXPECT_EQ(std::get<MemoryAddressing>(*decodedAddressing), std::get<MemoryAddressing>(*addressing));
        }
    }
}

INSTANTIATE_TEST_SUITE_P(Valid,
                         Opcode,
                         ::testing::Values(TestParameters{ 0x69, Instruction::ADC, immediate_t{} },
                                           TestParameters{ 0x65, Instruction::ADC, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x75, Instruction::ADC, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x6D, Instruction::ADC, MemoryAddressing::Absolute },
                                           TestParameters{ 0x7D, Instruction::ADC, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0x79, Instruction::ADC, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0x61, Instruction::ADC, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0x71, Instruction::ADC, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0x29, Instruction::AND, immediate_t{} },
                                           TestParameters{ 0x25, Instruction::AND, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x35, Instruction::AND, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x2D, Instruction::AND, MemoryAddressing::Absolute },
                                           TestParameters{ 0x3D, Instruction::AND, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0x39, Instruction::AND, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0x21, Instruction::AND, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0x31, Instruction::AND, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0x0A, Instruction::ASL, accumulator_t{} },
                                           TestParameters{ 0x06, Instruction::ASL, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x16, Instruction::ASL, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x0E, Instruction::ASL, MemoryAddressing::Absolute },
                                           TestParameters{ 0x1E, Instruction::ASL, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0x90, Instruction::BCC, relative_t{} },
                                           TestParameters{ 0xB0, Instruction::BCS, relative_t{} },
                                           TestParameters{ 0xF0, Instruction::BEQ, relative_t{} },
                                           TestParameters{ 0x30, Instruction::BMI, relative_t{} },
                                           TestParameters{ 0xD0, Instruction::BNE, relative_t{} },
                                           TestParameters{ 0x10, Instruction::BPL, relative_t{} },
                                           TestParameters{ 0x50, Instruction::BVC, relative_t{} },
                                           TestParameters{ 0x70, Instruction::BVS, relative_t{} },

                                           TestParameters{ 0x24, Instruction::BIT, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x2C, Instruction::BIT, MemoryAddressing::Absolute },

                                           TestParameters{ 0x00, Instruction::BRK, implicit_t{} },

                                           TestParameters{ 0x18, Instruction::CLC, implicit_t{} },
                                           TestParameters{ 0xD8, Instruction::CLD, implicit_t{} },
                                           TestParameters{ 0x58, Instruction::CLI, implicit_t{} },
                                           TestParameters{ 0xB8, Instruction::CLV, implicit_t{} },

                                           TestParameters{ 0xC9, Instruction::CMP, immediate_t{} },
                                           TestParameters{ 0xC5, Instruction::CMP, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xD5, Instruction::CMP, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0xCD, Instruction::CMP, MemoryAddressing::Absolute },
                                           TestParameters{ 0xDD, Instruction::CMP, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0xD9, Instruction::CMP, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0xC1, Instruction::CMP, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0xD1, Instruction::CMP, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0xE0, Instruction::CPX, immediate_t{} },
                                           TestParameters{ 0xE4, Instruction::CPX, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xEC, Instruction::CPX, MemoryAddressing::Absolute },

                                           TestParameters{ 0xC0, Instruction::CPY, immediate_t{} },
                                           TestParameters{ 0xC4, Instruction::CPY, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xCC, Instruction::CPY, MemoryAddressing::Absolute },

                                           TestParameters{ 0xC6, Instruction::DEC, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xD6, Instruction::DEC, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0xCE, Instruction::DEC, MemoryAddressing::Absolute },
                                           TestParameters{ 0xDE, Instruction::DEC, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0xCA, Instruction::DEX, implicit_t{} },
                                           TestParameters{ 0x88, Instruction::DEY, implicit_t{} },

                                           TestParameters{ 0x49, Instruction::EOR, immediate_t{} },
                                           TestParameters{ 0x45, Instruction::EOR, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x55, Instruction::EOR, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x4D, Instruction::EOR, MemoryAddressing::Absolute },
                                           TestParameters{ 0x5D, Instruction::EOR, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0x59, Instruction::EOR, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0x41, Instruction::EOR, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0x51, Instruction::EOR, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0xE6, Instruction::INC, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xF6, Instruction::INC, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0xEE, Instruction::INC, MemoryAddressing::Absolute },
                                           TestParameters{ 0xFE, Instruction::INC, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0xE8, Instruction::INX, implicit_t{} },
                                           TestParameters{ 0xC8, Instruction::INY, implicit_t{} },

                                           TestParameters{ 0x4C, Instruction::JMP, MemoryAddressing::Absolute },
                                           TestParameters{ 0x6C, Instruction::JMP, MemoryAddressing::Indirect },

                                           TestParameters{ 0x20, Instruction::JSR, MemoryAddressing::Absolute },

                                           TestParameters{ 0xA9, Instruction::LDA, immediate_t{} },
                                           TestParameters{ 0xA5, Instruction::LDA, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xB5, Instruction::LDA, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0xAD, Instruction::LDA, MemoryAddressing::Absolute },
                                           TestParameters{ 0xBD, Instruction::LDA, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0xB9, Instruction::LDA, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0xA1, Instruction::LDA, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0xB1, Instruction::LDA, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0xA2, Instruction::LDX, immediate_t{} },
                                           TestParameters{ 0xA6, Instruction::LDX, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xB6, Instruction::LDX, MemoryAddressing::ZeroPageY },
                                           TestParameters{ 0xAE, Instruction::LDX, MemoryAddressing::Absolute },
                                           TestParameters{ 0xBE, Instruction::LDX, MemoryAddressing::AbsoluteY },

                                           TestParameters{ 0xA0, Instruction::LDY, immediate_t{} },
                                           TestParameters{ 0xA4, Instruction::LDY, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xB4, Instruction::LDY, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0xAC, Instruction::LDY, MemoryAddressing::Absolute },
                                           TestParameters{ 0xBC, Instruction::LDY, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0x4A, Instruction::LSR, accumulator_t{} },
                                           TestParameters{ 0x46, Instruction::LSR, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x56, Instruction::LSR, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x4E, Instruction::LSR, MemoryAddressing::Absolute },
                                           TestParameters{ 0x5E, Instruction::LSR, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0xEA, Instruction::NOP, implicit_t{} },

                                           TestParameters{ 0x09, Instruction::ORA, immediate_t{} },
                                           TestParameters{ 0x05, Instruction::ORA, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x15, Instruction::ORA, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x0D, Instruction::ORA, MemoryAddressing::Absolute },
                                           TestParameters{ 0x1D, Instruction::ORA, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0x19, Instruction::ORA, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0x01, Instruction::ORA, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0x11, Instruction::ORA, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0x48, Instruction::PHA, implicit_t{} },
                                           TestParameters{ 0x08, Instruction::PHP, implicit_t{} },

                                           TestParameters{ 0x68, Instruction::PLA, implicit_t{} },
                                           TestParameters{ 0x28, Instruction::PLP, implicit_t{} },

                                           TestParameters{ 0x2A, Instruction::ROL, accumulator_t{} },
                                           TestParameters{ 0x26, Instruction::ROL, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x36, Instruction::ROL, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x2E, Instruction::ROL, MemoryAddressing::Absolute },
                                           TestParameters{ 0x3E, Instruction::ROL, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0x6A, Instruction::ROR, accumulator_t{} },
                                           TestParameters{ 0x66, Instruction::ROR, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x76, Instruction::ROR, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x6E, Instruction::ROR, MemoryAddressing::Absolute },
                                           TestParameters{ 0x7E, Instruction::ROR, MemoryAddressing::AbsoluteX },

                                           TestParameters{ 0x40, Instruction::RTI, implicit_t{} },
                                           TestParameters{ 0x60, Instruction::RTS, implicit_t{} },

                                           TestParameters{ 0xE9, Instruction::SBC, immediate_t{} },
                                           TestParameters{ 0xE5, Instruction::SBC, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0xF5, Instruction::SBC, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0xED, Instruction::SBC, MemoryAddressing::Absolute },
                                           TestParameters{ 0xFD, Instruction::SBC, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0xF9, Instruction::SBC, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0xE1, Instruction::SBC, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0xF1, Instruction::SBC, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0x38, Instruction::SEC, implicit_t{} },
                                           TestParameters{ 0xF8, Instruction::SED, implicit_t{} },
                                           TestParameters{ 0x78, Instruction::SEI, implicit_t{} },

                                           TestParameters{ 0x85, Instruction::STA, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x95, Instruction::STA, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x8D, Instruction::STA, MemoryAddressing::Absolute },
                                           TestParameters{ 0x9D, Instruction::STA, MemoryAddressing::AbsoluteX },
                                           TestParameters{ 0x99, Instruction::STA, MemoryAddressing::AbsoluteY },
                                           TestParameters{ 0x81, Instruction::STA, MemoryAddressing::IndexedIndirect },
                                           TestParameters{ 0x91, Instruction::STA, MemoryAddressing::IndirectIndexed },

                                           TestParameters{ 0x86, Instruction::STX, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x96, Instruction::STX, MemoryAddressing::ZeroPageY },
                                           TestParameters{ 0x8E, Instruction::STX, MemoryAddressing::Absolute },

                                           TestParameters{ 0x84, Instruction::STY, MemoryAddressing::ZeroPage },
                                           TestParameters{ 0x94, Instruction::STY, MemoryAddressing::ZeroPageX },
                                           TestParameters{ 0x8C, Instruction::STY, MemoryAddressing::Absolute },

                                           TestParameters{ 0xAA, Instruction::TAX, implicit_t{} },
                                           TestParameters{ 0xA8, Instruction::TAY, implicit_t{} },
                                           TestParameters{ 0xBA, Instruction::TSX, implicit_t{} },
                                           TestParameters{ 0x8A, Instruction::TXA, implicit_t{} },
                                           TestParameters{ 0x9A, Instruction::TXS, implicit_t{} },
                                           TestParameters{ 0x98, Instruction::TYA, implicit_t{} }));

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
