#include "bus.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#define RAM 0x0000
#define RAM_MIRRORS_END 0x1FFF
#define PPU_REGISTERS 0x2000
#define PPU_REGISTERS_MIRRORS_END 0x3FFF

#define RAM_ADDRESS_SPACE 0x07FF
#define PPU_REGISTERS_ADDRESS_SPACE 0x2007

static uint8_t _readPrgRom(Bus *bus, uint16_t address) {
	address -= 0x8000;
	if( (bus->rom.prgSize == 0x4000) && (address >= 0x4000) ) {
		address %= 0x4000;
	}

	return bus->rom.prgRom[address];
}

void busInit(Bus *bus, ROM rom) {
	memset(bus->cpuVRAM, 0, 2048);
	bus->rom = rom;
}

uint8_t busRead(Bus *bus, const uint16_t ADDRESS) {
	if( ADDRESS <= RAM_MIRRORS_END ) {
		return bus->cpuVRAM[ADDRESS & RAM_ADDRESS_SPACE];
	}

	if( ADDRESS >= PPU_REGISTERS && ADDRESS <= PPU_REGISTERS_MIRRORS_END ) {
		const uint16_t MIRROR = (ADDRESS & PPU_REGISTERS_ADDRESS_SPACE);
		(void)MIRROR;
	}

	return _readPrgRom(bus, ADDRESS);
}

uint16_t busRead16(Bus *bus, const uint16_t ADDRESS) {
	const uint16_t LO = (uint16_t)busRead(bus, ADDRESS);
	const uint16_t HI = (uint16_t)busRead(bus, ADDRESS + 1);

	return (HI << 8) | LO;
}

void busWrite(Bus *bus, const uint16_t ADDRESS, const uint8_t VALUE) {
	if( ADDRESS <= RAM_MIRRORS_END ) {
		bus->cpuVRAM[ADDRESS & RAM_ADDRESS_SPACE] = VALUE;
		return;
	}

	if( ADDRESS >= PPU_REGISTERS && ADDRESS <= PPU_REGISTERS_MIRRORS_END ) {
		// TODO: PPU not yet implemented
	}

	errPrint(C_RED, "Attempted to write to PRGROM address %04x (read-only)",
			 ADDRESS);
	exit(53);
}

void busWrite16(Bus *bus, const uint16_t ADDRESS, const uint16_t VALUE) {
	busWrite(bus, ADDRESS, (uint8_t)(VALUE & 0xFF));
	busWrite(bus, ADDRESS + 1, (uint8_t)(VALUE >> 8));
}
