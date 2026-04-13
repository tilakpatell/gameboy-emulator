#pragma once

#include "types.h"

enum class Button {
	RIGHT = 0,
	LEFT = 1,
	UP = 2,
	DOWN = 3,
	A = 4,
	B = 5,
	SELECT = 6,
	START = 7
};

class Joypad {
private:
	u8 actions = 0;
	u8 directions = 0;
	u8 select = 0;
public:
	void write(u8 value) {
		select = value & 0x30;
	}

	u8 read() {
		u8 result = 0xFF;

		if (!(select & 0x10)) {
			result &= ~(directions & 0x0F);
		}
		if (!(select & 0x20)) {
			result &= ~(actions & 0x0F);
		}

		return (result & 0x0F) | select;
	}

	bool press(int button) {
		bool was_pressed;
		if (button < 4) {
			was_pressed = directions & (1 << button);
			directions |= (1 << button);
		}
		else {
			was_pressed = actions & (1 << (button - 4));
			actions |= (1 << (button - 4));
		}
		return !was_pressed; 
	}

	void release(int button) {
		if (button < 4) {
			directions &= ~(1 << button);
		}
		else {
			actions &= ~(1 << (button - 4));
		}
	}
};