#pragma once
#include "Memory/Bus.hpp"
#include "CPU/Z80.hpp"
#include "VDP/TMS9918.hpp"
#include "PSG/AY8910.hpp"
#include <string>
#include <vector>
#include <cstdint>

class Emulator {
public:
    Emulator();
    ~Emulator() = default;

    // Initialize the system (Load BIOS)
    void LoadSystemROM(const std::string& biosPath);

    // Debugging helpers
    void EnableOpcodeTrace(bool enabled);

    // Get pointers to subsystems if you need to access them from outside (e.g., GUI)
    // TMS9918* GetVDP() { return &vdp; }
    const TMS9918& GetVDP() const { return vdp; }
    AY8910* GetPSG() { return &psg; }

    void InsertCartridge(const std::vector<uint8_t>& romData) {
        bus.InsertDevice(1, 0x0000, 0x3FFF, romData, true); // Slot 1, Page 0
    }

    // Add these public "Hardware Probes"
    void Out(uint8_t port, uint8_t value) { bus.IO_Write(port, value); }
    uint8_t In(uint8_t port) { return bus.IO_Read(port); }

    void Initialize(const std::vector<uint8_t>& bios);

    // Your main loop methods...
    void RunFrame();

    void DumpVideo() {
      vdp.DebugPrintScreen();
    }

    TMS9918& GetVDP() { return vdp; }

private:
    Bus bus;
    Z80 z80;
    TMS9918 vdp;
    AY8910 psg;

    // Callbacks for the Z80 to access memory/IO
    uint8_t CPURead(uint16_t addr);
    void CPUWrite(uint16_t addr, uint8_t val);
    uint8_t CPUIO_Read(uint8_t port);
    void CPUIO_Write(uint8_t port, uint8_t val);

    void MapIODevices(); // Connect VDP and PSG to the Bus I/O ports
};
