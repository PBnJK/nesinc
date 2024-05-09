#include "ppu_mask.h"

void maskInit(MaskReg *mask) {
	mask->bits = 0;
}

void maskUpdate(MaskReg *mask, const uint8_t DATA) {
	mask->bits = DATA;
}
