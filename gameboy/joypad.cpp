#include "joypad.h"

void Joypad::write(u8 value) {
	select = value & 0x30;
}

u8 Joypad::read() {
	u8 result = 0xFF;

	if (!(select & 0x10)) {
		result &= ~(directions & 0x0F);
	}
	if (!(select & 0x20)) {
		result &= ~(actions & 0x0F);
	}

	return (result & 0x0F) | select;
}

bool Joypad::press(int button) {
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

void Joypad::release(int button) {
	if (button < 4) {
		directions &= ~(1 << button);
	}
	else {
		actions &= ~(1 << (button - 4));
	}
}