//
// Created by Mikhail Tsaritsyn on Apr 22, 2025.
//

/******************************************************************************************************************
 * Code snippets are taken from
 * https://web.archive.org/web/20221112220344if_/http://archive.6502.org/datasheets/synertek_programming_manual.pdf
 ******************************************************************************************************************/

#include "CPU.hpp"
#include "helpers.hpp"
#include "linker.hpp"
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
/**
 * @brief An unsupported instruction to halt the CPU
 *
 * When the CPU encounters an illegal opcode, it terminates.
 * It can be used in testing to terminate right after the provided code is executed.
 */
static constexpr mtl::u8 HLT{ 0x02 };

/**
 * @brief Number of cycles elapsing during the startup routine
 */
static constexpr size_t STARTUP_DURATION = 7;

TEST(Program, Empty) {}

/// Example 2.3:
///
/// first number at {H1, L1}
/// second number at {H2, L2}
/// result written to {H3, L3}
///
/// LDA L1
/// CLC
/// ADC L2
/// STA L3
/// LDA H1
/// ADC H2
/// STA H3
TEST(Program, Add16Bit) {
    constexpr uint8_t L1 = 0x00;
    constexpr uint8_t H1 = 0x01;
    constexpr uint8_t L2 = 0x02;
    constexpr uint8_t H2 = 0x03;
    constexpr uint8_t L3 = 0x04;
    constexpr uint8_t H3 = 0x05;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0x18 }, // CLC: 1 byte, 2 cycles

                            mtl::u8{ 0xA5 }, // LDA zero page: 2 bytes, 3 cycles
                            mtl::u8{ L1 },
                            mtl::u8{ 0x65 }, // ADC zero page: 2 bytes, 3 cycles
                            mtl::u8{ L2 },
                            mtl::u8{ 0x85 }, // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ L3 },

                            mtl::u8{ 0xA5 }, // LDA zero page: 2 bytes, 3 cycles
                            mtl::u8{ H1 },
                            mtl::u8{ 0x65 }, // ADC zero page: 2 bytes, 3 cycles
                            mtl::u8{ H2 },
                            mtl::u8{ 0x85 }, // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{H3},

        HLT
    };

    constexpr size_t code_duration = 2 + (3 + 3 + 3) * 2;

    // insert the code to the memory
    auto data = linker::assemble({main, MAIN_ADDR});

    // initialize the arguments
    data[L1] = mtl::u8(0x93);
    data[L2] = mtl::u8(0x75);
    data[H1] = mtl::u8(0x03);
    data[H2] = mtl::u8(0x34);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{main.size()}.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(L3)], 0x08);                          // (data[L1] + data[L2]) % 0x100
    EXPECT_EQ(memory[mtl::u16(H3)], 0x38);                          // data[H1] + data[H2] + carry
}

/// Example 2.12
///
/// First number stored at ADDR_FIRST
/// Second number is stored at ADDR_SECOND
/// Result is written to ADDR_RESULT
///
/// CLC
/// SED
/// LDA ADDR_FIRST
/// ADC ADDR_SECOND
/// STA ADDR_RESULT
TEST(Program, DecimalAddition) {
    constexpr uint8_t ADDR_FIRST  = 0x00;
    constexpr uint8_t ADDR_SECOND = 0x01;
    constexpr uint8_t ADDR_RESULT = 0x02;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0xF8 },   // SED: 1 byte, 2 cycles
                            mtl::u8{ 0x18 },   // CLC: 1 byte, 2 cycles
                            mtl::u8{ 0xA5 },   // LDA zero page: 2 bytes, 3 cycles
                            mtl::u8{ ADDR_FIRST },
                            mtl::u8{ 0x65 },   // ADC zero page: 2 bytes, 3 cycles
                            mtl::u8{ ADDR_SECOND },
                            mtl::u8{ 0x85 },   // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ ADDR_RESULT },
                            HLT };

    constexpr size_t code_duration = 2 + 2 + 3 + 3 + 3;

    // insert the code to the memory
    auto data = linker::assemble({ main, MAIN_ADDR });

    // initialize the arguments
    data[ADDR_FIRST]  = mtl::u8(0x79);
    data[ADDR_SECOND] = mtl::u8(0x14);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{main.size()}.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0x93);                 // 79 + 14 = 93
}

/// Example 2.15:
///
/// first number at {H1, L1}
/// second number at {H2, L2}
/// result written to {H3, L3}
///
/// SEC
/// LDA L1
/// SBC L2
/// STA L3
/// LDA H1
/// SBC H2
/// STA H3
TEST(Program, Subtract16Bit) {
    constexpr uint8_t L1 = 0x00;
    constexpr uint8_t H1 = 0x01;
    constexpr uint8_t L2 = 0x02;
    constexpr uint8_t H2 = 0x03;
    constexpr uint8_t L3 = 0x04;
    constexpr uint8_t H3 = 0x05;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0x38 }, // SEC: 1 byte, 2 cycles

                            mtl::u8{ 0xA5 }, // LDA zero page: 2 bytes, 3 cycles
                            mtl::u8{ L1 },
                            mtl::u8{ 0xE5 }, // SBC zero page: 2 bytes, 3 cycles
                            mtl::u8{ L2 },
                            mtl::u8{ 0x85 }, // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{L3},

                            mtl::u8{0xA5}, // LDA zero page: 2 bytes, 3 cycles
                            mtl::u8{ H1 },
                            mtl::u8{ 0xE5 }, // SBC zero page: 2 bytes, 3 cycles
                            mtl::u8{ H2 },
                            mtl::u8{ 0x85 }, // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ H3 },

                            HLT };

    constexpr size_t code_duration = 2 + (3 + 3 + 3) * 2;

    // insert the code to the memory
    auto data = linker::assemble({ main, MAIN_ADDR });

    // initialize the arguments
    data[L1] = mtl::u8(0x75);
    data[L2] = mtl::u8(0x93);
    data[H1] = mtl::u8(0x03);
    data[H2] = mtl::u8(0x34);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{main.size()}.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(L3)], 0xE2);                          // (0x75 - 0x93) % 0x100
    EXPECT_EQ(memory[mtl::u16(H3)], 0xCE);                          // (0x03 - 0x34 - carry) % 0x100
}

/// Example 2.18
///
/// First number stored at ADDR_FIRST
/// Second number is stored at ADDR_SECOND
/// Result is written to ADDR_RESULT
///
/// SED
/// SEC
/// LDA ADDR_FIRST
/// SBC ADDR_SECOND
/// STA ADDR_RESULT
TEST(Program, DecimalSubtract) {
    constexpr uint8_t ADDR_FIRST  = 0x00;
    constexpr uint8_t ADDR_SECOND = 0x01;
    constexpr uint8_t ADDR_RESULT = 0x02;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0xF8 },                         // SED: 1 byte, 2 cycles
                            mtl::u8{ 0x38 },                         // SEC: 1 byte, 2 cycles
                            mtl::u8{0xA5}, // LDA zero page: 2 bytes, 3 cycles
                            mtl::u8{ADDR_FIRST},
                            mtl::u8{0xE5}, // SBC zero page: 2 bytes, 3 cycles
                            mtl::u8{ ADDR_SECOND }, mtl::u8{ 0x85 }, // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ ADDR_RESULT }, HLT };

    constexpr size_t code_duration = 2 + 2 + 3 + 3 + 3;

    // insert the code to the memory
    auto data = linker::assemble({main, MAIN_ADDR});

    // initialize the arguments
    data[ADDR_FIRST]  = mtl::u8(0x44);
    data[ADDR_SECOND] = mtl::u8(0x29);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{ main.size() }.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0x15);                 // 44 - 29 = 15
}

/**
 * @brief Example 2.19: clearing a bit with AND
 *
 * The argument and the mask are given as immediate values.
 * The result is written to @p ADDR_RESULT.
 *
 * @code
 * LDA #1100X111; X is 0 or 1
 * AND #11110111
 * STA ADDR_RESULT
 * @endcode
 */
TEST(Program, And) {
    constexpr uint8_t ADDR_RESULT = 0x00;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0xA9 },       // LDA immediate: 2 bytes, 2 cycles
                            mtl::u8{ 0b11001111 },
                            mtl::u8{ 0x29 },       // AND immediate: 2 bytes, 2 cycles
                            mtl::u8{ 0b11110111 },
                            mtl::u8{ 0x85 },       // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ADDR_RESULT},
                            HLT
    };

    constexpr size_t code_duration = 2 + 2 + 3;

    // insert the code to the memory
    const auto data = linker::assemble({main, MAIN_ADDR});

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{ main.size() }.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0b11000111);           // 0b11001111 & 0b111101111 = 0b11000111
}

/**
 * @brief Example 2.20: setting a bit with OR
 *
 * The argument and the mask are given as immediate values.
 * The result is written to @p ADDR_RESULT.
 *
 * @code
 * LDA #1110X111; X is 0 or 1
 * ORA #00001000
 * STA ADDR_RESULT
 * @endcode
 */
TEST(Program, Or) {
    constexpr uint8_t ADDR_RESULT = 0x00;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0xA9 },                         // LDA immediate: 2 bytes, 2 cycles
                            mtl::u8{ 0b11100111 },
                            mtl::u8{ 0x09 }, // ORA immediate: 2 bytes, 2 cycles
                            mtl::u8{ 0b00001000 },
                            mtl::u8{0x85}, // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ADDR_RESULT},
                            HLT
    };

    constexpr size_t code_duration = 2 + 2 + 3;

    // insert the code to the memory
    const auto data = linker::assemble({ main, MAIN_ADDR });

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{ main.size() }.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0b11101111);           // 0b11100111 | 0b00001000 = 0b11101111
}

/**
 * @brief Example 2.21: complementing a byte with EOR
 *
 * The argument and the mask are given as immediate values.
 * The result is written to @p ADDR_RESULT.
 *
 * @code
 * LDA #10101111
 * EOR #11111111
 * STA ADDR_RESULT
 * @endcode
 */
TEST(Program, Xor) {
    constexpr uint8_t ADDR_RESULT = 0x00;

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };

    const std::vector main{ mtl::u8{ 0xA9 },      // LDA immediate: 2 bytes, 2 cycles
                            mtl::u8{ 0b10101111 },
                            mtl::u8{ 0x49 },      // EOR immediate: 2 bytes, 2 cycles
                            mtl::u8{0b11111111},
                            mtl::u8{ 0x85 },      // STA zero page: 2 bytes, 3 cycles
                            mtl::u8{ ADDR_RESULT },
                            HLT };

    constexpr size_t code_duration = 2 + 2 + 3;

    // insert the code to the memory
    const auto data = linker::assemble({ main, MAIN_ADDR });

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    Registers registers{};
    CPU cpu{};
    cpu.start(memory, clock, registers);

    // check the results
    EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{main.size()}.unsafe_cast<uint16_t>());
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0b01010000);             // 0b10101111 ^ 0b11111111 = 0b01010000
}

/**
 * @brief Example 4.4: Illustration of "branch on carry set"
 *
 * Arguments:
 * 1. ADDR1 - absolute address of the first argument
 * 2. ADDR2 - absolute address of the second argument
 * 3. ADDR_RES - absolute address of the result
 *
 * @code
 * LDA ADDR1; opcode at address 0x0200
 * ADC ADDR2
 * BCS *+50
 * STA ADDR_RES
 * SED; opcode at address 0x0258, set decimal just to check that we were here
 * @endcode
 */
TEST(Program, BranchOnCarrySet) {
    constexpr mtl::u16 ADDR1{ 0x0300 };
    constexpr mtl::u16 ADDR2{ 0x0302 };
    constexpr mtl::u16 ADDR_RES{ 0x0304 };
    constexpr mtl::u8 OFFSET{ 0x50 };

    constexpr mtl::u16 MAIN_ADDR{ 0x0400 };
    constexpr mtl::u16 BRANCH_ADDR{ 0x458 };

    const std::vector main{ mtl::u8{ 0xAD }, // LDA absolute: 3 bytes, 4 cycles
                            low_byte(ADDR1),
                            high_byte(ADDR1),
                            mtl::u8{ 0x6D }, // ADC absolute: 3 bytes, 4 cycles
                            low_byte(ADDR2),
                            high_byte(ADDR2),
                            mtl::u8{ 0xB0 }, // BCS: 2 bytes, 2 cycles if fails, 3 if succeeds, 4 if to a new page
                            OFFSET,
                            mtl::u8{ 0x8D }, // STA absolute: 3 bytes, 4 cycles
                            low_byte(ADDR_RES),
                            high_byte(ADDR_RES),
                            HLT };

    const std::vector branch{ mtl::u8{ 0xF8 }, // SED: 1 byte, 2 cycles
                              HLT };

    auto data = linker::assemble(
            {.binary = main, .address_start = MAIN_ADDR}, {{.binary = branch, .address_start = BRANCH_ADDR}});

    { // branch not successful
        constexpr size_t code_duration = 4 + 4 + 2 + 4;

        // initialize the arguments
        data[ADDR1.to_underlying()] = mtl::u8(0x44);
        data[ADDR2.to_underlying()] = mtl::u8(0x29);

        // execute the program
        Clock clock(std::chrono::nanoseconds(0));
        Memory memory{ data };
        Registers registers{};
        CPU cpu{};
        cpu.start(memory, clock, registers);

        // check the results
        EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{main.size()}.unsafe_cast<uint16_t>());
        EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
        EXPECT_EQ(memory[ADDR_RES], 0x6D);                              // 0x44 + 0x29 = 0x6D
        EXPECT_FALSE(registers.SR.carry);                               // precondition for the branch to not happen
    }

    { // branch successful
        constexpr size_t code_duration = 4 + 4 + 3 + 2;

        // initialize the arguments
        data[ADDR1.to_underlying()]    = mtl::u8(0xFF);
        data[ADDR2.to_underlying()]    = mtl::u8(0xFF);
        data[ADDR_RES.to_underlying()] = mtl::u8(0x12); // canary value

        // execute the program
        Clock clock(std::chrono::nanoseconds(0));
        Memory memory{ data };
        Registers registers{};
        CPU cpu{};
        cpu.start(memory, clock, registers);

        // check the results
        EXPECT_EQ(registers.PC, BRANCH_ADDR + mtl::u16{ 2 });           // 1 for SED and 1 for HLT
        EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
        EXPECT_EQ(memory[ADDR_RES], 0x12);                              // the canary
        EXPECT_TRUE(registers.SR.carry);                                // precondition for the branch
        EXPECT_TRUE(registers.SR.decimal);                              // check that instruction after branch executed
    }
}

/**
 * @brief Example 4.5: Sequencing two branch instructions
 *
 * @code
 * LDA ADDR1
 * ADC ADDR2
 * BCS *+50
 * BMI *-75
 * STA
 * ADDR_RES
 * @endcode
 */
TEST(Program, SequencingTwoBranchInstructions) {
    constexpr mtl::u16 ADDR1{ 0x0400 };
    constexpr mtl::u16 ADDR2{ 0x0402 };
    constexpr mtl::u16 ADDR_RES{ 0x0404 };

    constexpr mtl::u8 OFFSET1{ 0x50 };
    constexpr auto OFFSET2 = std::bit_cast<mtl::u8>(mtl::i8{ -0x75 });

    constexpr mtl::u16 MAIN_ADDR{ 0x0300 };
    constexpr mtl::u16 CARRY_BRANCH_ADDR{ 0x0358 };
    constexpr mtl::u16 NEGATIVE_BRANCH_ADDR{ 0x0295 };

    const std::vector main{ mtl::u8{ 0xAD }, // LDA absolute: 3 bytes, 4 cycles
                            low_byte(ADDR1),
                            high_byte(ADDR1),
                            mtl::u8{ 0x6D }, // ADC absolute: 3 bytes, 4 cycles
                            low_byte(ADDR2),
                            high_byte(ADDR2),
                            mtl::u8{ 0xB0 }, // BCS: 2 bytes, 2 cycles if fails, 3 if succeeds, 4 if to a new page
                            OFFSET1,
                            mtl::u8{ 0x30 }, // BMI: 2 bytes, 2 cycles if fails, 3 if succeeds, 4 if to a new page
                            OFFSET2,
                            mtl::u8{ 0x8D }, // STA absolute: 3 bytes, 4 cycles
                            low_byte(ADDR_RES),
                            high_byte(ADDR_RES),
                            HLT };

    const std::vector carry_branch{ mtl::u8{ 0xF8 }, // SED: 1 byte, 2 cycles
                                    HLT };

    const std::vector negative_branch{ mtl::u8{ 0x78 }, // SEI: 1 byte, 2 cycles
                                       HLT };

    // assemble the code
    auto data = linker::assemble(
            {
                    .binary = main, .address_start = MAIN_ADDR
              },
            { { .binary = carry_branch, .address_start = CARRY_BRANCH_ADDR },
              { .binary = negative_branch, .address_start = NEGATIVE_BRANCH_ADDR } });

    { // no branching
        constexpr size_t code_duration = 4 + 4 + 2 + 2 + 4;

        // initialize the arguments
        data[ADDR1.to_underlying()] = mtl::u8(0x44);
        data[ADDR2.to_underlying()] = mtl::u8(0x29);

        // execute the program
        Clock clock(std::chrono::nanoseconds(0));
        Memory memory{ data };
        Registers registers{};
        CPU cpu{};
        cpu.start(memory, clock, registers);

        // check the results
        EXPECT_EQ(registers.PC, MAIN_ADDR + mtl::StrongInt{main.size()}.unsafe_cast<uint16_t>());
        EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
        EXPECT_EQ(memory[ADDR_RES], 0x6D);                              // 0x44 + 0x29 = 0x6D
        EXPECT_FALSE(registers.SR.carry);    // precondition for the first branch to not happen
        EXPECT_FALSE(registers.SR.negative); // precondition for the second branch to not happen
    }

    { // branch on carry
        constexpr size_t code_duration = 4 + 4 + 3 + 2;

        // initialize the arguments
        data[ADDR1.to_underlying()]    = mtl::u8(0xFF);
        data[ADDR2.to_underlying()]    = mtl::u8(0xFF);
        data[ADDR_RES.to_underlying()] = mtl::u8(0x12); // canary value

        // execute the program
        Clock clock(std::chrono::nanoseconds(0));
        Memory memory{ data };
        Registers registers{};
        CPU cpu{};
        cpu.start(memory, clock, registers);

        // check the results
        EXPECT_EQ(registers.PC, CARRY_BRANCH_ADDR + mtl::StrongInt{carry_branch.size()}.unsafe_cast<uint16_t>());
        EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
        EXPECT_EQ(memory[ADDR_RES], 0x12);                              // the canary
        EXPECT_TRUE(registers.SR.carry);                                // precondition for the branch
        EXPECT_TRUE(registers.SR.decimal);                              // check that instruction after branch executed
    }

    {                                                       // branch on negative
        constexpr size_t code_duration = 4 + 4 + 2 + 4 + 2; // page changed

        // initialize the arguments
        data[ADDR1.to_underlying()]    = mtl::u8(0x50);
        data[ADDR2.to_underlying()]    = mtl::u8(0x50);
        data[ADDR_RES.to_underlying()] = mtl::u8(0x12); // canary value

        // execute the program
        Clock clock(std::chrono::nanoseconds(0));
        Memory memory{ data };
        Registers registers{};
        CPU cpu{};
        cpu.start(memory, clock, registers);

        // check the results
        EXPECT_EQ(registers.PC,
                  NEGATIVE_BRANCH_ADDR + mtl::StrongInt{ negative_branch.size() }.unsafe_cast<uint16_t>());
        EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
        EXPECT_EQ(memory[ADDR_RES], 0x12);                              // the canary
        EXPECT_FALSE(registers.SR.carry);            // precondition for the previous branch to not happen
        EXPECT_TRUE(registers.SR.negative);          // precondition for the branch
        EXPECT_TRUE(registers.SR.interrupt_disable); // check that instruction after branch executed
    }
}
} // namespace emulator::mos_6502::test