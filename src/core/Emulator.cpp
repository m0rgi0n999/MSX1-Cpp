#include "core/Emulator.hpp"
#include "io/FileManager.hpp"
#include <iostream>

Emulator::Emulator() {
    // 1. Set Memory Callbacks (RAM/ROM)
    z80.SetMemoryCallbacks(
        [this](uint16_t addr) { return this->bus.Read(addr); },
        [this](uint16_t addr, uint8_t val) { this->bus.Write(addr, val); }
    );

    // 2. Set I/O Callbacks (Ports for VDP/PSG)
    z80.SetIOCallbacks(
        [this](uint8_t port) { return this->bus.IO_Read(port); },  // Fixed: use port
        [this](uint8_t port, uint8_t val) { this->bus.IO_Write(port, val); } // Fixed: use port
    );

    z80.Reset();

    // --- MAP IO DEVICES ---
    // Connect TMS9918 to Port 0x98 (Data) and 0x99 (Command)
    bus.MapIO(0x98,
        [this](uint8_t port) { return this->vdp.Read(port); },
        [this](uint8_t port, uint8_t val) { this->vdp.Write(port, val); }
    );

    bus.MapIO(0x99,
        [this](uint8_t port) { return this->vdp.Read(port); },
        [this](uint8_t port, uint8_t val) { this->vdp.Write(port, val); }
    );

    // Connect AY8910 to port 0xA0 (Latch/Address) and 0xA1 (Read/Write Data)
    bus.MapIO(0xA0,
        [this](uint8_t port) { return this->psg.Read(port); },
        [this](uint8_t port, uint8_t val) { this->psg.Write(port, val); }
    );

    bus.MapIO(0xA1,
        [this](uint8_t port) { return this->psg.Read(port); },
        [this](uint8_t port, uint8_t val) { this->psg.Write(port, val); }
    );
}


void Emulator::LoadSystemROM(const std::string& biosPath) {
    auto biosData = FileManager::LoadROM(biosPath);
    if (biosData.empty()) {
        std::cerr << "Failed to load BIOS ROM: " << biosPath << std::endl;
        return;
    }
    bus.LoadToRAM(0x0000, biosData);
    std::cout << "BIOS Loaded. Size: " << biosData.size() << " bytes." << std::endl;
}

void Emulator::RunFrame() {
    // A very basic loop
    for(int i = 0; i < 100000; ++i) {
        uint32_t cycles = z80.ExecuteInstruction();
        (void)cycles; // Silences the "unused variable" warning
        // We would pass these cycles to vdp.Update(cycles) here in the future
    }
}

