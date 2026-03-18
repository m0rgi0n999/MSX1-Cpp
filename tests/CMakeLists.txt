cmake_minimum_required(VERSION 3.16)

# Build test executable
add_executable(test_z80
    test_z80.cpp
)

# Link test binary with emulator core + GoogleTest
target_link_libraries(test_z80 PRIVATE
    msx_emulator_core
    gtest_main
)

# Include project headers
target_include_directories(test_z80 PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Automatically register tests with CTest
include(GoogleTest)
gtest_discover_tests(test_z80)
