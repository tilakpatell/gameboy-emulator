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
		u8 actions = 0; // All buttons released (active low)
		u8 directions = 0; // D-pad bits (right, left, up, down)
		u8 select = 0; // Button select bits (A, B, Select, Start)
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

	void press(int button) {
		if (button < 4) {
			directions |= (1 << button);   // button 0 sets bit 0, button 3 sets bit 3
		}
		else {
			actions |= (1 << (button - 4)); // button 4 sets bit 0, button 7 sets bit 3
		}
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