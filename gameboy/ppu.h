#pragma once

#include "types.h"
#include "mmu.h"
#include <array>

class MMU;

class PPU {
private:
	u16 counter = 0;
	MMU& mmu;
	u8 mode = 0;
	u8 current_line = 0;
	bool scanline_rendered = false;
	std::array<u8, 160 * 144 * 4> framebuffer; // Simple framebuffer for demonstration

public:
	PPU(MMU& mmu) : mmu(mmu) { framebuffer.fill(0); }

	void render_scanline() {

		u8 lcdc = mmu.read(0xFF40);
		if ((lcdc & 0x01) == 0) {
			framebuffer.fill(0); 
			return;
		}

		u16 tile_map_base;
		if (lcdc & 0x08) {
			tile_map_base = 0x9C00;
		}
		else {
			tile_map_base = 0x9800;
		}

		u8 scx = mmu.read(0xFF43); // Scroll X  (in the screen)
		u8 scy = mmu.read(0xFF42); // Scroll Y

		for (int i = 0; i < 160; i++) {
			u8 bg_scx = (i + scx) & 0xFF; // where it is in the background for x
			u8 bg_scy = (current_line + scy) & 0xFF; // where it is in the background for y 

			u8 tile_col_index = bg_scx / 8;
			u8 tile_row_index = bg_scy / 8;

			u16 map_addr = tile_map_base + (tile_row_index * 32) + tile_col_index;
			u8 tile_index = mmu.read(map_addr);

			u16 data_address = (lcdc & 0x10) ? (0x8000 + tile_index * 16) : (0x9000 + static_cast<i8>(tile_index) * 16);

			u8 tile_row = bg_scy % 8;  // which row within the 8x8 tile (0-7)
			u16 row_addr = data_address + (tile_row * 2);

			u8 byte1 = mmu.read(row_addr);
			u8 byte2 = mmu.read(row_addr + 1);

			u8 bit = 7 - (bg_scx % 8);
			u8 low = (byte1 >> bit) & 1;
			u8 high = (byte2 >> bit) & 1;
			u8 color_index = (high << 1) | low;

			u8 shade = (mmu.read(0xFF47) >> (color_index * 2)) & 0x03; // Get the shade for this color index

			const u8 shades[] = { 255, 170, 85, 0 };
			int fb_index = (current_line * 160 + i) * 4;
			framebuffer[fb_index + 0] = shades[shade];  // R
			framebuffer[fb_index + 1] = shades[shade];  // G
			framebuffer[fb_index + 2] = shades[shade];  // B
			framebuffer[fb_index + 3] = 255;             // A


		}


	}


	void tick(u8 cycles) {
		counter += cycles;
		
		if (current_line < 144) {
			if (counter <= 80) {
				mode = 2; // OAM Search
			}
			else if (counter <= 252) {
				mode = 3; // Pixel Transfer
				if (!scanline_rendered) {
					render_scanline();
					scanline_rendered = true;
				}
			}
			else {
				mode = 0; // HBlank
			}
		}
		else {
			mode = 1; // VBlank
		}

		u8 stat = mmu.read(0xFF41);
		mmu.write_direct(0xFF41, (stat & 0xFC) | mode);

		if (counter >= 456) {
			counter -= 456;
			current_line++;
			scanline_rendered = false;

			if (current_line == 144) {
				// Trigger VBlank interrupt
				mmu.write(0xFF0F, mmu.read(0xFF0F) | 0x01);
			}
			
			if (current_line > 153) {
				current_line = 0;
			}
			
			mmu.write_direct(0xFF44, current_line);

		}
	}

	const std::array<u8, 160 * 144 * 4>& get_framebuffer() const {
		return framebuffer;
	}
};
