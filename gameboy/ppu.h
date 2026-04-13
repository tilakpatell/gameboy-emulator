#pragma once

#include "types.h"
#include <array>

class MMU;

class PPU {
private:
	u16 counter = 0;
	MMU& mmu;
	u8 mode = 2;
	u8 current_line = 0;
	bool scanline_rendered = false;
	bool stat_irq_line = false;
	std::array<u8, 160 * 144 * 4> framebuffer;
	std::array<u8, 160> bg_color_ids;

	void update_stat_irq();

public:
	PPU(MMU& mmu);

	void render_sprites();
	void render_scanline();
	void tick(u8 cycles);

	const std::array<u8, 160 * 144 * 4>& get_framebuffer() const;
};