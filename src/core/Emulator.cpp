#include "core/Emulator.hpp"
#include "io/FileManager.hpp"
#include <iostream>

// =============================
//  Z80 <-> BUS BRIDGE FUNCTIONS
// =============================

static Emulator* g_emulator = nullptr;

static uint8_t Z80_MemRead(uint16_t addr) {
    return g_emulator->bus.Read(addr);
}

static void Z80_MemWrite(uint16_t addr, uint8_t value) {
    g_emulator->bus.Write(addr, value);
}

static uint8_t Z80_IORead(uint8_t port) {
    return g_emulator->bus.ReadIO(port);
}

static void Z80_IOWrite(uint8_t port, uint8_t value) {
    g_emulator->bus.WriteIO(port, value);
}

Emulator::Emulator()
{
    // Make this instance available for static callbacks
    g_emulator = this;

    // Connect Z80 to BUS using plain function pointers
    z80.readMemory  = Z80_ReadMem;
    z80.writeMemory = Z80_WriteMem;
    z80.readIO      = Z80_ReadIO;
    z80.writeIO     = Z80_WriteIO;

    z80.Reset();

    // -------------------------
    // MAP IO DEVICES
    // -------------------------

    bus.MapIO(0x98,
        uint8_t port { return vdp.Read(port); },
        [this](uint8_t port, uint8_t value) { vdp.Write(port, value); }
    );

    bus.MapIO(0x99,
        [this](uint8_t port) { return vdp.Read(port); },
        [this](uint8_t port, uint8_t value) { vdp.Write(port, value); }
    );

    bus.MapIO(0xA0,
        [this](uint8_t port) { return psg.Read(port); },
        [this](uint8_t port, uint8_t value) { psg.Write(port, value); }
    );

    bus.MapIO(0xA1,
        [this](uint8_t port) { return psg.Read(port); },
        [this](uint8_t port, uint8_t value) { psg.Write(port, value); }
    );
}
