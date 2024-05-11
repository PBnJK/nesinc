#include "rom.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"

#define PRGROM_PAGE_SIZE (16 * 1024)
#define CHRROM_PAGE_SIZE (8 * 1024)

#define START_ADDR ((0xFFFC - 0x8000) & 0x3FFF)

static const uint8_t TEST_ROM[16] = {'N', 'E', 'S', 0x1A, 1,   0,	1,	 0,
									 0,	  0,   0,	'P',  'B', 'n', 'J', 'K'};

bool romInit(ROM *rom, const uint8_t *RAW) {
	/* Verifies the iNES magic ('N' 'E' 'S' '^Z') */
	if( RAW[0] != 'N' || RAW[1] != 'E' || RAW[2] != 'S' || RAW[3] != 0x1A ) {
		errPrint(C_YELLOW, "Somente ROMs iNES sao suportadas!");
		return false;
	}

	rom->mapper = (RAW[7] & 0xF0) | (RAW[6] >> 4);

	/* Verifies the iNES format version */
	if( ((RAW[7] >> 2) & 3) != 0 ) {
		errPrint(C_YELLOW, "ROMs iNES 2.0 ainda nao sao suportadas");
		return false;
	}

	if( (RAW[6] & 8) != 0 ) {
		rom->mirroring = FOUR_SCREEN;
	} else if( (RAW[6] & 1) != 0 ) {
		rom->mirroring = VERTICAL;
	} else {
		rom->mirroring = HORIZONTAL;
	}

	rom->prgSize = RAW[4] * PRGROM_PAGE_SIZE;
	rom->chrSize = RAW[5] * CHRROM_PAGE_SIZE;

	const size_t SKIP_TRAINER = ((RAW[6] & 4) != 0) ? 512 : 0;

	const size_t PRGROM_START = 16 + SKIP_TRAINER;
	const size_t CHRROM_START = PRGROM_START + rom->prgSize;

	rom->prgRom = malloc(rom->prgSize);
	memcpy(rom->prgRom, RAW + PRGROM_START, rom->prgSize);

	rom->chrRom = malloc(rom->chrSize);
	memcpy(rom->chrRom, RAW + CHRROM_START, rom->chrSize);

	return true;
}

void romFree(ROM *rom) {
	free(rom->prgRom);
	free(rom->chrRom);
}

void romCreateTestROM(ROM *rom) {
	romInit(rom, TEST_ROM);
	rom->prgRom[START_ADDR + 1] = 0x06;
}

void romCreateFromFile(ROM *rom, const char *PATH) {
	FILE *file = fopen(PATH, "rb");
	if( file == NULL ) {
		errPrint(C_RED, "Couldn't open ROM '%s'", PATH);
		exit(74);
	}

	fseek(file, 0L, SEEK_END);
	const size_t FILE_SIZE = (size_t)ftell(file);
	rewind(file);

	uint8_t *buffer = (uint8_t *)malloc(FILE_SIZE + 1);
	if( buffer == NULL ) {
		errPrint(C_RED, "Sem memoria o bastante para ler o arquivo '%s'", PATH);
		exit(74);
	}

	const size_t BYTES_READ = fread(buffer, sizeof(uint8_t), FILE_SIZE, file);
	if( BYTES_READ < FILE_SIZE ) {
		errPrint(C_RED, "Sem memoria o bastante para ler o arquivo '%s'", PATH);
		exit(74);
	}

	buffer[BYTES_READ] = '\0';
	fclose(file);

	romInit(rom, buffer);
}
