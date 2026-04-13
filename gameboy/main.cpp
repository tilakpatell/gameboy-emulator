#include "gameboy.h"
#include "sdl.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <memory>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: gameboy <rom_file>" << std::endl;
		return 1;
	}

	std::ifstream file(argv[1], std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		std::cerr << "Failed to open ROM: " << argv[1] << std::endl;
		return 1;
	}

	auto size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<u8> rom_data(size);
	file.read(reinterpret_cast<char*>(rom_data.data()), size);
	file.close();


	auto gb = std::make_unique<Gameboy>();
	Renderer renderer;
	renderer.init();
	gb->load_rom(rom_data);
	while (renderer.is_running()) {
		gb->run_frame();
		renderer.draw_frame(gb->get_framebuffer());
		Joypad& joypad = gb->get_joypad();
		renderer.poll_events(joypad);
	}

	renderer.cleanup();

	return 0;
}