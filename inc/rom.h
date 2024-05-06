#ifndef GUARD_NESINC_ROM_H_
#define GUARD_NESINC_ROM_H_

#include "common.h"

typedef enum {
	VERTICAL,
	HORIZONTAL,
	FOUR_SCREEN,
} Mirroring;

typedef struct _ROM {
	uint32_t prgSize;
	uint8_t *prgRom;

	uint32_t chrSize;
	uint8_t *chrRom;

	uint8_t mapper;
	Mirroring mirroring;
} ROM;

bool romInit(ROM *rom, const uint8_t *RAW);
void romFree(ROM *rom);

void romCreateTestROM(ROM *rom);
void romCreateFromFile(ROM *rom, const char *PATH);

#endif	// GUARD_NESINC_ROM_H_
