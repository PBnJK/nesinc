#include "cpu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL2/SDL.h"
#include "error.h"
#include "frame.h"
#include "screen.h"

typedef enum {
	M_IMMEDIATE,
	M_ZEROPAGE,
	M_ZEROPAGE_X,
	M_ZEROPAGE_Y,
	M_RELATIVE,
	M_ABSOLUTE,
	M_ABSOLUTE_X,
	M_ABSOLUTE_Y,
	M_INDIRECT,
	M_INDIRECT_X,
	M_INDIRECT_Y,
	M_IMPLIED,
} AddressingMode;

#define ARGP cpuRead(cpu, (*pc)++)

#define UPDATE(X) _updateZeroAndNeg(cpu, X)
#define ADDR _getAddressFromMode(cpu, &pc, MODE)
#define MEMADDR cpuRead(cpu, ADDR)

static void _gameCallback(PPU *ppu) {
	ppuRender(ppu);

	SDL_UpdateTexture(gScreen.texture, NULL, &ppu->frame.data, 256 * 3);
	SDL_RenderCopy(gScreen.renderer, gScreen.texture, NULL, NULL);
	SDL_RenderPresent(gScreen.renderer);

	SDL_Event e;
	while( SDL_PollEvent(&e) ) {
		switch( e.type ) {
			case SDL_QUIT:
				exit(23);
			case SDL_KEYDOWN:
				if( e.key.keysym.sym == SDLK_ESCAPE ) {
					exit(24);
				}

				break;
			default:
				break;
		}
	}
}

static inline void _setCarry(CPU *cpu) {
	cpu->status.carry = 1;
}

static inline void _clearCarry(CPU *cpu) {
	cpu->status.carry = 0;
}

static inline void _setZero(CPU *cpu) {
	cpu->status.zero = 1;
}

static inline void _clearZero(CPU *cpu) {
	cpu->status.zero = 0;
}

static inline void _setIntr(CPU *cpu) {
	cpu->status.interrupt = 1;
}

static inline void _clearIntr(CPU *cpu) {
	cpu->status.interrupt = 1;
}

static inline void _setDecimal(CPU *cpu) {
	cpu->status.decimal = 1;
}

static inline void _clearDecimal(CPU *cpu) {
	cpu->status.decimal = 0;
}

static inline void _setBFlag1(CPU *cpu) {
	cpu->status.bFlag1 = 1;
}

static inline void _clearBFlag1(CPU *cpu) {
	cpu->status.bFlag1 = 0;
}

static inline void _setBFlag2(CPU *cpu) {
	cpu->status.bFlag2 = 1;
}

static inline void _clearBFlag2(CPU *cpu) {
	cpu->status.bFlag2 = 0;
}

static inline void _setOverflow(CPU *cpu) {
	cpu->status.overflow = 1;
}

static inline void _clearOverflow(CPU *cpu) {
	cpu->status.overflow = 0;
}

static inline void _setNegative(CPU *cpu) {
	cpu->status.negative = 1;
}

static inline void _clearNegative(CPU *cpu) {
	cpu->status.negative = 0;
}

static uint16_t _getAddressFromMode(CPU *cpu, uint16_t *pc,
									const AddressingMode MODE) {
	switch( MODE ) {
		case M_RELATIVE:
		case M_IMMEDIATE:
			return (*pc)++;

		case M_ZEROPAGE:
			return ARGP;

		case M_ZEROPAGE_X:
			return (ARGP + cpu->regX) & 0xFF;

		case M_ZEROPAGE_Y:
			return (ARGP + cpu->regY) & 0xFF;

		case M_ABSOLUTE: {
			const uint16_t LO = ARGP;
			const uint16_t HI = ARGP;

			return ((HI << 8) | LO);
		}

		case M_ABSOLUTE_X: {
			const uint16_t LO = ARGP;
			const uint16_t HI = ARGP;

			const uint16_t ADDRESS = (uint16_t)((HI << 8) | LO);
			return (uint16_t)(ADDRESS + cpu->regX);
		}

		case M_ABSOLUTE_Y: {
			const uint16_t LO = ARGP;
			const uint16_t HI = ARGP;

			const uint16_t ADDRESS = (uint16_t)((HI << 8) | LO);
			return (uint16_t)(ADDRESS + cpu->regY);
		}

		case M_INDIRECT: {
			uint8_t loByte = ARGP;
			const uint8_t HI = ARGP;

			const uint8_t LO_PTR = cpuRead(cpu, (HI << 8) | (loByte++));
			const uint8_t HI_PTR = cpuRead(cpu, (HI << 8) | loByte);

			return (HI_PTR << 8) | LO_PTR;
		}

		case M_INDIRECT_X: {
			uint8_t zpAddress = ARGP + cpu->regX;

			const uint8_t LO = cpuRead(cpu, zpAddress++);
			const uint8_t HI = cpuRead(cpu, zpAddress);

			return (HI << 8) | LO;
		}

		case M_INDIRECT_Y: {
			uint8_t ptr = ARGP;

			const uint16_t LO = cpuRead(cpu, ptr++);
			const uint16_t HI = cpuRead(cpu, ptr);

			const uint16_t DEREF = (HI << 8) | LO;
			return DEREF + cpu->regY;
		}

		case M_IMPLIED:
			return 0;

		default:
			fprintf(stderr, "Unknown addressing mode %u\n", MODE);
			return 0;
	}
}

static void _updateZeroAndNeg(CPU *cpu, const uint8_t RESULT) {
	if( RESULT == 0 ) {
		_setZero(cpu);
	} else {
		_clearZero(cpu);
	}

	if( (RESULT & SIGN_BIT) != 0 ) {
		_setNegative(cpu);
	} else {
		_clearNegative(cpu);
	}
}

static void _addToA(CPU *cpu, const uint8_t VALUE) {
	uint16_t sum = (uint16_t)(cpu->regA + VALUE);
	sum += ((cpu->status.carry) ? 1 : 0);

	if( sum > UINT8_MAX ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	const uint8_t RESULT = (uint8_t)sum;

	if( ((VALUE ^ RESULT) & (RESULT ^ cpu->regA) & 0x80) != 0 ) {
		_setOverflow(cpu);
	} else {
		_clearOverflow(cpu);
	}

	cpu->regA = RESULT;
	UPDATE(cpu->regA);
}

static void _lsrA(CPU *cpu) {
	if( (cpu->regA & 1) == 1 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	cpu->regA >>= 1;
	UPDATE(cpu->regA);
}

static void _rorA(CPU *cpu) {
	const bool OLD_CARRY = (cpu->status.carry == 1);

	if( (cpu->regA & 1) == 1 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	cpu->regA >>= 1;

	if( OLD_CARRY ) {
		cpu->regA |= SIGN_BIT;
	}

	UPDATE(cpu->regA);
}

static uint16_t _branch(CPU *cpu, uint16_t pc, const uint8_t COMPARISON) {
	if( COMPARISON ) {
		return (uint16_t)(pc + ((int8_t)cpuRead(cpu, pc)) + 1);
	}

	return pc + 1;
}

static void _push(CPU *cpu, const uint8_t VALUE) {
	cpuWrite(cpu, 0x0100 + cpu->stack, VALUE);
	--cpu->stack;
}

static void _push16(CPU *cpu, const uint16_t VALUE) {
	const uint8_t HI = (uint8_t)(VALUE >> 8);
	const uint8_t LO = (uint8_t)(VALUE & 0xFF);

	_push(cpu, HI);
	_push(cpu, LO);
}

static uint8_t _pull(CPU *cpu) {
	++cpu->stack;
	return cpuRead(cpu, 0x0100 + cpu->stack);
}

static uint16_t _pull16(CPU *cpu) {
	const uint16_t LO = _pull(cpu);
	const uint16_t HI = _pull(cpu);

	return (HI << 8) | LO;
}

#define OP_FN(N) \
	static uint16_t N(CPU *cpu, uint16_t pc, const AddressingMode MODE)
typedef uint16_t (*OpFn)(CPU *cpu, uint16_t, const AddressingMode);

typedef struct _Op {
	OpFn fn;
	AddressingMode mode;
	uint8_t cycles;
	uint8_t bytes;
	const char NAME[5];
} Op;

#define NOP       \
	UNUSED(cpu);  \
	UNUSED(MODE); \
	return pc

/* Adds a byte to the accumulator, with carry */
OP_FN(_adc) {
	_addToA(cpu, MEMADDR);
	return pc;
}

/* ANDs the X register with the accumulator, then ANDs the accumulator with 7.
 * The result is stored in memory
 */
OP_FN(_ahx) {
	const uint8_t RESULT = (cpu->regA & cpu->regX) & 7;
	cpuWrite(cpu, ADDR, RESULT);

	return pc;
}

/* ANDs a byte with accumulator, then preforms a logical shift right on the
 * accumulator
 */
OP_FN(_alr) {
	UNUSED(MODE);

	cpu->regA &= cpuRead(cpu, ++pc);
	_lsrA(cpu);

	return pc;
}

/* ANDs a byte with accumulator. If the result is negative, sets the carry
 * flag
 */
OP_FN(_anc) {
	cpu->regA &= MEMADDR;
	UPDATE(cpu->regA);

	if( cpu->status.negative != 0 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	return pc;
}

/* ANDs byte with the accumulator */
OP_FN(_and) {
	cpu->regA &= MEMADDR;
	UPDATE(cpu->regA);

	return pc;
}

/* ANDs a byte with the accumulator, then rotatse the accumulator bits right.
 * Status flags are set according to the table:
 * ┌───────────────┬───────┐
 * │  RESULT BITS  │ FLAGS │ Where C is the Carry flag,
 * ├───────┬───────┼───┬───┤ and V is the Overflow flag
 * │ bit 5 │ bit 6 │ C │ V │
 * ├───────┼───────┼───┼───┤
 * │   0   │   0   │ 1 │ 0 │
 * ├───────┼───────┼───┼───┤
 * │   1   │   1   │ 0 │ 0 │
 * ├───────┼───────┼───┼───┤
 * │   1   │   0   │ 0 │ 1 │
 * ├───────┼───────┼───┼───┤
 * │   0   │   1   │ 1 │ 1 │
 * └───────┴───────┴───┴───┘
 */
OP_FN(_arr) {
	cpu->regA &= MEMADDR;
	_rorA(cpu);
	UPDATE(cpu->regA);

	const uint8_t BIT_5 = (cpu->regA >> 5) & 1;
	const uint8_t BIT_6 = (cpu->regA >> 6) & 1;

	if( BIT_6 == 1 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	if( (BIT_5 ^ BIT_6) == 1 ) {
		_setOverflow(cpu);
	} else {
		_clearOverflow(cpu);
	}

	return pc;
}

/* Performs an arithmetic shift left on a byte */
OP_FN(_asl) {
	if( MODE == M_IMPLIED ) {
		if( cpu->regA > 127 ) {
			_setCarry(cpu);
		} else {
			_clearCarry(cpu);
		}

		cpu->regA = (uint8_t)(cpu->regA << 1);
		UPDATE(cpu->regA);
	} else {
		const uint16_t ADDRESS = ADDR;
		const uint8_t VALUE = cpuRead(cpu, ADDRESS);

		if( VALUE > 127 ) {
			_setCarry(cpu);
		} else {
			_clearCarry(cpu);
		}

		cpuWrite(cpu, ADDRESS, VALUE << 1);
		UPDATE(VALUE);
	}

	return pc;
}

/* ANDs a byte with the accumulator, and then transfers the result to the X
 * register
 */
OP_FN(_atx) {
	cpu->regA &= MEMADDR;
	cpu->regX = cpu->regA;

	UPDATE(cpu->regA);

	return pc;
}

/* ANDs the X register with the accumulator, and subtracts a byte from the
 * result
 */
OP_FN(_axs) {
	const uint8_t VALUE = MEMADDR;

	cpu->regX &= cpu->regA;
	if( VALUE <= cpu->regX ) {
		_setCarry(cpu);
	}

	cpu->regX -= VALUE;
	UPDATE(cpu->regX);

	return pc;
}

/* Branches if the carry flag is clear */
OP_FN(_bcc) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.carry == 0);
}

/* Branches if the carry flag is set */
OP_FN(_bcs) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.carry == 1);
}

/* Branches if the zero flag is set */
OP_FN(_beq) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.zero == 1);
}

/* Tests if the bits of a memory location are set by ANDing them with a mask
 *
 * Status flags are set like the following:
 * - Zero flag: set if the result is zero;
 * - Overflow flag: set to bit 6 of the memory location;
 * - Negative flag: set to bit 7 of the memory location
 */
OP_FN(_bit) {
	const uint8_t VALUE = MEMADDR;

	if( (cpu->regA & VALUE) == 0 ) {
		_setZero(cpu);
	} else {
		_clearZero(cpu);
	}

	if( (VALUE & SIGN_BIT) != 0 ) {
		_setNegative(cpu);
	} else {
		_clearNegative(cpu);
	}

	if( (VALUE & OVERFLOW_BIT) != 0 ) {
		_setOverflow(cpu);
	} else {
		_clearOverflow(cpu);
	}

	return pc;
}

/* Branches if the negative flag is set */
OP_FN(_bmi) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.negative == 1);
}

/* Branches if the zero flag is clear */
OP_FN(_bne) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.zero == 0);
}

/* Branches if the negative flag is clear */
OP_FN(_bpl) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.negative == 0);
}

/* Branches if the overflow flag is clear */
OP_FN(_bvc) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.overflow == 0);
}

/* Branches if the overflow flag is set */
OP_FN(_bvs) {
	UNUSED(MODE);
	return _branch(cpu, pc, cpu->status.overflow == 1);
}

/* Clears the carry flag */
OP_FN(_clc) {
	UNUSED(MODE);
	_clearCarry(cpu);

	return pc;
}

/* Clears the decimal flag */
OP_FN(_cld) {
	UNUSED(MODE);
	_clearDecimal(cpu);

	return pc;
}

/* Clears the interrupt flag */
OP_FN(_cli) {
	UNUSED(MODE);
	_clearIntr(cpu);

	return pc;
}

/* Clears the overflow flag */
OP_FN(_clv) {
	UNUSED(MODE);
	_clearOverflow(cpu);

	return pc;
}

/* Compares the accumulator (A) to a value in memory (M)
 *
 * Status flags are set like the following:
 * - Carry flag: set if A >= M;
 * - Zero flag: set if A = M;
 * - Negative flag: set if A - M is negative
 */
OP_FN(_cmp) {
	const uint8_t VALUE = MEMADDR;

	if( VALUE <= cpu->regA ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	UPDATE(cpu->regA - VALUE);

	return pc;
}

/* Same as CMP, but compares with the X register instead of the accumulator */
OP_FN(_cpx) {
	const uint8_t VALUE = MEMADDR;

	if( VALUE <= cpu->regX ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	UPDATE(cpu->regX - VALUE);

	return pc;
}

/* Same as CMP, but compares with the Y register instead of the accumulator */
OP_FN(_cpy) {
	const uint8_t VALUE = MEMADDR;

	if( VALUE <= cpu->regY ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	UPDATE(cpu->regY - VALUE);

	return pc;
}

/* Subtracts 1 from a value in memory, without borrow */
OP_FN(_dcp) {
	const uint16_t ADDRESS = ADDR;

	const uint8_t VALUE = cpuRead(cpu, ADDRESS) - 1;
	cpuWrite(cpu, ADDRESS, VALUE);

	if( VALUE <= cpu->regA ) {
		_setCarry(cpu);
	}

	UPDATE(cpu->regA - VALUE);
	return pc;
}

/* Decreases a values in memory by 1 */
OP_FN(_dec) {
	UNUSED(MODE);

	const uint16_t ADDRESS = ADDR;
	const uint8_t VALUE = cpuRead(cpu, ADDRESS) - 1;

	cpuWrite(cpu, ADDRESS, VALUE);
	UPDATE(VALUE);

	return pc;
}

/* Decreases the value in the X register by 1 */
OP_FN(_dex) {
	UNUSED(MODE);

	--cpu->regX;
	UPDATE(cpu->regX);

	return pc;
}

/* Decreases the value in the Y register by 1 */
OP_FN(_dey) {
	UNUSED(MODE);

	--cpu->regY;
	UPDATE(cpu->regY);

	return pc;
}

/* XORs (exclusive or) the accumulator with a byte */
OP_FN(_eor) {
	cpu->regA ^= MEMADDR;
	UPDATE(cpu->regA);

	return pc;
}

/* Increases a value in memory by 1 */
OP_FN(_inc) {
	const uint16_t ADDRESS = ADDR;
	const uint8_t VALUE = cpuRead(cpu, ADDRESS) + 1;

	cpuWrite(cpu, ADDRESS, VALUE);
	UPDATE(VALUE);

	return pc;
}

/* Increases the value in the X register by 1 */
OP_FN(_inx) {
	UNUSED(MODE);

	++cpu->regX;
	UPDATE(cpu->regX);

	return pc;
}

/* Increases the value in the Y register by 1 */
OP_FN(_iny) {
	UNUSED(MODE);

	++cpu->regY;
	UPDATE(cpu->regY);

	return pc;
}

/* Increases a value in memory by 1, then subtracts the result from the
 * accumulator
 */
OP_FN(_isb) {
	const uint16_t ADDRESS = ADDR;
	const uint8_t VALUE = cpuRead(cpu, ADDRESS) + 1;

	cpuWrite(cpu, ADDRESS, VALUE);
	UPDATE(VALUE);

	_addToA(cpu, 255 - cpuRead(cpu, ADDRESS));

	return pc;
}

/* Jumps (sets the PC to) an address */
OP_FN(_jmp) {
	UNUSED(MODE);
	return ADDR;
}

/* Jumps (sets the PC to) an address, and pushes the current address to the
 * stack (- 1)
 */
OP_FN(_jsr) {
	UNUSED(MODE);
	_push16(cpu, pc + 1);

	return ADDR;
}

/* ANDs a value in memory with the stack pointer.
 * The result is stored on the accumulator, X register and stack pointer
 */
OP_FN(_las) {
	cpu->stack &= MEMADDR;
	UPDATE(cpu->stack);

	cpu->regA = cpu->stack;
	cpu->regX = cpu->stack;

	return pc;
}

/* Loads the X register and the accumulator with a value in memory */
OP_FN(_lax) {
	const uint8_t VALUE = MEMADDR;
	UPDATE(VALUE);

	cpu->regA = VALUE;
	cpu->regX = VALUE;

	return pc;
}

/* Loads the accumulator with a value in memory */
OP_FN(_lda) {
	cpu->regA = MEMADDR;
	UPDATE(cpu->regA);

	return pc;
}

/* Loads the X register with a value in memory */
OP_FN(_ldx) {
	cpu->regX = MEMADDR;
	UPDATE(cpu->regX);

	return pc;
}

/* Loads the Y register with a value in memory */
OP_FN(_ldy) {
	cpu->regY = MEMADDR;
	UPDATE(cpu->regY);

	return pc;
}

/* Performs a logical shift right on a byte */
OP_FN(_lsr) {
	if( MODE == M_IMPLIED ) {
		_lsrA(cpu);
	} else {
		const uint16_t ADDRESS = ADDR;
		const uint8_t VALUE = cpuRead(cpu, ADDRESS);

		if( (VALUE & 1) == 1 ) {
			_setCarry(cpu);
		} else {
			_clearCarry(cpu);
		}

		cpuWrite(cpu, ADDRESS, VALUE >> 1);
		UPDATE(VALUE);
	}

	return pc;
}

/* No operation */
OP_FN(_nop) {
	(void)ADDR;
	return pc;
}

/* ORs a byte with the accumulator */
OP_FN(_ora) {
	cpu->regA |= MEMADDR;
	UPDATE(cpu->regA);

	return pc;
}

/* Pushes a copy of the accumulator to the stack */
OP_FN(_pha) {
	UNUSED(MODE);
	_push(cpu, cpu->regA);

	return pc;
}

/* Pushes a copy of the status flags (P register) to the stack
 * The B flag and the 1 flag of the P register are set
 */
OP_FN(_php) {
	UNUSED(MODE);
	_push(cpu, cpu->status.bits | BFLAG1_BIT | BFLAG2_BIT);

	return pc;
}

/* Pulls a value from the stack and sets the accumulator to that value */
OP_FN(_pla) {
	UNUSED(MODE);

	cpu->regA = _pull(cpu);
	UPDATE(cpu->regA);

	return pc;
}

/* Pulls a value from the stack and sets the status flags (P register) to that
 * value
 *
 * The 1 flag is set
 */
OP_FN(_plp) {
	UNUSED(MODE);

	cpu->status.bits = _pull(cpu);
	cpu->status.bFlag2 = 1;

	_clearBFlag1(cpu);

	return pc;
}

/* Rotates a value in memory left (see: ROL,) and AND the result with the
 * accumulator
 */
OP_FN(_rla) {
	const uint16_t ADDRESS = ADDR;
	const uint8_t VALUE = cpuRead(cpu, ADDRESS);
	const bool OLD_CARRY = (cpu->status.carry == 1);

	if( (VALUE >> 7) == 1 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	uint8_t val = VALUE << 1;
	if( OLD_CARRY ) {
		val |= 1;
	}

	cpuWrite(cpu, ADDRESS, val);

	cpu->regA &= val;
	UPDATE(cpu->regA);

	return pc;
}

/* Rotates a value in memory right (see: ROR,) and adds the result to the
 * accumulator
 */
OP_FN(_rra) {
	const uint16_t ADDRESS = ADDR;
	const uint8_t VALUE = cpuRead(cpu, ADDRESS);
	const bool OLD_CARRY = (cpu->status.carry == 1);

	if( (VALUE & 1) == 1 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	uint8_t val = (VALUE >> 1);
	if( OLD_CARRY ) {
		val |= SIGN_BIT;
	}

	cpuWrite(cpu, ADDRESS, val);
	_addToA(cpu, val);

	return pc;
}

/* Rotates a byte's bits left. This is the same as a LSR, but the 7th bit, that
 * is normally "pushed" out, instead wraps around to the 0th bit
 *
 *   1  0  0  0  1  0  1  0
 *   │  │  │  │  │  │  │  │
 * ┌─┘┌─┘┌─┘┌─┘┌─┘┌─┘┌─┘ x┘
 * │  │  │  │  │  │  │
 * 1  0  0  0  1  0  1  1
 * └────────────────────┘
 */
OP_FN(_rol) {
	if( MODE == M_IMPLIED ) {
		const bool OLD_CARRY = (cpu->status.carry == 1);

		if( (cpu->regA >> 7) == 1 ) {
			_setCarry(cpu);
		} else {
			_clearCarry(cpu);
		}

		cpu->regA = (uint8_t)(cpu->regA << 1);
		if( OLD_CARRY ) {
			cpu->regA |= 1;
		}

		UPDATE(cpu->regA);
	} else {
		const uint16_t ADDRESS = ADDR;
		const uint8_t VALUE = cpuRead(cpu, ADDRESS);
		const bool OLD_CARRY = (cpu->status.carry == 1);

		if( (VALUE >> 7) == 1 ) {
			_setCarry(cpu);
		} else {
			_clearCarry(cpu);
		}

		uint8_t val = VALUE << 1;
		if( OLD_CARRY ) {
			val |= 1;
		}

		cpuWrite(cpu, ADDRESS, val);
		UPDATE(val);
	}

	return pc;
}

/* See: ROL, but rotates right */
OP_FN(_ror) {
	if( MODE == M_IMPLIED ) {
		_rorA(cpu);
	} else {
		const uint16_t ADDRESS = ADDR;
		const uint8_t VALUE = cpuRead(cpu, ADDRESS);
		const bool OLD_CARRY = (cpu->status.carry == 1);

		if( (VALUE & 1) == 1 ) {
			_setCarry(cpu);
		} else {
			_clearCarry(cpu);
		}

		uint8_t val = (VALUE >> 1);
		if( OLD_CARRY ) {
			val |= SIGN_BIT;
		}

		cpuWrite(cpu, ADDRESS, val);
		UPDATE(val);
	}

	return pc;
}

/* Returns from an interrupt by pulling the status flags (P register) from the
 * stack, followed by the PC
 *
 * The 1 flag is set, and the B flag is cleared
 */
OP_FN(_rti) {
	UNUSED(MODE);
	UNUSED(pc);

	cpu->status.bits = _pull(cpu);
	cpu->status.bFlag2 = 1;

	_clearBFlag1(cpu);

	return _pull16(cpu);
}
/* Returns from a subroutine by pulling the PC from the stack */
OP_FN(_rts) {
	UNUSED(MODE);
	UNUSED(pc);

	return _pull16(cpu) + 1;
}

/* ANDs the A and X registers together and stores the result in memory */
OP_FN(_sax) {
	cpuWrite(cpu, ADDR, cpu->regA & cpu->regX);
	return pc;
}

/* Subtracts a byte from the accumulator, with carry */
OP_FN(_sbc) {
	_addToA(cpu, 255 - cpuRead(cpu, ADDR));
	return pc;
}

/* Sets the carry flag */
OP_FN(_sec) {
	UNUSED(MODE);
	_setCarry(cpu);

	return pc;
}

/* Sets the decimal flag */
OP_FN(_sed) {
	UNUSED(MODE);
	_setDecimal(cpu);

	return pc;
}

/* Sets the interrupt flag */
OP_FN(_sei) {
	UNUSED(MODE);
	_setIntr(cpu);

	return pc;
}

/* ANDs the X register with the high byte of the given address, + 1
 * The result is stored in memory
 */
OP_FN(_shx) {
	const uint16_t ADDRESS = ADDR;
	const uint8_t HI_BYTE = (uint8_t)((ADDRESS >> 8) + 1);

	cpuWrite(cpu, ADDRESS, cpu->regX & HI_BYTE);

	return pc;
}

/* ANDs the Y register with the high byte of the given address, + 1
 * The result is stored in memory
 */
OP_FN(_shy) {
	const uint16_t ADDRESS = ADDR;
	const uint8_t HI_BYTE = (uint8_t)((ADDRESS >> 8) + 1);

	cpuWrite(cpu, ADDRESS, cpu->regY & HI_BYTE);

	return pc;
}

/* ASLs a value in memory and ORs the accumulator with the result */
OP_FN(_slo) {
	const uint16_t ADDRESS = ADDR;
	uint8_t value = cpuRead(cpu, ADDRESS);

	if( value > 127 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	value = (uint8_t)(value << 1);

	cpuWrite(cpu, ADDRESS, value);

	cpu->regA |= value;
	UPDATE(cpu->regA);

	return pc;
}

/* LSRs a value in memory and XORs the accumulator with the result */
OP_FN(_sre) {
	const uint16_t ADDRESS = ADDR;
	uint8_t value = cpuRead(cpu, ADDRESS);

	if( (value & 1) == 1 ) {
		_setCarry(cpu);
	} else {
		_clearCarry(cpu);
	}

	value >>= 1;

	cpuWrite(cpu, ADDRESS, value);
	cpu->regA ^= value;

	return pc;
}

/* Stores the accumulator contents in memory */
OP_FN(_sta) {
	cpuWrite(cpu, ADDR, cpu->regA);
	return pc;
}

/* Stores the X register contents in memory */
OP_FN(_stx) {
	cpuWrite(cpu, ADDR, cpu->regX);
	return pc;
}

/* Stores the Y register contents in memory */
OP_FN(_sty) {
	cpuWrite(cpu, ADDR, cpu->regY);
	return pc;
}

/* ANDs the accumulator with the X register and stores the result on the stack
 * pointer
 *
 * The stack pointer is then ANDed with the high byte of the given memory
 * address + 1, and the result of that operation is stored in memory
 *
 * A very useful operation with many, many *obvious* use cases :)
 */
OP_FN(_tas) {
	cpu->stack = cpu->regA & cpu->regX;

	const uint16_t ADDRESS = ADDR;
	const uint8_t HI_BYTE = (uint8_t)((ADDR >> 8) + 1);

	cpuWrite(cpu, ADDRESS, cpu->stack & HI_BYTE);
	return pc;
}

/* Transfer the contents of the accumulator to the X register */
OP_FN(_tax) {
	UNUSED(MODE);

	cpu->regX = cpu->regA;
	UPDATE(cpu->regX);

	return pc;
}

/* Transfer the contents of the accumulator to the Y register */
OP_FN(_tay) {
	UNUSED(MODE);

	cpu->regY = cpu->regA;
	UPDATE(cpu->regY);

	return pc;
}

/* Transfer the stack pointer to the X register */
OP_FN(_tsx) {
	UNUSED(MODE);

	cpu->regX = cpu->stack;
	UPDATE(cpu->regX);

	return pc;
}

/* Transfer the contents of the X register to the accumulator */
OP_FN(_txa) {
	UNUSED(MODE);

	cpu->regA = cpu->regX;
	UPDATE(cpu->regA);

	return pc;
}

/* Transfer the contents of the X register to the stack pointer */
OP_FN(_txs) {
	UNUSED(MODE);
	cpu->stack = cpu->regX;

	return pc;
}

/* Transfer the contents of the Y register to the accumulator */
OP_FN(_tya) {
	UNUSED(MODE);

	cpu->regA = cpu->regY;
	UPDATE(cpu->regA);

	return pc;
}

/* This operation is weird and "unpredictable"
 *
 * This implementation is based on this page:
 * https://www.nesdev.org/wiki/Visual6502wiki/6502_Opcode_8B_(XAA,_ANE)
 *
 * Though not 100% accurate due to the very nature of this OP, this
 * implementation ANDs the accumulator with the X register, the input byte and
 * the accumulator itself
 *
 * The input accumulator is first ORed with a "magic" (in this case, 0xFF,)
 * which emulates the hardware weirdness that allows only certain bits of the
 * accumulator to be ANDed with itself.
 *
 * The magic numbers tend to be: 0xFF, 0xFE, 0xEE or 0x00
 *
 * Different 6502 models/versions have different magic numbers
 */
OP_FN(_xaa) {
	cpu->regA = (cpu->regA | 0xFF) & cpu->regX & MEMADDR;
	UPDATE(cpu->regA);

	return pc;
}

static const Op OPS[256] = {
	[0x69] = {_adc, M_IMMEDIATE, 2, 2, "ADC"},
	[0x65] = {_adc, M_ZEROPAGE, 3, 2, "ADC"},
	[0x75] = {_adc, M_ZEROPAGE_X, 4, 2, "ADC"},
	[0x6D] = {_adc, M_ABSOLUTE, 4, 3, "ADC"},
	[0x7D] = {_adc, M_ABSOLUTE_X, 4, 3, "ADC"},
	[0x79] = {_adc, M_ABSOLUTE_Y, 4, 3, "ADC"},
	[0x61] = {_adc, M_INDIRECT_X, 6, 2, "ADC"},
	[0x71] = {_adc, M_INDIRECT_Y, 5, 2, "ADC"},

	[0x9F] = {_ahx, M_ABSOLUTE_Y, 5, 3, "*AHX"},
	[0x93] = {_ahx, M_INDIRECT_Y, 6, 3, "*AHX"},

	[0x4B] = {_alr, M_IMMEDIATE, 2, 2, "*ALR"},

	[0x0B] = {_anc, M_IMMEDIATE, 2, 2, "*ANC"},
	[0x2B] = {_anc, M_IMMEDIATE, 2, 2, "*ANC"},

	[0x29] = {_and, M_IMMEDIATE, 2, 2, "AND"},
	[0x25] = {_and, M_ZEROPAGE, 3, 2, "AND"},
	[0x35] = {_and, M_ZEROPAGE_X, 4, 2, "AND"},
	[0x2D] = {_and, M_ABSOLUTE, 4, 3, "AND"},
	[0x3D] = {_and, M_ABSOLUTE_X, 4, 3, "AND"},
	[0x39] = {_and, M_ABSOLUTE_Y, 4, 3, "AND"},
	[0x21] = {_and, M_INDIRECT_X, 6, 2, "AND"},
	[0x31] = {_and, M_INDIRECT_Y, 5, 2, "AND"},

	[0x6B] = {_arr, M_IMMEDIATE, 2, 2, "*ARR"},

	[0x0A] = {_asl, M_IMPLIED, 2, 1, "ASL"},
	[0x06] = {_asl, M_ZEROPAGE, 5, 2, "ASL"},
	[0x16] = {_asl, M_ZEROPAGE_X, 6, 2, "ASL"},
	[0x0E] = {_asl, M_ABSOLUTE, 6, 3, "ASL"},
	[0x1E] = {_asl, M_ABSOLUTE_X, 7, 3, "ASL"},

	[0XAB] = {_atx, M_IMMEDIATE, 2, 2, "*ATX"},

	[0XCB] = {_axs, M_IMMEDIATE, 2, 2, "*AXS"},

	[0x90] = {_bcc, M_RELATIVE, 2, 2, "BCC"},
	[0xB0] = {_bcs, M_RELATIVE, 2, 2, "BCS"},
	[0xF0] = {_beq, M_RELATIVE, 2, 2, "BEQ"},
	[0xD0] = {_bne, M_RELATIVE, 2, 2, "BNE"},

	[0x24] = {_bit, M_ZEROPAGE, 3, 2, "BIT"},
	[0x2C] = {_bit, M_ABSOLUTE, 4, 3, "BIT"},

	[0x30] = {_bmi, M_RELATIVE, 2, 2, "BMI"},
	[0x10] = {_bpl, M_RELATIVE, 2, 2, "BPL"},
	[0x50] = {_bvc, M_RELATIVE, 2, 2, "BVC"},
	[0x70] = {_bvs, M_RELATIVE, 2, 2, "BVS"},

	[0x18] = {_clc, M_IMPLIED, 2, 1, "CLC"},
	[0xD8] = {_cld, M_IMPLIED, 2, 1, "CLD"},
	[0x58] = {_cli, M_IMPLIED, 2, 1, "CLI"},
	[0xB8] = {_clv, M_IMPLIED, 2, 1, "CLV"},

	[0xC9] = {_cmp, M_IMMEDIATE, 2, 2, "CMP"},
	[0xC5] = {_cmp, M_ZEROPAGE, 3, 2, "CMP"},
	[0xD5] = {_cmp, M_ZEROPAGE_X, 4, 2, "CMP"},
	[0xCD] = {_cmp, M_ABSOLUTE, 4, 3, "CMP"},
	[0xDD] = {_cmp, M_ABSOLUTE_X, 4, 3, "CMP"},
	[0xD9] = {_cmp, M_ABSOLUTE_Y, 4, 3, "CMP"},
	[0xC1] = {_cmp, M_INDIRECT_X, 6, 2, "CMP"},
	[0xD1] = {_cmp, M_INDIRECT_Y, 5, 2, "CMP"},

	[0xE0] = {_cpx, M_IMMEDIATE, 2, 2, "CPX"},
	[0xE4] = {_cpx, M_ZEROPAGE, 3, 2, "CPX"},
	[0xEC] = {_cpx, M_ABSOLUTE, 4, 3, "CPX"},

	[0xC0] = {_cpy, M_IMMEDIATE, 2, 2, "CPY"},
	[0xC4] = {_cpy, M_ZEROPAGE, 3, 2, "CPY"},
	[0xCC] = {_cpy, M_ABSOLUTE, 4, 3, "CPY"},

	[0xC7] = {_dcp, M_ZEROPAGE, 5, 2, "*DCP"},
	[0xD7] = {_dcp, M_ZEROPAGE_X, 6, 2, "*DCP"},
	[0xCF] = {_dcp, M_ABSOLUTE, 6, 3, "*DCP"},
	[0xDF] = {_dcp, M_ABSOLUTE_X, 7, 3, "*DCP"},
	[0xDB] = {_dcp, M_ABSOLUTE_Y, 7, 3, "*DCP"},
	[0xC3] = {_dcp, M_INDIRECT_X, 8, 2, "*DCP"},
	[0xD3] = {_dcp, M_INDIRECT_Y, 8, 2, "*DCP"},

	[0xC6] = {_dec, M_ZEROPAGE, 5, 2, "DEC"},
	[0xD6] = {_dec, M_ZEROPAGE_X, 6, 2, "DEC"},
	[0xCE] = {_dec, M_ABSOLUTE, 6, 3, "DEC"},
	[0xDE] = {_dec, M_ABSOLUTE_X, 7, 3, "DEC"},

	[0xCA] = {_dex, M_IMPLIED, 2, 1, "DEX"},
	[0x88] = {_dey, M_IMPLIED, 2, 1, "DEY"},

	[0x49] = {_eor, M_IMMEDIATE, 2, 2, "EOR"},
	[0x45] = {_eor, M_ZEROPAGE, 3, 2, "EOR"},
	[0x55] = {_eor, M_ZEROPAGE_X, 4, 2, "EOR"},
	[0x4D] = {_eor, M_ABSOLUTE, 4, 3, "EOR"},
	[0x5D] = {_eor, M_ABSOLUTE_X, 4, 3, "EOR"},
	[0x59] = {_eor, M_ABSOLUTE_Y, 4, 3, "EOR"},
	[0x41] = {_eor, M_INDIRECT_X, 6, 2, "EOR"},
	[0x51] = {_eor, M_INDIRECT_Y, 5, 2, "EOR"},

	[0xE6] = {_inc, M_ZEROPAGE, 5, 2, "INC"},
	[0xF6] = {_inc, M_ZEROPAGE_X, 6, 2, "INC"},
	[0xEE] = {_inc, M_ABSOLUTE, 6, 3, "INC"},
	[0xFE] = {_inc, M_ABSOLUTE_X, 7, 3, "INC"},

	[0xE8] = {_inx, M_IMPLIED, 2, 1, "INX"},
	[0xC8] = {_iny, M_IMPLIED, 2, 1, "INY"},

	[0xE7] = {_isb, M_ZEROPAGE, 5, 2, "*ISB"},
	[0xF7] = {_isb, M_ZEROPAGE_X, 6, 2, "*ISB"},
	[0xEF] = {_isb, M_ABSOLUTE, 6, 3, "*ISB"},
	[0xFF] = {_isb, M_ABSOLUTE_X, 7, 3, "*ISB"},
	[0xFB] = {_isb, M_ABSOLUTE_Y, 7, 3, "*ISB"},
	[0xE3] = {_isb, M_INDIRECT_X, 8, 2, "*ISB"},
	[0xF3] = {_isb, M_INDIRECT_Y, 8, 2, "*ISB"},

	[0x4C] = {_jmp, M_ABSOLUTE, 3, 3, "JMP"},
	[0x6C] = {_jmp, M_INDIRECT, 5, 3, "JMP"},

	[0x20] = {_jsr, M_ABSOLUTE, 6, 3, "JSR"},

	[0xBB] = {_las, M_ABSOLUTE_Y, 4, 3, "*LAS"},

	[0xA7] = {_lax, M_ZEROPAGE, 3, 2, "*LAX"},
	[0xB7] = {_lax, M_ZEROPAGE_Y, 4, 2, "*LAX"},
	[0xAF] = {_lax, M_ABSOLUTE, 4, 3, "*LAX"},
	[0xBF] = {_lax, M_ABSOLUTE_Y, 4, 3, "*LAX"},
	[0xA3] = {_lax, M_INDIRECT_X, 6, 2, "*LAX"},
	[0xB3] = {_lax, M_INDIRECT_Y, 5, 2, "*LAX"},

	[0xA9] = {_lda, M_IMMEDIATE, 2, 2, "LDA"},
	[0xA5] = {_lda, M_ZEROPAGE, 3, 2, "LDA"},
	[0xB5] = {_lda, M_ZEROPAGE_X, 4, 2, "LDA"},
	[0xAD] = {_lda, M_ABSOLUTE, 4, 3, "LDA"},
	[0xBD] = {_lda, M_ABSOLUTE_X, 4, 3, "LDA"},
	[0xB9] = {_lda, M_ABSOLUTE_Y, 4, 3, "LDA"},
	[0xA1] = {_lda, M_INDIRECT_X, 6, 2, "LDA"},
	[0xB1] = {_lda, M_INDIRECT_Y, 5, 2, "LDA"},

	[0xA2] = {_ldx, M_IMMEDIATE, 2, 2, "LDX"},
	[0xA6] = {_ldx, M_ZEROPAGE, 3, 2, "LDX"},
	[0xB6] = {_ldx, M_ZEROPAGE_Y, 4, 2, "LDX"},
	[0xAE] = {_ldx, M_ABSOLUTE, 4, 3, "LDX"},
	[0xBE] = {_ldx, M_ABSOLUTE_Y, 4, 3, "LDX"},

	[0xA0] = {_ldy, M_IMMEDIATE, 2, 2, "LDY"},
	[0xA4] = {_ldy, M_ZEROPAGE, 3, 2, "LDY"},
	[0xB4] = {_ldy, M_ZEROPAGE_X, 4, 2, "LDY"},
	[0xAC] = {_ldy, M_ABSOLUTE, 4, 3, "LDY"},
	[0xBC] = {_ldy, M_ABSOLUTE_X, 4, 3, "LDY"},

	[0x4A] = {_lsr, M_IMPLIED, 2, 1, "LSR"},
	[0x46] = {_lsr, M_ZEROPAGE, 5, 2, "LSR"},
	[0x56] = {_lsr, M_ZEROPAGE_X, 6, 2, "LSR"},
	[0x4E] = {_lsr, M_ABSOLUTE, 6, 3, "LSR"},
	[0x5E] = {_lsr, M_ABSOLUTE_X, 7, 3, "LSR"},

	[0xEA] = {_nop, M_IMPLIED, 2, 1, "NOP"},

	[0x1A] = {_nop, M_IMPLIED, 2, 1, "*NOP"}, /* Unofficial NOPs */
	[0x3A] = {_nop, M_IMPLIED, 2, 1, "*NOP"},
	[0x5A] = {_nop, M_IMPLIED, 2, 1, "*NOP"},
	[0x7A] = {_nop, M_IMPLIED, 2, 1, "*NOP"},
	[0xDA] = {_nop, M_IMPLIED, 2, 1, "*NOP"},
	[0xFA] = {_nop, M_IMPLIED, 2, 1, "*NOP"},

	[0x04] = {_nop, M_ZEROPAGE, 3, 2, "*NOP"}, /* DOP (double NOP) */
	[0x14] = {_nop, M_ZEROPAGE_X, 4, 2, "*NOP"},
	[0x34] = {_nop, M_ZEROPAGE_X, 4, 2, "*NOP"},
	[0x44] = {_nop, M_ZEROPAGE, 3, 2, "*NOP"},
	[0x54] = {_nop, M_ZEROPAGE_X, 4, 2, "*NOP"},
	[0x64] = {_nop, M_ZEROPAGE, 3, 2, "*NOP"},
	[0x74] = {_nop, M_ZEROPAGE_X, 4, 2, "*NOP"},
	[0x80] = {_nop, M_IMMEDIATE, 2, 2, "*NOP"},
	[0x82] = {_nop, M_IMMEDIATE, 2, 2, "*NOP"},
	[0x89] = {_nop, M_IMMEDIATE, 2, 2, "*NOP"},
	[0xC2] = {_nop, M_IMMEDIATE, 2, 2, "*NOP"},
	[0xD4] = {_nop, M_ZEROPAGE_X, 4, 2, "*NOP"},
	[0xE2] = {_nop, M_IMMEDIATE, 2, 2, "*NOP"},
	[0xF4] = {_nop, M_ZEROPAGE_X, 4, 2, "*NOP"},

	[0x0C] = {_nop, M_ABSOLUTE, 4, 3, "*NOP"}, /* TOP (triple NOP) */
	[0x1C] = {_nop, M_ABSOLUTE_X, 4, 3, "*NOP"},
	[0x3C] = {_nop, M_ABSOLUTE_X, 4, 3, "*NOP"},
	[0x5C] = {_nop, M_ABSOLUTE_X, 4, 3, "*NOP"},
	[0x7C] = {_nop, M_ABSOLUTE_X, 4, 3, "*NOP"},
	[0xDC] = {_nop, M_ABSOLUTE_X, 4, 3, "*NOP"},
	[0xFC] = {_nop, M_ABSOLUTE_X, 4, 3, "*NOP"},

	[0x09] = {_ora, M_IMMEDIATE, 2, 2, "ORA"},
	[0x05] = {_ora, M_ZEROPAGE, 3, 2, "ORA"},
	[0x15] = {_ora, M_ZEROPAGE_X, 4, 2, "ORA"},
	[0x0D] = {_ora, M_ABSOLUTE, 4, 3, "ORA"},
	[0x1D] = {_ora, M_ABSOLUTE_X, 4, 3, "ORA"},
	[0x19] = {_ora, M_ABSOLUTE_Y, 4, 3, "ORA"},
	[0x01] = {_ora, M_INDIRECT_X, 6, 2, "ORA"},
	[0x11] = {_ora, M_INDIRECT_Y, 5, 2, "ORA"},

	[0x48] = {_pha, M_IMPLIED, 3, 1, "PHA"},
	[0x08] = {_php, M_IMPLIED, 3, 1, "PHP"},
	[0x68] = {_pla, M_IMPLIED, 4, 1, "PLA"},
	[0x28] = {_plp, M_IMPLIED, 4, 1, "PLP"},

	[0x27] = {_rla, M_ZEROPAGE, 5, 2, "*RLA"},
	[0x37] = {_rla, M_ZEROPAGE_X, 6, 2, "*RLA"},
	[0x2F] = {_rla, M_ABSOLUTE, 6, 3, "*RLA"},
	[0x3F] = {_rla, M_ABSOLUTE_X, 7, 3, "*RLA"},
	[0x3B] = {_rla, M_ABSOLUTE_Y, 7, 3, "*RLA"},
	[0x23] = {_rla, M_INDIRECT_X, 8, 2, "*RLA"},
	[0x33] = {_rla, M_INDIRECT_Y, 8, 2, "*RLA"},

	[0x67] = {_rra, M_ZEROPAGE, 5, 2, "*RRA"},
	[0x77] = {_rra, M_ZEROPAGE_X, 6, 2, "*RRA"},
	[0x6F] = {_rra, M_ABSOLUTE, 6, 3, "*RRA"},
	[0x7F] = {_rra, M_ABSOLUTE_X, 7, 3, "*RRA"},
	[0x7B] = {_rra, M_ABSOLUTE_Y, 7, 3, "*RRA"},
	[0x63] = {_rra, M_INDIRECT_X, 8, 2, "*RRA"},
	[0x73] = {_rra, M_INDIRECT_Y, 8, 2, "*RRA"},

	[0x2A] = {_rol, M_IMPLIED, 2, 1, "ROL"},
	[0x26] = {_rol, M_ZEROPAGE, 5, 2, "ROL"},
	[0x36] = {_rol, M_ZEROPAGE_X, 6, 2, "ROL"},
	[0x2E] = {_rol, M_ABSOLUTE, 6, 3, "ROL"},
	[0x3E] = {_rol, M_ABSOLUTE_X, 7, 3, "ROL"},

	[0x6A] = {_ror, M_IMPLIED, 2, 1, "ROR"},
	[0x66] = {_ror, M_ZEROPAGE, 5, 2, "ROR"},
	[0x76] = {_ror, M_ZEROPAGE_X, 6, 2, "ROR"},
	[0x6E] = {_ror, M_ABSOLUTE, 6, 3, "ROR"},
	[0x7E] = {_ror, M_ABSOLUTE_X, 7, 3, "ROR"},

	[0x40] = {_rti, M_IMPLIED, 6, 1, "RTI"},
	[0x60] = {_rts, M_IMPLIED, 6, 1, "RTS"},

	[0x87] = {_sax, M_ZEROPAGE, 3, 2, "*SAX"},
	[0x97] = {_sax, M_ZEROPAGE_Y, 4, 2, "*SAX"},
	[0x83] = {_sax, M_INDIRECT_X, 6, 2, "*SAX"},
	[0x8F] = {_sax, M_ABSOLUTE, 4, 3, "*SAX"},

	[0xE9] = {_sbc, M_IMMEDIATE, 2, 2, "SBC"},
	[0xEB] = {_sbc, M_IMMEDIATE, 2, 2, "*SBC"},

	[0xE5] = {_sbc, M_ZEROPAGE, 3, 2, "SBC"},
	[0xF5] = {_sbc, M_ZEROPAGE_X, 4, 2, "SBC"},
	[0xED] = {_sbc, M_ABSOLUTE, 4, 3, "SBC"},
	[0xFD] = {_sbc, M_ABSOLUTE_X, 4, 3, "SBC"},
	[0xF9] = {_sbc, M_ABSOLUTE_Y, 4, 3, "SBC"},
	[0xE1] = {_sbc, M_INDIRECT_X, 6, 2, "SBC"},
	[0xF1] = {_sbc, M_INDIRECT_Y, 5, 2, "SBC"},

	[0x38] = {_sec, M_IMPLIED, 2, 1, "SEC"},
	[0xF8] = {_sed, M_IMPLIED, 2, 1, "SED"},
	[0x78] = {_sei, M_IMPLIED, 2, 1, "SEI"},

	[0x9E] = {_shx, M_ABSOLUTE_Y, 3, 3, "*SHX"},
	[0x9C] = {_shy, M_ABSOLUTE_X, 5, 3, "*SHY"},

	[0x07] = {_slo, M_ZEROPAGE, 5, 2, "*SLO"},
	[0x17] = {_slo, M_ZEROPAGE_X, 6, 2, "*SLO"},
	[0x0F] = {_slo, M_ABSOLUTE, 6, 3, "*SLO"},
	[0x1F] = {_slo, M_ABSOLUTE_X, 7, 3, "*SLO"},
	[0x1B] = {_slo, M_ABSOLUTE_Y, 7, 3, "*SLO"},
	[0x03] = {_slo, M_INDIRECT_X, 8, 2, "*SLO"},
	[0x13] = {_slo, M_INDIRECT_Y, 8, 2, "*SLO"},

	[0x47] = {_sre, M_ZEROPAGE, 5, 2, "*SLO"},
	[0x57] = {_sre, M_ZEROPAGE_X, 6, 2, "*SLO"},
	[0x4F] = {_sre, M_ABSOLUTE, 6, 3, "*SLO"},
	[0x5F] = {_sre, M_ABSOLUTE_X, 7, 3, "*SLO"},
	[0x5B] = {_sre, M_ABSOLUTE_Y, 7, 3, "*SLO"},
	[0x43] = {_sre, M_INDIRECT_X, 8, 2, "*SLO"},
	[0x53] = {_sre, M_INDIRECT_Y, 8, 2, "*SLO"},

	[0x85] = {_sta, M_ZEROPAGE, 3, 2, "STA"},
	[0x95] = {_sta, M_ZEROPAGE_X, 4, 2, "STA"},
	[0x8D] = {_sta, M_ABSOLUTE, 4, 3, "STA"},
	[0x9D] = {_sta, M_ABSOLUTE_X, 5, 3, "STA"},
	[0x99] = {_sta, M_ABSOLUTE_Y, 5, 3, "STA"},
	[0x81] = {_sta, M_INDIRECT_X, 6, 2, "STA"},
	[0x91] = {_sta, M_INDIRECT_Y, 6, 2, "STA"},

	[0x86] = {_stx, M_ZEROPAGE, 3, 2, "STX"},
	[0x96] = {_stx, M_ZEROPAGE_Y, 4, 2, "STX"},
	[0x8E] = {_stx, M_ABSOLUTE, 4, 3, "STX"},

	[0x84] = {_sty, M_ZEROPAGE, 3, 2, "STY"},
	[0x94] = {_sty, M_ZEROPAGE_X, 4, 2, "STY"},
	[0x8C] = {_sty, M_ABSOLUTE, 4, 3, "STY"},

	[0x9B] = {_tas, M_ABSOLUTE_Y, 5, 3, "*TAS"},

	[0xAA] = {_tax, M_IMPLIED, 2, 1, "TAX"},
	[0xA8] = {_tay, M_IMPLIED, 2, 1, "TAY"},
	[0xBA] = {_tsx, M_IMPLIED, 2, 1, "TSX"},
	[0x8A] = {_txa, M_IMPLIED, 2, 1, "TXA"},
	[0x9A] = {_txs, M_IMPLIED, 2, 1, "TXS"},
	[0x98] = {_tya, M_IMPLIED, 2, 1, "TYA"},

	[0x8B] = {_xaa, M_IMMEDIATE, 2, 2, "*XAA"},
};

void cpuReset(CPU *cpu) {
	cpu->regA = 0;
	cpu->regX = 0;
	cpu->regY = 0;

	cpu->status.bits = 0;
	cpu->status.interrupt = 1;
	cpu->status.bFlag2 = 1;

	cpu->stack = 0xFD;
}

void cpuInit(CPU *cpu, Bus bus) {
	cpuReset(cpu);

	cpu->bus = bus;
	cpu->bus.callback = _gameCallback;
}

void cpuInitFromROM(CPU *cpu, ROM rom) {
	cpuReset(cpu);
	busInit(&cpu->bus, rom, _gameCallback);
}

uint8_t cpuRead(CPU *cpu, const uint16_t ADDRESS) {
	return busRead(&cpu->bus, ADDRESS);
}

uint16_t cpuRead16(CPU *cpu, const uint16_t ADDRESS) {
	return busRead16(&cpu->bus, ADDRESS);
}

void cpuWrite(CPU *cpu, const uint16_t ADDRESS, const uint8_t VALUE) {
	busWrite(&cpu->bus, ADDRESS, VALUE);
}

void cpuWrite16(CPU *cpu, const uint16_t ADDRESS, const uint16_t VALUE) {
	busWrite16(&cpu->bus, ADDRESS, VALUE);
}

void cpuLoad(CPU *cpu, const uint8_t *CODE, const uint16_t SIZE) {
	for( uint16_t i = 0x0600; i < (0x0600 + SIZE); ++i ) {
		cpuWrite(cpu, i, CODE[i - 0x0600]);
	}
}

void cpuLoadAndRun(CPU *cpu, const uint8_t *CODE, const uint16_t SIZE) {
	ROM rom;
	romInit(&rom, CODE);

	Bus bus;
	busInit(&bus, rom, _gameCallback);

	cpuInit(cpu, bus);
	cpuLoad(cpu, CODE, SIZE);
	cpuRun(cpu);
}
/*
static bool _handleInput(CPU *cpu) {
	SDL_Event e;

	while( SDL_PollEvent(&e) ) {
		if( e.type == SDL_QUIT ) {
			return true;
		}

		if( e.type == SDL_KEYDOWN ) {
			switch( e.key.keysym.sym ) {
				case SDLK_w:
					cpuWrite(cpu, 0xFF, 0x77);
					return false;
				case SDLK_a:
					cpuWrite(cpu, 0xFF, 0x61);
					return false;
				case SDLK_s:
					cpuWrite(cpu, 0xFF, 0x73);
					return false;
				case SDLK_d:
					cpuWrite(cpu, 0xFF, 0x64);
					return false;
				default:
					return false;
			}
		}
	}

	return false;
}
*/

void cpuTrace(CPU *cpu, uint16_t pc, const Op OP) {
	printf("%04X  ", pc);

	switch( OP.bytes ) {
		case 1:
			printf("%02X       ", cpuRead(cpu, pc));
			break;
		case 2:
			printf("%02X %02X    ", cpuRead(cpu, pc), cpuRead(cpu, pc + 1));
			break;
		case 3:
			printf("%02X %02X %02X ", cpuRead(cpu, pc), cpuRead(cpu, pc + 1),
				   cpuRead(cpu, pc + 2));
			break;
		default:
			errPrint(C_YELLOW, "Bad OP @ pc=%04X %02X, dumping info:\n", pc,
					 cpuRead(cpu, pc));
			printf(" * Addressing mode.. %02X\n", OP.mode);
			printf(" * Cycles........... %02X\n", OP.cycles);
			printf(" * Bytes............ %02X\n", OP.bytes);
			printf(" * Mnemonic......... %s\n", OP.NAME);
			break;
	}

	if( OP.NAME[0] != '*' ) {
		putchar(' ');
	}

	printf("%s ", OP.NAME);

	switch( OP.mode ) {
		case M_IMMEDIATE:
			printf("#$%02X                        ", cpuRead(cpu, pc + 1));
			break;

		case M_ZEROPAGE: {
			const uint8_t ADDRESS = cpuRead(cpu, pc + 1);
			printf("$%02X                         ", ADDRESS);
		} break;

		case M_ZEROPAGE_X: {
			const uint8_t ADDRESS = cpuRead(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("$%02X,X @ %02x                  ", ADDRESS, COMPUTED);
		} break;

		case M_ZEROPAGE_Y: {
			const uint8_t ADDRESS = cpuRead(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("$%02X,Y @ %02X                  ", ADDRESS, COMPUTED);
		} break;

		case M_RELATIVE: {
			printf("$%02X                       ",
				   pc + cpuRead(cpu, pc + 1) + 2);
		} break;

		case M_ABSOLUTE: {
			++pc;
			const uint16_t ADDRESS = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("$%04X                       ", ADDRESS);
		} break;

		case M_ABSOLUTE_X: {
			const uint16_t ADDRESS = cpuRead16(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("$%04X,X @ %04X              ", ADDRESS, COMPUTED);
		} break;

		case M_ABSOLUTE_Y: {
			const uint16_t ADDRESS = cpuRead16(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("$%04X,Y @ %04X              ", ADDRESS, COMPUTED);
		} break;

		case M_INDIRECT: {
			const uint16_t ADDRESS = cpuRead16(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("($%04X)                     ", ADDRESS);
		} break;

		case M_INDIRECT_X: {
			const uint16_t ADDRESS = cpuRead(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("($%02X,X) @ %02X                ", ADDRESS,
				   (ADDRESS + cpu->regX) & 0xFF);
		} break;

		case M_INDIRECT_Y: {
			const uint16_t ADDRESS = cpuRead(cpu, ++pc);
			const uint16_t COMPUTED = _getAddressFromMode(cpu, &pc, OP.mode);
			--pc;
			printf("($%02X),Y = %04X @ %04X       ", ADDRESS,
				   (COMPUTED - cpu->regY), COMPUTED);
		} break;

		case M_IMPLIED:
			printf("                            ");
			break;

		default:
			break;
	}

	printf("A:%02X X:%02X Y:%02X P:%02X SP:%02X\n", cpu->regA, cpu->regX,
		   cpu->regY, cpu->status.bits, cpu->stack);
}

static uint16_t _interruptNMI(CPU *cpu, const uint16_t PC) {
	_push16(cpu, PC);

	CPUStatus status = cpu->status;
	status.bFlag1 = 0;
	status.bFlag2 = 1;

	_push(cpu, status.bits);
	cpu->status.interrupt = 1;

	busTick(&cpu->bus, 2);

	cpu->bus.ppu.nmiInterrupt = false;

	return cpuRead16(cpu, 0xFFFA);
}

void cpuRun(CPU *cpu) {
	srand((unsigned int)time(NULL));

	register uint16_t pc = cpuRead16(cpu, 0xFFFC);
	register uint16_t pcState = 0;

	while( true ) {
		if( cpu->bus.ppu.nmiInterrupt ) {
			pc = _interruptNMI(cpu, pc);
		}

		const uint8_t OP = cpuRead(cpu, pc++);
		pcState = pc;

		switch( OP ) {
			case 0x00: /* BRK */
				cpuTrace(cpu, pc - 1, (Op){NULL, M_IMPLIED, 7, 1, "BRK"});
				return;
			case 0x02:	// KIL
			case 0x12:	// Unnoficial opcode, works basically the same as BRK
			case 0x22:	// (in practice, not quite. But for this emulator, it
			case 0x32:	// will do...
			case 0x42:
			case 0x52:
			case 0x62:
			case 0x72:
			case 0x92:
			case 0xB2:
			case 0xD2:
			case 0xF2:
				cpuTrace(cpu, pc - 1, (Op){NULL, M_IMPLIED, 7, 1, "*KIL"});
				return;

			default: {
				const Op OPERATION = OPS[OP];

				/* cpuTrace(cpu, pc - 1, OPERATION); */
				pc = OPERATION.fn(cpu, pc, OPERATION.mode);

				busTick(&cpu->bus, OPERATION.cycles);
				if( pcState == pc ) {
					pc += (uint16_t)(OPERATION.bytes - 1);
				}
			}
		}
	}
}
