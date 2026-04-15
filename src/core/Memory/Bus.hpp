#pragma once
#include <cstdint>
#include <vector>
#include <functional>
#include "MemoryMapper.hpp"

class Bus {
public:
    Bus();
    ~Bus() = default;

    // --- Type Definitions (Scoped to Bus) ---
    // Keeps these distinct from Z80's types to prevent collisions
    using ReadMemFunc = std::function<uint8_t(uint16_t address)>;
    using WriteMemFunc = std::function<void(uint16_t address, uint8_t value)>;

    using ReadIOFunc = std::function<uint8_t(uint8_t port)>;
    using WriteIOFunc = std::function<void(uint8_t port, uint8_t data)>;

    // --- Public Methods ---

    // Memory Access
    uint8_t Read(uint16_t address);

    // I/O Access
    uint8_t IO_Read(uint8_t port);
    void IO_Write(uint8_t port, uint8_t data);

    // Mapping
    void MapMemory(uint16_t start, uint16_t end, ReadMemFunc rFunc, WriteMemFunc wFunc);
    void MapIO(uint8_t port, ReadIOFunc r, WriteIOFunc w);

    // Utils
    void LoadToRAM(uint16_t address, const std::vector<uint8_t>& data);

    void Write(uint16_t address, uint8_t value);

    // Add a proxy method so the outside world can configure slots
    void InsertDevice(int slot, uint16_t start, uint16_t end, const std::vector<uint8_t>& data, bool readOnly) {
        mapper.InsertDevice(slot, start, end, data, readOnly);
    }

private:
    // Internal Struct for IO Mapping
    struct IOMapping {
        uint8_t port;
        ReadIOFunc read;
        WriteIOFunc write;
    };

    // Internal Struct for Memory Mapping
    struct MemoryMapping {
        uint16_t start;
        uint16_t end;
        ReadMemFunc read;
        WriteMemFunc write;
    };

    std::vector<uint8_t> biosBuffer; // Temporary storage for BIOS data
    MemoryMapper mapper;
    std::vector<IOMapping> ioMap;
    std::vector<MemoryMapping> memoryMap;
};

