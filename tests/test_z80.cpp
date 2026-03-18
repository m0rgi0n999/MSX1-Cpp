#include "core/CPU/Z80.hpp"
#include <gtest/gtest.h>   // or <catch2/catch_test_macros.hpp>

// Shared test memory buffer + callbacks
static uint8_t memory[65536];

static uint8_t TestReadMemory(uint16_t addr)  { return memory[addr]; }
static void    TestWriteMemory(uint16_t addr, uint8_t v) { memory[addr] = v; }

// I/O tests, future proofing.
static uint8_t lastPortWritten = 0;
static uint8_t lasPortValue    = 0;

static void TestgWriteIO(uint8_t port, uint8_t value) {
  lastPortWritten = port;
  lasPortValue = value;
}
static uint8_t TestReadIO(uint8_t port) {
  return 0x42; // example dummy value
}

// --- SECTION 1: INC r ---
TEST(Z80, INC_Registers) {
  Z80 cpu;

  cpu.readMemory = TestReadMemory;
  cpu.writeMemory = TestWriteMemory;

  struct TestCase {
    uint8_t opcode;
    uint8_t* reg;
  };

  TestCase tests[] = {
    {0x04, &cpu.B},
    {0x0C, &cpu.C},
    {0x14, &cpu.D},
    {0x1C, &cpu.E},
    {0x24, &cpu.H},
    {0x2C, &cpu.L},
    {0x3C, &cpu.A},
  };

  for (auto& t : tests) {
    memory[0] = t.opcode;
    *(t.reg) = 0x7F;

    cpu.PC = 0;
    cpu.ExecuteInstruction();

    EXPECT_EQ(*(t.reg), 0x80);    // value incremented
    EXPECT_TRUE(cpu.F & FLAG_S);  // sign should be set
    EXPECT_FALSE(cpu.F & FLAG_N); // N cleared
  }
}

// --- SECTION 2: INC (HL) ---
TEST(Z80, INC_at_HL) {
  Z80 cpu;

  cpu.readMemory = TestReadMemory;
  cpu.writeMemory = TestWriteMemory;

  cpu.H = 0x40;
  cpu.L = 0x00;

  memory[0x4000] = 0x0F; // value before increment
  memory[0] = 0x34; // INC (HL)
    
  cpu.PC = 0;
  cpu.ExecuteInstruction();

  EXPECT_EQ(memory[0x4000], 0x10); // memory incremented
  EXPECT_FALSE(cpu.F & FLAG_N);    // N cleared
}

// --- SECTION 3: DEC r ---
TEST(Z80, DEC_Registers) {
  Z80 cpu;
    
  cpu.readMemory = TestReadMemory;
  cpu.writeMemory = TestWriteMemory;

  struct TestCase {
    uint8_t opcode;
    uint8_t* reg;
  };

  TestCase tests[] = {
    {0x05, &cpu.B},
    {0x0D, &cpu.C},
    {0x15, &cpu.D},
    {0x1D, &cpu.E},
    {0x25, &cpu.H},
    {0x2D, &cpu.L},
    {0x3D, &cpu.A},
  };

  for (auto& t : tests) {
    memory[0] = t.opcode;
    *(t.reg) = 0x01;

    cpu.PC = 0;
    cpu.ExecuteInstruction();

    EXPECT_EQ(*(t.reg), 0x00);
    EXPECT_TRUE(cpu.F & FLAG_Z);    // Zero flag set
    EXPECT_TRUE(cpu.F & FLAG_N);    // DEC sets N
  }
}

// --- SECTION 4: DEC (HL) ---
TEST(Z80, DEC_at_HL) {
  Z80 cpu;

  cpu.readMemory = TestReadMemory;
  cpu.writeMemory = TestWriteMemory;

  cpu.H = 0x50;
  cpu.L = 0x00;

  memory[0x5000] = 0x01;
  memory[0] = 0x35;

  cpu.PC = 0;
  cpu.ExecuteInstruction();

  EXPECT_EQ(memory[0x5000], 0x00);
  EXPECT_TRUE(cpu.F & FLAG_Z);  // Zero = 1
  EXPECT_TRUE(cpu.F & FLAG_N);  // DEC sets N
}

// --- SECTION 5: LD r,(HL) ---
TEST(Z80, LD_r_from_HL) {
  Z80 cpu;

  cpu.readMemory = TestReadMemory;

  struct TestCase {
    uint8_t opcode;
    uint8_t* reg;
  };

  TestCase tests[] ={
    {0x46, &cpu.B},
    {0x4E, &cpu.C},
    {0x56, &cpu.D},
    {0x5E, &cpu.E},
    {0x66, &cpu.H},
    {0x6E, &cpu.L},
    {0x7E, &cpu.A},
  };

  cpu.H = 0x40;
  cpu.L = 0x10;

  memory[0x4010] = 0xAB;  //test value
    
  for (auto& t : tests) {
    memory[0] = t.opcode;
    cpu.PC = 0;
    cpu.ExecuteInstruction();
    EXPECT_EQ(*(t.reg), 0xAB);
  }
}

// --- SECTION 6: LD (HL),r ---
TEST(Z80, LD_HL_from_r) {
  Z80 cpu;

  cpu.readMemory = TestReadMemory;
  cpu.writeMemory = TestWriteMemory;

  cpu.H = 0x60;
  cpu.L = 0x00;

  struct TestCase {
    uint8_t opcode;
    uint8_t* value;
  };

  TestCase tests[] = {
    {0x70, 0x11},
    {0x71, 0x22},
    {0x72, 0x33},
    {0x73, 0x44},
    {0x74, 0x55},
    {0x75, 0x66},
    {0x77, 0x77},
  };

  for (auto& t : tests) {
    uint16_t addr = 0x6000;

    // assign register
    switch (t.opcode & 0x07) {
      case 0: cpu.B = t.value; break;
      case 1: cpu.C = t.value; break;
      case 2: cpu.D = t.value; break;
      case 3: cpu.E = t.value; break;
      case 4: cpu.H = t.value; break;
      case 5: cpu.L = t.value; break;
      case 7: cpu.A = t.value; break;
    }

    memory[0] = t.opcode;
    cpu.PC = 0;
    cpu.ExecuteInstruction();

    EXPECT_EQ(memory[addr], t.value);
  }
}

// --- SECTION 7: JP Z/NC/C ---
TEST(Z80, JP_Z_taken)   {
  Z80 cpu;
  cpu.readMemory = TestReadMemory;

  memory[0] = 0xCA;
  memory[1] = 0x34;
  memory[2] = 0x12;

  cpu.F = FLAG_Z; // Z=1

  cpu.PC = 0;
  cpu.ExecuteInstruction();

  EXPECT_EQ(cpu.PC, 0x1234);
}
TEST(Z80, JP_Z_not_taken)   {
  Z80 cpu;
  cpu.readMemory = TestReadMemory;
  
  memory[0] = 0xCA;
  memory[1] = 0x00;
  memory[2] = 0x80;

  cpu.F = 0; // Z=0

  cpu.PC = 0;
  cpu.ExecuteInstruction();

  EXPECT_EQ(cpu.PC, 3); // advanced normally
}
TEST(Z80, JP_NC_taken)  {
  Z80 cpu;
  cpu.readMemory = TestReadMemory;

  memory[0] = 0xD2;
  memory[1] = 0x20;
  memory[2] = 0x40;

  cpu.F = 0; // C flag clear

  cpu.PC = 0;
  cpu.ExecuteInstruction();

  EXPECT_EQ(cpu.PC, 0x4020);
}
TEST(Z80, JP_C_taken)   {
  Z80 cpu;
  cpu.readMemory = TestReadMemory;

  memory[0] = 0xDA;
  memory[1] = 0x00;
  memory[2] = 0x20;

  cpu.F = FLAG_C;

  cpu.PC = 0;
  cpu.ExecuteInstruction();

  EXPECT_EQ(cpu.PC, 0x2000);
}
