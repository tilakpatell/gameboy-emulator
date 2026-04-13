#pragma once
#include <iostream>
#include <array>
#include <vector>
#include "types.h"

class Joypad;

class MMU {
private:
	std::array<u8, 0x10000> memory;
	std::array<u8, 0x100> boot_rom;
	bool boot_rom_loaded = false;
	bool boot_rom_active = false;
	Joypad& joypad;
public:
	MMU(Joypad& jp);

	bool has_boot_rom() const;
	bool is_boot_rom_active() const;

	void load_boot_rom(const std::vector<u8>& data);
	void load_rom(const std::vector<u8>& rom_data);

	u8 read(u16 address);
	void write_direct(u16 address, u8 value);
	void set_post_boot_state();
	void write(u16 address, u8 value);
};