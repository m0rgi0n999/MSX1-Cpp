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

# Include directories
include_directories(src)

# Collect source files
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS 
    "src/*.cpp"
    "include/*.hpp"
)

# Create the executable
add_executable(msx-emulator ${SOURCES})

# If using a separate library structure later, you would use:
# add_library(msx-core ...)
# target_link_libraries(msx-emulator ...)

# Threads (often needed for audio/Video)
find_package(Threads REQUIRED)
target_link_libraries(msx-emulator Threads::Threads)

