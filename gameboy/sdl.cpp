#include "sdl.h"
#include "joypad.h"
#include <iostream>

void Renderer::init() {
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

void Renderer::draw_frame(const std::array<u8, 160 * 144 * 4>& framebuffer) {
	SDL_UpdateTexture(texture, NULL, framebuffer.data(), 160 * 4);
	SDL_RenderClear(renderer);
	SDL_RenderTexture(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void Renderer::poll_events(Joypad& joypad) {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		if (event.type == SDL_EVENT_QUIT) {
			running = false;
		}
		if (event.type == SDL_EVENT_KEY_DOWN) {
			switch (event.key.key) {
			case SDLK_RIGHT: case SDLK_D: joypad.press(0); break;
			case SDLK_LEFT:  case SDLK_A: joypad.press(1); break;
			case SDLK_UP:    case SDLK_W: joypad.press(2); break;
			case SDLK_DOWN:  case SDLK_S: joypad.press(3); break;
			case SDLK_Z:     case SDLK_J: joypad.press(4); break;
			case SDLK_X:     case SDLK_K: joypad.press(5); break;
			case SDLK_BACKSPACE:           joypad.press(6); break;
			case SDLK_RETURN:              joypad.press(7); break;
			}
		}
		if (event.type == SDL_EVENT_KEY_UP) {
			switch (event.key.key) {
			case SDLK_RIGHT: case SDLK_D: joypad.release(0); break;
			case SDLK_LEFT:  case SDLK_A: joypad.release(1); break;
			case SDLK_UP:    case SDLK_W: joypad.release(2); break;
			case SDLK_DOWN:  case SDLK_S: joypad.release(3); break;
			case SDLK_Z:     case SDLK_J: joypad.release(4); break;
			case SDLK_X:     case SDLK_K: joypad.release(5); break;
			case SDLK_BACKSPACE:           joypad.release(6); break;
			case SDLK_RETURN:              joypad.release(7); break;
			}
		}
	}
}

bool Renderer::is_running() const { return running; }

void Renderer::cleanup() {
	if (texture) SDL_DestroyTexture(texture);
	if (renderer) SDL_DestroyRenderer(renderer);
	if (window) SDL_DestroyWindow(window);
	SDL_Quit();
}