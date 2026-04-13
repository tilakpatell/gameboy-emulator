#include "gameboy.h"

Gameboy::Gameboy() : joypad(), mmu(joypad), cpu(mmu), timer(mmu), ppu(mmu) {}

Joypad& Gameboy::get_joypad() { return joypad; }

void Gameboy::load_boot_rom(const std::vector<u8>& boot_data) {
	mmu.load_boot_rom(boot_data);
	cpu.init_boot_rom();
}

void Gameboy::load_rom(const std::vector<u8>& rom_data) {
	mmu.load_rom(rom_data);
	if (!mmu.has_boot_rom()) {
		mmu.set_post_boot_state();
	}
}

const std::array<u8, 160 * 144 * 4>& Gameboy::get_framebuffer() const {
	return ppu.get_framebuffer();
}

void Gameboy::run_frame() {
	int cycles_this_frame = 0;
	while (cycles_this_frame < 70224) {
		u8 cycles = cpu.step();
		timer.tick(cycles);
		ppu.tick(cycles);
		cpu.handle_interrupt();
		cycles_this_frame += cycles;
	}
}