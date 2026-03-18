#pragma once
#include <cstdint>
#include <functional>

class Z80 {
public:
    // ================================
    // Constructor / Destructor
    // ================================
    Z80();
    ~Z80() = default;

    // ================================
    // Execution
    // ================================
    uint32_t ExecuteInstruction();
    void Reset();

    // ================================
    // Public Getters
    // ================================
    uint16_t GetPC() const { return PC; }
    uint16_t GetSP() const { return SP; }
    uint8_t  GetA()  const { return A; }

    // ================================
    // CALLBACK TYPES (std::function!)
    // ================================
    using ReadMemFunc  = std::function<uint8_t(uint16_t)>;
    using WriteMemFunc = std::function<void(uint16_t, uint8_t)>;

    using ReadIOFunc   = std::function<uint8_t(uint8_t)>;
    using WriteIOFunc  = std::function<void(uint8_t, uint8_t)>;

    // ================================
    // CALLBACK FIELDS (public)
    // ================================
    ReadMemFunc  readMemory;
    WriteMemFunc writeMemory;
    ReadIOFunc   readIO;
    WriteIOFunc  writeIO;

private:
    // ================================
    // Opcode execution
    // ================================
    uint32_t ExecuteMain(uint8_t opcode);
    uint32_t ExecuteCB(uint8_t opcode);
    uint32_t ExecuteED(uint8_t opcode);

    // Instruction helpers
    uint32_t DoLoadRegToReg(uint8_t opcode);
    uint32_t DoLoadRegImm(uint8_t opcode);
    uint32_t Do_RLC(uint8_t opcode);
    uint8_t& GetReg(uint8_t code);

    // ================================
    // Registers
    // ================================
    uint8_t A, F;
    uint8_t B, C;
    uint8_t D, E;
    uint8_t H, L;

    uint16_t PC;
    uint16_t SP;

    // ================================
    // Fetch & Stack
    // ================================
    uint8_t  FetchByte();
    uint16_t FetchWord();

    void PushWord(uint16_t value);
    uint16_t PopWord();

    // ================================
    // Flag helpers
    // ================================
    void SetZeroFlag(uint8_t value);
    void SetSignFlag(uint8_t value);
    void SetHalfCarryFlag(uint8_t result, uint8_t op1, uint8_t op2);
    void SetCarryFlag(uint16_t result);
    void SetParityFlag(uint8_t value);

    // ================================
    // Flag bits
    // ================================
    static constexpr uint8_t FLAG_C = 0x01;
    static constexpr uint8_t FLAG_N = 0x02;
    static constexpr uint8_t FLAG_P = 0x04;
    static constexpr uint8_t FLAG_X = 0x08;
    static constexpr uint8_t FLAG_H = 0x10;
    static constexpr uint8_t FLAG_Y = 0x20;
    static constexpr uint8_t FLAG_Z = 0x40;
    static constexpr uint8_t FLAG_S = 0x80;
};
