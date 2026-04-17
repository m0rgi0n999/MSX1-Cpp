#include "core/CPU/Z80.hpp"
#include "core/Memory/Bus.hpp"
#include <iostream>

// -------------------------------------------------------------
// Constructor
// -------------------------------------------------------------
Z80::Z80(Bus& busRef)
    : readMemory(nullptr),
      writeMemory(nullptr),
      readIO(nullptr),
      writeIO(nullptr),
      bus(busRef),
      A(0), F(0), B(0), C(0), D(0), E(0), H(0), L(0),
      PC(0), SP(0), IX(0), IY(0),
      halted(false),
      interruptMode(0),
      IFF1(false), IFF2(false)
{
}

uint8_t Z80::CPURead(uint16_t addr) {
  return bus.Read(addr);
}

// -------------------------------------------------------------
// Reset CPU
// -------------------------------------------------------------
void Z80::Reset() {
    A = F = 0;
    B = C = 0;
    D = E = 0;
    H = L = 0;
    IX = IY = 0;
    PC = 0x0000;
    SP = 0xFFFF;
    IFF1 = IFF2 = false;

    executedInstructionCount = 0;
    unimplementedInstructionCount = 0;
    unimplementedCBCount = 0;
    unimplementedEDCount = 0;
}

// -------------------------------------------------------------
// Fetch Byte from memory
// -------------------------------------------------------------
uint8_t Z80::FetchByte() {
  return CPURead(PC++);
//    if (!readMemory) {
//        std::cerr << "Z80 Error: readMemory callback is null!\n";
//        return 0xFF;
//    }
//    uint8_t data = readMemory(PC);
//    PC++;
//    return data;
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
    executedInstructionCount++;
    uint8_t opcode = FetchByte();

    bool useIX = false;
    bool useIY = false;

    if (opcode == 0xDD) {
        useIX = true;
        opcode = FetchByte();
    } else if (opcode == 0xFD) {
        useIY = true;
        opcode = FetchByte();
    }

    if (opcode == 0xCB) {
        if (useIX || useIY) {
            int8_t d = static_cast<int8_t>(FetchByte());
            uint8_t cbOpcode = FetchByte();
            return ExecuteCBIndexed(useIX ? IX : IY, d, cbOpcode);
        } else {
            return ExecuteCB(FetchByte());
        }
    }

    if (opcode == 0xED) {
        return ExecuteED(FetchByte());
    }

    if (opcode == 0xFF) {
      std::cout << "CRITICAL: CPU is executing Open Bus (0xFF) at PC: " << std::hex << PC << std::endl;
    }

    return ExecuteMain(opcode, useIX, useIY);
}

// -------------------------------------------------------------
// ExecuteMain: The "unprefixed" 00–FF block
// -------------------------------------------------------------
uint32_t Z80::ExecuteMain(uint8_t opcode, bool useIX, bool useIY) {
    auto ok = [&](uint32_t cycles) -> uint32_t {
        LogOpcode(opcode, true);
        return cycles;
    };

    if (opcode == 0x76) // HALT
        return ok(4);

    // ---------------------------------------------------------
    // LD r,r'
    // ---------------------------------------------------------
    if ((opcode & 0xC0) == 0x40)
        return ok(DoLoadRegToReg(opcode));

    // ---------------------------------------------------------
    // LD r,n  (immediate)
    // ---------------------------------------------------------
    if ((opcode & 0xC7) == 0x06)
        return ok(DoLoadRegImm(opcode));

    // ---------------------------------------------------------
    // LD r,(HL)
    // ---------------------------------------------------------
    if ((opcode & 0xC7) == 0x46) {
        uint8_t reg = (opcode >> 3) & 0x7;
        if (useIX || useIY) {
            int8_t d = FetchByte();
            uint16_t addr = (useIX ? IX : IY) + d;
            GetReg(reg) = readMemory(addr);
            return ok(19);
        } else {
            uint16_t addr = HLAddress();
            GetReg(reg) = readMemory(addr);
            return ok(7);
        }
    }

    // ---------------------------------------------------------
    // LD (HL),r
    // ---------------------------------------------------------
    if ((opcode & 0xF8) == 0x70) {
        if (opcode == 0x76)  // HALT
            return 4;

        uint8_t src = opcode & 0x7;
        if (useIX || useIY) {
            int8_t d = FetchByte();
            uint16_t addr = (useIX ? IX : IY) + d;
            writeMemory(addr, GetReg(src));
            return ok(19);
        } else {
            uint16_t addr = HLAddress();
            writeMemory(addr, GetReg(src));
            return ok(7);
        }
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

        case 0x37: { // SCF (Set Carry Flag)
            // C is set, H and N are cleared
            // S, Z, P/V are unaffected (though some documentation suggests H/N behaviour,
            // the standard Z80 behaviour is H=0, N=0, C=1)
            F |= FLAG_C;
            F &= ~FLAG_H;
            F &= ~FLAG_N;

            LogOpcode(opcode, true, "SCF");
            return ok(4);
        }
        case 0x3F: { // CCF (Complenment Carry Flag)
            // H takes previous C, N is cleared, C is inverted
            if (F & FLAG_C) {
              F |= FLAG_H;
              F &= ~FLAG_C;
            } else {
              F &= ~FLAG_H;
              F |= FLAG_C;
            }
            F &= ~FLAG_N;

            LogOpcode(opcode, true, "CCF");
            return ok(4);
        }
        case 0xF9: { //LD SP, HL
            SP = HLAddress();
            LogOpcode(opcode, true, "LD SP, HL");
            return ok(6);
        }
        case 0x22: { // LD (nn), HL
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            writeMemory(addr, L);
            writeMemory(addr + 1, H);
            LogOpcode(opcode, true, "LD (nn), HL");
            return 16;
        }
        case 0xFA: { // JP M, nn
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            if (F & FLAG_S) {
                PC = addr;
            }
            LogOpcode(opcode, true, "JP M, nn");
            return 10;
        }
        case 0xC2: { // JP NZ, nn
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            if (!(F & FLAG_Z)) {
                PC = addr;
            }
            LogOpcode(opcode, true, "JP NZ, nn");
            return 10;
        }
        case 0x1A: { // LD A, (DE)
            A = readMemory(DEAddress());
            return ok(7); // Takes 7 T-states
        }
        case 0x12: { // LD (DE), A
            writeMemory(DEAddress(), A);
            return ok(7);
        }
        case 0x00: // NOP
            return ok(4);

        case 0xC3: // JP nn
            PC = FetchWord();
            return ok(10);

        case 0xCA: { // JP Z,nn
            uint16_t addr = FetchWord();
            if (F & FLAG_Z) PC = addr;
            return ok(10);
        }

        case 0xD2: { // JP NC,nn
            uint16_t addr = FetchWord();
            if (!(F & FLAG_C)) PC = addr;
            return ok(10);
        }

        case 0xDA: { // JP C,nn
            uint16_t addr = FetchWord();
            if (F & FLAG_C) PC = addr;
            return ok(10);
        }

        // -----------------------------------------------------
        // I/O operations
        // -----------------------------------------------------
        case 0xD3: { // OUT (n),A
            if (!writeIO) return ok(11);
            uint8_t port = FetchByte();
            writeIO(port, A);
            return ok(11);
        }

        case 0xDB: { // IN A,(n)
            uint8_t port = FetchByte();
            A = readIO ? readIO(port) : 0xFF;
            return ok(11);
        }

        case 0x01: { // LD BC,nn
            uint16_t value = FetchWord();
            C = value & 0xFF;
            B = value >> 8;
            return ok(10);
        }

        case 0x11: { // LD DE,nn
            uint16_t value = FetchWord();
            E = value & 0xFF;
            D = value >> 8;
            return ok(10);
        }

        case 0x21: { // LD HL/IX/IY,nn
            uint16_t value = FetchWord();
            if (useIX) {
                IX = value;
                return ok(14);
            } else if (useIY) {
                IY = value;
                return ok(14);
            } else {
                L = value & 0xFF;
                H = value >> 8;
                return ok(10);
            }
        }

        case 0x31: { // LD SP,nn
            SP = FetchWord();
            return ok(10);
        }

        case 0xF3: { // DI
            IFF1 = IFF2 = false;
            return 4;
        }

        case 0x09: // ADD HL/IX/IY,BC
        case 0x19: // ADD HL/IX/IY,DE
        case 0x29: // ADD HL/IX/IY,HL/IX/IY
        case 0x39: { // ADD HL/IX/IY,SP
            uint16_t oldVal = useIX ? IX : useIY ? IY : HLAddress();
            uint16_t operand = 0;

            if (opcode == 0x09) operand = (B << 8) | C;
            else if (opcode == 0x19) operand = (D << 8) | E;
            else if (opcode == 0x29) operand = useIX ? IX : useIY ? IY : HLAddress();
            else operand = SP;
            
            uint32_t result = uint32_t(oldVal) + uint32_t(operand);
            uint32_t newVal = result & 0xFFFF;

            if (useIX) IX = newVal;
            else if (useIY) IY = newVal;
            else {
                H = (newVal >> 8) & 0xFF;
                L = newVal & 0xFF;
            }

            F &= ~(FLAG_N | FLAG_H | FLAG_C);
            if (((oldVal & 0x0FFF) + (operand & 0x0FFF)) > 0x0FFF) F |= FLAG_H;
            if (result > 0xFFFF) F |= FLAG_C;

            return ok(useIX || useIY ? 15 : 11);
        }

        case 0xD9: { // EXX
            std::swap(B, B_shadow); // You will need to add B_shadow, etc. to Z80.hpp
            std::swap(C, C_shadow);
            std::swap(D, D_shadow);
            std::swap(E, E_shadow);
            std::swap(H, H_shadow);
            std::swap(L, L_shadow);
            return 4;
        }

        case 0xC7: case 0xCF: case 0xD7: case 0xDF: case 0xE7: case 0xEF: case 0xF7: case 0xFF: {
            // RST n
            uint16_t addr = opcode & 0x38; // n*8
            PushWord(PC);
            PC = addr;
            return ok(11);
        }

        case 0xCD: { // CALL nn
            uint16_t addr = FetchWord();
            PushWord(PC);
            PC = addr;
            return ok(17);
        }

        case 0xC4: case 0xCC: case 0xD4: case 0xDC: case 0xE4: case 0xEC: case 0xF4: case 0xFC: {
            // CALL cc,nn
            uint16_t addr = FetchWord();
            bool condition = false;
            switch (opcode & 0x38) {
                case 0x00: condition = !(F & FLAG_Z); break; // NZ
                case 0x08: condition = (F & FLAG_Z); break;  // Z
                case 0x10: condition = !(F & FLAG_C); break; // NC
                case 0x18: condition = (F & FLAG_C); break;  // C
                case 0x20: condition = !(F & FLAG_P); break; // PO
                case 0x28: condition = (F & FLAG_P); break;  // PE
                case 0x30: condition = !(F & FLAG_S); break; // P
                case 0x38: condition = (F & FLAG_S); break;  // M
            }
            if (condition) {
                PushWord(PC);
                PC = addr;
                return ok(17);
            } else {
                return ok(10);
            }
        }

        case 0xC5: // PUSH BC
            PushWord((B << 8) | C);
            return ok(11);

        case 0xD5: // PUSH DE
            PushWord((D << 8) | E);
            return ok(11);

        case 0xE5: // PUSH HL
            PushWord((H << 8) | L);
            return ok(11);

        case 0xF5: // PUSH AF
            PushWord((A << 8) | F);
            return ok(11);

        case 0xC1: { // POP BC
            uint16_t value = PopWord();
            C = value & 0xFF;
            B = value >> 8;
            return ok(10);
        }

        case 0xD1: { // POP DE
            uint16_t value = PopWord();
            E = value & 0xFF;
            D = value >> 8;
            return ok(10);
        }

        case 0xE1: { // POP HL
            uint16_t value = PopWord();
            L = value & 0xFF;
            H = value >> 8;
            return ok(10);
        }

        case 0xF1: { // POP AF
            uint16_t value = PopWord();
            F = value & 0xFF;
            A = value >> 8;
            return ok(10);
        }

        case 0xF6: { // OR n
            uint8_t value = FetchByte();
            A |= value;

            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);
            F &= ~FLAG_N;
            F &= ~FLAG_H;
            F &= ~FLAG_C;

            return ok(7);
        }

        case 0x08: { // EX AF, AF'
            std::swap(A, A_shadow);
            std::swap(F, F_shadow);
            return ok(4);
        }
        
        case 0x3A: { // LD A, (nn)
            uint16_t addr = FetchWord();
            A = readMemory(addr);
            return ok(13);
        }
        
        case 0x32: { // LD (nn), A  <-- Good to add this too if missing!
            uint16_t addr = FetchWord();
            writeMemory(addr, A);
            return ok(13);
        }
        
        case 0xC9: // RET
            PC = PopWord();
            return ok(10);

        case 0xC0: case 0xD0: case 0xE0: case 0xF0:
        case 0xC8: case 0xD8: case 0xE8: case 0xF8: {
            bool take = false;
            switch (opcode & 0x38) {
                case 0x00: take = !(F & FLAG_Z); break; // NZ
                case 0x08: take =  (F & FLAG_Z); break; // Z
                case 0x10: take = !(F & FLAG_C); break; // NC
                case 0x18: take =  (F & FLAG_C); break; // C
                case 0x20: take = !(F & FLAG_P); break; // PO
                case 0x28: take =  (F & FLAG_P); break; // PE
                case 0x30: take = !(F & FLAG_S); break; // P
                case 0x38: take =  (F & FLAG_S); break; // M
            }
            if (take) {
                PC = PopWord();
                return ok(11);
            }
            return ok(5);
        }

        case 0x10: { // DJNZ e
            int8_t offset = static_cast<int8_t>(FetchByte());
            B--;
            if (B != 0) {
                PC += offset;
                return ok(13);
            }
            return ok(8);
        }

        case 0x18: { // JR e
            int8_t offset = (int8_t)FetchByte(); // Must be signed!
            PC += offset;
            return ok(12);
        }

        case 0x20: // JR NZ,e
        case 0x28: // JR Z,e
        case 0x30: // JR NC,e
        case 0x38: { // JR C,e
            int8_t offset = static_cast<int8_t>(FetchByte());
            bool take = false;
            switch (opcode & 0x38) {
                case 0x20: take = !(F & FLAG_Z); break;
                case 0x28: take =  (F & FLAG_Z); break;
                case 0x30: take = !(F & FLAG_C); break;
                case 0x38: take =  (F & FLAG_C); break;
            }
            if (take) {
                PC += offset;
                return ok(12);
            }
            return ok(7);
        }

        case 0x80: case 0x81: case 0x82: case 0x83:
        case 0x84: case 0x85: case 0x86: case 0x87: {
            uint8_t src = opcode & 0x07;
            uint8_t operand;
            uint32_t cycles;
            if (useIX || useIY) {
                int8_t d = FetchByte();
                operand = readMemory((useIX ? IX : IY) + d);
                cycles = 19;
            } else {
                operand = (src == 6 ? ReadHL() : GetReg(src));
                cycles = 4 + (src == 6 ? 3 : 0);
            }
            uint16_t result = uint16_t(A) + uint16_t(operand);

            A = uint8_t(result);

            F &= ~(FLAG_N | FLAG_H | FLAG_C | FLAG_Z | FLAG_S | FLAG_P);
            if (((A & 0x0F) + (operand & 0x0F)) & 0x10) F |= FLAG_H;
            if (result & 0x100) F |= FLAG_C;
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);

            return ok(cycles);
        }

        case 0x88: case 0x89: case 0x8A: case 0x8B: case 0x8C: case 0x8D: case 0x8E: case 0x8F: {
            uint8_t src = opcode & 0x07;
            uint8_t operand = (src == 6 ? ReadHL() : GetReg(src));
            uint16_t carry = (F & FLAG_C) ? 1 : 0;
            uint16_t result = uint16_t(A) + uint16_t(operand) + carry;

            A = uint8_t(result);

            F &= ~(FLAG_N | FLAG_H | FLAG_C | FLAG_Z | FLAG_S | FLAG_P);
            if (((A & 0x0F) + (operand & 0x0F) + carry) & 0x10) F |= FLAG_H;
            if (result & 0x100) F |= FLAG_C;
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);

            return ok(4 + (src == 6 ? 3 : 0));
        }
        case 0x90: case 0x91: case 0x92: case 0x93:
        case 0x94: case 0x95: case 0x96: case 0x97: { // SUB r
            uint8_t src = opcode & 0x07;
            uint8_t operand;
            uint32_t cycles;
        
            if (useIX || useIY) {
                int8_t d = FetchByte();
                operand = readMemory((useIX ? IX : IY) + d);
                cycles = 19;
            } else {
                operand = (src == 6 ? ReadHL() : GetReg(src));
                cycles = 4 + (src == 6 ? 3 : 0);
            }
        
            uint16_t res16 = (uint16_t)A - (uint16_t)operand;
            uint8_t result = (uint8_t)res16;
        
            // Half Carry: borrow from bit 4
            bool h = ((int32_t)(A & 0x0F) - (int32_t)(operand & 0x0F)) < 0;
            // Overflow (V): signs of A and operand were different, 
            // and sign of result is different from A
            bool v = ((A ^ operand) & 0x80) && ((A ^ result) & 0x80);
        
            A = result;
        
            F = FLAG_N; // Set N for subtraction
            if (res16 & 0x100) F |= FLAG_C;
            if (h) F |= FLAG_H;
            if (v) F |= FLAG_P; // Overflow bit
            
            SetZeroFlag(A);
            SetSignFlag(A);
        
            return ok(cycles);
        }
    
        case 0x98: case 0x99: case 0x9A: case 0x9B:
        case 0x9C: case 0x9D: case 0x9E: case 0x9F: { // SBC A, r
            uint8_t src = opcode & 0x07;
            uint8_t operand = (src == 6 ? ReadHL() : GetReg(src));
            uint8_t carry = (F & FLAG_C) ? 1 : 0;
            
            // Use 16-bit to detect borrow (Carry)
            uint16_t res16 = (uint16_t)A - (uint16_t)operand - (uint16_t)carry;
            uint8_t result = (uint8_t)res16;
        
            // Half Carry logic: check borrow from bit 4
            // (A & 0x0F) - (operand & 0x0F) - carry < 0
            bool h = ((int32_t)(A & 0x0F) - (int32_t)(operand & 0x0F) - (int32_t)carry) < 0;
        
            // Overflow logic (V): different signs result in unexpected sign
            // Only happens if A and operand had different signs, and result has different sign than A
            bool v = ((A ^ operand) & 0x80) && ((A ^ result) & 0x80);
        
            A = result;
        
            // Update Flags
            F = FLAG_N; // Always set for subtraction
            if (res16 & 0x100) F |= FLAG_C; // Borrow occurred
            if (h) F |= FLAG_H;
            if (v) F |= FLAG_P; // In Z80, V and P share the same bit
            
            SetZeroFlag(A);
            SetSignFlag(A);
            // Note: Do not use SetParityFlag here; SBC uses Overflow logic
        
            return ok(4 + (src == 6 ? 3 : 0));
        }

        case 0xA0: case 0xA1: case 0xA2: case 0xA3:
        case 0xA4: case 0xA5: case 0xA6: case 0xA7: {
            uint8_t src = opcode & 0x07;
            uint8_t operand = (src == 6 ? ReadHL() : GetReg(src));

            A &= operand;

            F = FLAG_H;
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);

            return ok(4 + (src == 6 ? 3 : 0));
        }

        case 0xA8: case 0xA9: case 0xAA: case 0xAB:
        case 0xAC: case 0xAD: case 0xAE: case 0xAF: {
            uint8_t src = opcode & 0x07;
            uint8_t operand = (src == 6 ? ReadHL() : GetReg(src));

            A ^= operand;
            F = 0;
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);

            return ok(4 + (src == 6 ? 3 : 0));
        }

        case 0xB0: case 0xB1: case 0xB2: case 0xB3:
        case 0xB4: case 0xB5: case 0xB6: case 0xB7: {
            uint8_t src = opcode & 0x07;
            uint8_t operand = (src == 6 ? ReadHL() : GetReg(src));

            A |= operand;
            F = FLAG_H;
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);

            return ok(4 + (src == 6 ? 3 : 0));
        }

        case 0xB8: case 0xB9: case 0xBA: case 0xBB:
        case 0xBC: case 0xBD: case 0xBE: case 0xBF: {
            uint8_t src = opcode & 0x07;
            uint8_t operand = (src == 6 ? ReadHL() : GetReg(src));
            uint8_t result = A - operand;

            F = FLAG_N;
            if (A < operand) F |= FLAG_C;
            if ((A & 0x0F) < (operand & 0x0F)) F |= FLAG_H;
            SetZeroFlag(result);
            SetSignFlag(result);
            SetParityFlag(result);

            return ok(4 + (src == 6 ? 3 : 0));
        }

        case 0x07: { // RLCA
            uint8_t bit7 = A >> 7;
            A = (A << 1) | bit7;
            F &= ~(FLAG_N | FLAG_H | FLAG_C);
            if (bit7) F |= FLAG_C;
            return ok(4);
        }

        case 0x0F: { // RRCA
            uint8_t bit0 = A & 1;
            A = (A >> 1) | (bit0 << 7);
            F &= ~(FLAG_N | FLAG_H | FLAG_C);
            if (bit0) F |= FLAG_C;
            return ok(4);
        }

        case 0x17: { // RLA
            uint8_t bit7 = A >> 7;
            A = (A << 1) | ((F & FLAG_C) ? 1 : 0);
            F &= ~(FLAG_N | FLAG_H | FLAG_C);
            if (bit7) F |= FLAG_C;
            return ok(4);
        }

        case 0x1F: { // RRA
            uint8_t bit0 = A & 1;
            A = (A >> 1) | (((F & FLAG_C) ? 1 : 0) << 7);
            F &= ~(FLAG_N | FLAG_H | FLAG_C);
            if (bit0) F |= FLAG_C;
            return ok(4);
        }

        case 0x27: { // DAA
            uint8_t adjustment = 0;
            if (!(F & FLAG_N)) { // after add
                if ((A & 0x0F) > 9 || (F & FLAG_H)) adjustment |= 0x06;
                if (A > 0x99 || (F & FLAG_C)) {
                    adjustment |= 0x60;
                    F |= FLAG_C;
                } else {
                    F &= ~FLAG_C;
                }
                A += adjustment;
            } else { // after sub
                if ((F & FLAG_H) || (A & 0x0F) > 9) adjustment |= 0x06;
                if ((F & FLAG_C) || A > 0x99) {
                    adjustment |= 0x60;
                }
                A -= adjustment;
                // C remains
            }
            F &= ~(FLAG_H);
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);
            return ok(4);
        }

        case 0x2F: { // CPL
            A = ~A;
            F |= FLAG_N | FLAG_H;
            return ok(4);
        }

        case 0xE6: { // AND n
            A &= FetchByte();
            F = FLAG_H;
            SetZeroFlag(A); SetSignFlag(A); SetParityFlag(A);
            return 7;
        }

        case 0xEE: { // XOR n
            A ^= FetchByte();
            F = 0;
            SetZeroFlag(A); SetSignFlag(A); SetParityFlag(A);
            return 7;
        }

        case 0xEB: { // EX DE, HL
            uint8_t tempH = H; uint8_t tempL = L;
            H = D; L = E;
            D = tempH; E = tempL;
            return 4;
        }
        case 0xFE: { // CP n (Immediate)
            uint8_t operand = FetchByte();
            uint8_t result = A - operand;
        
            F = FLAG_N;
            if (A < operand) F |= FLAG_C;
            if ((A & 0x0F) < (operand & 0x0F)) F |= FLAG_H;
            
            // Overflow bit (V)
            if (((A ^ operand) & 0x80) && ((A ^ result) & 0x80)) F |= FLAG_P;
        
            SetZeroFlag(result);
            SetSignFlag(result);
            return 7;
        }

        case 0x0B: { // DEC BC
            uint16_t bc = (uint16_t)B << 8 | C;
            bc--;
            B = (bc >> 8) & 0xFF;
            C = bc & 0xFF;
            return 6;
        }

        case 0x23: { // INC HL
            uint16_t hl = (uint16_t)H << 8 | L;
            hl++;
            H = (hl >> 8) & 0xFF;
            L = hl & 0xFF;
            return ok(6); // Takes 6 T-states
        }
        
        case 0x03: { // INC BC
            uint16_t bc = (uint16_t)B << 8 | C;
            bc++;
            B = (bc >> 8) & 0xFF;
            C = bc & 0xFF;
            return ok(6);
        }
        
        case 0x13: { // INC DE
            uint16_t de = (uint16_t)D << 8 | E;
            de++;
            D = (de >> 8) & 0xFF;
            E = de & 0xFF;
            return ok(6);
        }
        
        case 0x33: { // INC SP
            SP++;
            return ok(6);
        }

        case 0x1B: { // DEC DE
            uint16_t de = (uint16_t)D << 8 | E;
            de--;
            D = (de >> 8) & 0xFF;
            E = de & 0xFF;
            return ok(6);
        }
        
        case 0x2B: { // DEC HL
            uint16_t hl = (uint16_t)H << 8 | L;
            hl--;
            H = (hl >> 8) & 0xFF;
            L = hl & 0xFF;
            return ok(6);
        }

        case 0x2A: { // LD HL, (nn)
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            L = readMemory(addr);
            H = readMemory(addr + 1);
            LogOpcode(opcode, true, "LD HL, (nn)");
            return 16;
        }
        
        case 0xF2: { // JP P, nn
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            if (!(F & FLAG_S)) {
                PC = addr;
            }
            LogOpcode(opcode, true, "JP P, nn");
            return 10;
        }

        case 0xC6: { // ADD A, n
            uint8_t n = readMemory(PC++);
            uint16_t result = A + n;
            
            // Update Flags
            F = 0;
            if ((result & 0xFF) == 0) F |= FLAG_Z;
            if (result & 0x80) F |= FLAG_S;
            if (result > 0xFF) F |= FLAG_C;
            if (((A & 0x0F) + (n & 0x0F)) > 0x0F) F |= FLAG_H;
            // Overflow flag (V)
            if (((A ^ result) & (n ^ result) & 0x80)) F |= FLAG_P;
            
            A = result & 0xFF;
            LogOpcode(opcode, true, "ADD A, n");
            return 7;
        }

        case 0xE9: { // JP (HL)
            PC = HLAddress();
            LogOpcode(opcode, true, "JP (HL)");
            return 4;
        }

        case 0x3B: { // DEC SP
            SP--;
            return ok(6);
        }

        case 0xD6: { // SUB n
            uint8_t n = FetchByte();
            uint8_t res = A - n;
            
            // Flags
            F = FLAG_N; // Set Subtract flag
            if (res == 0) F |= FLAG_Z;
            if (res & 0x80) F |= FLAG_S;
            if (A < n) F |= FLAG_C;
            // Half-carry: borrow from bit 4
            if ((A & 0x0F) < (n & 0x0F)) F |= FLAG_H;
            // Overflow
            if (((A ^ n) & 0x80) && ((A ^ res) & 0x80)) F |= FLAG_P;
        
            A = res;
            return ok(7);
        }
        
        case 0xFB: { // EI
            IFF1 = IFF2 = true;            
            return ok(4);
        }

        default:
        //    std::printf("Unimplemented Main opcode: 0x%02X at PC: 0x%04X\n", opcode, PC - 1);
            unimplementedInstructionCount++; // Use the main counter here
            return 4;

    }
    
}

// -------------------------------------------------------------
// CB Prefix
// -------------------------------------------------------------
uint32_t Z80::ExecuteCB(uint8_t opcode) {
    uint8_t reg = opcode & 0x07;
    bool isHL = (reg == 6);
    uint8_t value = isHL ? ReadHL() : GetReg(reg);
    int opType = 0; // 0=shift/rotate, 1=bit, 2=res/set

    uint8_t op = opcode >> 3;
    switch (op) {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: { // RLC
            uint8_t bit7 = value >> 7;
            value = (value << 1) | bit7;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit7 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F: { // RRC
            uint8_t bit0 = value & 1;
            value = (value >> 1) | (bit0 << 7);
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: { // RL
            uint8_t bit7 = value >> 7;
            value = (value << 1) | ((F & FLAG_C) ? 1 : 0);
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit7 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F: { // RR
            uint8_t bit0 = value & 1;
            value = (value >> 1) | (((F & FLAG_C) ? 1 : 0) << 7);
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: { // SLA
            uint8_t bit7 = value >> 7;
            value <<= 1;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit7 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F: { // SRA
            uint8_t bit0 = value & 1;
            uint8_t bit7 = value & 0x80;
            value = (value >> 1) | bit7;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: { // SRL
            uint8_t bit0 = value & 1;
            value >>= 1;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        default: {
            if ((opcode & 0xC0) == 0x40) { // BIT
                opType = 1;
                uint8_t bit = opcode & 0x07;
                bool bitSet = value & (1 << bit);
                F = (F & ~(FLAG_Z | FLAG_H | FLAG_N)) | FLAG_H;
                if (!bitSet) F |= FLAG_Z;
            } else if ((opcode & 0xC0) == 0x80) { // RES
                opType = 2;
                uint8_t bit = opcode & 0x07;
                value &= ~(1 << bit);
            } else if ((opcode & 0xC0) == 0xC0) { // SET
                opType = 2;
                uint8_t bit = opcode & 0x07;
                value |= (1 << bit);
            } else {
                LogOpcode(opcode, false, "CB ");
                return 8;
            }
            break;
        }
    }

    // Write back if not BIT
    if (opType != 1) {
        if (isHL) {
            WriteHL(value);
        } else {
            GetReg(reg) = value;
        }
    }

    uint32_t cycles;
    if (opType == 0) cycles = isHL ? 15 : 8;
    else if (opType == 1) cycles = isHL ? 12 : 8;
    else cycles = isHL ? 15 : 8;

    LogOpcode(opcode, true, "CB ");
    return cycles;
}

// -------------------------------------------------------------
// ED Prefix
// -------------------------------------------------------------
uint32_t Z80::ExecuteED(uint8_t opcode) {
    switch (opcode) {
        case 0xA3: { // OUTI
            // 1. Read byte from (HL)
            uint8_t data = readMemory(HLAddress());
    
            // 2. Output to port (C)
            writeIO(C, data);
    
            // 3. Increment HL
            uint16_t hl = HLAddress();
            hl++;
            H = (hl >> 8) & 0xFF;
            L = hl & 0xFF;
    
            // 4. Decrement B
            B--;
    
            // 5. Update Flags
            // N is set, Z is set if B == 0
            F |= FLAG_N;
            if (B == 0) {
                F |= FLAG_Z;
            } else {
                F &= ~FLAG_Z;
            }
            // Note: S, H, P/V flags are technically modified based on the 
            // data written, but Z and N are the critical ones for MSX BIOS loops.

            LogOpcode(opcode, true, "OUTI");
            return 16; // Takes 16 T-states
        }
        case 0x51: { // OUT (C), D
            writeIO(C, D);
            LogOpcode(opcode, true, "OUT (C), D");
            return 12;
        }
        case 0x4B: { // LD BC, (nn)
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            C = readMemory(addr);
            B = readMemory(addr + 1);
            LogOpcode(opcode, true, "LD BC, (nn)");
            return 20;
        }
        case 0xB3: { // OTIR
            // Execution is the same as OUTI, but PC decrements by 2 
            // to repeat the instruction if B != 0
            uint8_t data = readMemory(HLAddress());
            writeIO(C, data);
    
            uint16_t hl = HLAddress();
            hl++;
            H = (hl >> 8) & 0xFF;
            L = hl & 0xFF;
    
            B--;
    
            F |= FLAG_N;
            if (B == 0) {
                F |= FLAG_Z;
                LogOpcode(opcode, true, "OTIR (Finished)");
                return 16;
            } else {
                F &= ~FLAG_Z;
                PC -= 2; // Repeat the ED B3 instruction
                LogOpcode(opcode, true, "OTIR (Repeating)");
                return 21; // Repeating takes 21 T-states
            }
        }
        case 0x5B: { // LD DE, (nn)
            uint8_t low = readMemory(PC++);
            uint8_t high = readMemory(PC++);
            uint16_t addr = (high << 8) | low;
            E = readMemory(addr);
            D = readMemory(addr + 1);
            LogOpcode(opcode, true, "LD DE, (nn)");
            return 20;
        }
        case 0x44: { // NEG
            uint8_t oldA = A;
            A = 0 - A;
            F = FLAG_N;
            if (A == 0) F |= FLAG_Z;
            if (A & 0x80) F |= FLAG_S;
            SetParityFlag(A);
            if (oldA != 0) F |= FLAG_C;
            if ((A & 0x0F) > (oldA & 0x0F)) F |= FLAG_H;
            LogOpcode(opcode, true, "ED ");
            return 8;
        }
        case 0x4D: { // RETI
            PC = PopWord();
            LogOpcode(opcode, true, "ED ");
            return 14;
        }
        case 0x45: { // RETN
            PC = PopWord();
            LogOpcode(opcode, true, "ED ");
            return 14;
        }
        case 0x46: case 0x4E: case 0x66: case 0x6E: { // IM 0
            LogOpcode(opcode, true, "ED ");
            return 8;
        }
        case 0x56: case 0x76: { // IM 1
            LogOpcode(opcode, true, "ED ");
            return 8;
        }
        case 0x5E: case 0x7E: { // IM 2
            LogOpcode(opcode, true, "ED ");
            return 8;
        }

        case 0xA0: { // LDI
            uint8_t value = ReadHL();
            WriteDE(value);
    
            // Increment HL and DE
            uint16_t hl = (uint16_t)((H << 8) | L) + 1;
            H = hl >> 8; L = hl & 0xFF;
            uint16_t de = (uint16_t)((D << 8) | E) + 1;
            D = de >> 8; E = de & 0xFF;
    
            // Decrement BC
            uint16_t bc = (uint16_t)((B << 8) | C) - 1;
            B = bc >> 8; C = bc & 0xFF;
    
            // Update Flags
            F &= ~(FLAG_H | FLAG_N | FLAG_P);
            if (bc != 0) F |= FLAG_P; // PV flag set if BC != 0
    
            LogOpcode(opcode, true, "ED ");
            return 16;
        }

        case 0xA8: { // LDD
            uint8_t value = ReadHL();
            WriteDE(value);
            uint16_t hl = HLAddress() - 1;
            H = hl >> 8;
            L = hl & 0xFF;
            uint16_t de = DEAddress() - 1;
            D = de >> 8;
            E = de & 0xFF;
            C--;
            F &= ~(FLAG_H | FLAG_P | FLAG_N);
            if (C != 0) F |= FLAG_P;
            LogOpcode(opcode, true, "ED ");
            return 16;
        }
        case 0x67: { // RRD
            uint8_t mem = ReadHL();
            uint8_t low = mem & 0x0F;
            uint8_t high = (mem >> 4) & 0x0F;
            uint8_t a_low = A & 0x0F;
            A = (A & 0xF0) | low;
            mem = (a_low << 4) | high;
            WriteHL(mem);
            F &= ~(FLAG_H | FLAG_N);
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);
            LogOpcode(opcode, true, "ED ");
            return 18;
        }
        case 0x6F: { // RLD
            uint8_t mem = ReadHL();
            uint8_t low = mem & 0x0F;
            uint8_t high = (mem >> 4) & 0x0F;
            uint8_t a_low = A & 0x0F;
            A = (A & 0xF0) | high;
            mem = (low << 4) | a_low;
            WriteHL(mem);
            F &= ~(FLAG_H | FLAG_N);
            SetZeroFlag(A);
            SetSignFlag(A);
            SetParityFlag(A);
            LogOpcode(opcode, true, "ED ");
            return 18;
        }

        case 0xB0: { // LDIR (Block Transfwer)
            uint8_t val = readMemory(HLAddress());
            writeMemory(DEAddress(), val);

            // Increment HL and DE
            uint16_t hl = HLAddress() + 1;
            H = hl >> 8; L = hl & 0xFF;
            uint16_t de = DEAddress() + 1;
            D = de >> 8; E = de & 0xFF;

            // Decrement BC
            uint16_t bc = ((B << 8) | C) - 1;
            B = bc >> 8; C = bc & 0xFF;

            F &= ~(FLAG_H | FLAG_P | FLAG_N); // Clear N, H and P
            if (bc != 0) {
                F |= FLAG_P; // Set P/V if BC != 0
                PC -= 2; // Loop the instruction (repeat ED B0)
                return 21; // 16 for the instruction + 5 for the loop overhead
            }
            return 16; // Last iteration
        }
        default:
        //    std::printf("Unimplemented ED opcode: 0x%02X at PC: 0x%04X\n", opcode, PC -1);
            unimplementedEDCount++;
            return 8;
    }
}

// -------------------------------------------------------------
// CB Indexed (IX/IY + d)
// -------------------------------------------------------------
uint32_t Z80::ExecuteCBIndexed(uint16_t index, int8_t d, uint8_t cbOpcode) {
    uint16_t addr = index + d;
    uint8_t value = readMemory ? readMemory(addr) : 0xFF;
    int opType = 0; // 0=shift/rotate, 1=bit, 2=res/set

    uint8_t op = cbOpcode >> 3;
    switch (op) {
        case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: { // RLC
            uint8_t bit7 = value >> 7;
            value = (value << 1) | bit7;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit7 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C: case 0x0D: case 0x0E: case 0x0F: { // RRC
            uint8_t bit0 = value & 1;
            value = (value >> 1) | (bit0 << 7);
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15: case 0x16: case 0x17: { // RL
            uint8_t bit7 = value >> 7;
            value = (value << 1) | ((F & FLAG_C) ? 1 : 0);
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit7 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x18: case 0x19: case 0x1A: case 0x1B: case 0x1C: case 0x1D: case 0x1E: case 0x1F: { // RR
            uint8_t bit0 = value & 1;
            value = (value >> 1) | (((F & FLAG_C) ? 1 : 0) << 7);
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x20: case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x26: case 0x27: { // SLA
            uint8_t bit7 = value >> 7;
            value <<= 1;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit7 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x28: case 0x29: case 0x2A: case 0x2B: case 0x2C: case 0x2D: case 0x2E: case 0x2F: { // SRA
            uint8_t bit0 = value & 1;
            uint8_t bit7 = value & 0x80;
            value = (value >> 1) | bit7;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        case 0x38: case 0x39: case 0x3A: case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F: { // SRL
            uint8_t bit0 = value & 1;
            value >>= 1;
            F &= ~(FLAG_N | FLAG_H);
            F |= (bit0 ? FLAG_C : 0);
            SetZeroFlag(value);
            SetSignFlag(value);
            SetParityFlag(value);
            break;
        }
        default: {
            if ((cbOpcode & 0xC0) == 0x40) { // BIT
                opType = 1;
                uint8_t bit = cbOpcode & 0x07;
                bool bitSet = value & (1 << bit);
                F = (F & ~(FLAG_Z | FLAG_H | FLAG_N)) | FLAG_H;
                if (!bitSet) F |= FLAG_Z;
            } else if ((cbOpcode & 0xC0) == 0x80) { // RES
                opType = 2;
                uint8_t bit = cbOpcode & 0x07;
                value &= ~(1 << bit);
            } else if ((cbOpcode & 0xC0) == 0xC0) { // SET
                opType = 2;
                uint8_t bit = cbOpcode & 0x07;
                value |= (1 << bit);
            } else {
                LogOpcode(cbOpcode, false, "CB ");
                return 8;
            }
            break;
        }
    }

    // Write back if not BIT
    if (opType != 1) {
        if (writeMemory) writeMemory(addr, value);
    }

    uint32_t cycles;
    if (opType == 0) cycles = 23; // for indexed shifts
    else if (opType == 1) cycles = 20; // for BIT
    else cycles = 23; // for RES/SET

    LogOpcode(cbOpcode, true, "CB ");
    return cycles;
}

// -------------------------------------------------------------
// DoLoadRegToReg
// -------------------------------------------------------------
uint32_t Z80::DoLoadRegToReg(uint8_t opcode) {
    uint8_t dest = (opcode >> 3) & 0x07;
    uint8_t src  = opcode & 0x07;

    // dest==6 means (HL) on the left side
    if (dest == 6) {
        WriteHL(GetReg(src));
        return 7;
    }

    if (src == 6) { // src==6 means (HL) on the right side
        GetReg(dest) = ReadHL();
        return 7;
    }

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

uint8_t& Z80::GetReg(uint8_t code) {
    switch (code) {
        case 0: return B;
        case 1: return C;
        case 2: return D;
        case 3: return E;
        case 4: return H;
        case 5: return L;
        case 7: return A;
        default:
            static uint8_t invalid = 0;
            return invalid;
    }
}

uint16_t Z80::HLAddress() const {
    return (uint16_t)(H) << 8 | L;
}

uint16_t Z80::DEAddress() const {
    return (uint16_t)(D) << 8 | E;
}

uint8_t Z80::ReadHL() {
    return readMemory ? readMemory(HLAddress()) : 0xFF;
}

void Z80::WriteHL(uint8_t value) {
    if (writeMemory) writeMemory(HLAddress(), value);
}

void Z80::WriteDE(uint8_t value) {
    if (writeMemory) writeMemory(DEAddress(), value);
}

void Z80::SetParityFlag(uint8_t value) {
    uint8_t bits = value;
    bits ^= bits >> 4;
    bits ^= bits >> 2;
    bits ^= bits >> 1;
    if ((bits & 1) == 0) F |= FLAG_P;
    else F &= ~FLAG_P;
}

void Z80::LogOpcode(uint8_t opcode, bool implemented, const char* prefix) {
    if (implemented) {
        if (opcode == 0x00)
            return;
        if (!traceOpcodes)
            return;
        std::cout << (prefix ? prefix : "")
                  << "Opcode OK: 0x" << std::hex << int(opcode) << std::dec << "\n";
    } else {
        if (!traceUnimplementedOpcodes)
            return;
        std::cerr << (prefix ? prefix : "")
                  << "Unimplemented opcode: 0x" << std::hex << int(opcode) << std::dec << "\n";
    }
}

void Z80::DumpOpcodeStats() const {
    std::cout << std::dec;
    std::cout << "Z80 opcode stats: executed=" << executedInstructionCount
              << ", unimplemented=" << unimplementedInstructionCount
              << ", unimplemented CB=" << unimplementedCBCount
              << ", unimplemented ED=" << unimplementedEDCount
              << "\n";
}

void Z80::HandleInterrupt() {
    // Maskable interrupts are only processed if IFF1 is true
    if (IFF1) {
        IFF1 = IFF2 = false; // Disable further interrupts
        
        PushWord(PC);        // Save current address to return later
//        PC = INT_VECTOR;     // Jump to MSX BIOS interrupt handler (0x0038)
        PC = 0x0038;
        //std::cout << "DEBUG: INTERRUPT FORCED!" << std::endl;
        
        // Note: In a real Z80, this takes roughly 13 T-states
    }
}
