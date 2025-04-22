//
// Created by Mikhail Tsaritsyn on Apr 22, 2025.
//
#include "CPU.hpp"
#include "helpers.hpp"
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
     * It can be used in testing to terminate exactly after the provided code is executed.
     */
    static constexpr uint8_t HLT = 0x02;

    /**
     * @brief Number of cycles elapsing during the startup routine
     */
    static constexpr size_t STARTUP_LENGTH = 7;

    void SetUp() override {
        _code        = {};
        _code_length = 0;
    }

    void TearDown() override {
        Memory::Data data;
        data[PROGRAM_START] = 0x58; // CLI, to clear the interrupt disable flag set at startup
        std::ranges::copy(_code, data.begin() + PROGRAM_START + 1);

        data[PROGRAM_START + 1 + _code.size()] = HLT;

        data[CPU::RES]     = low_byte(PROGRAM_START);
        data[CPU::RES + 1] = high_byte(PROGRAM_START);

        CPU cpu(std::chrono::nanoseconds(0), Memory(data));
        cpu.start();

        EXPECT_EQ(cpu.program_counter(), PROGRAM_START + _code.size() + 2); // 1 for CLI and 1 for HLT
        EXPECT_EQ(cpu.cycle(), _code_length + STARTUP_LENGTH + 3);          // 2 for CLI and 1 for HLT
    }

    std::vector<uint8_t> _code;
    size_t _code_length = 0; ///< Number of cycles elapsed to execute the code
};

TEST_F(Program, Empty) {}

// https://web.archive.org/web/20221112220344if_/http://archive.6502.org/datasheets/synertek_programming_manual.pdf
// Example 2.3
// first number at {H1, L1}
// second number at {H2, L2}
// result written to {H3, L3}
// LDA L1
// CLC
// ADC L2
// STA L3
// LDA H1
// ADC H2
// STA H3
TEST_F(Program, Add16Bit) {
    constexpr uint8_t L1 = 0x00;
    constexpr uint8_t H1 = 0x01;
    constexpr uint8_t L2 = 0x02;
    constexpr uint8_t H2 = 0x03;
    constexpr uint8_t L3 = 0x04;
    constexpr uint8_t H3 = 0x05;

    // LDA L1
    _code.push_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    _code.push_back(L1);
    _code_length += 3;

    // CLC
    _code.push_back(0x18); // CLC: 1 byte, 2 cycles
    _code_length += 2;

    // ADC L2
    _code.push_back(0x65); // ADC zero page: 2 bytes, 3 cycles
    _code.push_back(L2);
    _code_length += 3;

    // STA L3
    _code.push_back(0x85); // STA zero page: 2 bytes, 3 cycles
    _code.push_back(L3);
    _code_length += 3;

    // LDA H1
    _code.push_back(0xA5); // LDA zero page: 2 bytes, 3 cycles
    _code.push_back(H1);
    _code_length += 3;

    // ADC H2
    _code.push_back(0x65); // ADC zero page: 2 bytes, 3 cycles
    _code.push_back(H2);
    _code_length += 3;

    // STA H3
    _code.push_back(0x85); // STA zero page: 2 bytes, 3 cycles
    _code.push_back(H3);
    _code_length += 3;
}
} // namespace emulator::mos_6502::test