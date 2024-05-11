#ifndef GUARD_NESINC_JOYPAD_H_
#define GUARD_NESINC_JOYPAD_H_

#include "common.h"

typedef union _JoypadData {
	struct {
		uint8_t btnA : 1;
		uint8_t btnB : 1;
		uint8_t select : 1;
		uint8_t start : 1;
		uint8_t up : 1;
		uint8_t down : 1;
		uint8_t left : 1;
		uint8_t right : 1;
	};

	uint8_t bits;
} JoypadData;

typedef struct _Joypad {
	bool strobe;
	uint8_t pointer;
	JoypadData data;
} Joypad;

void joyInit(Joypad *joy);

void joyWrite(Joypad *joy, const uint8_t VALUE);
uint8_t joyRead(Joypad *joy);

#endif	// GUARD_NESINC_JOYPAD_H_
