#include "SDL2/SDL.h"
#include "common.h"
#include "error.h"
#include "screen.h"
#include "test.h"

int main(int argc, char *argv[]) {
	UNUSED(argc);
	UNUSED(argv);

	if( SDL_Init(SDL_INIT_VIDEO) < 0 ) {
		errPrint(C_RED,
				 "Couldn't initialize SDL.\n"
				 "SDL_Error: %s",
				 SDL_GetError());
		return 1;
	}

	screenInit(&gScreen);
	if( testRun() == true ) {
		screenFree(&gScreen);
		return 0;
	}

	screenFree(&gScreen);
	return 2;
}
