#include "core/VDP/TMS9918.hpp"
#include <iostream> // <--- Required for std::cout
#include <iomanip>  // <--- Often good to include for formatting, but iostream usually handles endl/hex

TMS9918::TMS9918() : vramAddress(0x0000), latchActive(false) {
    // Initialize VRAM (16KB)
    vram.resize(16 * 1024, 0);

    // Initialize framebuffer (256x212 pixels is standard for MSX)
    // Reserve space enough for line-by-line rendering
    framebuffer.resize(256 * 212, 0); 

    // Reset State
    for (int i = 0; i < 8; ++i) registers[i] = 0;
    statusReg = 0;
    firstByteLatch = 0;
}

void TMS9918::Update(uint32_t cpuCycles) {
    // In a real emulator, you would track the accumulated cycles
    // and trigger VSync after a certain amount.
    // For now, this is a placeholder.
    (void)cpuCycles; // Suppress unused warning
}

uint8_t TMS9918::Read(uint8_t port) {
    // Placeholder: Silence unused warning
    (void)port;

    // Port 0x98/0x99 usually reads status or data.
    // Simplified: Return Status register
    return statusReg;
}

// Update TMS9918::Write in src/core/VDP/TMS9918.cpp

void TMS9918::Write(uint8_t port, uint8_t data) {
    std::cout << "[TMS9918] Write() called. Port: 0x" << std::hex << (int)port 
              << ", Data: 0x" << (int)data << std::endl;

    if (port == 0x98) {
        if (vramAddress < vram.size()) {
            vram[vramAddress] = data;
            //std::cout << "[VDP] VRAM Write @ 0x" << vramAddress 
            //          << " = 0x" << (int)data << std::endl;
        }
        vramAddress++;
        if (vramAddress > 0x3FFF) { vramAddress = 0; }
        return; 
    }

    if (port == 0x99) {
        // FIX: Check the boolean flag, not the data value
        if (!latchActive) {
            // --- STEP 1: First Byte ---
            firstByteLatch = data;
            latchActive = true; // Mark as "Busy"
            std::cout << "[VDP] Command Byte 1 received: 0x" << (int)data << std::endl;
        } else {
            // --- STEP 2: Second Byte ---
            std::cout << "[VDP] Command Byte 2 received: 0x" << (int)data << std::endl;

            // Register Write?
            if (firstByteLatch & 0x80) { 
                uint8_t regIndex = firstByteLatch & 0x0F;
                if (regIndex < 8) {
                    registers[regIndex] = data;
                    std::cout << "[VDP] RESULT: Register " << (int)regIndex 
                              << " set to 0x" << (int)data << std::endl;
                }
            } 
            // Address Write?
            else {
                vramAddress = ((uint16_t)data << 8) | firstByteLatch;
                std::cout << "[VDP] RESULT: VRAM Address set to 0x" 
                          << std::hex << vramAddress << std::endl;
            }

            // Reset for next command
            firstByteLatch = 0;
            latchActive = false; // Mark as "Empty"
        }
    }
}

const std::vector<uint8_t>& TMS9918::GetFrameBuffer() const {
    return framebuffer;
}

// DumpVRAM routine
void TMS9918::DumpVRAM(uint16_t start, uint16_t length) const {
  std::cout << "--- VRAM Dump: 0x" << std::hex << start
            << " - 0x" << (start + length) << " ---" << std::endl;
  for (uint16_t i = 0; i < length; ++i) {
    if ((i % 16) == 0) {
      std::cout << std::hex << std::setw(4) << std::setfill('0') << (start + i) << ": ";
    }
    std::cout << std::hex << std::setw(2) << std::setfill('0')
              << static_cast<int>(vram[start + i]) << " ";

    if ((i % 16) == 15) std::cout << std::endl;
  }
  std::cout << std::dec << std::endl;
}
