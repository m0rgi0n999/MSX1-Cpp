#include "Bus.hpp"
#include <algorithm>
#include <iostream>

Bus::Bus() {
    // Constructor
}

void Bus::LoadToRAM(uint16_t address, const std::vector<uint8_t>& data) {
    if (address == 0x0000) {
        biosBuffer = data;
    }
}

uint8_t Bus::Read(uint16_t address) {
    // Check our loaded BIOS first
    if (address < biosBuffer.size()) {
        return biosBuffer[address];
    }
    
    // Fallback to mapper
    return mapper.Read(address);
}

void Bus::Write(uint16_t address, uint8_t data) {
    // You usually can't write to ROM (0x0000-0x7FFF), 
    // but you can let the mapper decide
    mapper.Write(address, data);
}

void Bus::MapMemory(uint16_t start, uint16_t end, ReadMemFunc rFunc, WriteMemFunc wFunc) {
    memoryMap.push_back({start, end, rFunc, wFunc});
}

void Bus::MapIO(uint8_t port, ReadIOFunc r, WriteIOFunc w) {
    ioMap.push_back({port, r, w});
}

uint8_t Bus::IO_Read(uint8_t port) {
    for (const auto& mapping : ioMap) {
        if (mapping.port == port && mapping.read) {
            return mapping.read(port);
        }
    }
    return 0xFF; // Open bus returns 0xFF
}

void Bus::IO_Write(uint8_t port, uint8_t data) {
    // MSX CRITICAL: Port 0xA8 is the Primary Slot Register
    if (port == 0xA8) {
        mapper.WritePortA8(data);
        return; 
    }

    for (const auto& mapping : ioMap) {
        if (mapping.port == port && mapping.write) {
            mapping.write(port, data);
            return;
        }
    }
}
