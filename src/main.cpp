#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <SDL2/SDL.h>  // Added SDL2
#include "core/Emulator.hpp"

std::vector<uint8_t> LoadFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file.is_open()) throw std::runtime_error("Failed to open file: " + filename);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(size);
    file.read(reinterpret_cast<char*>(buffer.data()), size);
    return buffer;
}

int main(int argc, char* argv[]) {
    // 1. Initialize SDL2
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create window scaled up 3x (768x576)
    SDL_Window* window = SDL_CreateWindow("MSX Emulator (C-BIOS)", 
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
        256 * 3, 192 * 3, SDL_WINDOW_SHOWN);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // This texture holds the raw MSX pixels (256x192)
    SDL_Texture* texture = SDL_CreateTexture(renderer, 
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 192);

    try {
        Emulator emulator;
        auto bios = LoadFile("assets/roms/cbios_main_msx1.rom");
        emulator.Initialize(bios);
        emulator.EnableOpcodeTrace(false);

        std::vector<uint32_t> pixelBuffer(256 * 192, 0);
        bool running = true;
        SDL_Event e;
        uint32_t frameCount = 0;

        while (running) {
            // 2. Handle Events (keeps window responsive)
            while (SDL_PollEvent(&e)) {
                if (e.type == SDL_QUIT) running = false;
            }

            // 3. Run Emulation
            emulator.RunFrame();

            // 4. Render to Buffer
            // Note: Ensure GetVDP() is public in Emulator.hpp!
            emulator.GetVDP().RenderScreen0(pixelBuffer.data());

            // 5. Update Texture and Draw
            SDL_UpdateTexture(texture, nullptr, pixelBuffer.data(), 256 * sizeof(uint32_t));
            
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, nullptr, nullptr); // Scaled automatically
            SDL_RenderPresent(renderer);

            // Keep the terminal dump for now just to double-check
            if (frameCount++ % 60 == 0) {
                std::cout << "MSX Frame: " << frameCount << " (Rendering to Window)" << std::endl;
            }

            // Sync to ~60FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    // Cleanup
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
