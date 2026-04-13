#include "gameboy.h"
#include "sdl.h"
#include <fstream>
#include <vector>
#include <iostream>
#include <memory>
#include <chrono>
#include <thread>

int main(int argc, char* argv[]) {
	if (argc < 2) {
		std::cerr << "Usage: gameboy <rom_file> [boot_rom]" << std::endl;
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

	// Load boot ROM if provided as second argument
	if (argc >= 3) {
		std::ifstream boot_file(argv[2], std::ios::binary | std::ios::ate);
		if (!boot_file.is_open()) {
			std::cerr << "Failed to open boot ROM: " << argv[2] << std::endl;
			return 1;
		}
		auto boot_size = boot_file.tellg();
		boot_file.seekg(0, std::ios::beg);
		std::vector<u8> boot_data(boot_size);
		boot_file.read(reinterpret_cast<char*>(boot_data.data()), boot_size);
		boot_file.close();

		gb->load_boot_rom(boot_data);
		std::cout << "Boot ROM loaded (" << boot_size << " bytes)" << std::endl;
	}

	Renderer renderer;
	renderer.init();
	gb->load_rom(rom_data);
	while (renderer.is_running()) {
		auto frame_start = std::chrono::high_resolution_clock::now();
		gb->run_frame();
		renderer.draw_frame(gb->get_framebuffer());
		Joypad& joypad = gb->get_joypad();
		renderer.poll_events(joypad);

		auto frame_end = std::chrono::high_resolution_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(frame_end - frame_start);
		auto target = std::chrono::microseconds(16742); // ~59.7 FPS
		if (elapsed < target) {
			std::this_thread::sleep_for(target - elapsed);
		}
	}

	renderer.cleanup();

	return 0;
}