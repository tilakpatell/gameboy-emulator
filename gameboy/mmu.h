#pragma once
#include <iostream>
#include <array>
#include <vector>
#include "types.h"

class MMU {
private:
	std::array<u8, 0x10000> memory;

public:
	MMU() {
		memory.fill(0);
	}

	void load_rom(const std::vector<u8>& rom_data) {
		size_t size = std::min(rom_data.size(), static_cast<size_t>(0x8000));
		std::copy(rom_data.begin(), rom_data.begin() + size, memory.begin());
	}

	u8 read(u16 address) {
		return memory[address];
	}

	void write(u16 address, u8 value) {
		if (address < 0x8000) {
			return;
		}

		memory[address] = value;

		if (address == 0xFF02 && value == 0x81) {
			std::cout << static_cast<char>(memory[0xFF01]);
		}
	}
};