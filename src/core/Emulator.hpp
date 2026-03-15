#pragma once
#include "CPU/Z80.hpp"
#include "Memory/Bus.hpp"
#include "VDP/TMS9918.hpp"
#include "PSG/AY8910.hpp"
#include <string>

class Emulator {
public:
    Emulator();
    ~Emulator() = default;

    // Initialize the system (Load BIOS)
    void LoadSystemROM(const std::string& biosPath);

    // Load a cartridge, tape, or disk
    void InsertCartridge(const std::string& romPath);

    // Execute one frame of emulation (until VSync)
    void RunFrame();

    // Get pointers to subsystems if you need to access them from outside (e.g., GUI)
    // TMS9918* GetVDP() { return &vdp; }
    const TMS9918& GetVDP() const { return vdp; }
    AY8910* GetPSG() { return &psg; }

private:
    Z80 z80;
    Bus bus;
    TMS9918 vdp;
    AY8910 psg;

    // Callbacks for the Z80 to access memory/IO
    uint8_t CPURead(uint16_t addr);
    void CPUWrite(uint16_t addr, uint8_t val);
    uint8_t CPUIO_Read(uint8_t port);
    void CPUIO_Write(uint8_t port, uint8_t val);

    void MapIODevices(); // Connect VDP and PSG to the Bus I/O ports
};

