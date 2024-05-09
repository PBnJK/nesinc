#ifndef GUARD_NESINC_PPU_SCROLL_REGISTER_H_
#define GUARD_NESINC_PPU_SCROLL_REGISTER_H_

#include "common.h"

typedef struct _ScrollReg {
	uint8_t x;
	uint8_t y;
	bool latch;
} ScrollReg;

void scrollInit(ScrollReg *scroll);

void scrollUpdate(ScrollReg *scroll, const uint8_t DATA);

void scrollResetLatch(ScrollReg *scroll);

#endif	// GUARD_NESINC_PPU_SCROLL_REGISTER_H_
