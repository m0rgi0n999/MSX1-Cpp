#include "core/CPU/Z80.hpp"
#include <iostream>

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
Z80::Z80()
    : A(0), F(0), B(0), C(0), D(0), E(0), H(0), L(0),
      PC(0), SP(0),
      readMemory(nullptr), writeMemory(nullptr),
      readIO(nullptr), writeIO(nullptr) 
{
    PC = 0x0000;
}

// -------------------------------------------------------------
// Reset CPU
// -------------------------------------------------------------
void Z80::Reset() {
    A = F = 0;
    B = C = 0;
    D = E = 0;
    H = L = 0;
    PC = 0x0000;
    SP = 0xFFFF;
}

// -------------------------------------------------------------
// Fetch Byte from memory
// -------------------------------------------------------------
uint8_t Z80::FetchByte() {
    if (!readMemory) {
        std::cerr << "Z80 Error: readMemory callback is null!\n";
        return 0xFF;
    }
    uint8_t data = readMemory(PC);
    PC++;
    return data;
}

// -------------------------------------------------------------
// Fetch Word (16-bit, little-endian)
// -------------------------------------------------------------
uint16_t Z80::FetchWord() {
    uint8_t lo = FetchByte();
    uint8_t hi = FetchByte();
    return (hi << 8) | lo;
}

// -------------------------------------------------------------
// PushWord (Stack is descending)
// -------------------------------------------------------------
void Z80::PushWord(uint16_t value) {
    if (!writeMemory) {
        std::cerr << "Z80 Error: writeMemory callback is null!\n";
        return;
    }
    SP--;
    writeMemory(SP, (value >> 8) & 0xFF);
    SP--;
    writeMemory(SP, value & 0xFF);
}

// -------------------------------------------------------------
// PopWord
// -------------------------------------------------------------
uint16_t Z80::PopWord() {
    if (!readMemory) {
        std::cerr << "Z80 Error: readMemory callback is null!\n";
        return 0;
    }
    uint8_t lo = readMemory(SP++);
    uint8_t hi = readMemory(SP++);
    return (hi << 8) | lo;
}

// -------------------------------------------------------------
// Main Instruction Decoder
// -------------------------------------------------------------
uint32_t Z80::ExecuteInstruction() {
    uint8_t opcode = FetchByte();

    // Prefix handling
    if (opcode == 0xCB)
        return ExecuteCB(FetchByte());
    if (opcode == 0xED)
        return ExecuteED(FetchByte());

    // DD/FD (IX/IY) not implemented yet
    if (opcode == 0xDD || opcode == 0xFD)
        return 0; 

    return ExecuteMain(opcode);
}

// -------------------------------------------------------------
// ExecuteMain: The "unprefixed" 00–FF block
// -------------------------------------------------------------
uint32_t Z80::ExecuteMain(uint8_t opcode) {

    // ---------------------------------------------------------
    // LD r,r'
    // ---------------------------------------------------------
    if ((opcode & 0xC0) == 0x40)
        return DoLoadRegToReg(opcode);

    // ---------------------------------------------------------
    // LD r,n  (immediate)
    // ---------------------------------------------------------
    if ((opcode & 0xC7) == 0x06)
        return DoLoadRegImm(opcode);

    // ---------------------------------------------------------
    // LD r,(HL)
    // ---------------------------------------------------------
    if ((opcode & 0xC7) == 0x46) {
        uint8_t reg = (opcode >> 3) & 0x7;
        uint16_t addr = (H << 8) | L;
        GetReg(reg) = readMemory(addr);
        return 7;
    }

    // ---------------------------------------------------------
    // LD (HL),r
    // ---------------------------------------------------------
    if ((opcode & 0xF8) == 0x70) {
        if (opcode == 0x76)  // HALT
            return 4;

        uint8_t src = opcode & 0x7;
        uint16_t addr = (H << 8) | L;
        writeMemory(addr, GetReg(src));
        return 7;
    }

    // ---------------------------------------------------------
    // INC r
    // ---------------------------------------------------------
    if ((opcode & 0xC7) == 0x04) {
        uint8_t reg = (opcode >> 3) & 0x07;
        uint8_t &r = GetReg(reg);

        uint8_t old = r;
        r++;

        SetZeroFlag(r);
        SetSignFlag(r);
        F &= ~FLAG_N;  
        SetHalfCarryFlag(r, old, 1);
        return (reg == 6 ? 11 : 4);
    }

    // ---------------------------------------------------------
    // DEC r
    // ---------------------------------------------------------
    if ((opcode & 0xC7) == 0x05) {
        uint8_t reg = (opcode >> 3) & 0x07;
        uint8_t &r = GetReg(reg);

        uint8_t old = r;
        r--;

        SetZeroFlag(r);
        SetSignFlag(r);
        F |= FLAG_N;
        F = (F & ~FLAG_H) | (((old & 0x0F) == 0) ? FLAG_H : 0);

        return (reg == 6 ? 11 : 4);
    }

    // ---------------------------------------------------------
    // UNIQUE OPCODES
    // ---------------------------------------------------------
    switch (opcode) {

        case 0x00: // NOP
            return 4;

        case 0xC3: // JP nn
            PC = FetchWord();
            return 10;

        case 0xCA: { // JP Z,nn
            uint16_t addr = FetchWord();
            if (F & FLAG_Z) PC = addr;
            return 10;
        }

        case 0xD2: { // JP NC,nn
            uint16_t addr = FetchWord();
            if (!(F & FLAG_C)) PC = addr;
            return 10;
        }

        case 0xDA: { // JP C,nn
            uint16_t addr = FetchWord();
            if (F & FLAG_C) PC = addr;
            return 10;
        }

        // -----------------------------------------------------
        // I/O operations
        // -----------------------------------------------------
        case 0xD3: { // OUT (n),A
            if (!writeIO) return 11;
            uint8_t port = FetchByte();
            writeIO(port, A);
            return 11;
        }

        case 0xDB: { // IN A,(n)
            uint8_t port = FetchByte();
            A = readIO ? readIO(port) : 0xFF;
            return 11;
        }
    }

    std::cerr << "Unimplemented opcode: 0x" 
              << std::hex << int(opcode) << "\n";
    return 0;
}

// -------------------------------------------------------------
// CB Prefix
// -------------------------------------------------------------
uint32_t Z80::ExecuteCB(uint8_t opcode) {
    // RLC r = CB 00–07
    if ((opcode & 0xF8) == 0x00)
        return Do_RLC(opcode);

    std::cerr << "Unimplemented CB opcode: 0x"
              << std::hex << int(opcode) << "\n";
    return 8;
}

// -------------------------------------------------------------
// ED Prefix
// -------------------------------------------------------------
uint32_t Z80::ExecuteED(uint8_t opcode) {
    std::cerr << "Unimplemented ED opcode: 0x"
              << std::hex << int(opcode) << "\n";
    return 8;
}

// -------------------------------------------------------------
// DoLoadRegToReg
// -------------------------------------------------------------
uint32_t Z80::DoLoadRegToReg(uint8_t opcode) {
    uint8_t dest = (opcode >> 3) & 0x07;
    uint8_t src  = opcode & 0x07;
    GetReg(dest) = GetReg(src);
    return 4;
}

// -------------------------------------------------------------
// DoLoadRegImm
// -------------------------------------------------------------
uint32_t Z80::DoLoadRegImm(uint8_t opcode) {
    uint8_t reg = (opcode >> 3) & 0x07;
    uint8_t value = FetchByte();

    if (reg == 6) { // (HL)
        uint16_t addr = (H << 8) | L;
        if (writeMemory) writeMemory(addr, value);
    } else {
        GetReg(reg) = value;
    }
    return 7;
}

// -------------------------------------------------------------
// Do_RLC
// -------------------------------------------------------------
uint32_t Z80::Do_RLC(uint8_t opcode) {
    uint8_t reg = opcode & 0x07;
    uint8_t &r = GetReg(reg);

    uint8_t oldBit7 = (r >> 7) & 1;
    r = (r << 1) | oldBit7;

    // Flags
    F = (F & ~(FLAG_N | FLAG_H));
    if (oldBit7) F |= FLAG_C; else F &= ~FLAG_C;

    SetZeroFlag(r);
    SetSignFlag(r);
    SetParityFlag(r);

    return 8;
}

// -------------------------------------------------------------
// Flag helpers
// -------------------------------------------------------------
void Z80::SetZeroFlag(uint8_t v) {
    if (v == 0) F |= FLAG_Z;
    else F &= ~FLAG_Z;
}

void Z80::SetSignFlag(uint8_t v) {
    if (v & 0x80) F |= FLAG_S;
    else F &= ~FLAG_S;
}

void Z80::SetHalfCarryFlag(uint8_t, uint8_t op1, uint8_t op2) {
    if (((op1 & 0xF) + (op2 & 0xF)) & 0x10) F |= FLAG_H;
    else F &= ~FLAG_H;
}

