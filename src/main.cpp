#include <iostream>
#include <fstream>
#include <vector>
#include "core/Emulator.hpp"

// Helper function to load binary files
std::vector<uint8_t> LoadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file: " + filename);
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
        return buffer;
    }
    return {};
}

int main() {
    try {
        Emulator emulator;
        
        // 1. Load the BIOS
        auto bios = LoadFile("/home/erwin/source/source/8-bit/Z80/z80-Cpp/msx-emulator/assets/roms/cbios_main_msx1.rom");
        
        // 2. Initialize (this maps the BIOS and resets the Z80)
        emulator.Initialize(bios);

        // 3. Run the frame (Added parentheses here)
        emulator.RunFrame();

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}