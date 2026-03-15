#pragma once
#include <cstdint>
#include <vector>

class TMS9918 {
public:
    TMS9918();
    ~TMS9918() = default;

    // Execute cycles (VDP runs in parallel with CPU)
    void Update(uint32_t cpuCycles);

    // Port interfaces for the MSX Bus
    uint8_t Read(uint8_t port); // Read Status (Port 1)
    void Write(uint8_t port, uint8_t data); // Write Data (Port 0) or Reg (Port 1)

    // Get the current frame buffer (for the display renderer)
    const std::vector<uint8_t>& GetFrameBuffer() const;

    void DumpVRAM(uint16_t start, uint16_t length) const;

private:
    // Internal State
    std::vector<uint8_t> vram; // 16KB VRAM
    std::vector<uint8_t> framebuffer; // 256x212 (approx) pixel buffer

    uint8_t registers[8];
    uint8_t statusReg;
    uint8_t firstByteLatch; // For handling 2-byte write sequences

    // Helper to convert VRAM to pixel data (skeletal implementation placeholder)
    void RefreshScreen(); 

    uint16_t vramAddress; // ADD THIS: Tracks where we are writing VRAM data
    bool latchActive;
};

