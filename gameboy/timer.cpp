#include "timer.h"
#include "mmu.h"

Timer::Timer(MMU& mmu) : mmu(mmu) {}

u16 Timer::get_tima_rate(u8 tac) {
	switch (tac & 0x03) {
	case 0: return 1024;
	case 1: return 16;
	case 2: return 64;
	case 3: return 256;
	}
	return 1024;
}

void Timer::tick(u8 cycles) {
	div += cycles;
	if (div >= 256) {
		div -= 256;
		mmu.write_direct(0xFF04, (mmu.read(0xFF04) + 1) & 0xFF);
	}
	u8 tac = mmu.read(0xFF07);
	if (tac & 0x04) {
		u16 threshold = get_tima_rate(tac);
		tima += cycles;
		if (tima >= threshold) {
			tima -= threshold;
			u8 new_tima = mmu.read(0xFF05) + 1;
			if (new_tima == 0) {
				mmu.write_direct(0xFF05, mmu.read(0xFF06));
				mmu.write(0xFF0F, mmu.read(0xFF0F) | 0x04);
			}
			else {
				mmu.write_direct(0xFF05, new_tima);
			}
		}
	}
}