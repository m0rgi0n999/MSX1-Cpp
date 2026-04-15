#include <iostream>
#include "core/Emulator.hpp"

int main() {
    Emulator emulator;

    // Use an absolute path for the BIOS ROM so the executable works
    // consistently regardless of the current working directory.
    emulator.LoadSystemROM("/home/erwin/source/source/8-bit/Z80/z80-Cpp/msx-emulator/assets/roms/zexall.com");

    std::cout << "Attempting to run a frame..." << std::endl;
    emulator.EnableOpcodeTrace(true);
    emulator.RunFrame();

    std::cout << "Frame execution finished." << std::endl;
    emulator.GetVDP().DumpVRAM(0x2000, 16); // Dump first 16 bytes
    return 0;
}

