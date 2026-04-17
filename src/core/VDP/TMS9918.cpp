#include "TMS9918.hpp"
#include <iostream>
#include <iomanip>

TMS9918::TMS9918() 
    : statusReg(0), 
      currentAddress(0), 
      addressBuffer(0), 
      addressLatch(false) 
{
    vram.resize(16 * 1024, 0);        // 16KB VRAM
    framebuffer.resize(256 * 192, 0); // Standard TMS resolution
    for(int i=0; i<8; ++i) registers[i] = 0;
}

// Port 0x99: Control/Address/Register writes
void TMS9918::WriteControl(uint8_t data) {
    if (!addressLatch) {
        // First byte: Store low 8 bits
        addressBuffer = data;
        addressLatch = true;
    } else {
        // Second byte: 
        // Bits 0-5: High 6 bits of address
        // Bits 6-7: Mode (00=Read, 01=Write VRAM, 10=Reg Write)
        uint8_t mode = (data >> 6) & 0x03;
        
        // Combine addressBuffer (low) and data (high 6 bits)
        currentAddress = ((static_cast<uint16_t>(data) & 0x3F) << 8) | addressBuffer;

        if (mode == 0x02) { // 10 in binary: Register Write
            uint8_t regNum = data & 0x07;
            registers[regNum] = addressBuffer;
        }
        
        // IMPORTANT: In MSX, any write to Port 0x99 resets the latch
        addressLatch = false;
        
        // Debug to verify address setup
        // std::cout << "VDP Address set to: " << std::hex << currentAddress << std::dec << " Mode: " << (int)mode << std::endl;
    }
}

// Port 0x98: Data writes (with auto-increment)
void TMS9918::WriteData(uint8_t data) {
    //std::cout << "DEBUG: VDP Writing " << std::hex << (int)data << " to " << currentAddress << std::dec << std::endl;
    vram[currentAddress & 0x3FFF] = data;
    currentAddress++;
    addressLatch = false;
}

// Port 0x98: Data reads
uint8_t TMS9918::ReadData() {
    uint8_t data = vram[currentAddress & 0x3FFF];
    currentAddress++;
    addressLatch = false;
    return data;
}

// Port 0x99: Status reads
uint8_t TMS9918::ReadStatus() {
    uint8_t status = statusReg;
    // Reading status clears the top bits (Interrupt and Collision flags)
    statusReg &= 0x1F; 
    addressLatch = false;
    return status;
}

void TMS9918::Update(uint32_t cpuCycles) {
    // Placeholder for future scanline/timing logic
    // Once the "Frame" is done (e.g. after 71,591 cycles):
    statusReg |= 0x80; // Set Bit 7 (The Frame/Interrupt Flag)
}

const std::vector<uint8_t>& TMS9918::GetFrameBuffer() const {
    return framebuffer;
}

void TMS9918::DumpVRAM(uint16_t start, uint16_t length) const {
    std::cout << "--- VRAM Dump: 0x" << std::hex << start << " ---" << std::endl;
    for (uint16_t i = 0; i < length; ++i) {
        if ((start + i) >= vram.size()) break;
        std::cout << std::setw(2) << std::setfill('0') << (int)vram[start + i] << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::dec << std::endl;
}

void TMS9918::DebugPrintScreen() {
    printf("\n--- VDP SCREEN DUMP (Screen 0) ---\n");
    // Name Table for Screen 0 is typically at 0x1800
    // It's 40 columns x 24 rows
    for (int y = 0; y < 24; y++) {
        for (int x = 0; x < 40; x++) {
            uint16_t addr = 0x1800 + (y * 40) + x;
            uint8_t c = vram[addr];
            // Only print printable ASCII, otherwise print a space
            if (c >= 32 && c <= 126) {
                printf("%c", c);
            } else {
                printf(" ");
            }
        }
        printf("\n");
    }
    printf("----------------------------------\n");
}

void TMS9918::RenderScreen0(uint32_t* pixelBuffer) {
  // Screen 0 is 40x24 characters
  for  (int row = 0; row < 24; row++) {
    for (int col = 0; col < 40; col++) {
      uint8_t charcode = vram[0x1800 + (row *40) + col];

      // Draw the 8x8 bitmap for this characters
      for (int y = 0; y < 8; y++) {
        // Patterns usually start at 0x0000 in VRAM
        uint8_t patternByte = vram[0x0000 + (charcode * 8) + y];

        for (int x = 0; x < 8; x++) {
          bool pixelOn = (patternByte << x) & 0x80;
          uint32_t color = pixelOn ? 0xFFFFFFFF : 0xFF0000FF; // White on Blue

          // Calculate buffer position (256x192 resolution)
          int screenX = (col * 6) + x; // Screen 0 is actually 6 pixels wide per character
          int screenY = (row * 8) + y;
          pixelBuffer[screenY * 256 + screenX] = color;
        }
      }
    }
  }
}
