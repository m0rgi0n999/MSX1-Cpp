#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <memory>

enum class FileType {
    ROM,
    CAS,
    DSK,
    UNKNOWN
};

class FileManager {
public:
    static FileType DetectFileType(const std::string& filename);

    // Load the whole file into a vector of bytes
    static bool LoadBinaryFile(const std::string& filename, std::vector<uint8_t>& buffer);

    // Generic loader for MSX media
    static std::vector<uint8_t> LoadROM(const std::string& filename);
    static std::vector<uint8_t> LoadCAS(const std::string& filename);
    static std::vector<uint8_t> LoadDSK(const std::string& filename); // DSK is usually 360KB or 720KB sectors
};

