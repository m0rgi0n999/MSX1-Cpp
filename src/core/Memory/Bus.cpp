#include "core/Memory/Bus.hpp"
#include <iostream>   // <--- NEEDED for std::cerr
#include <algorithm>  // <--- NEEDED for std::copy
#include <vector>
#include <functional>

Bus::Bus() {
    // Initialize 64KB of RAM
    ram.resize(64 * 1024, 0);
}

uint8_t Bus::Read(uint16_t address) {
    if (address < ram.size()) {
        return ram[address];
    }
    return 0;
}

void Bus::Write(uint16_t address, uint8_t data) {
    if (address < ram.size()) {
        ram[address] = data;
    }
}

// --- I/O Mapping ---

void Bus::MapIO(uint8_t port, ReadIOFunc r, WriteIOFunc w) {
    IOMapping mapping;
    mapping.port = port;
    mapping.read = r;
    mapping.write = w;
    ioMap.push_back(mapping);
}

uint8_t Bus::IO_Read(uint8_t port) {
    for (const auto& mapping : ioMap) {
        if (mapping.port == port && mapping.read) {
            return mapping.read(port);
        }
    }
    return 0xFF; // Default pull-up
}

void Bus::IO_Write(uint8_t port, uint8_t data) {
  // DIAGNOSTIC: Print every IO Write
  std::cout << "[BUS] Writing to port: 0x" << std::hex << static_cast<int>(port)
            << ", Data: 0x" << static_cast<int>(data) << std::endl;

    for (const auto& mapping : ioMap) {
        if (mapping.port == port && mapping.write) {
            mapping.write(port, data);
            return;
        }
    }
}

// --- Memory Mapping ---

void Bus::MapMemory(uint16_t start, uint16_t end, ReadMemFunc rFunc, WriteMemFunc wFunc) {
    MemoryMapping map;
    map.start = start;
    map.end = end;
    map.read = rFunc;
    map.write = wFunc;
    memoryMap.push_back(map);
}

void Bus::LoadToRAM(uint16_t startAddress, const std::vector<uint8_t>& data) {
    if (startAddress + data.size() > ram.size()) {
        std::cerr << "LoadToRAM: Data exceeds RAM size" << std::endl;
        return;
    }
    std::copy(data.begin(), data.end(), ram.begin() + startAddress);
}

