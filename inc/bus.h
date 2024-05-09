#ifndef GUARD_NESINC_BUS_H_
#define GUARD_NESINC_BUS_H_

#include "common.h"
#include "ppu.h"
#include "rom.h"

#define BUS_CALLBACK_FN void (*callback)(PPU * ppu)

typedef struct _Bus {
	uint8_t cpuVRAM[2048];
	ROM rom;
	PPU ppu;

	size_t cycles;

	BUS_CALLBACK_FN;
} Bus;

void busInit(Bus *bus, ROM rom, BUS_CALLBACK_FN);

uint8_t busRead(Bus *bus, const uint16_t ADDRESS);
uint16_t busRead16(Bus *bus, const uint16_t ADDRESS);

void busWrite(Bus *bus, const uint16_t ADDRESS, const uint8_t VALUE);
void busWrite16(Bus *bus, const uint16_t ADDRESS, const uint16_t VALUE);

void busTick(Bus *bus, const uint8_t CYCLES);

#endif	// GUARD_NESINC_BUS_H_
