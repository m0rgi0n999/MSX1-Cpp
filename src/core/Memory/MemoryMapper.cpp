#include "core/Memory/MemoryMapper.hpp"

MemoryMapper::MemoryMapper() {
    primarySlotRegister = 0x00; // Default: All pages point to Slot 0
}

void MemoryMapper::WritePortA8(uint8_t value) {
    primarySlotRegister = value;
}

int MemoryMapper::GetSlotForAddress(uint16_t address) {
    // Page 0: 0000-3FFF, Page 1: 4000-7FFF, etc.
    int page = address / 0x4000;
    // Extract 2 bits for the specific page from Port A8
    return (primarySlotRegister >> (page * 2)) & 0x03;
}

uint8_t MemoryMapper::Read(uint16_t address) {
    int slotIdx = GetSlotForAddress(address);
    for (auto& dev : slots[slotIdx]) {
        if (address >= dev.start && address <= dev.end) {
            return dev.buffer[address - dev.start];
        }
    }
    return 0xFF; // Open bus returns 0xFF
}

void MemoryMapper::Write(uint16_t address, uint8_t value) {
    int slotIdx = GetSlotForAddress(address);
    for (auto& dev : slots[slotIdx]) {
        if (address >= dev.start && address <= dev.end) {
            if (!dev.isReadOnly) {
                dev.buffer[address - dev.start] = value;
            }
            return;
        }
    }
}

void MemoryMapper::InsertDevice(int slot, uint16_t start, uint16_t end, const std::vector<uint8_t>& data, bool readOnly) {
    slots[slot].push_back({data, start, end, readOnly});
}