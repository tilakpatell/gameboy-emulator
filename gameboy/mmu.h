#pragma once
#include <iostream>
#include <array>
#include <vector>
#include "types.h"
#include "joypad.h"

class Joypad;

class MMU {
private:
	std::array<u8, 0x10000> memory;
	Joypad& joypad;	
public:
	MMU(Joypad& jp) : joypad(jp) {
		memory.fill(0);
		memory[0xFF40] = 0x91;  // LCDC post-boot value
		memory[0xFF47] = 0xFC;
	}

	void load_rom(const std::vector<u8>& rom_data) {
		size_t size = std::min(rom_data.size(), static_cast<size_t>(0x8000));
		std::copy(rom_data.begin(), rom_data.begin() + size, memory.begin());
	}

	u8 read(u16 address) {

		if (address == 0xFF00) {
			return joypad.read();
		}

		return memory[address];
	}

	void write_direct(u16 address, u8 value) {
		memory[address] = value;
	}

	void write(u16 address, u8 value) {
		if (address < 0x8000) {
			return;
		}

		if (address == 0xFF44) {
			memory[0xFF44] = 0;  // game writing to LY resets it
			return;
		}

		if (address == 0xFF04) {
			memory[address] = 0;
			return;
		}

		if (address == 0xFF00) {
			joypad.write(value);
			return;
		}

		memory[address] = value;

		if (address == 0xFF02 && value == 0x81) {
			std::cout << static_cast<char>(memory[0xFF01]);
		}
	}
};