#pragma once
#include <cstdint>
#include <array>

class AY8910 {
public:
    AY8910();
    ~AY8910() = default;

    // Run emulation for N cycles
    void Tick(uint32_t cycles);

    // I/O interfaces
    uint8_t Read(uint8_t port);
    void Write(uint8_t port, uint8_t data);

    // Get current sample (for audio output)
    int16_t GetSample();

private:
    std::array<uint8_t, 16> registers;
    // Envelopes, counters, and tone generator state would go here
};

