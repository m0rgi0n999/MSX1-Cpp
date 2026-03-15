#pragma once
#include <cstdint>
#include <array>
#include <functional> // REQUIRED for std::function

class Z80 {
public:
    // Constructor / Destructor
    Z80();
    ~Z80() = default;

    // Main execution function.
    uint32_t ExecuteInstruction();

    // Reset the CPU to its initial state.
    void Reset();

    // Public accessors for registers
    uint16_t GetPC() const { return PC; }
    uint16_t GetSP() const { return SP; }
    uint8_t GetA() const { return A; }

    // Callback Types for MEMORY (RAM/ROM)
    using ReadMemFunc = std::function<uint8_t(uint16_t address)>;
    using WriteMemFunc = std::function<void(uint16_t address, uint8_t value)>;

    // Callback Types for I/O (Ports)
    using ReadIOFunc = std::function<uint8_t(uint8_t port)>;
    using WriteIOFunc = std::function<void(uint8_t port, uint8_t value)>;

    // Set Memory callbacks
    void SetMemoryCallbacks(ReadMemFunc read, WriteMemFunc write) {
        readMemory = read;
        writeMemory = write;
    }

    // Set I/O callbacks
    void SetIOCallbacks(ReadIOFunc read, WriteIOFunc write) {
        readIO = read;
        writeIO = write;
    }

private:
    // <--- DECLARE NEW FUNTIONS HERE ---
    uint32_t ExecuteMain(uint8_t opcode);
    uint32_t ExecuteCB(uint8_t opcode);
    uint32_t ExecuteED(uint8_t opcode);

    // Helper Declarations
    uint32_t DoLoadRegToReg(uint8_t opcode);
    uint32_t DoLoadRegImm(uint8_t opcode);
    uint8_t& GetReg(uint8_t code);
    uint32_t Do_RLC(uint8_t opcode);

    // 8-bit registers
    uint8_t A, F; 
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;

    // 16-bit registers
    uint16_t PC; 
    uint16_t SP; 

    // Function Pointers for Memory
    ReadMemFunc readMemory;
    WriteMemFunc writeMemory;

    // Function Pointers for I/O
    ReadIOFunc readIO;
    WriteIOFunc writeIO;

    // Helper functions for instruction execution
    uint8_t FetchByte();
    uint16_t FetchWord();
    void PushWord(uint16_t value);
    uint16_t PopWord();

    // Helpers for flag calculations
    void SetZeroFlag(uint8_t value);
    void SetSignFlag(uint8_t value);
    void SetHalfCarryFlag(uint8_t result, uint8_t op1, uint8_t op2);
    void SetCarryFlag(uint16_t result);
    void SetParityFlag(uint8_t value);

    // Flag constants
    static constexpr uint8_t FLAG_C = 0x01; 
    static constexpr uint8_t FLAG_N = 0x02; 
    static constexpr uint8_t FLAG_P = 0x04; 
    static constexpr uint8_t FLAG_X = 0x08; 
    static constexpr uint8_t FLAG_H = 0x10; 
    static constexpr uint8_t FLAG_Y = 0x20; 
    static constexpr uint8_t FLAG_Z = 0x40; 
    static constexpr uint8_t FLAG_S = 0x80; 
};

