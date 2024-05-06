#include "screen.h"

#include "error.h"

Screen gScreen = {0};

void screenInit(Screen *screen) {
	screen->window = SDL_CreateWindow("NESINC Emulator", SDL_WINDOWPOS_CENTERED,
									  SDL_WINDOWPOS_CENTERED, SCR_W * SCR_SCALE,
									  SCR_H * SCR_SCALE, SDL_WINDOW_SHOWN);
	if( !screen->window ) {
		errPrint(C_RED, "Unable to create window.\n SDL_Error: %s",
				 SDL_GetError());
	}

	screen->renderer =
		SDL_CreateRenderer(screen->window, -1, SDL_RENDERER_ACCELERATED);
	if( !screen->renderer ) {
		errPrint(C_RED, "Unable to create renderer.\n SDL_Error: %s",
				 SDL_GetError());
	}

	SDL_RenderSetScale(screen->renderer, SCR_SCALE, SCR_SCALE);

	screen->texture = SDL_CreateTexture(screen->renderer, SDL_PIXELFORMAT_RGB24,
										SDL_TEXTUREACCESS_TARGET, SCR_W, SCR_H);
	if( !screen->texture ) {
		errPrint(C_RED, "Unable to create texture.\n SDL_Error: %s",
				 SDL_GetError());
	}
}

void screenFree(Screen *screen) {
	SDL_DestroyTexture(screen->texture);
	SDL_DestroyRenderer(screen->renderer);
	SDL_DestroyWindow(screen->window);
}
