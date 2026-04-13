#pragma once

#include "types.h"
#include <array>
#include <SDL3/SDL.h>

class Joypad;

class Renderer {
private:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	bool running = false;

public:
	void init();
	void draw_frame(const std::array<u8, 160 * 144 * 4>& framebuffer);
	void poll_events(Joypad& joypad);
	bool is_running() const;
	void cleanup();
};