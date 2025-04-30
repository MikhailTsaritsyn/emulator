//
// Created by Mikhail Tsaritsyn on Apr 22, 2025.
//

/******************************************************************************************************************
 * Code snippets are taken from
 * https://web.archive.org/web/20221112220344if_/http://archive.6502.org/datasheets/synertek_programming_manual.pdf
 ******************************************************************************************************************/

#include "CPU.hpp"
#include "helpers.hpp"
#include <mtl/panic.hpp>
#include <gtest/gtest.h>

namespace emulator::mos_6502::test {
struct Program : public ::testing::Test {
    /**
     * @brief The address where the program to execute starts
     */
    static constexpr uint16_t PROGRAM_START = 0x0200;

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

    /**
     * @brief Prepare a chunk of memory containing a piece of code
     *
     * The code is prepended with @p CLI instruction to enable interrupts.
     * The prepended code is inserted at the predefined @p PROGRAM_START address.
     * The code is appended with an illegal @p HLT opcode to terminate the execution.
     *
     * @retval first Chunk of memory containing the code, prepared to run a CPU on it.
     * @retval second Index in the resulting memory past the @p HLT opcode.
     */
    [[nodiscard]] static std::pair<Memory::Data, size_t> assemble(const std::vector<mtl::u8> &code) noexcept {
        Memory::Data result{};
        if (code.size() > result.size()) mtl::panic("Code is too long to fit in memory");

        result[PROGRAM_START] = mtl::u8(0x58); // CLI, to clear the interrupt disable flag set at startup
        std::ranges::copy(code, result.begin() + PROGRAM_START + 1);

        result[PROGRAM_START + 1 + code.size()] = HLT;

        result[CPU::RES.to_underlying()]     = low_byte(mtl::u16(PROGRAM_START));
        result[CPU::RES.to_underlying() + 1] = high_byte(mtl::u16(PROGRAM_START));
        return { result, PROGRAM_START + code.size() + 2 };
    }
};

TEST_F(Program, Empty) {}

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
TEST_F(Program, Add16Bit) {
    constexpr uint8_t L1 = 0x00;
    constexpr uint8_t H1 = 0x01;
    constexpr uint8_t L2 = 0x02;
    constexpr uint8_t H2 = 0x03;
    constexpr uint8_t L3 = 0x04;
    constexpr uint8_t H3 = 0x05;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // LDA L1
    code.emplace_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    code.emplace_back(L1);
    code_duration += 3;

    // CLC
    code.emplace_back(0x18); // CLC: 1 byte, 2 cycles
    code_duration += 2;

    // ADC L2
    code.emplace_back(0x65); // ADC zero page: 2 bytes, 3 cycles
    code.emplace_back(L2);
    code_duration += 3;

    // STA L3
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(L3);
    code_duration += 3;

    // LDA H1
    code.emplace_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    code.emplace_back(H1);
    code_duration += 3;

    // ADC H2
    code.emplace_back(0x65); // ADC zero page: 2 bytes, 3 cycles
    code.emplace_back(H2);
    code_duration += 3;

    // STA H3
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(H3);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // initialize the arguments
    data[L1] = mtl::u8(0x93);
    data[L2] = mtl::u8(0x75);
    data[H1] = mtl::u8(0x03);
    data[H2] = mtl::u8(0x34);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
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
TEST_F(Program, DecimalAddition) {
    constexpr uint8_t ADDR_FIRST  = 0x00;
    constexpr uint8_t ADDR_SECOND = 0x01;
    constexpr uint8_t ADDR_RESULT = 0x02;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // CLC
    code.emplace_back(0x18); // 1 byte, 2 cycles
    code_duration += 2;

    // SED
    code.emplace_back(0xF8); // 1 byte, 2 cycles
    code_duration += 2;

    // LDA zero page
    code.emplace_back(0xA5); // 2 bytes, 3 cycles
    code.emplace_back(ADDR_FIRST);
    code_duration += 3;

    // ADC zero page
    code.emplace_back(0x65); // 2 bytes, 3 cycles
    code.emplace_back(ADDR_SECOND);
    code_duration += 3;

    // STA zero page
    code.emplace_back(0x85); // 2 bytes, 3 cycles
    code.emplace_back(ADDR_RESULT);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // initialize the arguments
    data[ADDR_FIRST]  = mtl::u8(0x79);
    data[ADDR_SECOND] = mtl::u8(0x14);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
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
TEST_F(Program, Subtract16Bit) {
    constexpr uint8_t L1 = 0x00;
    constexpr uint8_t H1 = 0x01;
    constexpr uint8_t L2 = 0x02;
    constexpr uint8_t H2 = 0x03;
    constexpr uint8_t L3 = 0x04;
    constexpr uint8_t H3 = 0x05;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // SEC
    code.emplace_back(0x38); // 1 byte, 2 cycles
    code_duration += 2;

    // LDA L1
    code.emplace_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    code.emplace_back(L1);
    code_duration += 3;

    // SBC L2
    code.emplace_back(0xE5); // SBC zero page: 2 bytes, 3 cycles
    code.emplace_back(L2);
    code_duration += 3;

    // STA L3
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(L3);
    code_duration += 3;

    // LDA H1
    code.emplace_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    code.emplace_back(H1);
    code_duration += 3;

    // SBC H2
    code.emplace_back(0xE5); // SBC zero page: 2 bytes, 3 cycles
    code.emplace_back(H2);
    code_duration += 3;

    // STA H3
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(H3);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // initialize the arguments
    data[L1] = mtl::u8(0x75);
    data[L2] = mtl::u8(0x93);
    data[H1] = mtl::u8(0x03);
    data[H2] = mtl::u8(0x34);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
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
TEST_F(Program, DecimalSubtract) {
    constexpr uint8_t ADDR_FIRST  = 0x00;
    constexpr uint8_t ADDR_SECOND = 0x01;
    constexpr uint8_t ADDR_RESULT = 0x02;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // SED
    code.emplace_back(0xF8); // 1 byte, 2 cycles
    code_duration += 2;

    // SEC
    code.emplace_back(0x38); // 1 byte, 2 cycles
    code_duration += 2;

    // LDA ADDR_FIRST
    code.emplace_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    code.emplace_back(ADDR_FIRST);
    code_duration += 3;

    // SBC ADDR_SECOND
    code.emplace_back(0xE5); // SBC zero page: 2 bytes, 3 cycles
    code.emplace_back(ADDR_SECOND);
    code_duration += 3;

    // STA ADDR_RESULT
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(ADDR_RESULT);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // initialize the arguments
    data[ADDR_FIRST]  = mtl::u8(0x44);
    data[ADDR_SECOND] = mtl::u8(0x29);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0x15);         // 44 - 29 = 15
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
TEST_F(Program, And) {
    constexpr uint8_t ADDR_RESULT = 0x00;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // LDA #1100X111; X is 0 or 1
    code.emplace_back(0xA9); // LDA immediate: 2 bytes, 2 cycles
    code.emplace_back(0b11001111);
    code_duration += 2;

    // AND #11110111
    code.emplace_back(0x29); // AND immediate: 2 bytes, 2 cycles
    code.emplace_back(0b11110111);
    code_duration += 2;

    // STA ADDR_RESULT
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(ADDR_RESULT);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0b11000111);   // 0b11001111 & 0b111101111 = 0b11000111
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
TEST_F(Program, Or) {
    constexpr uint8_t ADDR_RESULT = 0x00;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // LDA #1100X111; X is 0 or 1
    code.emplace_back(0xA9); // LDA immediate: 2 bytes, 2 cycles
    code.emplace_back(0b11100111);
    code_duration += 2;

    // ORA #00001000
    code.emplace_back(0x09); // ORA immediate: 2 bytes, 2 cycles
    code.emplace_back(0b00001000);
    code_duration += 2;

    // STA ADDR_RESULT
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(ADDR_RESULT);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0b11101111);   // 0b11100111 | 0b00001000 = 0b11101111
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
TEST_F(Program, Xor) {
    constexpr uint8_t ADDR_RESULT = 0x00;

    std::vector<mtl::u8> code;
    size_t code_duration = 0;

    // LDA #10101111
    code.emplace_back(0xA9); // LDA immediate: 2 bytes, 2 cycles
    code.emplace_back(0b10101111);
    code_duration += 2;

    // EOR #11111111
    code.emplace_back(0x49); // EOR immediate: 2 bytes, 2 cycles
    code.emplace_back(0b11111111);
    code_duration += 2;

    // STA ADDR_RESULT
    code.emplace_back(0x85); // STA zero page: 2 bytes, 3 cycles
    code.emplace_back(ADDR_RESULT);
    code_duration += 3;

    // insert the code to the memory
    auto [data, program_end] = assemble(code);

    // execute the program
    Clock clock(std::chrono::nanoseconds(0));
    Memory memory{ data };
    CPU cpu{};
    cpu.start(memory, clock);

    // check the results
    EXPECT_EQ(cpu.program_counter(), program_end);                // 1 for CLI and 1 for HLT
    EXPECT_EQ(clock.cycle(), code_duration + STARTUP_DURATION + 3); // 2 for CLI and 1 for HLT
    EXPECT_EQ(memory[mtl::u16(ADDR_RESULT)], 0b01010000);             // 0b10101111 ^ 0b11111111 = 0b01010000
}
} // namespace emulator::mos_6502::test