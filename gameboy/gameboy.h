#pragma once
#include "mmu.h"
#include "cpu.h"
#include "timer.h"
#include "ppu.h"
#include "joypad.h"
#include <vector>
#include <array>

class Gameboy {
private:
	Joypad joypad;
	MMU mmu;
	CPU cpu;
	Timer timer;
	PPU ppu;
public:
	Gameboy();

	Joypad& get_joypad();

	void load_boot_rom(const std::vector<u8>& boot_data);
	void load_rom(const std::vector<u8>& rom_data);

	const std::array<u8, 160 * 144 * 4>& get_framebuffer() const;

	void run_frame();
};