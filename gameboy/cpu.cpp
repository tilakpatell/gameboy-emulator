#include "cpu.h"
#include "mmu.h"

CPU::CPU(MMU& mmu) : mmu(mmu), ime(false), ime_pending(false), halted(false) {
	regs.af = 0x01B0;
	regs.bc = 0x0013;
	regs.de = 0x00D8;
	regs.hl = 0x014D;
	regs.sp = 0xFFFE;
	regs.pc = 0x0100;
}

bool CPU::get_flag(Flag flag) const {
	return (regs.get_f() & static_cast<u8>(flag)) != 0;
}

void CPU::set_flag(Flag flag, bool value) {
	if (value) {
		regs.set_f(regs.get_f() | static_cast<u8>(flag));
	}
	else {
		regs.set_f(regs.get_f() & ~static_cast<u8>(flag));
	}
}

u8 CPU::fetch_byte() {
	u8 byte = mmu.read(regs.pc);
	regs.pc++;
	return byte;
}

u16 CPU::fetch_word() {
	u8 lower_byte = fetch_byte();
	u8 higher_byte = fetch_byte();
	return (higher_byte << 8) | lower_byte;
}

void CPU::push_stack(u16 value) {
	regs.sp -= 2;
	mmu.write(regs.sp, value & 0xFF);
	mmu.write(regs.sp + 1, value >> 8);
}

u16 CPU::pop_stack() {
	u16 value = mmu.read(regs.sp) | (mmu.read(regs.sp + 1) << 8);
	regs.sp += 2;
	return value;
}

u8 CPU::get_reg_by_index(u8 index) {
	switch (index) {
	case 0: return regs.get_b();
	case 1: return regs.get_c();
	case 2: return regs.get_d();
	case 3: return regs.get_e();
	case 4: return regs.get_h();
	case 5: return regs.get_l();
	case 6: return mmu.read(regs.hl);
	case 7: return regs.get_a();
	default: return 0xFF;
	}
}

void CPU::set_reg_by_index(u8 index, u8 value) {
	switch (index) {
	case 0: regs.set_b(value); break;
	case 1: regs.set_c(value); break;
	case 2: regs.set_d(value); break;
	case 3: regs.set_e(value); break;
	case 4: regs.set_h(value); break;
	case 5: regs.set_l(value); break;
	case 6: mmu.write(regs.hl, value); break;
	case 7: regs.set_a(value); break;
	}
}

void CPU::handle_interrupt() {
	u8 if_bit = mmu.read(0xFF0F);
	u8 ie_bit = mmu.read(0xFFFF);

	if (ie_bit & if_bit & 0x1F) {
		if (halted) {
			halted = false;
		}

		if (!ime) {
			return;
		}

		for (int i = 0; i < 5; i++) {
			if ((if_bit & ie_bit) & (1 << i)) {
				ime = false;
				mmu.write(0xFF0F, if_bit & ~(1 << i));
				push_stack(regs.pc);
				regs.pc = 0x0040 + (i * 0x08);
				break;
			}
		}
	}
}

u8 CPU::step() {
	if (halted) {
		return 4;
	}

	if (ime_pending) {
		ime = true;
		ime_pending = false;
	}

	u8 opcode = fetch_byte();

	switch (opcode) {

		// 0x00 - 0x0F
	case 0x00: return 4;  // NOP
	case 0x01: regs.bc = fetch_word(); return 12;
	case 0x02: mmu.write(regs.bc, regs.get_a()); return 8;
	case 0x03: regs.bc++; return 8;
	case 0x04: {
		regs.set_b(regs.get_b() + 1);
		set_flag(Flag::Zero, regs.get_b() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_b() & 0x0F) == 0x00);
		return 4;
	}
	case 0x05: {
		regs.set_b(regs.get_b() - 1);
		set_flag(Flag::Zero, regs.get_b() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_b() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x06: regs.set_b(fetch_byte()); return 8;
	case 0x07: {
		u8 old_a = regs.get_a();
		u8 bit7 = (old_a >> 7) & 1;
		regs.set_a((old_a << 1) | bit7);
		set_flag(Flag::Carry, bit7);
		set_flag(Flag::Zero, false);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, false);
		return 4;
	}
	case 0x08: {
		u16 addr = fetch_word();
		mmu.write(addr, regs.sp & 0xFF);
		mmu.write(addr + 1, regs.sp >> 8);
		return 20;
	}
	case 0x09: {
		u16 old_hl = regs.hl;
		regs.hl += regs.bc;
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (regs.bc & 0x0FFF)) > 0x0FFF);
		set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(regs.bc)) > 0xFFFF);
		return 8;
	}
	case 0x0A: regs.set_a(mmu.read(regs.bc)); return 8;
	case 0x0B: regs.bc--; return 8;
	case 0x0C: {
		regs.set_c(regs.get_c() + 1);
		set_flag(Flag::Zero, regs.get_c() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_c() & 0x0F) == 0x00);
		return 4;
	}
	case 0x0D: {
		regs.set_c(regs.get_c() - 1);
		set_flag(Flag::Zero, regs.get_c() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_c() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x0E: regs.set_c(fetch_byte()); return 8;
	case 0x0F: {
		u8 old_a = regs.get_a();
		u8 bit0 = old_a & 1;
		regs.set_a((old_a >> 1) | (bit0 << 7));
		set_flag(Flag::Carry, bit0);
		set_flag(Flag::Zero, false);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, false);
		return 4;
	}

			 // 0x10 - 0x1F
	case 0x10: halted = true; return 4;
	case 0x11: regs.de = fetch_word(); return 12;
	case 0x12: mmu.write(regs.de, regs.get_a()); return 8;
	case 0x13: regs.de++; return 8;
	case 0x14: {
		regs.set_d(regs.get_d() + 1);
		set_flag(Flag::Zero, regs.get_d() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_d() & 0x0F) == 0x00);
		return 4;
	}
	case 0x15: {
		regs.set_d(regs.get_d() - 1);
		set_flag(Flag::Zero, regs.get_d() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_d() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x16: regs.set_d(fetch_byte()); return 8;
	case 0x17: {
		u8 old_a = regs.get_a();
		u8 old_carry = get_flag(Flag::Carry) ? 1 : 0;
		regs.set_a((old_a << 1) | old_carry);
		set_flag(Flag::Carry, (old_a >> 7) & 1);
		set_flag(Flag::Zero, false);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, false);
		return 4;
	}
	case 0x18: {
		i8 offset = static_cast<i8>(fetch_byte());
		regs.pc += offset;
		return 12;
	}
	case 0x19: {
		u16 old_hl = regs.hl;
		regs.hl += regs.de;
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (regs.de & 0x0FFF)) > 0x0FFF);
		set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(regs.de)) > 0xFFFF);
		return 8;
	}
	case 0x1A: regs.set_a(mmu.read(regs.de)); return 8;
	case 0x1B: regs.de--; return 8;
	case 0x1C: {
		regs.set_e(regs.get_e() + 1);
		set_flag(Flag::Zero, regs.get_e() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_e() & 0x0F) == 0x00);
		return 4;
	}
	case 0x1D: {
		regs.set_e(regs.get_e() - 1);
		set_flag(Flag::Zero, regs.get_e() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_e() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x1E: regs.set_e(fetch_byte()); return 8;
	case 0x1F: {
		u8 old_a = regs.get_a();
		u8 old_carry = get_flag(Flag::Carry) ? 1 : 0;
		regs.set_a((old_a >> 1) | (old_carry << 7));
		set_flag(Flag::Carry, old_a & 1);
		set_flag(Flag::Zero, false);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, false);
		return 4;
	}

			 // 0x20 - 0x2F
	case 0x20: {
		i8 offset = static_cast<i8>(fetch_byte());
		if (!get_flag(Flag::Zero)) { regs.pc += offset; return 12; }
		return 8;
	}
	case 0x21: regs.hl = fetch_word(); return 12;
	case 0x22: mmu.write(regs.hl, regs.get_a()); regs.hl++; return 8;
	case 0x23: regs.hl++; return 8;
	case 0x24: {
		regs.set_h(regs.get_h() + 1);
		set_flag(Flag::Zero, regs.get_h() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_h() & 0x0F) == 0x00);
		return 4;
	}
	case 0x25: {
		regs.set_h(regs.get_h() - 1);
		set_flag(Flag::Zero, regs.get_h() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_h() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x26: regs.set_h(fetch_byte()); return 8;
	case 0x27: {
		u8 a = regs.get_a();
		if (!get_flag(Flag::Subtract)) {
			if (get_flag(Flag::Carry) || a > 0x99) { a += 0x60; set_flag(Flag::Carry, true); }
			if (get_flag(Flag::HalfCarry) || (a & 0x0F) > 0x09) { a += 0x06; }
		}
		else {
			if (get_flag(Flag::Carry)) { a -= 0x60; }
			if (get_flag(Flag::HalfCarry)) { a -= 0x06; }
		}
		regs.set_a(a);
		set_flag(Flag::Zero, a == 0);
		set_flag(Flag::HalfCarry, false);
		return 4;
	}
	case 0x28: {
		i8 offset = static_cast<i8>(fetch_byte());
		if (get_flag(Flag::Zero)) { regs.pc += offset; return 12; }
		return 8;
	}
	case 0x29: {
		u16 old_hl = regs.hl;
		regs.hl += regs.hl;
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (old_hl & 0x0FFF)) > 0x0FFF);
		set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(old_hl)) > 0xFFFF);
		return 8;
	}
	case 0x2A: regs.set_a(mmu.read(regs.hl)); regs.hl++; return 8;
	case 0x2B: regs.hl--; return 8;
	case 0x2C: {
		regs.set_l(regs.get_l() + 1);
		set_flag(Flag::Zero, regs.get_l() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_l() & 0x0F) == 0x00);
		return 4;
	}
	case 0x2D: {
		regs.set_l(regs.get_l() - 1);
		set_flag(Flag::Zero, regs.get_l() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_l() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x2E: regs.set_l(fetch_byte()); return 8;
	case 0x2F:
		regs.set_a(~regs.get_a());
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, true);
		return 4;

		// 0x30 - 0x3F
	case 0x30: {
		i8 offset = static_cast<i8>(fetch_byte());
		if (!get_flag(Flag::Carry)) { regs.pc += offset; return 12; }
		return 8;
	}
	case 0x31: regs.sp = fetch_word(); return 12;
	case 0x32: mmu.write(regs.hl, regs.get_a()); regs.hl--; return 8;
	case 0x33: regs.sp++; return 8;
	case 0x34: {
		u8 val = mmu.read(regs.hl); val++;
		mmu.write(regs.hl, val);
		set_flag(Flag::Zero, val == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (val & 0x0F) == 0x00);
		return 12;
	}
	case 0x35: {
		u8 val = mmu.read(regs.hl); val--;
		mmu.write(regs.hl, val);
		set_flag(Flag::Zero, val == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (val & 0x0F) == 0x0F);
		return 12;
	}
	case 0x36: mmu.write(regs.hl, fetch_byte()); return 12;
	case 0x37:
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, false);
		set_flag(Flag::Carry, true);
		return 4;
	case 0x38: {
		i8 offset = static_cast<i8>(fetch_byte());
		if (get_flag(Flag::Carry)) { regs.pc += offset; return 12; }
		return 8;
	}
	case 0x39: {
		u16 old_hl = regs.hl;
		regs.hl += regs.sp;
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (regs.sp & 0x0FFF)) > 0x0FFF);
		set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(regs.sp)) > 0xFFFF);
		return 8;
	}
	case 0x3A: regs.set_a(mmu.read(regs.hl)); regs.hl--; return 8;
	case 0x3B: regs.sp--; return 8;
	case 0x3C: {
		regs.set_a(regs.get_a() + 1);
		set_flag(Flag::Zero, regs.get_a() == 0);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, (regs.get_a() & 0x0F) == 0x00);
		return 4;
	}
	case 0x3D: {
		regs.set_a(regs.get_a() - 1);
		set_flag(Flag::Zero, regs.get_a() == 0);
		set_flag(Flag::Subtract, true);
		set_flag(Flag::HalfCarry, (regs.get_a() & 0x0F) == 0x0F);
		return 4;
	}
	case 0x3E: regs.set_a(fetch_byte()); return 8;
	case 0x3F:
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, false);
		set_flag(Flag::Carry, !get_flag(Flag::Carry));
		return 4;

		// 0x40 - 0x7F: LD r,r (4 cycles, 8 if (HL) involved)
	case 0x40: return 4;
	case 0x41: regs.set_b(regs.get_c()); return 4;
	case 0x42: regs.set_b(regs.get_d()); return 4;
	case 0x43: regs.set_b(regs.get_e()); return 4;
	case 0x44: regs.set_b(regs.get_h()); return 4;
	case 0x45: regs.set_b(regs.get_l()); return 4;
	case 0x46: regs.set_b(mmu.read(regs.hl)); return 8;
	case 0x47: regs.set_b(regs.get_a()); return 4;
	case 0x48: regs.set_c(regs.get_b()); return 4;
	case 0x49: return 4;
	case 0x4A: regs.set_c(regs.get_d()); return 4;
	case 0x4B: regs.set_c(regs.get_e()); return 4;
	case 0x4C: regs.set_c(regs.get_h()); return 4;
	case 0x4D: regs.set_c(regs.get_l()); return 4;
	case 0x4E: regs.set_c(mmu.read(regs.hl)); return 8;
	case 0x4F: regs.set_c(regs.get_a()); return 4;
	case 0x50: regs.set_d(regs.get_b()); return 4;
	case 0x51: regs.set_d(regs.get_c()); return 4;
	case 0x52: return 4;
	case 0x53: regs.set_d(regs.get_e()); return 4;
	case 0x54: regs.set_d(regs.get_h()); return 4;
	case 0x55: regs.set_d(regs.get_l()); return 4;
	case 0x56: regs.set_d(mmu.read(regs.hl)); return 8;
	case 0x57: regs.set_d(regs.get_a()); return 4;
	case 0x58: regs.set_e(regs.get_b()); return 4;
	case 0x59: regs.set_e(regs.get_c()); return 4;
	case 0x5A: regs.set_e(regs.get_d()); return 4;
	case 0x5B: return 4;
	case 0x5C: regs.set_e(regs.get_h()); return 4;
	case 0x5D: regs.set_e(regs.get_l()); return 4;
	case 0x5E: regs.set_e(mmu.read(regs.hl)); return 8;
	case 0x5F: regs.set_e(regs.get_a()); return 4;
	case 0x60: regs.set_h(regs.get_b()); return 4;
	case 0x61: regs.set_h(regs.get_c()); return 4;
	case 0x62: regs.set_h(regs.get_d()); return 4;
	case 0x63: regs.set_h(regs.get_e()); return 4;
	case 0x64: return 4;
	case 0x65: regs.set_h(regs.get_l()); return 4;
	case 0x66: regs.set_h(mmu.read(regs.hl)); return 8;
	case 0x67: regs.set_h(regs.get_a()); return 4;
	case 0x68: regs.set_l(regs.get_b()); return 4;
	case 0x69: regs.set_l(regs.get_c()); return 4;
	case 0x6A: regs.set_l(regs.get_d()); return 4;
	case 0x6B: regs.set_l(regs.get_e()); return 4;
	case 0x6C: regs.set_l(regs.get_h()); return 4;
	case 0x6D: return 4;
	case 0x6E: regs.set_l(mmu.read(regs.hl)); return 8;
	case 0x6F: regs.set_l(regs.get_a()); return 4;
	case 0x70: mmu.write(regs.hl, regs.get_b()); return 8;
	case 0x71: mmu.write(regs.hl, regs.get_c()); return 8;
	case 0x72: mmu.write(regs.hl, regs.get_d()); return 8;
	case 0x73: mmu.write(regs.hl, regs.get_e()); return 8;
	case 0x74: mmu.write(regs.hl, regs.get_h()); return 8;
	case 0x75: mmu.write(regs.hl, regs.get_l()); return 8;
	case 0x76: halted = true; return 4;
	case 0x77: mmu.write(regs.hl, regs.get_a()); return 8;
	case 0x78: regs.set_a(regs.get_b()); return 4;
	case 0x79: regs.set_a(regs.get_c()); return 4;
	case 0x7A: regs.set_a(regs.get_d()); return 4;
	case 0x7B: regs.set_a(regs.get_e()); return 4;
	case 0x7C: regs.set_a(regs.get_h()); return 4;
	case 0x7D: regs.set_a(regs.get_l()); return 4;
	case 0x7E: regs.set_a(mmu.read(regs.hl)); return 8;
	case 0x7F: return 4;

		// 0x80 - 0x87: ADD A, r (4 cycles, 8 if (HL))
	case 0x80: { u8 a = regs.get_a(); u8 v = regs.get_b(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 4; }
	case 0x81: { u8 a = regs.get_a(); u8 v = regs.get_c(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 4; }
	case 0x82: { u8 a = regs.get_a(); u8 v = regs.get_d(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 4; }
	case 0x83: { u8 a = regs.get_a(); u8 v = regs.get_e(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 4; }
	case 0x84: { u8 a = regs.get_a(); u8 v = regs.get_h(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 4; }
	case 0x85: { u8 a = regs.get_a(); u8 v = regs.get_l(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 4; }
	case 0x86: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 8; }
	case 0x87: { u8 a = regs.get_a(); u8 r = a + a; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (a & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + a) > 0xFF); return 4; }

			 // 0x88 - 0x8F: ADC A, r
	case 0x88: { u8 a = regs.get_a(); u8 v = regs.get_b(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 4; }
	case 0x89: { u8 a = regs.get_a(); u8 v = regs.get_c(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 4; }
	case 0x8A: { u8 a = regs.get_a(); u8 v = regs.get_d(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 4; }
	case 0x8B: { u8 a = regs.get_a(); u8 v = regs.get_e(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 4; }
	case 0x8C: { u8 a = regs.get_a(); u8 v = regs.get_h(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 4; }
	case 0x8D: { u8 a = regs.get_a(); u8 v = regs.get_l(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 4; }
	case 0x8E: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 8; }
	case 0x8F: { u8 a = regs.get_a(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + a + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (a & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + a + c) > 0xFF); return 4; }

			 // 0x90 - 0x97: SUB A, r
	case 0x90: { u8 a = regs.get_a(); u8 v = regs.get_b(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0x91: { u8 a = regs.get_a(); u8 v = regs.get_c(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0x92: { u8 a = regs.get_a(); u8 v = regs.get_d(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0x93: { u8 a = regs.get_a(); u8 v = regs.get_e(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0x94: { u8 a = regs.get_a(); u8 v = regs.get_h(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0x95: { u8 a = regs.get_a(); u8 v = regs.get_l(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0x96: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 8; }
	case 0x97: { regs.set_a(0); set_flag(Flag::Zero, true); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4; }

			 // 0x98 - 0x9F: SBC A, r
	case 0x98: { u8 a = regs.get_a(); u8 v = regs.get_b(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 4; }
	case 0x99: { u8 a = regs.get_a(); u8 v = regs.get_c(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 4; }
	case 0x9A: { u8 a = regs.get_a(); u8 v = regs.get_d(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 4; }
	case 0x9B: { u8 a = regs.get_a(); u8 v = regs.get_e(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 4; }
	case 0x9C: { u8 a = regs.get_a(); u8 v = regs.get_h(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 4; }
	case 0x9D: { u8 a = regs.get_a(); u8 v = regs.get_l(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 4; }
	case 0x9E: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 8; }
	case 0x9F: { u8 a = regs.get_a(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - a - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, c != 0); set_flag(Flag::Carry, c != 0); return 4; }

			 // 0xA0 - 0xA7: AND A, r
	case 0xA0: regs.set_a(regs.get_a() & regs.get_b()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;
	case 0xA1: regs.set_a(regs.get_a() & regs.get_c()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;
	case 0xA2: regs.set_a(regs.get_a() & regs.get_d()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;
	case 0xA3: regs.set_a(regs.get_a() & regs.get_e()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;
	case 0xA4: regs.set_a(regs.get_a() & regs.get_h()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;
	case 0xA5: regs.set_a(regs.get_a() & regs.get_l()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;
	case 0xA6: regs.set_a(regs.get_a() & mmu.read(regs.hl)); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 8;
	case 0xA7: set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 4;

		// 0xA8 - 0xAF: XOR A, r
	case 0xA8: regs.set_a(regs.get_a() ^ regs.get_b()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xA9: regs.set_a(regs.get_a() ^ regs.get_c()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xAA: regs.set_a(regs.get_a() ^ regs.get_d()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xAB: regs.set_a(regs.get_a() ^ regs.get_e()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xAC: regs.set_a(regs.get_a() ^ regs.get_h()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xAD: regs.set_a(regs.get_a() ^ regs.get_l()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xAE: regs.set_a(regs.get_a() ^ mmu.read(regs.hl)); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 8;
	case 0xAF: regs.set_a(0); set_flag(Flag::Zero, true); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;

		// 0xB0 - 0xB7: OR A, r
	case 0xB0: regs.set_a(regs.get_a() | regs.get_b()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xB1: regs.set_a(regs.get_a() | regs.get_c()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xB2: regs.set_a(regs.get_a() | regs.get_d()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xB3: regs.set_a(regs.get_a() | regs.get_e()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xB4: regs.set_a(regs.get_a() | regs.get_h()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xB5: regs.set_a(regs.get_a() | regs.get_l()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;
	case 0xB6: regs.set_a(regs.get_a() | mmu.read(regs.hl)); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 8;
	case 0xB7: set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;

		// 0xB8 - 0xBF: CP A, r
	case 0xB8: { u8 a = regs.get_a(); u8 v = regs.get_b(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0xB9: { u8 a = regs.get_a(); u8 v = regs.get_c(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0xBA: { u8 a = regs.get_a(); u8 v = regs.get_d(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0xBB: { u8 a = regs.get_a(); u8 v = regs.get_e(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0xBC: { u8 a = regs.get_a(); u8 v = regs.get_h(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0xBD: { u8 a = regs.get_a(); u8 v = regs.get_l(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 4; }
	case 0xBE: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 8; }
	case 0xBF: set_flag(Flag::Zero, true); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 4;

		// 0xC0 - 0xFF
	case 0xC0: if (!get_flag(Flag::Zero)) { regs.pc = pop_stack(); return 20; } return 8;
	case 0xC1: regs.bc = pop_stack(); return 12;
	case 0xC2: { u16 addr = fetch_word(); if (!get_flag(Flag::Zero)) { regs.pc = addr; return 16; } return 12; }
	case 0xC3: regs.pc = fetch_word(); return 16;
	case 0xC4: { u16 addr = fetch_word(); if (!get_flag(Flag::Zero)) { push_stack(regs.pc); regs.pc = addr; return 24; } return 12; }
	case 0xC5: push_stack(regs.bc); return 16;
	case 0xC6: { u8 a = regs.get_a(); u8 v = fetch_byte(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); return 8; }
	case 0xC7: push_stack(regs.pc); regs.pc = 0x0000; return 16;
	case 0xC8: if (get_flag(Flag::Zero)) { regs.pc = pop_stack(); return 20; } return 8;
	case 0xC9: regs.pc = pop_stack(); return 16;
	case 0xCA: { u16 addr = fetch_word(); if (get_flag(Flag::Zero)) { regs.pc = addr; return 16; } return 12; }

	case 0xCB: {
		u8 cb = fetch_byte();
		u8 reg = cb & 0x07;
		u8 bit = (cb >> 3) & 0x07;
		u8 group = (cb >> 6) & 0x03;
		u8 val = get_reg_by_index(reg);
		bool is_hl = (reg == 6);

		if (group == 0) {
			u8 result;
			switch (bit) {
			case 0: set_flag(Flag::Carry, (val >> 7) & 1); result = (val << 1) | (val >> 7); break;
			case 1: set_flag(Flag::Carry, val & 1); result = (val >> 1) | (val << 7); break;
			case 2: { u8 oc = get_flag(Flag::Carry) ? 1 : 0; set_flag(Flag::Carry, (val >> 7) & 1); result = (val << 1) | oc; break; }
			case 3: { u8 oc = get_flag(Flag::Carry) ? 1 : 0; set_flag(Flag::Carry, val & 1); result = (val >> 1) | (oc << 7); break; }
			case 4: set_flag(Flag::Carry, (val >> 7) & 1); result = val << 1; break;
			case 5: set_flag(Flag::Carry, val & 1); result = (val >> 1) | (val & (1 << 7)); break;
			case 6: set_flag(Flag::Carry, false); result = (val << 4) | (val >> 4); break;
			case 7: set_flag(Flag::Carry, val & 1); result = val >> 1; break;
			}
			set_reg_by_index(reg, result);
			set_flag(Flag::Zero, result == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			return is_hl ? 16 : 8;
		}
		else if (group == 1) {
			set_flag(Flag::Zero, ((val >> bit) & 1) == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, true);
			return is_hl ? 12 : 8;
		}
		else if (group == 2) {
			set_reg_by_index(reg, val & ~(1 << bit));
			return is_hl ? 16 : 8;
		}
		else {
			set_reg_by_index(reg, val | (1 << bit));
			return is_hl ? 16 : 8;
		}
	}

	case 0xCC: { u16 addr = fetch_word(); if (get_flag(Flag::Zero)) { push_stack(regs.pc); regs.pc = addr; return 24; } return 12; }
	case 0xCD: { u16 addr = fetch_word(); push_stack(regs.pc); regs.pc = addr; return 24; }
	case 0xCE: { u8 a = regs.get_a(); u8 v = fetch_byte(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); return 8; }
	case 0xCF: push_stack(regs.pc); regs.pc = 0x0008; return 16;
	case 0xD0: if (!get_flag(Flag::Carry)) { regs.pc = pop_stack(); return 20; } return 8;
	case 0xD1: regs.de = pop_stack(); return 12;
	case 0xD2: { u16 addr = fetch_word(); if (!get_flag(Flag::Carry)) { regs.pc = addr; return 16; } return 12; }
	case 0xD4: { u16 addr = fetch_word(); if (!get_flag(Flag::Carry)) { push_stack(regs.pc); regs.pc = addr; return 24; } return 12; }
	case 0xD5: push_stack(regs.de); return 16;
	case 0xD6: { u8 a = regs.get_a(); u8 v = fetch_byte(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 8; }
	case 0xD7: push_stack(regs.pc); regs.pc = 0x0010; return 16;
	case 0xD8: if (get_flag(Flag::Carry)) { regs.pc = pop_stack(); return 20; } return 8;
	case 0xD9: regs.pc = pop_stack(); ime = true; return 16;
	case 0xDA: { u16 addr = fetch_word(); if (get_flag(Flag::Carry)) { regs.pc = addr; return 16; } return 12; }
	case 0xDC: { u16 addr = fetch_word(); if (get_flag(Flag::Carry)) { push_stack(regs.pc); regs.pc = addr; return 24; } return 12; }
	case 0xDE: { u8 a = regs.get_a(); u8 v = fetch_byte(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); return 8; }
	case 0xDF: push_stack(regs.pc); regs.pc = 0x0018; return 16;
	case 0xE0: mmu.write(0xFF00 + fetch_byte(), regs.get_a()); return 12;
	case 0xE1: regs.hl = pop_stack(); return 12;
	case 0xE2: mmu.write(0xFF00 + regs.get_c(), regs.get_a()); return 8;
	case 0xE5: push_stack(regs.hl); return 16;
	case 0xE6: { regs.set_a(regs.get_a() & fetch_byte()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); return 8; }
	case 0xE7: push_stack(regs.pc); regs.pc = 0x0020; return 16;
	case 0xE8: {
		i8 offset = static_cast<i8>(fetch_byte());
		u8 uoffset = static_cast<u8>(offset);
		set_flag(Flag::Zero, false);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, ((regs.sp & 0x0F) + (uoffset & 0x0F)) > 0x0F);
		set_flag(Flag::Carry, ((regs.sp & 0xFF) + uoffset) > 0xFF);
		regs.sp = static_cast<u16>(static_cast<i32>(regs.sp) + offset);
		return 16;
	}
	case 0xE9: regs.pc = regs.hl; return 4;
	case 0xEA: { u16 addr = fetch_word(); mmu.write(addr, regs.get_a()); return 16; }
	case 0xEE: { regs.set_a(regs.get_a() ^ fetch_byte()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 8; }
	case 0xEF: push_stack(regs.pc); regs.pc = 0x0028; return 16;
	case 0xF0: regs.set_a(mmu.read(0xFF00 + fetch_byte())); return 12;
	case 0xF1: { u16 val = pop_stack(); regs.af = val & 0xFFF0; return 12; }
	case 0xF2: regs.set_a(mmu.read(0xFF00 + regs.get_c())); return 8;
	case 0xF3: ime = false; return 4;
	case 0xF5: push_stack(regs.af); return 16;
	case 0xF6: { regs.set_a(regs.get_a() | fetch_byte()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); return 8; }
	case 0xF7: push_stack(regs.pc); regs.pc = 0x0030; return 16;
	case 0xF8: {
		i8 offset = static_cast<i8>(fetch_byte());
		u8 uoffset = static_cast<u8>(offset);
		set_flag(Flag::Zero, false);
		set_flag(Flag::Subtract, false);
		set_flag(Flag::HalfCarry, ((regs.sp & 0x0F) + (uoffset & 0x0F)) > 0x0F);
		set_flag(Flag::Carry, ((regs.sp & 0xFF) + uoffset) > 0xFF);
		regs.hl = static_cast<u16>(static_cast<i32>(regs.sp) + offset);
		return 12;
	}
	case 0xF9: regs.sp = regs.hl; return 8;
	case 0xFA: { u16 addr = fetch_word(); regs.set_a(mmu.read(addr)); return 16; }
	case 0xFB: ime_pending = true; return 4;
	case 0xFE: { u8 a = regs.get_a(); u8 v = fetch_byte(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); return 8; }
	case 0xFF: push_stack(regs.pc); regs.pc = 0x0038; return 16;
	}

	return 4;
}