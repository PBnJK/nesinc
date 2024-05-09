#ifndef GUARD_NESINC_PPU_MASK_REGISTER_H_
#define GUARD_NESINC_PPU_MASK_REGISTER_H_

#include "common.h"

/* (From https://www.nesdev.org/wiki/PPU_registers#Mask_($2001)_%3E_write)
 * 7  bit  0
 * ──── ────
 * BGRs bMmG
 * ││││ ││││
 * ││││ │││└─ Greyscale (0: normal color, 1: produce a greyscale display)
 * ││││ ││└── 1: Show background in leftmost 8 pixels of screen, 0: Hide
 * ││││ │└─── 1: Show sprites in leftmost 8 pixels of screen, 0: Hide
 * ││││ └──── 1: Show background
 * │││└────── 1: Show sprites
 * ││└─────── Emphasize red (green on PAL/Dendy)
 * │└──────── Emphasize green (red on PAL/Dendy)
 * └───────── Emphasize blue
 */

typedef union _MaskReg {
	struct {
		uint8_t greyscale : 1;
		uint8_t showLeftBG : 1;
		uint8_t showLeftSpr : 1;
		uint8_t showBG : 1;
		uint8_t showSpr : 1;
		uint8_t emphasizeR : 1;
		uint8_t emphasizeG : 1;
		uint8_t emphasizeB : 1;
	};

	uint8_t bits;
} MaskReg;

void maskInit(MaskReg *mask);

void maskUpdate(MaskReg *mask, const uint8_t DATA);

#endif	// GUARD_NESINC_PPU_MASK_REGISTER_H_
