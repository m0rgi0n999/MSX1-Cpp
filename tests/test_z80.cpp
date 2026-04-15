#include "core/CPU/Z80.hpp"
#include <gtest/gtest.h>   // or <catch2/catch_test_macros.hpp>
#include "core/Memory/Bus.hpp"          // Add this
#include "core/Memory/MemoryMapper.hpp"  // Add this

// Shared test memory buffer + callbacks
static uint8_t memory[65536];

static uint8_t TestReadMemory(uint16_t addr)  { return memory[addr]; }
static void    TestWriteMemory(uint16_t addr, uint8_t v) { memory[addr] = v; }

// I/O tests, future proofing.
static uint8_t lastPortWritten = 0;
static uint8_t lasPortValue    = 0;

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
    uint8_t value;
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

TEST(MemoryMapperTest, SlotSwitching) {
    MemoryMapper mapper;

    // Slot 0: BIOS (Read Only)
    std::vector<uint8_t> bios(0x4000, 0xF1); 
    mapper.InsertDevice(0, 0x0000, 0x3FFF, bios, true);

    // Slot 1: RAM (Writable)
    std::vector<uint8_t> ram(0x4000, 0x00);
    mapper.InsertDevice(1, 0x0000, 0x3FFF, ram, false);

    // Set Port 0xA8 to Slot 0 for Page 0 (bits 0-1 = 00)
    mapper.WritePortA8(0x00);
    EXPECT_EQ(mapper.Read(0x0000), 0xF1);

    // Set Port 0xA8 to Slot 1 for Page 0 (bits 0-1 = 01)
    mapper.WritePortA8(0x01);
    mapper.Write(0x0000, 0x42);
    EXPECT_EQ(mapper.Read(0x0000), 0x42);

    // Switch back to Slot 0 (BIOS) - should still be 0xF1
    mapper.WritePortA8(0x00);
    EXPECT_EQ(mapper.Read(0x0000), 0xF1);
}

TEST(BusTest, LoadAndReadBack) {
    Bus bus;
    
    // 1. Setup a dummy RAM block in Slot 0 for the whole 64KB range
    std::vector<uint8_t> dummyRam(0x10000, 0);
    
    // Use the new proxy method instead of the bus.mapper.InsertDvice
    bus.InsertDevice(0, 0x0000, 0xFFFF, dummyRam, false);

    // To set A8, use your existing IO_Write
    bus.IO_Write(0xA8, 0x00); // Point all pages to Slot 0
    
    // 2. Load some data
    std::vector<uint8_t> testData = {0x01, 0x02, 0x03, 0x04};
    bus.LoadToRAM(0xC000, testData);
    
    // 3. Verify
    EXPECT_EQ(bus.Read(0xC000), 0x01);
    EXPECT_EQ(bus.Read(0xC003), 0x04);
}

TEST(MSX_Integration, BootSlotState) {
    Bus bus;
    
    // 1. Use the new bridge method instead of bus.mapper
    std::vector<uint8_t> bios(0x4000, 0xF3);
    bus.InsertDevice(0, 0x0000, 0x3FFF, bios, true);
    
    std::vector<uint8_t> ram(0x4000, 0x00);
    bus.InsertDevice(3, 0xC000, 0xFFFF, ram, false);

    // 2. Use IO_Write to change slots (this tests the Mapper through the Port 0xA8 logic)
    bus.IO_Write(0xA8, 0x00); // Select Slot 0 for all pages
    
    EXPECT_EQ(bus.Read(0x0000), 0xF3);
}