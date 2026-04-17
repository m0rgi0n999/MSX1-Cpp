#include "core/Emulator.hpp"
#include "CPU/Z80.hpp"
#include "Memory/MemoryMapper.hpp"
#include "io/FileManager.hpp"
#include <iostream>

// The ": z80(bus)" part is the fix. It passes the bus to the CPU immediately.
Emulator::Emulator() : z80(bus) {
    // 1. Link Z80 functions to the Bus
    z80.readMemory = [this](uint16_t addr) { return bus.Read(addr); };
    z80.writeMemory = [this](uint16_t addr, uint8_t val) { bus.Write(addr, val); };
    z80.readIO = [this](uint8_t port) { return bus.IO_Read(port); };
    z80.writeIO = [this](uint8_t port, uint8_t val) { bus.IO_Write(port, val); };

    MapIODevices();
    z80.Reset();
}

void Emulator::MapIODevices() {
    // Primary Slot Register
    bus.MapIO(0xA8, 
        [this](uint8_t) { return bus.mapper.ReadPortA8(); },
        [this](uint8_t, uint8_t data) { bus.mapper.WritePortA8(data); }
    );

    // VDP (Removed unused parameter warnings by omitting the name)
    bus.MapIO(0x98, 
        [this](uint8_t) { return vdp.ReadData(); },
        [this](uint8_t /*unused*/, uint8_t data) { vdp.WriteData(data); }
    );
    bus.MapIO(0x99, 
        [this](uint8_t) { return vdp.ReadStatus(); },
        [this](uint8_t /*unused*/, uint8_t data) { vdp.WriteControl(data); }
    );

    // PSG ADDR (0xA0)
    bus.MapIO(
        0xA0,
        [this](uint8_t port) { return psg.Read(port); },
        [this](uint8_t port, uint8_t val) { psg.Write(port, val); }
    );

    // PSG DATA (0xA1)
    bus.MapIO(
        0xA1,
        [this](uint8_t port) { return psg.Read(port); },
        [this](uint8_t port, uint8_t val) { psg.Write(port, val); }
    );
}

void Emulator::EnableOpcodeTrace(bool enabled) {
    z80.traceOpcodes = enabled;
}

void Emulator::LoadSystemROM(const std::string& biosPath) {
    auto rom = FileManager::LoadROM(biosPath);
    if (rom.empty()) {
        std::cerr << "Failed to load BIOS ROM: " << biosPath << std::endl;
        return;
    }
    bus.LoadToRAM(0x0000, rom);
}
void Emulator::RunFrame() {
    // 50Hz (PAL) = 71,591 cycles per frame
    // 60 Hz (NTSC) = 59,659 cycles per frame
    const uint32_t CYCLES_PER_FRAME = 71591; // Approx cycles for 50Hz MSX
    uint32_t cyclesThisFrame = 0;
    int safetyCounter = 0;

    while (cyclesThisFrame < CYCLES_PER_FRAME) {
        cyclesThisFrame += z80.ExecuteInstruction();

        // Safety: If we've run 1 million instrctions and still haven't
        // hit the cycle target, something is wrong with our cycle counting.
        if (++safetyCounter > 1000000) {
          std::cout << "DEBUG: Emergebcy Break! Cycles stuck at: " << cyclesThisFrame << std::endl;
          break;
        }
    }

    // 1. Tell the VDP that time has passed so it sets its internal "V-Blank" flag
    vdp.Update(cyclesThisFrame); 

    // 2. Trigger the CPU interrupt
    // Now when the BIOS jumps to 0x0038, it will read VDP Port 0x99, 
    // see the flag, and run the keyboard/timer logic!
    //std::cout << "DEBUG: Frame Finished. Triggering Interrupt..." << std::endl;
    z80.HandleInterrupt();
}

void Emulator::Initialize(const std::vector<uint8_t>& bios) {
    bus.mapper.WritePortA8(0x00);
    // 1. Put BIOS in slot 0 (Pages 0 and 1: 0x0000-0x7FFF)
    bus.mapper.InsertDevice(0, 0x0000, 0x7FFF, bios, true);

    // 2. Put RAM in Slot 3 (Pages 2 and 3: 0x8000-0xFFFF)
    //  MSX1 typically needs at least 16Kb-64Kb RAM at the top
    std::vector<uint8_t> ram(0x8000, 0x00); // 32Kb RAM
    bus.mapper.InsertDevice(3, 0x8000, 0xFFFF, ram, false);

    z80.Reset();
}
