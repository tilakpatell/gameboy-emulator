#pragma once
#include "mmu.h"
#include "cpu.h"
#include "timer.h"
#include "ppu.h"
#include "joypad.h"

class Gameboy {
private:
	Joypad joypad;
	MMU mmu;
	CPU cpu;
	Timer timer;
	PPU ppu;
public:
	Gameboy() : joypad(), mmu(joypad), cpu(mmu), timer(mmu), ppu(mmu) {}

	Joypad& get_joypad() { return joypad; }

	void load_rom(const std::vector<u8>& rom_data) {
		mmu.load_rom(rom_data);
	}

	const std::array<u8, 160 * 144 * 4>& get_framebuffer() const {
		return ppu.get_framebuffer();
	}

	void run_frame() {
		int cycles_this_frame = 0;
		while (cycles_this_frame < 70224) { // Assuming 70224 cycles per frame for Gameboy
			u8 cycles = cpu.step();
			timer.tick(cycles); // Assuming each instruction takes 4 cycles for simplicity right now
			ppu.tick(cycles);
			cpu.handle_interrupt();
			cycles_this_frame += cycles;
		}
	}
};