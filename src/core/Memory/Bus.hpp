#pragma once
#include <cstdint>
#include <vector>
#include <functional>

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
    void Write(uint16_t address, uint8_t data);

    // I/O Access
    uint8_t IO_Read(uint8_t port);
    void IO_Write(uint8_t port, uint8_t data);

    // Mapping
    void MapMemory(uint16_t start, uint16_t end, ReadMemFunc rFunc, WriteMemFunc wFunc);
    void MapIO(uint8_t port, ReadIOFunc r, WriteIOFunc w);

    // Utils
    void LoadToRAM(uint16_t startAddress, const std::vector<uint8_t>& data);

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

    std::vector<uint8_t> ram;
    std::vector<IOMapping> ioMap;
    std::vector<MemoryMapping> memoryMap;
};

