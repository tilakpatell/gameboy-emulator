#pragma once

#include "types.h"
#include "mmu.h"
#include <array>

class MMU;

class PPU {
private:
	u16 counter = 0;
	MMU& mmu;
	u8 mode = 2;  // Start in OAM scan (mode 2) at line 0
	u8 current_line = 0;
	bool scanline_rendered = false;
	bool stat_irq_line = false;  // Previous STAT interrupt line state (for edge detection)
	std::array<u8, 160 * 144 * 4> framebuffer;
	std::array<u8, 160> bg_color_ids;  // per-pixel BG color ID for sprite priority

	// Evaluate combined STAT interrupt line and fire on rising edge
	void update_stat_irq() {
		u8 stat = mmu.read(0xFF41);
		u8 lyc = mmu.read(0xFF45);

		bool condition = false;
		// Mode 0 (HBlank) interrupt source
		if ((stat & 0x08) && mode == 0) condition = true;
		// Mode 1 (VBlank) interrupt source
		if ((stat & 0x10) && mode == 1) condition = true;
		// Mode 2 (OAM scan) interrupt source
		if ((stat & 0x20) && mode == 2) condition = true;
		// LYC=LY coincidence interrupt source
		if ((stat & 0x40) && current_line == lyc) condition = true;

		// Fire on rising edge only (0 → 1)
		if (condition && !stat_irq_line) {
			mmu.write(0xFF0F, mmu.read(0xFF0F) | 0x02);
		}
		stat_irq_line = condition;
	}

public:
	PPU(MMU& mmu) : mmu(mmu) {
		framebuffer.fill(0);
		bg_color_ids.fill(0);
		stat_irq_line = false;
	}

	void render_sprites() {
		u8 lcdc = mmu.read(0xFF40);
		if (!(lcdc & 0x02)) return;

		u8 sprite_height = (lcdc & 0x04) ? 16 : 8;

		struct SpriteEntry {
			int y;
			int x;
			u8 tile;
			u8 attrs;
			u8 oam_index;
		};

		std::array<SpriteEntry, 10> sprites_on_line;
		int sprite_count = 0;

		for (int i = 0; i < 40 && sprite_count < 10; i++) {
			u16 base = 0xFE00 + (i * 4);
			int sy = mmu.read(base) - 16;
			int sx = mmu.read(base + 1) - 8;

			if (current_line >= sy && current_line < sy + sprite_height) {
				sprites_on_line[sprite_count] = {
					sy, sx,
					mmu.read(base + 2),
					mmu.read(base + 3),
					static_cast<u8>(i)
				};
				sprite_count++;
			}
		}

		// Sort: lower X priority = drawn last (on top). Equal X: lower OAM index wins.
		for (int i = 0; i < sprite_count - 1; i++) {
			for (int j = i + 1; j < sprite_count; j++) {
				bool do_swap = false;
				if (sprites_on_line[i].x < sprites_on_line[j].x) {
					do_swap = true;
				}
				else if (sprites_on_line[i].x == sprites_on_line[j].x) {
					if (sprites_on_line[i].oam_index < sprites_on_line[j].oam_index) {
						do_swap = true;
					}
				}
				if (do_swap) {
					std::swap(sprites_on_line[i], sprites_on_line[j]);
				}
			}
		}

		for (int s = 0; s < sprite_count; s++) {
			SpriteEntry& spr = sprites_on_line[s];

			int row = current_line - spr.y;
			if (spr.attrs & 0x40) {
				row = (sprite_height - 1) - row;
			}

			u8 tile = spr.tile;
			if (sprite_height == 16) {
				tile &= 0xFE;
			}

			u16 tile_addr = 0x8000 + (tile * 16) + (row * 2);
			u8 byte1 = mmu.read(tile_addr);
			u8 byte2 = mmu.read(tile_addr + 1);

			u8 palette = (spr.attrs & 0x10) ? mmu.read(0xFF49) : mmu.read(0xFF48);

			for (int px = 0; px < 8; px++) {
				int screen_x = spr.x + px;
				if (screen_x < 0 || screen_x >= 160) continue;

				u8 bit = (spr.attrs & 0x20) ? px : 7 - px;

				u8 low = (byte1 >> bit) & 1;
				u8 high = (byte2 >> bit) & 1;
				u8 color_id = (high << 1) | low;

				if (color_id == 0) continue;

				// BG-over-OBJ priority: skip sprite pixel if BG color is non-zero
				if (spr.attrs & 0x80) {
					if (bg_color_ids[screen_x] != 0) continue;
				}

				u8 shade = (palette >> (color_id * 2)) & 0x03;
				const u8 shades[] = { 255, 170, 85, 0 };
				int fb_index = (current_line * 160 + screen_x) * 4;
				framebuffer[fb_index + 0] = shades[shade];
				framebuffer[fb_index + 1] = shades[shade];
				framebuffer[fb_index + 2] = shades[shade];
				framebuffer[fb_index + 3] = 255;
			}
		}
	}

	void render_scanline() {
		u8 lcdc = mmu.read(0xFF40);
		if ((lcdc & 0x01) == 0) {
			// BG disabled: fill line with white, color IDs all 0
			for (int i = 0; i < 160; i++) {
				int fb_index = (current_line * 160 + i) * 4;
				framebuffer[fb_index + 0] = 255;
				framebuffer[fb_index + 1] = 255;
				framebuffer[fb_index + 2] = 255;
				framebuffer[fb_index + 3] = 255;
				bg_color_ids[i] = 0;
			}
			render_sprites();
			return;
		}

		u16 bg_tile_map_base = (lcdc & 0x08) ? 0x9C00 : 0x9800;

		u8 scx = mmu.read(0xFF43);
		u8 scy = mmu.read(0xFF42);

		// Window registers
		bool window_enabled = (lcdc & 0x20) != 0;
		u8 wy = mmu.read(0xFF4A);
		u8 wx = mmu.read(0xFF4B);
		u16 win_tile_map_base = (lcdc & 0x40) ? 0x9C00 : 0x9800;

		for (int i = 0; i < 160; i++) {
			bool use_window = false;

			// Check if this pixel falls in the window
			if (window_enabled && current_line >= wy && i >= (wx - 7)) {
				use_window = true;
			}

			u16 tile_map_base;
			u8 map_x, map_y;

			if (use_window) {
				tile_map_base = win_tile_map_base;
				map_x = i - (wx - 7);
				map_y = current_line - wy;
			}
			else {
				tile_map_base = bg_tile_map_base;
				map_x = (i + scx) & 0xFF;
				map_y = (current_line + scy) & 0xFF;
			}

			u8 tile_col_index = map_x / 8;
			u8 tile_row_index = map_y / 8;

			u16 map_addr = tile_map_base + (tile_row_index * 32) + tile_col_index;
			u8 tile_index = mmu.read(map_addr);

			u16 data_address = (lcdc & 0x10)
				? (0x8000 + tile_index * 16)
				: (0x9000 + static_cast<i8>(tile_index) * 16);

			u8 tile_row = map_y % 8;
			u16 row_addr = data_address + (tile_row * 2);

			u8 byte1 = mmu.read(row_addr);
			u8 byte2 = mmu.read(row_addr + 1);

			u8 bit = 7 - (map_x % 8);
			u8 low = (byte1 >> bit) & 1;
			u8 high = (byte2 >> bit) & 1;
			u8 color_index = (high << 1) | low;

			// Store BG color ID for sprite priority
			bg_color_ids[i] = color_index;

			u8 shade = (mmu.read(0xFF47) >> (color_index * 2)) & 0x03;

			const u8 shades[] = { 255, 170, 85, 0 };
			int fb_index = (current_line * 160 + i) * 4;
			framebuffer[fb_index + 0] = shades[shade];
			framebuffer[fb_index + 1] = shades[shade];
			framebuffer[fb_index + 2] = shades[shade];
			framebuffer[fb_index + 3] = 255;
		}

		render_sprites();
	}

	void tick(u8 cycles) {
		counter += cycles;

		u8 old_mode = mode;

		if (current_line < 144) {
			if (counter <= 80) {
				mode = 2;  // OAM scan
			}
			else if (counter <= 252) {
				mode = 3;  // Pixel transfer
				if (!scanline_rendered) {
					render_scanline();
					scanline_rendered = true;
				}
			}
			else {
				mode = 0;  // HBlank
			}
		}
		else {
			mode = 1;  // VBlank
		}

		// Update STAT register mode bits
		u8 stat = mmu.read(0xFF41);
		mmu.write_direct(0xFF41, (stat & 0xFC) | mode);

		// Check for STAT interrupt on mode change
		if (mode != old_mode) {
			update_stat_irq();
		}

		if (counter >= 456) {
			counter -= 456;
			current_line++;
			scanline_rendered = false;

			if (current_line == 144) {
				mmu.write(0xFF0F, mmu.read(0xFF0F) | 0x01);  // VBlank interrupt
			}

			if (current_line > 153) {
				current_line = 0;
			}

			mmu.write_direct(0xFF44, current_line);

			// Update LY=LYC coincidence flag
			u8 lyc = mmu.read(0xFF45);
			u8 stat2 = mmu.read(0xFF41);
			if (current_line == lyc) {
				stat2 |= 0x04;   // set coincidence flag
			}
			else {
				stat2 &= ~0x04;  // clear coincidence flag
			}
			mmu.write_direct(0xFF41, (stat2 & 0xFC) | mode);

			// Re-evaluate STAT interrupt line (LYC match or new mode at line start)
			update_stat_irq();
		}
	}

	const std::array<u8, 160 * 144 * 4>& get_framebuffer() const {
		return framebuffer;
	}
};