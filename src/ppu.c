#include "ppu.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "rect.h"

static uint16_t _mirrorVramAddress(PPU *ppu, const uint16_t ADDRESS) {
	const uint16_t MIRRORED = (ADDRESS & 0x2FFF);
	const uint16_t INDEX = (MIRRORED - 0x2000);
	const uint16_t NAMETABLE = (INDEX / 0x0400);

	if( ppu->mirroring == VERTICAL ) {
		if( NAMETABLE == 2 || NAMETABLE == 3 ) {
			return INDEX - 0x0800;
		}
	} else if( ppu->mirroring == HORIZONTAL ) {
		if( NAMETABLE == 1 || NAMETABLE == 2 ) {
			return INDEX - 0x0400;
		} else if( NAMETABLE == 3 ) {
			return INDEX - 0x0800;
		}
	}

	return INDEX;
}

void ppuInit(PPU *ppu, uint8_t *chrRom, const Mirroring MIRRORING) {
	ppu->chrRom = chrRom;
	ppu->mirroring = MIRRORING;

	memset(ppu->palTable, 0, 32);
	memset(ppu->vram, 0, 2048);

	ppu->oamAddr = 0;
	memset(ppu->oam, 0, 256);

	controlInit(&ppu->control);
	maskInit(&ppu->mask);
	statusInit(&ppu->status);
	scrollInit(&ppu->scroll);
	addrInit(&ppu->addr);

	ppu->cycles = 0;
	ppu->nmiInterrupt = false;

	frameInit(&ppu->frame);
}

void ppuInitEmpty(PPU *ppu) {
	uint8_t CHR_ROM[256] = {0};

	ppuInit(ppu, CHR_ROM, HORIZONTAL);
}

void ppuInitEmptyVertical(PPU *ppu) {
	uint8_t CHR_ROM[256] = {0};

	ppuInit(ppu, CHR_ROM, VERTICAL);
}

void ppuVramIncrement(PPU *ppu) {
	addrIncrement(&ppu->addr, controlVramIncrement(&ppu->control));
}

void ppuWrite(PPU *ppu, const uint8_t VALUE) {
	uint16_t address = addrGet(&ppu->addr);

	if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 ||
		address == 0x3F1C ) {
		address -= 0x10;
	}

	switch( address ) {
		case 0 ... 0x1FFF:
			/* TODO: check chrram */
			ppu->chrRom[address] = VALUE;
			break;

			// errPrint(C_RED, "Attempted to write to CHRROM @ %04X", address);
			// exit(3);

		case 0x2000 ... 0x2FFF:
			ppu->vram[_mirrorVramAddress(ppu, address)] = VALUE;
			break;

		case 0x3F00 ... 0x3FFF: {
			const uint16_t MIRRORED = (uint16_t)((address - 0x3F00) % 32);
			ppu->palTable[MIRRORED] = VALUE;
		} break;
	}

	ppuVramIncrement(ppu);
}

void ppuWriteOAMAddr(PPU *ppu, const uint8_t VALUE) {
	ppu->oamAddr = VALUE;
}

void ppuWriteOAM(PPU *ppu, const uint8_t VALUE) {
	ppu->oam[ppu->oamAddr++] = VALUE;
}

void ppuWriteOAMDMA(PPU *ppu, const uint8_t VALUE[256]) {
	uint8_t v = 0;

	do {
		ppu->oam[ppu->oamAddr++] = VALUE[v++];
	} while( v != 0 );
}

void ppuWriteControl(PPU *ppu, const uint8_t VALUE) {
	const uint8_t BEFORE_NMI = ppu->control.generateNMI;
	controlUpdate(&ppu->control, VALUE);

	if( !BEFORE_NMI && ppu->control.generateNMI && ppu->status.vblankStarted ) {
		ppu->nmiInterrupt = true;
	}
}

void ppuWriteMask(PPU *ppu, const uint8_t VALUE) {
	maskUpdate(&ppu->mask, VALUE);
}

void ppuWriteScroll(PPU *ppu, const uint8_t VALUE) {
	scrollUpdate(&ppu->scroll, VALUE);
}

void ppuWriteAddr(PPU *ppu, const uint8_t VALUE) {
	addrUpdate(&ppu->addr, VALUE);
}

uint8_t ppuRead(PPU *ppu) {
	uint16_t address = addrGet(&ppu->addr);
	ppuVramIncrement(ppu);

	switch( address ) {
		case 0 ... 0x1FFF: {
			const uint8_t RESULT = ppu->internalBuffer;
			ppu->internalBuffer = ppu->chrRom[address];
			return RESULT;
		}

		case 0x2000 ... 0x2FFF: {
			const uint8_t RESULT = ppu->internalBuffer;
			ppu->internalBuffer = ppuReadStatus(ppu).bits;
			return RESULT;
		}

		case 0x3000 ... 0x3EFF: {
			errPrint(C_RED, "Tried to read %04X, which shouldn't be read!",
					 address);
			exit(3);
		}

		case 0x3F00 ... 0x3FFF: {
			ppu->internalBuffer = (uint8_t)(address - 0x1000);
			if( address == 0x3F10 || address == 0x3F14 || address == 0x3F18 ||
				address == 0x3F1C ) {
				address -= 0x10;
			}

			return ppu->palTable[address - 0x3F00];
		}

		default:
			errPrint(C_RED, "Unexpected read to mirrored space %04X", address);
			exit(3);
	}
}

uint8_t ppuReadOAM(PPU *ppu) {
	return ppu->oam[ppu->oamAddr];
}

StatusReg ppuReadStatus(PPU *ppu) {
	const StatusReg STATUS = ppu->status;

	ppu->status.vblankStarted = 0;

	addrResetLatch(&ppu->addr);
	scrollResetLatch(&ppu->scroll);

	return STATUS;
}

static bool _isZeroHit(PPU *ppu) {
	const uint8_t TILE_X = ppu->oam[0];
	const uint8_t TILE_Y = ppu->oam[3];

	if( TILE_Y == ppu->scanline && TILE_X <= ppu->cycles &&
		ppu->mask.showSpr ) {
		return true;
	}

	return false;
}

bool ppuTick(PPU *ppu, const uint8_t CYCLES) {
	ppu->cycles += CYCLES;

	if( ppu->cycles >= 341 ) {
		if( _isZeroHit(ppu) ) {
			ppu->status.sprZeroHit = 1;
		}

		ppu->cycles -= 341;
		++ppu->scanline;

		if( ppu->scanline == 241 ) {
			ppu->status.vblankStarted = 1;
			ppu->status.sprZeroHit = 0;

			if( ppu->control.generateNMI ) {
				ppu->nmiInterrupt = true;
			}

			return false;
		}

		if( ppu->scanline >= 262 ) {
			ppu->scanline = 0;
			ppu->nmiInterrupt = false;

			ppu->status.vblankStarted = 0;
			ppu->status.sprZeroHit = 0;
			return true;
		}
	}

	return false;
}

void _getBGPalette(PPU *ppu, const size_t COL, const size_t ROW,
				   uint8_t attrTable[64], uint8_t palette[4]) {
	const uint16_t ATTR_TABLE_IDX = (uint16_t)((ROW / 4) * 8 + (COL / 4));
	const uint8_t ATTR_BYTE = attrTable[ATTR_TABLE_IDX];

	uint8_t paletteIdx = 0;

	const uint8_t INDEX = (uint8_t)((((COL % 4) / 2) << 1) | ((ROW % 4) / 2));
	switch( INDEX ) {
		case 0:
			paletteIdx = ATTR_BYTE & 3;
			break;
		case 1:
			paletteIdx = (ATTR_BYTE >> 4) & 3;
			break;
		case 2:
			paletteIdx = (ATTR_BYTE >> 2) & 3;
			break;
		case 3:
			paletteIdx = (ATTR_BYTE >> 6) & 3;
			break;
	}

	uint8_t palStart = (uint8_t)(1 + (paletteIdx * 4));

	palette[0] = ppu->palTable[0];
	palette[1] = ppu->palTable[palStart];
	palette[2] = ppu->palTable[palStart + 1];
	palette[3] = ppu->palTable[palStart + 2];
}

void _getSprPalette(PPU *ppu, const uint8_t IDX, uint8_t palette[4]) {
	uint8_t palStart = (uint8_t)(0x11 + (IDX * 4));

	palette[0] = 0;
	palette[1] = ppu->palTable[palStart++];
	palette[2] = ppu->palTable[palStart++];
	palette[3] = ppu->palTable[palStart];
}

static void _renderNametable(PPU *ppu, uint8_t nametable[1024], Rect viewport,
							 const int64_t SHIFT_X, const int64_t SHIFT_Y) {
	const uint16_t BG_BANK = controlBGPatternAddr(&ppu->control);

	uint8_t attrTable[64];
	memcpy(attrTable, nametable + 0x03C0, 64);

	for( uint16_t i = 0; i < 0x03C0; ++i ) {
		const uint16_t TILE_BYTE = (uint16_t)nametable[i];

		const uint8_t TILE_X = i % 32;
		const uint8_t TILE_Y = (uint8_t)(i / 32);

		uint8_t tile[16];
		memcpy(tile, ppu->chrRom + (BG_BANK + TILE_BYTE * 16), 16);

		uint8_t palette[4];
		_getBGPalette(ppu, TILE_X, TILE_Y, attrTable, palette);

		for( uint8_t y = 0; y < 8; ++y ) {
			uint8_t upper = tile[y];
			uint8_t lower = tile[y + 8];

			for( uint8_t x = 7; x != 255; --x ) {
				const uint8_t VALUE =
					(uint8_t)(((1 & lower) << 1) | (1 & upper));

				upper >>= 1;
				lower >>= 1;

				SDL_Color rgb = SYS_PAL[palette[VALUE]];

				const size_t PX = (size_t)(TILE_X * 8 + x);
				const size_t PY = (size_t)(TILE_Y * 8 + y);

				if( PX >= viewport.x1 && PX < viewport.x2 &&
					PY >= viewport.y1 && PY < viewport.y2 ) {
					frameSetPixel(&ppu->frame, (size_t)(SHIFT_X + PX),
								  (size_t)(SHIFT_Y + PY), rgb);
				}
			}
		}
	}
}

void ppuRender(PPU *ppu) {
	/* Draw BG */
	size_t scrollX = (size_t)ppu->scroll.x;
	size_t scrollY = (size_t)ppu->scroll.y;

	uint8_t mainNametable[1024];
	uint8_t secNametable[1024];

	if( ppu->mirroring == VERTICAL ) {
		switch( controlNametableAddr(&ppu->control) ) {
			case 0x2000:
			case 0x2800:
				memcpy(mainNametable, ppu->vram, 1024);
				memcpy(secNametable, ppu->vram + 0x0400, 1024);
				break;
			case 0x2400:
			case 0x2C00:
				memcpy(mainNametable, ppu->vram + 0x0400, 1024);
				memcpy(secNametable, ppu->vram, 1024);
				break;
		}
	} else if( ppu->mirroring == HORIZONTAL ) {
		switch( controlNametableAddr(&ppu->control) ) {
			case 0x2000:
			case 0x2400:
				memcpy(mainNametable, ppu->vram, 1024);
				memcpy(secNametable, ppu->vram + 0x0400, 1024);
				break;
			case 0x2800:
			case 0x2C00:
				memcpy(mainNametable, ppu->vram + 0x0400, 1024);
				memcpy(secNametable, ppu->vram, 1024);
				break;
		}
	}

	_renderNametable(ppu, mainNametable, RECT(scrollX, scrollY, SCR_W, SCR_H),
					 -((int64_t)scrollX), -((int64_t)scrollY));

	if( (int64_t)(scrollX) > 0 ) {
		const int64_t SX = (int64_t)(SCR_W) - scrollX;
		_renderNametable(ppu, secNametable, RECT(0, 0, scrollX, SCR_H), SX, 0);
	} else if( (int64_t)(scrollY) > 0 ) {
		const int64_t SY = (int64_t)(SCR_H) - scrollY;
		_renderNametable(ppu, secNametable, RECT(0, 0, SCR_W, scrollY), 0, SY);
	}

	/* Draw sprites */
	const uint16_t SPR_BANK = controlSprPatternAddr(&ppu->control);

	for( int16_t i = 252; i >= 0; i -= 4 ) {
		const uint8_t TILE_Y = ppu->oam[i];
		const uint8_t TILE_BYTE = ppu->oam[i + 1];
		const uint8_t TILE_ATTR = ppu->oam[i + 2];
		const uint8_t TILE_X = ppu->oam[i + 3];

		const bool FLIP_V = ((TILE_ATTR >> 7) & 1) == 1;
		const bool FLIP_H = ((TILE_ATTR >> 6) & 1) == 0;

		const uint8_t PAL_INDEX = TILE_ATTR & 3;

		uint8_t palette[4];
		_getSprPalette(ppu, PAL_INDEX, palette);

		uint8_t tile[16];
		memcpy(tile, ppu->chrRom + (SPR_BANK + TILE_BYTE * 16), 16);

		for( uint8_t y = 0; y < 8; ++y ) {
			uint8_t upper = tile[y];
			uint8_t lower = tile[y + 8];

			for( uint8_t x = 0; x < 8; ++x ) {
				const uint8_t VALUE =
					(uint8_t)(((1 & lower) << 1) | (1 & upper));

				upper >>= 1;
				lower >>= 1;

				if( VALUE == 0 ) continue;
				SDL_Color rgb = SYS_PAL[palette[VALUE]];

				const size_t TX_FINAL =
					(size_t)(FLIP_H ? TILE_X + 7 - x : TILE_X + x);
				const size_t TY_FINAL =
					(size_t)(FLIP_V ? TILE_Y + 7 - y : TILE_Y + y);

				frameSetPixel(&ppu->frame, TX_FINAL, TY_FINAL, rgb);
			}
		}
	}
}
