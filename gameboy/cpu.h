#pragma once
#include "types.h"

class MMU; // forward declaration of the MMU class, we will define it later in mmu.h, we need this to avoid circular dependency between cpu.h and mmu.h


// the flags register (F) is the lower 8 bits of the AF register, and it contains the following flags:
enum class Flag : u8 {
	Zero = 1 << 7,
	Subtract = 1 << 6,
	HalfCarry = 1 << 5,
	Carry = 1 << 4
};

inline u8 operator&(u16 af, Flag flag) {
	return static_cast<u8>(af) & static_cast<u8>(flag); // we are static casting the AF register to u8 and the flag to u8, and then applying bitwise AND to it (check if the bits are set)
};

inline void operator|=(u16& val, Flag f) {
	val |= static_cast<u8>(f); // we are static casting the flag to u8 and then applying bitwise OR to it (set the bits)
}

inline u8 operator~(Flag f) {
	return ~static_cast<u8>(f); // we are static casting the flag to u8 and then applying bitwise NOT to it (change/invert the bits)
}


struct Registers {
	u16 af, bc, de, hl = 0;
	u16 sp, pc = 0; // stack pointer and the program counter are 16-bit registers

	// Getters for the upper 8 bits of the registers
	u8 get_a() const {
		return (af >> 8) & 0xFF;
	}

	u8 get_b() const {
		return (bc >> 8) & 0xFF;
	}

	u8 get_d() const {
		return (de >> 8) & 0xFF;
	}

	u8 get_h() const {
		return (hl >> 8) & 0xFF;
	}

	// Getters for the lower 8 bits of the registers

	u8 get_f() const {
		return af & 0xFF;
	}

	u8 get_c() const {
		return bc & 0xFF;
	}


	u8 get_e() const {
		return de & 0xFF;
	}

	u8 get_l() const {
		return hl & 0xFF;
	}


	// Setters for the upper 8 bits of the registers
	void set_a(u8 value) {
		af = (af & 0x00FF) | (value << 8); // Set the upper 8 bits of AF to value, while keeping the lower 8 bits unchanged
	};

	void set_b(u8 value) {
		bc = (bc & 0x00FF) | (value << 8);
	};

	void set_d(u8 value) {
		de = (de & 0x00FF) | (value << 8);
	};

	void set_h(u8 value) {
		hl = (hl & 0x00FF) | (value << 8);
	};

	// Setters for the lower 8 bits of the registers
	void set_f(u8 value) {
		af = (af & 0xFF00) | (value & 0xF0); // Lower 4 bits of F are always 0
	}

	void set_c(u8 value) {
		bc = (bc & 0xFF00) | value; // lower 8 bits of BC and OR with value
	}

	void set_e(u8 value) {
		de = (de & 0xFF00) | value;
	}

	void set_l(u8 value) {
		hl = (hl & 0xFF00) | value;
	}


};


class CPU {
private:
	Registers regs;
	MMU& mmu; // reference to the MMU, we will use this to read and write memory
	bool ime; // interrupt master enable flag, when this is true, the CPU will respond to interrupts, when false, it will ignore them
	bool halted; // when this is true, the CPU will stop executing instructions until an interrupt occurs

public:
	CPU(MMU& mmu) : mmu(mmu), ime(false), halted(false) {};

	bool get_flag(Flag flag) const {
		return (regs.get_f() & static_cast<u8>(flag)) != 0; // we are getting the F register and applying bitwise AND with the flag, if the result is not 0, then the flag is set
	}

	void set_flag(Flag flag, bool value) {
		if (value) {
			regs.set_f(regs.get_f() | static_cast<u8>(flag)); // we are getting the F register and applying bitwise OR with the flag to set it
		}
		else {
			regs.set_f(regs.get_f() & ~static_cast<u8>(flag)); // we are getting the F register and applying bitwise AND with the NOT of the flag to clear it
		}
	}

	u8 fetch_byte() {
		u8 byte = mmu.read(regs.pc); // read a byte from memory at the address of the program counter
		regs.pc++; // increment the program counter to point to the next instruction
		return byte; // return the fetched byte

	};

	u16 fetch_word() {
		u8 lower_byte = fetch_byte(); // fetch the lower byte
		u8 higher_byte = fetch_byte(); // fetch the upper byte
		u16 word = (higher_byte << 8) | lower_byte; // combine the two bytes into a 16-bit word (higher byte is shifted left by 8 bits and then OR with the lower byte)
		return word; // return the fetched word
	}

	void push_stack(u16 value) {
		regs.sp -= 2; // decrement the stack pointer by 2 to make space for the new value
		mmu.write(regs.sp, value & 0xFF); // write the value to memory at the address of the stack pointer
		mmu.write(regs.sp + 1, value >> 8); // write the upper byte of the value to memory at the address of the stack pointer + 1
	}

	u16 pop_stack() {
		u16 value = mmu.read(regs.sp) | (mmu.read(regs.sp + 1) << 8);
		regs.sp += 2;
		return value;
	}

	u8 get_reg_by_index(u8 index) {
		switch (index) {
		case 0: return regs.get_b();
		case 1: return regs.get_c();
		case 2: return regs.get_d();
		case 3: return regs.get_e();
		case 4: return regs.get_h();
		case 5: return regs.get_l();
		case 6: return mmu.read(regs.hl); // (HL)
		case 7: return regs.get_a();
		default: throw std::runtime_error("Invalid register index");
		}
	}

	void set_reg_by_index(u8 index, u8 value) {
		switch (index) {
		case 0: regs.set_b(value); break;
		case 1: regs.set_c(value); break;
		case 2: regs.set_d(value); break;
		case 3: regs.set_e(value); break;
		case 4: regs.set_h(value); break;
		case 5: regs.set_l(value); break;
		case 6: mmu.write(regs.hl, value); break; // (HL)
		case 7: regs.set_a(value); break;
		default: throw std::runtime_error("Invalid register index");
		}
	}

	void step() {
		if (halted) {
			return;
		}

		u8 opcode = fetch_byte();

		switch (opcode) {

			// =============================================
			// 0x00 - 0x0F
			// =============================================
		case 0x00: // NOP
			break;
		case 0x01: // LD BC, n16
			regs.bc = fetch_word();
			break;
		case 0x02: // LD (BC), A
			mmu.write(regs.bc, regs.get_a());
			break;
		case 0x03: // INC BC
			regs.bc++;
			break;
		case 0x04: { // INC B
			regs.set_b(regs.get_b() + 1);
			set_flag(Flag::Zero, regs.get_b() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_b() & 0x0F) == 0x00);
			break;
		}
		case 0x05: { // DEC B
			regs.set_b(regs.get_b() - 1);
			set_flag(Flag::Zero, regs.get_b() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_b() & 0x0F) == 0x0F);
			break;
		}
		case 0x06: // LD B, n8
			regs.set_b(fetch_byte());
			break;
		case 0x07: { // RLCA
			u8 old_a = regs.get_a();
			u8 bit7 = (old_a >> 7) & 1;
			regs.set_a((old_a << 1) | bit7);
			set_flag(Flag::Carry, bit7);
			set_flag(Flag::Zero, false);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			break;
		}
		case 0x08: { // LD (n16), SP
			u16 addr = fetch_word();
			mmu.write(addr, regs.sp & 0xFF);
			mmu.write(addr + 1, regs.sp >> 8);
			break;
		}
		case 0x09: { // ADD HL, BC
			u16 old_hl = regs.hl;
			regs.hl += regs.bc;
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (regs.bc & 0x0FFF)) > 0x0FFF);
			set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(regs.bc)) > 0xFFFF);
			break;
		}
		case 0x0A: // LD A, (BC)
			regs.set_a(mmu.read(regs.bc));
			break;
		case 0x0B: // DEC BC
			regs.bc--;
			break;
		case 0x0C: { // INC C
			regs.set_c(regs.get_c() + 1);
			set_flag(Flag::Zero, regs.get_c() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_c() & 0x0F) == 0x00);
			break;
		}
		case 0x0D: { // DEC C
			regs.set_c(regs.get_c() - 1);
			set_flag(Flag::Zero, regs.get_c() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_c() & 0x0F) == 0x0F);
			break;
		}
		case 0x0E: // LD C, n8
			regs.set_c(fetch_byte());
			break;
		case 0x0F: { // RRCA
			u8 old_a = regs.get_a();
			u8 bit0 = old_a & 1;
			regs.set_a((old_a >> 1) | (bit0 << 7));
			set_flag(Flag::Carry, bit0);
			set_flag(Flag::Zero, false);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			break;
		}

				 // =============================================
				 // 0x10 - 0x1F
				 // =============================================
		case 0x10: // STOP
			halted = true;
			break;
		case 0x11: // LD DE, n16
			regs.de = fetch_word();
			break;
		case 0x12: // LD (DE), A
			mmu.write(regs.de, regs.get_a());
			break;
		case 0x13: // INC DE
			regs.de++;
			break;
		case 0x14: { // INC D
			regs.set_d(regs.get_d() + 1);
			set_flag(Flag::Zero, regs.get_d() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_d() & 0x0F) == 0x00);
			break;
		}
		case 0x15: { // DEC D
			regs.set_d(regs.get_d() - 1);
			set_flag(Flag::Zero, regs.get_d() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_d() & 0x0F) == 0x0F);
			break;
		}
		case 0x16: // LD D, n8
			regs.set_d(fetch_byte());
			break;
		case 0x17: { // RLA
			u8 old_a = regs.get_a();
			u8 old_carry = get_flag(Flag::Carry) ? 1 : 0;
			regs.set_a((old_a << 1) | old_carry);
			set_flag(Flag::Carry, (old_a >> 7) & 1);
			set_flag(Flag::Zero, false);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			break;
		}
		case 0x18: { // JR e8
			i8 offset = static_cast<i8>(fetch_byte());
			regs.pc += offset;
			break;
		}
		case 0x19: { // ADD HL, DE
			u16 old_hl = regs.hl;
			regs.hl += regs.de;
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (regs.de & 0x0FFF)) > 0x0FFF);
			set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(regs.de)) > 0xFFFF);
			break;
		}
		case 0x1A: // LD A, (DE)
			regs.set_a(mmu.read(regs.de));
			break;
		case 0x1B: // DEC DE
			regs.de--;
			break;
		case 0x1C: { // INC E
			regs.set_e(regs.get_e() + 1);
			set_flag(Flag::Zero, regs.get_e() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_e() & 0x0F) == 0x00);
			break;
		}
		case 0x1D: { // DEC E
			regs.set_e(regs.get_e() - 1);
			set_flag(Flag::Zero, regs.get_e() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_e() & 0x0F) == 0x0F);
			break;
		}
		case 0x1E: // LD E, n8
			regs.set_e(fetch_byte());
			break;
		case 0x1F: { // RRA
			u8 old_a = regs.get_a();
			u8 old_carry = get_flag(Flag::Carry) ? 1 : 0;
			regs.set_a((old_a >> 1) | (old_carry << 7));
			set_flag(Flag::Carry, old_a & 1);
			set_flag(Flag::Zero, false);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			break;
		}

				 // =============================================
				 // 0x20 - 0x2F
				 // =============================================
		case 0x20: { // JR NZ, e8
			i8 offset = static_cast<i8>(fetch_byte());
			if (!get_flag(Flag::Zero)) {
				regs.pc += offset;
			}
			break;
		}
		case 0x21: // LD HL, n16
			regs.hl = fetch_word();
			break;
		case 0x22: // LD (HL+), A
			mmu.write(regs.hl, regs.get_a());
			regs.hl++;
			break;
		case 0x23: // INC HL
			regs.hl++;
			break;
		case 0x24: { // INC H
			regs.set_h(regs.get_h() + 1);
			set_flag(Flag::Zero, regs.get_h() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_h() & 0x0F) == 0x00);
			break;
		}
		case 0x25: { // DEC H
			regs.set_h(regs.get_h() - 1);
			set_flag(Flag::Zero, regs.get_h() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_h() & 0x0F) == 0x0F);
			break;
		}
		case 0x26: // LD H, n8
			regs.set_h(fetch_byte());
			break;
		case 0x27: { // DAA
			u8 a = regs.get_a();
			if (!get_flag(Flag::Subtract)) {
				if (get_flag(Flag::Carry) || a > 0x99) {
					a += 0x60;
					set_flag(Flag::Carry, true);
				}
				if (get_flag(Flag::HalfCarry) || (a & 0x0F) > 0x09) {
					a += 0x06;
				}
			}
			else {
				if (get_flag(Flag::Carry)) {
					a -= 0x60;
				}
				if (get_flag(Flag::HalfCarry)) {
					a -= 0x06;
				}
			}
			regs.set_a(a);
			set_flag(Flag::Zero, a == 0);
			set_flag(Flag::HalfCarry, false);
			break;
		}
		case 0x28: { // JR Z, e8
			i8 offset = static_cast<i8>(fetch_byte());
			if (get_flag(Flag::Zero)) {
				regs.pc += offset;
			}
			break;
		}
		case 0x29: { // ADD HL, HL
			u16 old_hl = regs.hl;
			regs.hl += regs.hl;
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (old_hl & 0x0FFF)) > 0x0FFF);
			set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(old_hl)) > 0xFFFF);
			break;
		}
		case 0x2A: // LD A, (HL+)
			regs.set_a(mmu.read(regs.hl));
			regs.hl++;
			break;
		case 0x2B: // DEC HL
			regs.hl--;
			break;
		case 0x2C: { // INC L
			regs.set_l(regs.get_l() + 1);
			set_flag(Flag::Zero, regs.get_l() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_l() & 0x0F) == 0x00);
			break;
		}
		case 0x2D: { // DEC L
			regs.set_l(regs.get_l() - 1);
			set_flag(Flag::Zero, regs.get_l() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_l() & 0x0F) == 0x0F);
			break;
		}
		case 0x2E: // LD L, n8
			regs.set_l(fetch_byte());
			break;
		case 0x2F: // CPL
			regs.set_a(~regs.get_a());
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, true);
			break;

			// =============================================
			// 0x30 - 0x3F
			// =============================================
		case 0x30: { // JR NC, e8
			i8 offset = static_cast<i8>(fetch_byte());
			if (!get_flag(Flag::Carry)) {
				regs.pc += offset;
			}
			break;
		}
		case 0x31: // LD SP, n16
			regs.sp = fetch_word();
			break;
		case 0x32: // LD (HL-), A
			mmu.write(regs.hl, regs.get_a());
			regs.hl--;
			break;
		case 0x33: // INC SP
			regs.sp++;
			break;
		case 0x34: { // INC (HL)
			u8 val = mmu.read(regs.hl);
			val++;
			mmu.write(regs.hl, val);
			set_flag(Flag::Zero, val == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (val & 0x0F) == 0x00);
			break;
		}
		case 0x35: { // DEC (HL)
			u8 val = mmu.read(regs.hl);
			val--;
			mmu.write(regs.hl, val);
			set_flag(Flag::Zero, val == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (val & 0x0F) == 0x0F);
			break;
		}
		case 0x36: // LD (HL), n8
			mmu.write(regs.hl, fetch_byte());
			break;
		case 0x37: // SCF
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			set_flag(Flag::Carry, true);
			break;
		case 0x38: { // JR C, e8
			i8 offset = static_cast<i8>(fetch_byte());
			if (get_flag(Flag::Carry)) {
				regs.pc += offset;
			}
			break;
		}
		case 0x39: { // ADD HL, SP
			u16 old_hl = regs.hl;
			regs.hl += regs.sp;
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((old_hl & 0x0FFF) + (regs.sp & 0x0FFF)) > 0x0FFF);
			set_flag(Flag::Carry, (static_cast<u32>(old_hl) + static_cast<u32>(regs.sp)) > 0xFFFF);
			break;
		}
		case 0x3A: // LD A, (HL-)
			regs.set_a(mmu.read(regs.hl));
			regs.hl--;
			break;
		case 0x3B: // DEC SP
			regs.sp--;
			break;
		case 0x3C: { // INC A
			regs.set_a(regs.get_a() + 1);
			set_flag(Flag::Zero, regs.get_a() == 0);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, (regs.get_a() & 0x0F) == 0x00);
			break;
		}
		case 0x3D: { // DEC A
			regs.set_a(regs.get_a() - 1);
			set_flag(Flag::Zero, regs.get_a() == 0);
			set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (regs.get_a() & 0x0F) == 0x0F);
			break;
		}
		case 0x3E: // LD A, n8
			regs.set_a(fetch_byte());
			break;
		case 0x3F: // CCF
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false);
			set_flag(Flag::Carry, !get_flag(Flag::Carry));
			break;

			// =============================================
			// 0x40 - 0x7F: LD register to register
			// =============================================
		case 0x40: break;
		case 0x41: regs.set_b(regs.get_c()); break;
		case 0x42: regs.set_b(regs.get_d()); break;
		case 0x43: regs.set_b(regs.get_e()); break;
		case 0x44: regs.set_b(regs.get_h()); break;
		case 0x45: regs.set_b(regs.get_l()); break;
		case 0x46: regs.set_b(mmu.read(regs.hl)); break;
		case 0x47: regs.set_b(regs.get_a()); break;
		case 0x48: regs.set_c(regs.get_b()); break;
		case 0x49: break;
		case 0x4A: regs.set_c(regs.get_d()); break;
		case 0x4B: regs.set_c(regs.get_e()); break;
		case 0x4C: regs.set_c(regs.get_h()); break;
		case 0x4D: regs.set_c(regs.get_l()); break;
		case 0x4E: regs.set_c(mmu.read(regs.hl)); break;
		case 0x4F: regs.set_c(regs.get_a()); break;
		case 0x50: regs.set_d(regs.get_b()); break;
		case 0x51: regs.set_d(regs.get_c()); break;
		case 0x52: break;
		case 0x53: regs.set_d(regs.get_e()); break;
		case 0x54: regs.set_d(regs.get_h()); break;
		case 0x55: regs.set_d(regs.get_l()); break;
		case 0x56: regs.set_d(mmu.read(regs.hl)); break;
		case 0x57: regs.set_d(regs.get_a()); break;
		case 0x58: regs.set_e(regs.get_b()); break;
		case 0x59: regs.set_e(regs.get_c()); break;
		case 0x5A: regs.set_e(regs.get_d()); break;
		case 0x5B: break;
		case 0x5C: regs.set_e(regs.get_h()); break;
		case 0x5D: regs.set_e(regs.get_l()); break;
		case 0x5E: regs.set_e(mmu.read(regs.hl)); break;
		case 0x5F: regs.set_e(regs.get_a()); break;
		case 0x60: regs.set_h(regs.get_b()); break;
		case 0x61: regs.set_h(regs.get_c()); break;
		case 0x62: regs.set_h(regs.get_d()); break;
		case 0x63: regs.set_h(regs.get_e()); break;
		case 0x64: break;
		case 0x65: regs.set_h(regs.get_l()); break;
		case 0x66: regs.set_h(mmu.read(regs.hl)); break;
		case 0x67: regs.set_h(regs.get_a()); break;
		case 0x68: regs.set_l(regs.get_b()); break;
		case 0x69: regs.set_l(regs.get_c()); break;
		case 0x6A: regs.set_l(regs.get_d()); break;
		case 0x6B: regs.set_l(regs.get_e()); break;
		case 0x6C: regs.set_l(regs.get_h()); break;
		case 0x6D: break;
		case 0x6E: regs.set_l(mmu.read(regs.hl)); break;
		case 0x6F: regs.set_l(regs.get_a()); break;
		case 0x70: mmu.write(regs.hl, regs.get_b()); break;
		case 0x71: mmu.write(regs.hl, regs.get_c()); break;
		case 0x72: mmu.write(regs.hl, regs.get_d()); break;
		case 0x73: mmu.write(regs.hl, regs.get_e()); break;
		case 0x74: mmu.write(regs.hl, regs.get_h()); break;
		case 0x75: mmu.write(regs.hl, regs.get_l()); break;
		case 0x76: halted = true; break; // HALT
		case 0x77: mmu.write(regs.hl, regs.get_a()); break;
		case 0x78: regs.set_a(regs.get_b()); break;
		case 0x79: regs.set_a(regs.get_c()); break;
		case 0x7A: regs.set_a(regs.get_d()); break;
		case 0x7B: regs.set_a(regs.get_e()); break;
		case 0x7C: regs.set_a(regs.get_h()); break;
		case 0x7D: regs.set_a(regs.get_l()); break;
		case 0x7E: regs.set_a(mmu.read(regs.hl)); break;
		case 0x7F: break;

			// =============================================
			// 0x80 - 0x87: ADD A, r
			// =============================================
		case 0x80: { u8 a = regs.get_a(); u8 v = regs.get_b(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x81: { u8 a = regs.get_a(); u8 v = regs.get_c(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x82: { u8 a = regs.get_a(); u8 v = regs.get_d(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x83: { u8 a = regs.get_a(); u8 v = regs.get_e(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x84: { u8 a = regs.get_a(); u8 v = regs.get_h(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x85: { u8 a = regs.get_a(); u8 v = regs.get_l(); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x86: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); u8 r = a + v; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF); break; }
		case 0x87: { u8 a = regs.get_a(); u8 r = a + a; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (a & 0x0F)) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + a) > 0xFF); break; }

				 // =============================================
				 // 0x88 - 0x8F: ADC A, r
				 // =============================================
		case 0x88: { u8 a = regs.get_a(); u8 v = regs.get_b(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x89: { u8 a = regs.get_a(); u8 v = regs.get_c(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x8A: { u8 a = regs.get_a(); u8 v = regs.get_d(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x8B: { u8 a = regs.get_a(); u8 v = regs.get_e(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x8C: { u8 a = regs.get_a(); u8 v = regs.get_h(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x8D: { u8 a = regs.get_a(); u8 v = regs.get_l(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x8E: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF); break; }
		case 0x8F: { u8 a = regs.get_a(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + a + c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, ((a & 0x0F) + (a & 0x0F) + c) > 0x0F); set_flag(Flag::Carry, (static_cast<u16>(a) + a + c) > 0xFF); break; }

				 // =============================================
				 // 0x90 - 0x97: SUB A, r
				 // =============================================
		case 0x90: { u8 a = regs.get_a(); u8 v = regs.get_b(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x91: { u8 a = regs.get_a(); u8 v = regs.get_c(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x92: { u8 a = regs.get_a(); u8 v = regs.get_d(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x93: { u8 a = regs.get_a(); u8 v = regs.get_e(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x94: { u8 a = regs.get_a(); u8 v = regs.get_h(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x95: { u8 a = regs.get_a(); u8 v = regs.get_l(); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x96: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); regs.set_a(a - v); set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0x97: { regs.set_a(0); set_flag(Flag::Zero, true); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break; }

				 // =============================================
				 // 0x98 - 0x9F: SBC A, r
				 // =============================================
		case 0x98: { u8 a = regs.get_a(); u8 v = regs.get_b(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x99: { u8 a = regs.get_a(); u8 v = regs.get_c(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x9A: { u8 a = regs.get_a(); u8 v = regs.get_d(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x9B: { u8 a = regs.get_a(); u8 v = regs.get_e(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x9C: { u8 a = regs.get_a(); u8 v = regs.get_h(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x9D: { u8 a = regs.get_a(); u8 v = regs.get_l(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x9E: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c); set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c); break; }
		case 0x9F: { u8 a = regs.get_a(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - a - c; regs.set_a(r); set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, c != 0); set_flag(Flag::Carry, c != 0); break; }

				 // =============================================
				 // 0xA0 - 0xA7: AND A, r
				 // =============================================
		case 0xA0: regs.set_a(regs.get_a() & regs.get_b()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA1: regs.set_a(regs.get_a() & regs.get_c()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA2: regs.set_a(regs.get_a() & regs.get_d()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA3: regs.set_a(regs.get_a() & regs.get_e()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA4: regs.set_a(regs.get_a() & regs.get_h()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA5: regs.set_a(regs.get_a() & regs.get_l()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA6: regs.set_a(regs.get_a() & mmu.read(regs.hl)); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;
		case 0xA7: set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false); break;

			// =============================================
			// 0xA8 - 0xAF: XOR A, r
			// =============================================
		case 0xA8: regs.set_a(regs.get_a() ^ regs.get_b()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xA9: regs.set_a(regs.get_a() ^ regs.get_c()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xAA: regs.set_a(regs.get_a() ^ regs.get_d()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xAB: regs.set_a(regs.get_a() ^ regs.get_e()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xAC: regs.set_a(regs.get_a() ^ regs.get_h()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xAD: regs.set_a(regs.get_a() ^ regs.get_l()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xAE: regs.set_a(regs.get_a() ^ mmu.read(regs.hl)); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xAF: regs.set_a(0); set_flag(Flag::Zero, true); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;

			// =============================================
			// 0xB0 - 0xB7: OR A, r
			// =============================================
		case 0xB0: regs.set_a(regs.get_a() | regs.get_b()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB1: regs.set_a(regs.get_a() | regs.get_c()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB2: regs.set_a(regs.get_a() | regs.get_d()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB3: regs.set_a(regs.get_a() | regs.get_e()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB4: regs.set_a(regs.get_a() | regs.get_h()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB5: regs.set_a(regs.get_a() | regs.get_l()); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB6: regs.set_a(regs.get_a() | mmu.read(regs.hl)); set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;
		case 0xB7: set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;

			// =============================================
			// 0xB8 - 0xBF: CP A, r
			// =============================================
		case 0xB8: { u8 a = regs.get_a(); u8 v = regs.get_b(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xB9: { u8 a = regs.get_a(); u8 v = regs.get_c(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xBA: { u8 a = regs.get_a(); u8 v = regs.get_d(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xBB: { u8 a = regs.get_a(); u8 v = regs.get_e(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xBC: { u8 a = regs.get_a(); u8 v = regs.get_h(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xBD: { u8 a = regs.get_a(); u8 v = regs.get_l(); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xBE: { u8 a = regs.get_a(); u8 v = mmu.read(regs.hl); set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F)); set_flag(Flag::Carry, a < v); break; }
		case 0xBF: set_flag(Flag::Zero, true); set_flag(Flag::Subtract, true); set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false); break;

			// =============================================
			// 0xC0 - 0xFF
			// =============================================
		case 0xC0: // RET NZ
			if (!get_flag(Flag::Zero)) { regs.pc = pop_stack(); }
			break;
		case 0xC1: // POP BC
			regs.bc = pop_stack();
			break;
		case 0xC2: { // JP NZ, n16
			u16 addr = fetch_word();
			if (!get_flag(Flag::Zero)) { regs.pc = addr; }
			break;
		}
		case 0xC3: // JP n16
			regs.pc = fetch_word();
			break;
		case 0xC4: { // CALL NZ, n16
			u16 addr = fetch_word();
			if (!get_flag(Flag::Zero)) { push_stack(regs.pc); regs.pc = addr; }
			break;
		}
		case 0xC5: // PUSH BC
			push_stack(regs.bc);
			break;
		case 0xC6: { // ADD A, n8
			u8 a = regs.get_a(); u8 v = fetch_byte(); u8 r = a + v; regs.set_a(r);
			set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F)) > 0x0F);
			set_flag(Flag::Carry, (static_cast<u16>(a) + v) > 0xFF);
			break;
		}
		case 0xC7: // RST 00h
			push_stack(regs.pc); regs.pc = 0x0000;
			break;
		case 0xC8: // RET Z
			if (get_flag(Flag::Zero)) { regs.pc = pop_stack(); }
			break;
		case 0xC9: // RET
			regs.pc = pop_stack();
			break;
		case 0xCA: { // JP Z, n16
			u16 addr = fetch_word();
			if (get_flag(Flag::Zero)) { regs.pc = addr; }
			break;
		}
		case 0xCB: {
			u8 cb = fetch_byte();

			u8 reg = cb & 0x07; // we only care about the last 3 bits for the register index, since there are 8 registers (B, C, D, E, H, L, (HL), A)	
			u8 bit = (cb >> 3) & 0x07; // we shift right by 3 to get the bit index, but we only care about the last 3 bits
			u8 group = (cb >> 6) & 0x03; // we basically shift right by 6, but we only care about the last 2 bits

			u8 val = get_reg_by_index(reg); // get the current register

			if (group == 0) {
				u8 result;

				switch (bit) {
				case 0: // RLC
					set_flag(Flag::Carry, (val >> 7) & 1);
					result = (val << 1) | (val >> 7);
					break;
				case 1: // RRC
					set_flag(Flag::Carry, val & 1);
					result = (val >> 1) | (val << 7);
					break;
				case 2: { // RL
					u8 old_carry = get_flag(Flag::Carry) ? 1 : 0;
					set_flag(Flag::Carry, (val >> 7) & 1);
					result = (val << 1) | old_carry;
					break;
				}
				case 3: { // RR
					u8 old_carry = get_flag(Flag::Carry) ? 1 : 0;
					set_flag(Flag::Carry, val & 1);
					result = (val >> 1) | (old_carry << 7);
					break;
				}
				case 4: // SLA
					set_flag(Flag::Carry, (val >> 7) & 1);
					result = val << 1;
					break;
				case 5: // SRA
					set_flag(Flag::Carry, val & 1);
					result = (val >> 1) | (val & (1 << 7));
					break;
				case 6: // SWAP
					set_flag(Flag::Carry, false);
					result = (val << 4) | (val >> 4);
					break;
				case 7: // SRL
					set_flag(Flag::Carry, val & 1);
					result = val >> 1;
					break;
				}

				set_reg_by_index(reg, result);
				set_flag(Flag::Zero, result == 0);
				set_flag(Flag::Subtract, false);
				set_flag(Flag::HalfCarry, false);
			}
			else if (group == 1) { // BIT
				set_flag(Flag::Zero, ((val >> bit) & 1) == 0);
				set_flag(Flag::Subtract, false);
				set_flag(Flag::HalfCarry, true);
			}
			else if (group == 2) { // RES
				set_reg_by_index(reg, val & ~(1 << bit)); // we basically clear the bit at the given index by ANDing with a mask that has all bits set except the one we want to clear
			}
			else { // SET
				set_reg_by_index(reg, val | (1 << bit)); // we basically set the bit at the given index by ORing with a mask that has only that bit set
			}
			break;
		}


			break;
		case 0xCC: { // CALL Z, n16
			u16 addr = fetch_word();
			if (get_flag(Flag::Zero)) { push_stack(regs.pc); regs.pc = addr; }
			break;
		}
		case 0xCD: { // CALL n16
			u16 addr = fetch_word();
			push_stack(regs.pc);
			regs.pc = addr;
			break;
		}
		case 0xCE: { // ADC A, n8
			u8 a = regs.get_a(); u8 v = fetch_byte(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a + v + c; regs.set_a(r);
			set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((a & 0x0F) + (v & 0x0F) + c) > 0x0F);
			set_flag(Flag::Carry, (static_cast<u16>(a) + v + c) > 0xFF);
			break;
		}
		case 0xCF: // RST 08h
			push_stack(regs.pc); regs.pc = 0x0008;
			break;
		case 0xD0: // RET NC
			if (!get_flag(Flag::Carry)) { regs.pc = pop_stack(); }
			break;
		case 0xD1: // POP DE
			regs.de = pop_stack();
			break;
		case 0xD2: { // JP NC, n16
			u16 addr = fetch_word();
			if (!get_flag(Flag::Carry)) { regs.pc = addr; }
			break;
		}
		case 0xD4: { // CALL NC, n16
			u16 addr = fetch_word();
			if (!get_flag(Flag::Carry)) { push_stack(regs.pc); regs.pc = addr; }
			break;
		}
		case 0xD5: // PUSH DE
			push_stack(regs.de);
			break;
		case 0xD6: { // SUB A, n8
			u8 a = regs.get_a(); u8 v = fetch_byte(); regs.set_a(a - v);
			set_flag(Flag::Zero, (u8)(a - v) == 0); set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F));
			set_flag(Flag::Carry, a < v);
			break;
		}
		case 0xD7: // RST 10h
			push_stack(regs.pc); regs.pc = 0x0010;
			break;
		case 0xD8: // RET C
			if (get_flag(Flag::Carry)) { regs.pc = pop_stack(); }
			break;
		case 0xD9: // RETI
			regs.pc = pop_stack();
			ime = true;
			break;
		case 0xDA: { // JP C, n16
			u16 addr = fetch_word();
			if (get_flag(Flag::Carry)) { regs.pc = addr; }
			break;
		}
		case 0xDC: { // CALL C, n16
			u16 addr = fetch_word();
			if (get_flag(Flag::Carry)) { push_stack(regs.pc); regs.pc = addr; }
			break;
		}
		case 0xDE: { // SBC A, n8
			u8 a = regs.get_a(); u8 v = fetch_byte(); u8 c = get_flag(Flag::Carry) ? 1 : 0; u8 r = a - v - c; regs.set_a(r);
			set_flag(Flag::Zero, r == 0); set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F) + c);
			set_flag(Flag::Carry, static_cast<u16>(a) < static_cast<u16>(v) + c);
			break;
		}
		case 0xDF: // RST 18h
			push_stack(regs.pc); regs.pc = 0x0018;
			break;
		case 0xE0: // LD (FF00+n8), A
			mmu.write(0xFF00 + fetch_byte(), regs.get_a());
			break;
		case 0xE1: // POP HL
			regs.hl = pop_stack();
			break;
		case 0xE2: // LD (FF00+C), A
			mmu.write(0xFF00 + regs.get_c(), regs.get_a());
			break;
		case 0xE5: // PUSH HL
			push_stack(regs.hl);
			break;
		case 0xE6: { // AND A, n8
			regs.set_a(regs.get_a() & fetch_byte());
			set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, true); set_flag(Flag::Carry, false);
			break;
		}
		case 0xE7: // RST 20h
			push_stack(regs.pc); regs.pc = 0x0020;
			break;
		case 0xE8: { // ADD SP, e8
			i8 offset = static_cast<i8>(fetch_byte());
			u8 unsigned_offset = static_cast<u8>(offset);
			set_flag(Flag::Zero, false);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((regs.sp & 0x0F) + (unsigned_offset & 0x0F)) > 0x0F);
			set_flag(Flag::Carry, ((regs.sp & 0xFF) + unsigned_offset) > 0xFF);
			regs.sp = static_cast<u16>(static_cast<i32>(regs.sp) + offset);
			break;
		}
		case 0xE9: // JP HL
			regs.pc = regs.hl;
			break;
		case 0xEA: { // LD (n16), A
			u16 addr = fetch_word();
			mmu.write(addr, regs.get_a());
			break;
		}
		case 0xEE: { // XOR A, n8
			regs.set_a(regs.get_a() ^ fetch_byte());
			set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false);
			break;
		}
		case 0xEF: // RST 28h
			push_stack(regs.pc); regs.pc = 0x0028;
			break;
		case 0xF0: // LD A, (FF00+n8)
			regs.set_a(mmu.read(0xFF00 + fetch_byte()));
			break;
		case 0xF1: { // POP AF
			u16 val = pop_stack();
			regs.af = val & 0xFFF0;
			break;
		}
		case 0xF2: // LD A, (FF00+C)
			regs.set_a(mmu.read(0xFF00 + regs.get_c()));
			break;
		case 0xF3: // DI
			ime = false;
			break;
		case 0xF5: // PUSH AF
			push_stack(regs.af);
			break;
		case 0xF6: { // OR A, n8
			regs.set_a(regs.get_a() | fetch_byte());
			set_flag(Flag::Zero, regs.get_a() == 0); set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, false); set_flag(Flag::Carry, false);
			break;
		}
		case 0xF7: // RST 30h
			push_stack(regs.pc); regs.pc = 0x0030;
			break;
		case 0xF8: { // LD HL, SP+e8
			i8 offset = static_cast<i8>(fetch_byte());
			u8 unsigned_offset = static_cast<u8>(offset);
			set_flag(Flag::Zero, false);
			set_flag(Flag::Subtract, false);
			set_flag(Flag::HalfCarry, ((regs.sp & 0x0F) + (unsigned_offset & 0x0F)) > 0x0F);
			set_flag(Flag::Carry, ((regs.sp & 0xFF) + unsigned_offset) > 0xFF);
			regs.hl = static_cast<u16>(static_cast<i32>(regs.sp) + offset);
			break;
		}
		case 0xF9: // LD SP, HL
			regs.sp = regs.hl;
			break;
		case 0xFA: { // LD A, (n16)
			u16 addr = fetch_word();
			regs.set_a(mmu.read(addr));
			break;
		}
		case 0xFB: // EI
			ime = true;
			break;
		case 0xFE: { // CP A, n8
			u8 a = regs.get_a(); u8 v = fetch_byte();
			set_flag(Flag::Zero, a == v); set_flag(Flag::Subtract, true);
			set_flag(Flag::HalfCarry, (a & 0x0F) < (v & 0x0F));
			set_flag(Flag::Carry, a < v);
			break;
		}
		case 0xFF: // RST 38h
			push_stack(regs.pc); regs.pc = 0x0038;
			break;
		}
	}
};