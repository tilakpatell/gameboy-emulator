cmake_minimum_required(VERSION 3.12)
project(gameboy)

set(CMAKE_CXX_STANDARD 20)

add_executable(gameboy
	main.cpp
	cpu.cpp)