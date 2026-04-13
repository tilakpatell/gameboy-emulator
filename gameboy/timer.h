#pragma once
#include "types.h"

class MMU;

class Timer {
private:
	MMU& mmu;
	u16 div = 0;
	u16 tima = 0;

public:
	Timer(MMU& mmu);

	u16 get_tima_rate(u8 tac);
	void tick(u8 cycles);
};