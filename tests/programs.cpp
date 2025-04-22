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
        EXPECT_EQ(cpu.cycle(), _code_length + STARTUP_LENGTH + 2);          // 1 for CLI and 1 for HLT
    }

    std::vector<uint8_t> _code;
    size_t _code_length = 0; ///< Number of cycles elapsed to execute the code
};

TEST_F(Program, Empty) {}
} // namespace emulator::mos_6502::test