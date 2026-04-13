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

	void load_boot_rom(const std::vector<u8>& boot_data) {
		mmu.load_boot_rom(boot_data);
		cpu.init_boot_rom();  // Zero out registers, start at PC=0x0000
	}

	void load_rom(const std::vector<u8>& rom_data) {
		mmu.load_rom(rom_data);
		// If no boot ROM was loaded, set the post-boot hardware state
		if (!mmu.has_boot_rom()) {
			mmu.set_post_boot_state();
		}
	}

	const std::array<u8, 160 * 144 * 4>& get_framebuffer() const {
		return ppu.get_framebuffer();
	}

	void run_frame() {
		int cycles_this_frame = 0;
		while (cycles_this_frame < 70224) {
			u8 cycles = cpu.step();
			timer.tick(cycles);
			ppu.tick(cycles);
			cpu.handle_interrupt();
			cycles_this_frame += cycles;
		}
	}
};