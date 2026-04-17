cmake_minimum_required(VERSION 3.20)
project(msx-emulator VERSION 0.1 LANGUAGES CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# Collect core source files
file(GLOB_RECURSE CORE_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.c"
)

# Exclude the application entry-point from the library sources
list(FILTER CORE_SOURCES EXCLUDE REGEX ".*/main\\.cpp$")

# Core library target
add_library(msx_emulator_core STATIC ${CORE_SOURCES})

target_include_directories(msx_emulator_core
    PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${CMAKE_SOURCE_DIR}/include
)

target_compile_features(msx_emulator_core PUBLIC cxx_std_20)

# Threads (often needed for audio/video)
find_package(Threads REQUIRED)

target_link_libraries(msx_emulator_core PUBLIC Threads::Threads)

# SDL2 for graphical output
find_package(SDL2 REQUIRED)

# Executable target
add_executable(msx-emulator src/main.cpp)

# Link everything to the executable
target_link_libraries(msx-emulator
    PRIVATE
        msx_emulator_core
        Threads::Threads
        ${SDL2_LIBRARIES}
)

# Set include directories for the executable
target_include_directories(msx-emulator
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS}
)

# --- End of File ---
