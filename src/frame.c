#include "frame.h"

#include <stdio.h>
#include <string.h>

#include "ppu.h"

void frameInit(Frame *frame) {
	memset(frame->data, 0, FRAME_SIZE);
}

void frameSetPixel(Frame *frame, const size_t X, const size_t Y,
				   SDL_Color color) {
	const size_t BASE = Y * 3 * SCR_W + X * 3;

	if( (BASE + 2) < FRAME_SIZE ) {
		frame->data[BASE] = color.r;
		frame->data[BASE + 1] = color.g;
		frame->data[BASE + 2] = color.b;
	}
}
