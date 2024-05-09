#ifndef GUARD_NESINC_PPU_H_
#define GUARD_NESINC_PPU_H_

#include "common.h"
#include "frame.h"
#include "ppu_addr.h"
#include "ppu_control.h"
#include "ppu_mask.h"
#include "ppu_scroll.h"
#include "ppu_status.h"
#include "rom.h"

typedef struct _Frame Frame;

typedef struct _PPU {
	uint8_t *chrRom;
	uint8_t palTable[32];
	uint8_t vram[2048];

	uint8_t oamAddr;
	uint8_t oam[256];

	uint8_t internalBuffer;

	ControlReg control;
	MaskReg mask;
	StatusReg status;
	ScrollReg scroll;
	AddrReg addr;

	Mirroring mirroring;

	uint16_t scanline;
	size_t cycles;

	bool nmiInterrupt;
	Frame frame;
} PPU;

void ppuInit(PPU *ppu, uint8_t *chrRom, const Mirroring MIRRORING);
void ppuInitEmpty(PPU *ppu);
void ppuInitEmptyVertical(PPU *ppu);

void ppuVramIncrement(PPU *ppu);

void ppuWrite(PPU *ppu, const uint8_t VALUE);

void ppuWriteOAMAddr(PPU *ppu, const uint8_t VALUE);
void ppuWriteOAM(PPU *ppu, const uint8_t VALUE);
void ppuWriteOAMDMA(PPU *ppu, const uint8_t VALUE[256]);

void ppuWriteControl(PPU *ppu, const uint8_t VALUE);
void ppuWriteMask(PPU *ppu, const uint8_t VALUE);
void ppuWriteScroll(PPU *ppu, const uint8_t VALUE);
void ppuWriteAddr(PPU *ppu, const uint8_t VALUE);

uint8_t ppuRead(PPU *ppu);
uint8_t ppuReadOAM(PPU *ppu);
StatusReg ppuReadStatus(PPU *ppu);

bool ppuTick(PPU *ppu, const uint8_t CYCLES);
void ppuRender(PPU *ppu);

#endif	// GUARD_NESINC_PPU_H_
