#ifndef GUARD_NESINC_PPU_H_
#define GUARD_NESINC_PPU_H_

#include "common.h"

#include "addr_register.h"
#include "rom.h"

typedef struct _PPU {
	uint8_t *chrRom;
	uint8_t palTable[32];
	uint8_t vram[2048];
	uint8_t oam[256];

	AddrReg addr;
	Mirroring mirroring;
} PPU;

void ppuInit(PPU *ppu, uint8_t *chrRom, const Mirroring MIRRORING);

#endif	// GUARD_NESINC_PPU_H_
