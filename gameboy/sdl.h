#pragma once

#include "types.h"
#include "joypad.h"
#include <iostream>
#include <array>
#include <SDL3/SDL.h>

class Renderer {
private:
	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* texture = nullptr;
	bool running = false;

public:
	void init() {
		if (!SDL_Init(SDL_INIT_VIDEO)) {
			std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
			exit(1);
		}

		window = SDL_CreateWindow("Gameboy Emulator", 160 * 4, 144 * 4, 0);
		if (!window) {
			std::cerr << "Failed to create window: " << SDL_GetError() << std::endl;
			SDL_Quit();
			exit(1);
		}

		renderer = SDL_CreateRenderer(window, NULL);
		if (!renderer) {
			std::cerr << "Failed to create renderer: " << SDL_GetError() << std::endl;
			SDL_DestroyWindow(window);
			SDL_Quit();
			exit(1);
		}

		texture = SDL_CreateTexture(renderer,
			SDL_PIXELFORMAT_RGBA32,
			SDL_TEXTUREACCESS_STREAMING,
			160, 144);

		SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

		running = true;
	}

	void draw_frame(const std::array<u8, 160 * 144 * 4>& framebuffer) {
		SDL_UpdateTexture(texture, NULL, framebuffer.data(), 160 * 4);
		SDL_RenderClear(renderer);
		SDL_RenderTexture(renderer, texture, NULL, NULL);
		SDL_RenderPresent(renderer);
	}

	void poll_events(Joypad& joypad) {
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			if (event.type == SDL_EVENT_QUIT) {
				running = false;
			}
			if (event.type == SDL_EVENT_KEY_DOWN) {
				switch (event.key.key) {
				case SDLK_RIGHT:     joypad.press(0); break;
				case SDLK_LEFT:      joypad.press(1); break;
				case SDLK_UP:        joypad.press(2); break;
				case SDLK_DOWN:      joypad.press(3); break;
				case SDLK_Z:         joypad.press(4); break;
				case SDLK_X:         joypad.press(5); break;
				case SDLK_BACKSPACE: joypad.press(6); break;
				case SDLK_RETURN:    joypad.press(7); break;
				}
			}
			if (event.type == SDL_EVENT_KEY_UP) {
				switch (event.key.key) {
				case SDLK_RIGHT:     joypad.release(0); break;
				case SDLK_LEFT:      joypad.release(1); break;
				case SDLK_UP:        joypad.release(2); break;
				case SDLK_DOWN:      joypad.release(3); break;
				case SDLK_Z:         joypad.release(4); break;
				case SDLK_X:         joypad.release(5); break;
				case SDLK_BACKSPACE: joypad.release(6); break;
				case SDLK_RETURN:    joypad.release(7); break;
				}
			}
		}
	}

	bool is_running() const { return running; }

	void cleanup() {
		if (texture) SDL_DestroyTexture(texture);
		if (renderer) SDL_DestroyRenderer(renderer);
		if (window) SDL_DestroyWindow(window);
		SDL_Quit();
	}
};