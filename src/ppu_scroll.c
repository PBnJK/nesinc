#include "ppu_scroll.h"

void scrollInit(ScrollReg *scroll) {
	scroll->x = 0;
	scroll->y = 0;
	scroll->latch = false;
}

void scrollUpdate(ScrollReg *scroll, const uint8_t DATA) {
	if( scroll->latch ) {
		scroll->y = DATA;
	} else {
		scroll->x = DATA;
	}

	scroll->latch = !scroll->latch;
}

void scrollResetLatch(ScrollReg *scroll) {
	scroll->latch = false;
}
