cmake_minimum_required(VERSION 3.12)
project(gameboy)

set(CMAKE_CXX_STANDARD 20)

list(APPEND CMAKE_PREFIX_PATH "C:/Users/tilak/libs/SDL3-3.4.4/cmake")

find_package(SDL3 REQUIRED)

add_executable(gameboy
    main.cpp
    cpu.cpp
    mmu.cpp
    ppu.cpp
    timer.cpp
    joypad.cpp
    sdl.cpp
    gameboy.cpp
)

target_link_libraries(gameboy SDL3::SDL3)
target_include_directories(gameboy PRIVATE "C:/Users/tilak/libs/SDL3-3.4.4/include")

add_custom_command(TARGET gameboy POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:SDL3::SDL3>
    $<TARGET_FILE_DIR:gameboy>
)