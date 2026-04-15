#ifndef MEMORYMAPPER_HPP
#define MEMORYMAPPER_HPP

#include <vector>
#include <cstdint>
#include <array>

struct SlotDevice {
    std::vector<uint8_t> buffer;
    uint16_t start;
    uint16_t end;
    bool isReadOnly;
};

class MemoryMapper {
public:
    MemoryMapper();
    
    // MSX Port 0xA8: Primary Slot Select
    void WritePortA8(uint8_t value);
    uint8_t ReadPortA8() const { return primarySlotRegister; }

    // Memory Access
    uint8_t Read(uint16_t address);
    void Write(uint16_t address, uint8_t value);

    // Setup: Add a ROM or RAM block to a specific slot
    void InsertDevice(int slot, uint16_t start, uint16_t end, const std::vector<uint8_t>& data, bool readOnly);

private:
    uint8_t primarySlotRegister = 0x00;
    // 4 Slots, each can have multiple devices
    std::array<std::vector<SlotDevice>, 4> slots;
    
    int GetSlotForAddress(uint16_t address);
};

#endif