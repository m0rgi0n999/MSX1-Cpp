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

    // VDP DATA (0x98)
    bus.MapIO(
        0x98,
        [this](uint8_t port) { return vdp.Read(port); },
        [this](uint8_t port, uint8_t val) { vdp.Write(port, val); }
    );

    // VDP CTRL (0x99)
    bus.MapIO(
        0x99,
        [this](uint8_t port) { return vdp.Read(port); },
        [this](uint8_t port, uint8_t val) { vdp.Write(port, val); }
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
    // Simple instruction batch execution for one frame.
    uint32_t totalCycles = 0;
    for (int i = 0; i < 1000; ++i) {
        uint32_t cycles = z80.ExecuteInstruction();
        totalCycles += cycles;
    }

    // Update peripherals
    vdp.Update(totalCycles);
    psg.Tick(totalCycles);

    z80.DumpOpcodeStats();

    // Dump VRAM for debugging
    vdp.DumpVRAM(0x2000, 16);
}

