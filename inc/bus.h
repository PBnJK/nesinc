#ifndef GUARD_NESINC_BUS_H_
#define GUARD_NESINC_BUS_H_

#include "common.h"
#include "rom.h"

typedef struct _Bus {
	uint8_t cpuVRAM[2048];
	ROM rom;
} Bus;

void busInit(Bus *bus, ROM rom);

uint8_t busRead(Bus *bus, const uint16_t ADDRESS);
uint16_t busRead16(Bus *bus, const uint16_t ADDRESS);

void busWrite(Bus *bus, const uint16_t ADDRESS, const uint8_t VALUE);
void busWrite16(Bus *bus, const uint16_t ADDRESS, const uint16_t VALUE);

#endif	// GUARD_NESINC_BUS_H_
