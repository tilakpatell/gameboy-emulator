#pragma once
#include "mmu.h"
#include "cpu.h"
#include "timer.h"

class Gameboy {
private:
	MMU mmu;
	CPU cpu;
	Timer timer;
public:
	Gameboy() : mmu(), cpu(mmu), timer(mmu) {}

	void load_rom(const std::vector<u8>& rom_data) {
		mmu.load_rom(rom_data);
	}

	void run() {
		while (true) {
			u8 cycles = cpu.step();
			timer.tick(cycles); // Assuming each instruction takes 4 cycles for simplicity right now
			cpu.handle_interrupt();
		}
	}
};