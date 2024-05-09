#ifndef GUARD_NESINC_PPU_CONTROL_REGISTER_H_
#define GUARD_NESINC_PPU_CONTROL_REGISTER_H_

#include "common.h"

/* (From https://www.nesdev.org/wiki/PPU_registers#Controller_($2000)_%3E_write)
 * 7  bit  0
 * ──── ────
 * VPHB SINN
 * ││││ ││││
 * ││││ ││└┴─ Nametable address
 * ││││ ││    (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00)
 * ││││ │└─── VRAM address increment per CPU read/write of PPUDATA
 * ││││ │     (0: add 1, going across; 1: add 32, going down)
 * ││││ └──── Sprite pattern table address for 8x8 sprites
 * ││││       (0: $0000; 1: $1000; ignored in 8x16 mode)
 * │││└────── Background pattern table address (0: $0000; 1: $1000)
 * ││└─────── Sprite size (0: 8x8 pixels; 1: 8x16 pixels)
 * │└──────── PPU master/slave select
 * │          (0: read backdrop from EXT pins; 1: output color on EXT pins)
 * └───────── Generate an NMI at the start of the
 *            vertical blanking interval (0: off; 1: on)
 */

typedef union _ControlReg {
	struct {
		uint8_t nametableAddr : 2;
		uint8_t vramIncrementAddr : 1;
		uint8_t spritePatternAddr : 1;
		uint8_t bgPatternAddr : 1;
		uint8_t spriteSize : 1;
		uint8_t masterSlaveSelect : 1;
		uint8_t generateNMI : 1;
	};

	uint8_t bits;
} ControlReg;

void controlInit(ControlReg *control);

void controlUpdate(ControlReg *control, const uint8_t DATA);

uint16_t controlNametableAddr(ControlReg *control);
uint8_t controlVramIncrement(ControlReg *control);

uint16_t controlSprPatternAddr(ControlReg *control);
uint16_t controlBGPatternAddr(ControlReg *control);

uint8_t controlSprSize(ControlReg *control);

#endif	// GUARD_NESINC_PPU_CONTROL_REGISTER_H_
