#pragma once
#ifndef Z80_HPP
#define Z80_HPP

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

// 1. ADD THIS FORWARD DECLARATION HERE
class Bus;

// 2. DEFINE TYPES FIRST
using ReadMemFunc  = std::function<uint8_t(uint16_t)>;
using WriteMemFunc = std::function<void(uint16_t, uint8_t)>;
using ReadIOFunc   = std::function<uint8_t(uint8_t)>;
using WriteIOFunc  = std::function<void(uint8_t, uint8_t)>;

class Z80 {
public:
    // 2. NOW THESE TPES ARE VALID
    ReadMemFunc  readMemory;
    WriteMemFunc writeMemory;
    ReadIOFunc   readIO;
    WriteIOFunc  writeIO;

    Z80(Bus& busRef);
    virtual ~Z80() = default;

    // ================================
    // Execution
    // ================================
    void Reset();
    uint32_t ExecuteInstruction();
    void HandleInterrupt();

    // ================================
    // Public Getters
    // ================================
    uint16_t GetPC() const { return PC; }
    uint16_t GetSP() const { return SP; }
    uint8_t  GetA()  const { return A; }

    // --- Shadow Registers (Alternate Set) ---
    uint8_t A_shadow, F_shadow;
    uint8_t B_shadow, C_shadow;
    uint8_t D_shadow, E_shadow;
    uint8_t H_shadow, L_shadow;    

    bool traceOpcodes = false;
    bool traceUnimplementedOpcodes = false;
    uint32_t executedInstructionCount = 0;
    uint32_t unimplementedInstructionCount = 0;
    uint32_t unimplementedCBCount = 0;
    uint32_t unimplementedEDCount = 0;

    // IFF
    static const uint16_t INT_VECTOR = 0x0038;

    void DumpOpcodeStats() const;

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

    // Member functions for reading/writing
    uint8_t CPURead(uint16_t addr);
    void CPUWrite(uint16_t addr, uint8_t val);

private:
    // 3. REORDER THESE TO MATCH YOUR CONSTRUCTOR TO FIX -Wreorder
    Bus& bus;
    uint8_t A, F, B, C, D, E, H, L;
    uint16_t PC, SP, IX, IY;
    uint8_t I, R;
    bool halted;
    int interruptMode;
    bool IFF1, IFF2;

    // ================================
    // Opcode execution
    // ================================
    uint32_t ExecuteMain(uint8_t opcode, bool useIX, bool useIY);
    uint32_t ExecuteCB(uint8_t opcode);
    uint32_t ExecuteED(uint8_t opcode);
    uint32_t ExecuteCBIndexed(uint16_t index, int8_t d, uint8_t cbOpcode);

    // Instruction helpers
    uint32_t DoLoadRegToReg(uint8_t opcode);
    uint32_t DoLoadRegImm(uint8_t opcode);
    uint32_t Do_RLC(uint8_t opcode);
    uint8_t& GetReg(uint8_t code);

    uint16_t HLAddress() const;
    uint16_t DEAddress() const;
    uint8_t ReadHL();
    void WriteHL(uint8_t code);
    void WriteDE(uint8_t code);

    void LogOpcode(uint8_t opcode, bool implemented, const char* prefix = nullptr);

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
};

inline constexpr uint8_t FLAG_C = Z80::FLAG_C;
inline constexpr uint8_t FLAG_N = Z80::FLAG_N;
inline constexpr uint8_t FLAG_P = Z80::FLAG_P;
inline constexpr uint8_t FLAG_X = Z80::FLAG_X;
inline constexpr uint8_t FLAG_H = Z80::FLAG_H;
inline constexpr uint8_t FLAG_Y = Z80::FLAG_Y;
inline constexpr uint8_t FLAG_Z = Z80::FLAG_Z;
inline constexpr uint8_t FLAG_S = Z80::FLAG_S;


#endif
