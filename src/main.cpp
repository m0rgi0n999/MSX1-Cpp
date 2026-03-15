#include <iostream>
#include "core/Emulator.hpp"

int main() {
    Emulator emulator;

    // Attempt to load a dummy BIOS (or real one if have it)
    // Create a dummy file first for testing
    // Or just initialize and reset.

    emulator.LoadSystemROM("assets/roms/zexall.com");

    std::cout << "Attempting to run a frame..." << std::endl;
    emulator.RunFrame();

    std::cout << "Frame execution finished." << std::endl;
    emulator.GetVDP().DumpVRAM(0x2000, 16); // Dump first 16 bytes
    return 0;
}

