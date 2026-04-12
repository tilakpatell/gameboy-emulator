#pragma once
#include "types.h"
#include <stdexcept>

class MMU;

enum class Flag : u8 {
	Zero = 1 << 7,
	Subtract = 1 << 6,
	HalfCarry = 1 << 5,
	Carry = 1 << 4
};

inline u8 operator&(u16 af, Flag flag) {
	return static_cast<u8>(af) & static_cast<u8>(flag);
};

inline void operator|=(u16& val, Flag f) {
	val |= static_cast<u8>(f);
}

inline u8 operator~(Flag f) {
	return ~static_cast<u8>(f);
}

struct Registers {
	u16 af, bc, de, hl = 0;
	u16 sp, pc = 0;

	u8 get_a() const { return (af >> 8) & 0xFF; }
	u8 get_b() const { return (bc >> 8) & 0xFF; }
	u8 get_d() const { return (de >> 8) & 0xFF; }
	u8 get_h() const { return (hl >> 8) & 0xFF; }
	u8 get_f() const { return af & 0xFF; }
	u8 get_c() const { return bc & 0xFF; }
	u8 get_e() const { return de & 0xFF; }
	u8 get_l() const { return hl & 0xFF; }

	void set_a(u8 value) { af = (af & 0x00FF) | (value << 8); }
	void set_b(u8 value) { bc = (bc & 0x00FF) | (value << 8); }
	void set_d(u8 value) { de = (de & 0x00FF) | (value << 8); }
	void set_h(u8 value) { hl = (hl & 0x00FF) | (value << 8); }
	void set_f(u8 value) { af = (af & 0xFF00) | (value & 0xF0); }
	void set_c(u8 value) { bc = (bc & 0xFF00) | value; }
	void set_e(u8 value) { de = (de & 0xFF00) | value; }
	void set_l(u8 value) { hl = (hl & 0xFF00) | value; }
};

class CPU {
private:
	Registers regs;
	MMU& mmu;
	bool ime;
	bool ime_pending;
	bool halted;

public:
	CPU(MMU& mmu);

	bool get_flag(Flag flag) const;
	void set_flag(Flag flag, bool value);

	u8 fetch_byte();
	u16 fetch_word();
	void push_stack(u16 value);
	u16 pop_stack();

	u8 get_reg_by_index(u8 index);
	void set_reg_by_index(u8 index, u8 value);
	void handle_interrupt();
	u8 step();
};