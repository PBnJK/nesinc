#include "ppu.h"

#include <string.h>

void ppuInit(PPU *ppu, uint8_t *chrRom, const Mirroring MIRRORING) {
	ppu->chrRom = chrRom;
	ppu->mirroring = MIRRORING;

	memset(ppu->palTable, 0, 32);
	memset(ppu->vram, 0, 2048);
	memset(ppu->oam, 0, 256);
}
