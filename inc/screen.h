#ifndef GUARD_NESINC_SCREEN_H_
#define GUARD_NESINC_SCREEN_H_

#include "SDL2/SDL.h"
#include "common.h"

#define SCR_W 256u
#define SCR_H 240u

#define SCR_SCALE 2.0f

typedef struct _Screen {
	SDL_Window *window;
	SDL_Renderer *renderer;
	SDL_Texture *texture;
} Screen;

extern Screen gScreen;

void screenInit(Screen *screen);
void screenFree(Screen *screen);

#endif	// GUARD_NESINC_SCREEN_H_
