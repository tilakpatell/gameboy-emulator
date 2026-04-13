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
	std::array<u8, 0x100> boot_rom;
	bool boot_rom_loaded = false;
	bool boot_rom_active = false;
	Joypad& joypad;
public:
	MMU(Joypad& jp) : joypad(jp) {
		memory.fill(0);
		boot_rom.fill(0);
		// Don't set post-boot defaults here; they'll be set when boot ROM
		// finishes (or by CPU constructor if no boot ROM is loaded).
	}

	bool has_boot_rom() const { return boot_rom_loaded; }
	bool is_boot_rom_active() const { return boot_rom_active; }

	void load_boot_rom(const std::vector<u8>& data) {
		if (data.size() >= 0x100) {
			std::copy(data.begin(), data.begin() + 0x100, boot_rom.begin());
			boot_rom_loaded = true;
			boot_rom_active = true;
		}
	}

	void load_rom(const std::vector<u8>& rom_data) {
		size_t size = std::min(rom_data.size(), static_cast<size_t>(0x8000));
		std::copy(rom_data.begin(), rom_data.begin() + size, memory.begin());
	}

	u8 read(u16 address) {
		// Boot ROM overlay: 0x0000-0x00FF reads from boot ROM while active
		if (boot_rom_active && address < 0x0100) {
			return boot_rom[address];
		}

		if (address == 0xFF00) {
			return joypad.read();
		}

		return memory[address];
	}

	void write_direct(u16 address, u8 value) {
		memory[address] = value;
	}

	// Set post-boot hardware register defaults (used when no boot ROM)
	void set_post_boot_state() {
		memory[0xFF40] = 0x91;  // LCDC
		memory[0xFF47] = 0xFC;  // BGP
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

		if (address == 0xFF46) {
			u16 source = value << 8;
			for (u16 i = 0; i < 0xA0; i++) {
				memory[0xFE00 + i] = memory[source + i];
			}
			memory[address] = value;
			return;
		}

		// Boot ROM disable register
		if (address == 0xFF50 && boot_rom_active) {
			boot_rom_active = false;
			memory[address] = value;
			return;
		}

		memory[address] = value;

		if (address == 0xFF02 && value == 0x81) {
			std::cout << static_cast<char>(memory[0xFF01]);
		}
	}
};