// CORRECTED: Path is relative to the 'src' folder
#include "io/FileManager.hpp"
#include <fstream>
#include <vector>

bool FileManager::LoadBinaryFile(const std::string& filename, std::vector<uint8_t>& buffer) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        return false;
    }
    file.unsetf(std::ios::skipws);
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    buffer.resize(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    return true;
}

std::vector<uint8_t> FileManager::LoadROM(const std::string& filename) {
    std::vector<uint8_t> buffer;
    if (LoadBinaryFile(filename, buffer)) {
        return buffer;
    }
    return {};
}

// Add stubs for CAS and DSK similarly
std::vector<uint8_t> FileManager::LoadCAS(const std::string& filename) { return {}; }
std::vector<uint8_t> FileManager::LoadDSK(const std::string& filename) { return {}; }

