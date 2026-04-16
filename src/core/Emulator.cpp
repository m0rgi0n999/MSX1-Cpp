#include "core/Emulator.hpp"
#include "io/FileManager.hpp"
#include <iostream>

Emulator::Emulator()
{
    // Z80 MEMORY CALLBACKS -> BUS
    z80.readMemory = [this](uint16_t addr) {
        return bus.Read(addr);
    };

    z80.writeMemory = [this](uint16_t addr, uint8_t value) {
        bus.Write(addr, value);
    };

    // Z80 IO CALLBACKS -> BUS
    z80.readIO = [this](uint8_t port) {
        return bus.IO_Read(port);
    };

    z80.writeIO = [this](uint8_t port, uint8_t value) {
        bus.IO_Write(port, value);
    };

    z80.Reset();

    // MAP IO DEVICES

    // Port 0x98: VDP Data Port
    bus.MapIO(0x98, 
        [this](uint8_t) { 
            return vdp.ReadData(); 
        }, 
        [this](uint8_t portVal, uint8_t dataVal) { 
            vdp.WriteData(dataVal); 
        }
    );

    // Port 0x99: VDP Control/Status Port
    bus.MapIO(0x99, 
        [this](uint8_t) { 
            return vdp.ReadStatus(); 
        }, 
        [this](uint8_t portVal, uint8_t dataVal) { 
            vdp.WriteControl(dataVal); 
        }
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
    const uint32_t CYCLES_PER_FRAME = 59659; // Approx cycles for 60Hz MSX
    uint32_t cyclesThisFrame = 0;

    while (cyclesThisFrame < CYCLES_PER_FRAME) {
        cyclesThisFrame += z80.ExecuteInstruction();
    }

    // 1. Tell the VDP that time has passed so it sets its internal "V-Blank" flag
    vdp.Update(cyclesThisFrame); 

    // 2. Trigger the CPU interrupt
    // Now when the BIOS jumps to 0x0038, it will read VDP Port 0x99, 
    // see the flag, and run the keyboard/timer logic!
    z80.HandleInterrupt();
}

void Emulator::Initialize(const std::vector<uint8_t>& bios) {
    // 1. Load the BIOS bytes into memory
    bus.LoadToRAM(0x0000, bios);
    
    // 2. Ensure the CPU starts at the beginning
    z80.Reset(); 
    
    std::cout << "System initialized: BIOS loaded to 0x0000" << std::endl;
}