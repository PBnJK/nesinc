#include "ppu_control.h"

void controlInit(ControlReg *control) {
	control->bits = 0;
}

void controlUpdate(ControlReg *control, const uint8_t DATA) {
	control->bits = DATA;
}

uint16_t controlNametableAddr(ControlReg *control) {
	switch( control->nametableAddr ) {
		case 0:
			return 0x2000;
		case 1:
			return 0x2400;
		case 2:
			return 0x2800;
		case 3:
			return 0x2C00;
		default:
			/* This is unreacheable, since nametableAddr is only 2 bits
			 * Linter complains, however, so here we are...
			 */
			return 0;
	}
}

uint8_t controlVramIncrement(ControlReg *control) {
	return control->vramIncrementAddr ? 32 : 1;
}

uint16_t controlSprPatternAddr(ControlReg *control) {
	return control->spritePatternAddr * 0x1000;
}

uint16_t controlBGPatternAddr(ControlReg *control) {
	return control->bgPatternAddr * 0x1000;
}

uint8_t controlSprSize(ControlReg *control) {
	return (uint8_t)(8 + (control->spriteSize * 8));
}
