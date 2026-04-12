#pragma once
#include "mmu.h"
#include "cpu.h"

class Gameboy {
private:
	MMU mmu;
	CPU cpu;
public:
	Gameboy() : mmu(), cpu(mmu) {}

	void load_rom(const std::vector<u8>& rom_data) {
		mmu.load_rom(rom_data);
	}

	void run() {
		while (true) {
			cpu.step();
			cpu.handle_interrupt();
		}
	}
};