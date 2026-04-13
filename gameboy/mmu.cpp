#include "mmu.h"
#include "joypad.h"

MMU::MMU(Joypad& jp) : joypad(jp) {
	memory.fill(0);
	boot_rom.fill(0);
}

bool MMU::has_boot_rom() const { return boot_rom_loaded; }
bool MMU::is_boot_rom_active() const { return boot_rom_active; }

void MMU::load_boot_rom(const std::vector<u8>& data) {
	if (data.size() >= 0x100) {
		std::copy(data.begin(), data.begin() + 0x100, boot_rom.begin());
		boot_rom_loaded = true;
		boot_rom_active = true;
	}
}

void MMU::load_rom(const std::vector<u8>& rom_data) {
	size_t size = std::min(rom_data.size(), static_cast<size_t>(0x8000));
	std::copy(rom_data.begin(), rom_data.begin() + size, memory.begin());
}

u8 MMU::read(u16 address) {
	// Boot ROM overlay: 0x0000-0x00FF reads from boot ROM while active
	if (boot_rom_active && address < 0x0100) {
		return boot_rom[address];
	}

	if (address == 0xFF00) {
		return joypad.read();
	}

	return memory[address];
}

void MMU::write_direct(u16 address, u8 value) {
	memory[address] = value;
}

void MMU::set_post_boot_state() {
	memory[0xFF40] = 0x91;  // LCDC
	memory[0xFF47] = 0xFC;  // BGP
}

void MMU::write(u16 address, u8 value) {
	if (address < 0x8000) {
		return;
	}

	if (address == 0xFF44) {
		memory[0xFF44] = 0;  // game writing to LY resets it
		return;
	}

	if (address == 0xFF04) {
		memory[address] = 0;
		return;
	}

	if (address == 0xFF00) {
		joypad.write(value);
		return;
	}

	if (address == 0xFF46) {
		u16 source = value << 8;
		for (u16 i = 0; i < 0xA0; i++) {
			memory[0xFE00 + i] = memory[source + i];
		}
		memory[address] = value;
		return;
	}

	// Boot ROM disable register
	if (address == 0xFF50 && boot_rom_active) {
		boot_rom_active = false;
		memory[address] = value;
		return;
	}

	memory[address] = value;

	if (address == 0xFF02 && value == 0x81) {
		std::cout << static_cast<char>(memory[0xFF01]);
	}
}