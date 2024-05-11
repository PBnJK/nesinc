#include "joypad.h"

void joyInit(Joypad *joy) {
	joy->strobe = false;
	joy->pointer = 0;
	joy->data.bits = 0;
}

void joyWrite(Joypad *joy, const uint8_t VALUE) {
	/* We only care about the LSB, which turns the strobe ON/OFF */
	joy->strobe = (VALUE & 1) == 1;

	if( joy->strobe ) {
		joy->pointer = 0;
	}
}

uint8_t joyRead(Joypad *joy) {
	if( joy->pointer > 7 ) {
		return 0x40;
	}

	uint8_t controller = joy->data.bits & (uint8_t)(1 << joy->pointer);
	controller >>= joy->pointer;
	controller |= 0x40;

	if( !joy->strobe ) {
		++joy->pointer;
	}

	return controller;
}
