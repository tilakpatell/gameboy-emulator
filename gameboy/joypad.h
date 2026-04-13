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
	void write(u8 value);
	u8 read();
	bool press(int button);
	void release(int button);
};