#ifndef GUARD_NESINC_PPU_STATUS_REGISTER_H_
#define GUARD_NESINC_PPU_STATUS_REGISTER_H_

#include "common.h"

/* (From https://www.nesdev.org/wiki/PPU_registers#Status_($2002)_%3C_read)
 * 7  bit  0
 * ──── ────
 * VSO~ ~~~~
 * ││││ ││││
 * │││└─┴┴┴┴─ PPU open bus. Returns stale PPU bus contents.
 * ││└─────── Sprite overflow. The intent was for this flag to be set
 * ││         whenever more than eight sprites appear on a scanline, but a
 * ││         hardware bug causes the actual behavior to be more complicated
 * ││         and generate false positives as well as false negatives; see
 * ││         PPU sprite evaluation. This flag is set during sprite
 * ││         evaluation and cleared at dot 1 (the second dot) of the
 * ││         pre─render line.
 * │└──────── Sprite 0 Hit.  Set when a nonzero pixel of sprite 0 overlaps
 * │          a nonzero background pixel; cleared at dot 1 of the pre─render
 * │          line.  Used for raster timing.
 * └───────── Vertical blank has started (0: not in vblank; 1: in vblank).
 *            Set at dot 1 of line 241 (the line *after* the post─render
 *            line); cleared after reading $2002 and at dot 1 of the
 *            pre─render line.
 */

typedef union _StatusReg {
	struct {
		uint8_t openBus : 5;
		uint8_t sprOverflow : 1;
		uint8_t sprZeroHit : 1;
		uint8_t vblankStarted : 1;
	};

	uint8_t bits;
} StatusReg;

void statusInit(StatusReg *status);

#endif	// GUARD_NESINC_PPU_STATUS_REGISTER_H_
