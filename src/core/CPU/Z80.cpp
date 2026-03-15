// CORRECTED: Path is relative to the 'src' folder
#include "core/CPU/Z80.hpp"
#include <iostream> 

// Constructor
Z80::Z80() : A(0), F(0), B(0), C(0), D(0), E(0), H(0), L(0), PC(0), SP(0), 
             readMemory(nullptr), writeMemory(nullptr), 
             readIO(nullptr), writeIO(nullptr) { 
    PC = 0x0000;
}

// Reset
void Z80::Reset() {
    A = 0; F = 0; B = 0; C = 0; D = 0; E = 0; H = 0; L = 0;
    PC = 0x0000;
    SP = 0xFFFF;
}

// FetchByte
uint8_t Z80::FetchByte() {
    if (!readMemory) {
        std::cerr << "Error: readMemory callback is not set!" << std::endl;
        return 0;
    }
    uint8_t data = readMemory(PC);
    PC++;
    return data;
}

// FetchWord
uint16_t Z80::FetchWord() {
    uint8_t low = FetchByte();
    uint8_t high = FetchByte();
    return (high << 8) | low;
}

// PushWord
void Z80::PushWord(uint16_t value) {
    if (!writeMemory) {
        std::cerr << "Error: writeMemory callback is not set!" << std::endl;
        return;
    }
    SP--;
    writeMemory(SP, (value >> 8) & 0xFF); 
    SP--;
    writeMemory(SP, value & 0xFF);        
}

// PopWord
uint16_t Z80::PopWord() {
    if (!readMemory) {
        std::cerr << "Error: readMemory callback is not set!" << std::endl;
        return 0;
    }
    uint8_t low = readMemory(SP);
    SP++;
    uint8_t high = readMemory(SP);
    SP++;
    return (high << 8) | low;
}

// Major Opcode selector
uint32_t Z80::ExecuteInstruction() {
  // 1. Fetch the Byte
  uint8_t opcode = FetchByte();

  // 2. Check for PreFixes (State Machine)
  // If we see a prefix, we recall the loop to get the "real" opcode next time.
  if (opcode == 0xCB) {
    uint8_t cbOpcode = FetchByte();
    return ExecuteCB(cbOpcode); // Switch to CB Handler
  }
  else if (opcode == 0xED) {
    uint8_t edOpcode = FetchByte();
    return ExecuteED(edOpcode); // Switch to ED Handler
  }
  else if (opcode == 0xDD || opcode == 0xFD) {
    // DD/FD are tricky (they affect the "next" instruction's addressing)
    // Often handled by a state variable: prefixMode = opcode;
    // But dor now, keep it simple or ignore IX/IY opcodes.
    // uint8_t ddOpcode = FetchByte();
    // Logic for DD...
    return 0;
  }
  else {
    // 3. Main 256 opcodes
    return ExecuteMain(opcode);
  }
}

uint32_t Z80::ExecuteMain(uint8_t opcode) {

  // <--- ENTER PATTERN CHECKS HERE (The if statements) ---

  // Example: Check for "LD r, r' " (0x40 - 0x7F)
  // We check if the top bits are '01'
  if ((opcode & 0xC0) == 0x40) {
    return DoLoadRegToReg(opcode); // Call helper
  }

  // Example: Check for "LD r, n " (0x06 - 0x0E...)
  if ((opcode & 0xC7) == 0x06) {
    return DoLoadRegImm(opcode); // Call Helper
  }

  // <--- ENTER UNIQUE OPCODES HERE (The switch statements)
  switch (opcode) {
    case 0x00: return 4; // NOP
    //case 0x76: halted=true; PC--; return 4; // HALT
    case 0xC3: PC = FetchWord(); return 10; // JP nn
    // Add more simple cases here...
    // --- I/O INSTRUCTIONS ---

    // OUT (n), A (0xD3)
    case 0xD3: {
        // Open curly brace allows 'port' to exist safely here
        if (!writeIO) { return 11; }
        uint8_t port = FetchByte();
        writeIO(port, A);
        return 11;
    } // Close curly brace. Scope of 'port' ends here.

    // IN A, (n) (0xDB)
    case 0xDB: {
        // Open curly brace is REQUIRED here to prevent scope errors
        if (!readIO) { A = 0; return 11; }
        uint8_t port = FetchByte();
        A = readIO(port);
        return 11;
    } // Close curly brace
  }


  return 0; //Should never get here
}

/*
uint32_t Z80::ExecuteInstruction() {
    uint8_t opcode = FetchByte();

    // --- THE SWITCH LOOP I PREVIOUSLY FORGOT ---
    switch (opcode) {
    // -------------------------------------------

    // --- LOAD INSTRUCTIONS ---

    // LD A, n (0x3E) - Load immediate 8-bit value into A
    case 0x3E:
        A = FetchByte();
        return 8;

    // LD B, n (0x06)
    case 0x06:
        B = FetchByte();
        return 8;

    // LD C, n (0x0E)
    case 0x0E:
        C = FetchByte();
        return 8;

    // LD D, n (0x16)
    case 0x16:
        D = FetchByte();
        return 8;

    // LD E, n (0x1E)
    case 0x1E:
        E = FetchByte();
        return 8;

    // LD H, n (0x26)
    case 0x26:
        H = FetchByte();
        return 8;

    // LD L, n (0x2E)
    case 0x2E:
        L = FetchByte();
        return 8;

    // LD A, B (0x78)
    case 0x78:
        A = B;
        return 4;

    // LD A, C (0x79)
    case 0x79:
        A = C;
        return 4;

    // LD A, D (0x7A)
    case 0x7A:
        A = D;
        return 4;

    // LD A, E (0x7B)
    case 0x7B:
        A = E;
        return 4;

    // LD A, H (0x7C)
    case 0x7C:
        A = H;
        return 4;

    // LD A, L (0x7D)
    case 0x7D:
        A = L;
        return 4;

    // LD B, A (0x47)
    case 0x47:
        B = A;
        return 4;

    // LD HL, nn (0x21) - Load 16-bit address into HL pair
    case 0x21:
        L = FetchByte();
        H = FetchByte();
        return 10;

    // LD (HL), n (0x36) - Load immediate value into memory address pointed to by HL
    case 0x36:
        if (!writeMemory) { return 0; }
        writeMemory((H << 8) | L, FetchByte());
        return 10;

    // --- JUMP INSTRUCTIONS ---

    // JP nn (0xC3) - Unconditional jump to 16-bit immediate address
    case 0xC3:
        L = FetchByte();
        H = FetchByte();
        PC = (H << 8) | L;
        return 10;

    // JP NZ, nn (0xC2) - Jump if Zero Flag is NOT set
    case 0xC2: {
        uint16_t addr = FetchWord();
        if (!(F & FLAG_Z)) {
            PC = addr;
            return 10;
        }
        return 10;
    }

    // --- I/O INSTRUCTIONS ---

    // OUT (n), A (0xD3)
    case 0xD3: {
        // Open curly brace allows 'port' to exist safely here
        if (!writeIO) { return 11; }
        uint8_t port = FetchByte();
        writeIO(port, A);
        return 11;
    } // Close curly brace. Scope of 'port' ends here.

    // IN A, (n) (0xDB)
    case 0xDB: {
        // Open curly brace is REQUIRED here to prevent scope errors
        if (!readIO) { A = 0; return 11; }
        uint8_t port = FetchByte();
        A = readIO(port);
        return 11;
    } // Close curly brace


    // NOP (0x00) - No Operation (Keep this at the bottom or default)
    case 0x00:
        return 4;

    default:
        std::cerr << "Unimplemented opcode: 0x" << std::hex << static_cast<int>(opcode) << std::endl;
        return 0; // Stop execution
    } // END OF SWITCH
}
*/

// SetZeroFlag
void Z80::SetZeroFlag(uint8_t value) {
    F = (F & ~FLAG_Z) | ((value == 0) ? FLAG_Z : 0);
}

// SetSignFlag
void Z80::SetSignFlag(uint8_t value) {
    F = (F & ~FLAG_S) | ((value & 0x80) ? FLAG_S : 0);
}

// SetHalfCarryFlag
void Z80::SetHalfCarryFlag(uint8_t result, uint8_t op1, uint8_t op2) {
    (void)result; // Suppress unused parameter warning
    F = (F & ~FLAG_H) | ((((op1 & 0x0F) + (op2 & 0x0F)) & 0x10) ? FLAG_H : 0);
}

// SetCarryFlag
void Z80::SetCarryFlag(uint16_t result) {
    F = (F & ~FLAG_C) | ((result > 0xFF) ? FLAG_C : 0);
}

// SetParityFlag
void Z80::SetParityFlag(uint8_t value) {
    int count = 0;
    for (int i = 0; i < 8; i++) {
        if (value & (1 << i)) count++;
    }
    F = (F & ~FLAG_P) | ((count % 2 == 0) ? FLAG_P : 0);
}

// Helper 1: Register Selection
uint8_t& Z80::GetReg(uint8_t code) {
  // Z80 3-bit register code:
  // 000 = B, 001 = C, 010 = D, 011 = E
  // 100 = H, 101 = L. 110 = (HL) <-- Special Case!
  // 111 = A
  switch (code) {
    case 0: return B;
    case 1: return C;
    case 2: return D;
    case 3: return E;
    case 4: return H;
    case 5: return L;
    case 7: return A;
    default: return A; // Placeholder for (HL) case 6
  }
  return A;
}

// Helper 2: Logic for LD r, r'
uint32_t Z80::DoLoadRegToReg(uint8_t opcode) {
  uint8_t dest = (opcode >> 3) & 0x07;
  uint8_t src  = (opcode) & 0x07;

  // The actual work happens here
  GetReg(dest) = GetReg(src);

  return 4; // return cycles
}

// Helper 3: Logic for LD r, n
uint32_t Z80::DoLoadRegImm(uint8_t opcode) {
  uint8_t dest = (opcode >> 3) & 0x07;
  uint8_t value = FetchByte();
  if (dest == 6 ) {
    uint16_t address = (H << 8) | L;
    if (writeMemory) {
      writeMemory(address, value);
    }
  } else {
    GetReg(dest) = value;
  }
  return 7;
}

uint32_t Z80::Do_RLC(uint8_t opcode) {
  uint8_t regCode = opcode & 0x07;
  uint8_t& r = GetReg(regCode);

  // Save the old MSB (Bit 7)
  uint8_t oldBit7 = (r >> 7) & 1;

  // Shift Left, bring old MSB to LSB
  r = (r << 1) | oldBit7;
  
  // Flags: Carry = Old Bit 7
  if (oldBit7) F |= FLAG_C; else F &= ~FLAG_C;
  // Flags: HalfCarry = 0, Add/Sub = 0
  F &= ~(FLAG_H | FLAG_N);
  // Flags: Zero/Sign/Parity
  SetZeroFlag(r);
  SetSignFlag(r);
  SetParityFlag(r);

  return 0;
}

// --- STUB: CB PREFIX EXECUTOR ---
uint32_t Z80::ExecuteCB(uint8_t opcode) {
    // Pattern for RLC r is 00000rrr (0x00 to 0x07)
    if ((opcode & 0xF8) == 0x00) {
      return Do_RLC(opcode);
    }
    // TODO: Implement Bit Shifts, Rotates, and Bit Tests (RLC, RRC, SET, RES, etc.)
    std::cerr << "Unimplemented CB Opcode: 0x" << std::hex << static_cast<int>(opcode) << std::endl;

    // Return default 8 cycles (common for CB instructions)
    return 8;
}

// --- STUB: ED PREFIX EXECUTOR ---
uint32_t Z80::ExecuteED(uint8_t opcode) {
    // TODO: Implement Special Block Transfers, I/O, and Multi-byte math (LDI, INIR, etc.)
    std::cerr << "Unimplemented ED Opcode: 0x" << std::hex << static_cast<int>(opcode) << std::endl;

    // Return default cycles
    return 8;
}

