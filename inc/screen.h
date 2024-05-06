#ifndef GUARD_NESINC_SCREEN_H_
#define GUARD_NESINC_SCREEN_H_

#include "SDL2/SDL.h"
#include "common.h"

#define SCR_W 32
#define SCR_H 32

#define SCR_SCALE 10.0f

typedef struct _Screen {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
} Screen;

extern Screen gScreen;

void screenInit(Screen *screen);
void screenFree(Screen *screen);

#endif	// GUARD_NESINC_SCREEN_H_
