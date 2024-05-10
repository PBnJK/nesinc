#include "SDL2/SDL.h"
#include "common.h"
#include "cpu.h"
#include "error.h"
#include "rom.h"
#include "screen.h"
#include "test.h"

int main(int argc, char *argv[]) {
	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		errPrint(C_RED,
				 "Couldn't initialize SDL.\n"
				 "SDL_Error: %s",
				 SDL_GetError());
		return 1;
	}

	screenInit(&gScreen);

	if( argc == 2 ) {
		ROM rom; /* TODO: move into fn */
		romCreateFromFile(&rom, argv[1]);
		CPU cpu;
		cpuInitFromROM(&cpu, rom);
		cpuRun(&cpu);

		return 0;
	}

	const bool TEST_RESULT = testRun();
	screenFree(&gScreen);

	return TEST_RESULT ? 0 : 2;
}
