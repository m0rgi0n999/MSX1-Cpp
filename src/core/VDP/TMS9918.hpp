#pragma once
#include <cstdint>
#include <vector>

class TMS9918 {
public:
    TMS9918();
    ~TMS9918() = default;

    // Execute cycles (VDP runs in parallel with CPU)
    void Update(uint32_t cpuCycles);

    // --- Port interfaces for the MSX Bus ---
    // Port 0x98: Data Port
    uint8_t ReadData();
    void WriteData(uint8_t data);

    // Port 0x99: Control/Status Port
    uint8_t ReadStatus();
    void WriteControl(uint8_t data);

    // Get the current frame buffer (for the display renderer)
    const std::vector<uint8_t>& GetFrameBuffer() const;

    void DumpVRAM(uint16_t start, uint16_t length) const;

private:
    // Internal Memory
    std::vector<uint8_t> vram;         // 16KB VRAM
    std::vector<uint8_t> framebuffer;  // Pixel buffer for rendering

    // Internal State Registers
    uint8_t registers[8];
    uint8_t statusReg;

    // --- Communication Logic ---
    uint16_t currentAddress = 0;   // The internal VRAM pointer
    uint8_t addressBuffer = 0;     // Stores the first byte of a 2-byte sequence
    bool addressLatch = false;     // False = waiting for 1st byte, True = waiting for 2nd

    void RefreshScreen(); 
};