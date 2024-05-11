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

void busInit(Bus *bus, ROM rom, BUS_CALLBACK_FN) {
	memset(bus->cpuVRAM, 0, 2048);

	bus->rom = rom;
	bus->cycles = 0;
	bus->callback = callback;

	ppuInit(&bus->ppu, bus->rom.chrRom, bus->rom.mirroring);

	joyInit(&bus->joy1);
	joyInit(&bus->joy2);
}

uint8_t busRead(Bus *bus, const uint16_t ADDRESS) {
	switch( ADDRESS ) {
		case 0 ... RAM_MIRRORS_END:
			return bus->cpuVRAM[ADDRESS & RAM_ADDRESS_SPACE];

		case 0x2000:
		case 0x2001:
		case 0x2003:
		case 0x2005:
		case 0x2006:
		case 0x4014:
			return 0;

		case 0x2002: {
			const uint8_t RESULT = bus->ppu.status.bits;
			bus->ppu.status.vblankStarted = 0;
			return RESULT;
		}

		case 0x2004:
			return ppuReadOAM(&bus->ppu);

		case 0x2007:
			return ppuRead(&bus->ppu);

		case 0x2008 ... PPU_REGISTERS_MIRRORS_END:
			return busRead(bus, ADDRESS & PPU_REGISTERS_ADDRESS_SPACE);

		case 0x4016:
			return joyRead(&bus->joy1);

		case 0x4017:
			return joyRead(&bus->joy2);

		case 0x8000 ... 0xFFFF:
			return _readPrgRom(bus, ADDRESS);

		default:
			return 0;
	}
}

uint16_t busRead16(Bus *bus, const uint16_t ADDRESS) {
	const uint16_t LO = (uint16_t)busRead(bus, ADDRESS);
	const uint16_t HI = (uint16_t)busRead(bus, ADDRESS + 1);

	return (HI << 8) | LO;
}

void busWrite(Bus *bus, const uint16_t ADDRESS, const uint8_t VALUE) {
	switch( ADDRESS ) {
		case 0 ... RAM_MIRRORS_END:
			bus->cpuVRAM[ADDRESS & RAM_ADDRESS_SPACE] = VALUE;
			return;

		case 0x2000:
			ppuWriteControl(&bus->ppu, VALUE);
			return;

		case 0x2001:
			ppuWriteMask(&bus->ppu, VALUE);
			return;

		case 0x2003:
			ppuWriteOAMAddr(&bus->ppu, VALUE);
			return;

		case 0x2004:
			ppuWriteOAM(&bus->ppu, VALUE);
			return;

		case 0x2005:
			ppuWriteScroll(&bus->ppu, VALUE);
			return;

		case 0x2006:
			ppuWriteAddr(&bus->ppu, VALUE);
			return;

		case 0x2007:
			ppuWrite(&bus->ppu, VALUE);
			return;

		case 0x2008 ... PPU_REGISTERS_MIRRORS_END:
			busWrite(bus, ADDRESS & 0x2007, VALUE);
			return;

		case 0x4000 ... 0x4013:
		case 0x4015: /* APU */
			return;

		case 0x4014: {
			uint8_t buffer[256] = {0};
			const uint16_t HI = VALUE << 8;

			for( uint16_t i = 0; i < 256; ++i ) {
				buffer[i] = busRead(bus, HI + i);
			}

			ppuWriteOAMDMA(&bus->ppu, buffer);
		}
			return;

		case 0x4016:
			joyWrite(&bus->joy1, VALUE);
			break;

		case 0x4017:
			joyWrite(&bus->joy2, VALUE);
			break;

		case 0x8000 ... 0xFFFF:
			errPrint(C_RED, "Attempted to write to ROM @ %04X\n", ADDRESS);
			exit(4);
	}
}

void busWrite16(Bus *bus, const uint16_t ADDRESS, const uint16_t VALUE) {
	busWrite(bus, ADDRESS, (uint8_t)(VALUE & 0xFF));
	busWrite(bus, ADDRESS + 1, (uint8_t)(VALUE >> 8));
}

void busTick(Bus *bus, const uint8_t CYCLES) {
	bus->cycles += CYCLES;

	if( ppuTick(&bus->ppu, CYCLES * 3) ) {
		bus->callback(&bus->ppu, &bus->joy1, &bus->joy2);
	}
}
