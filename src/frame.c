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

Frame frameShowTile(uint8_t *CHR_ROM, const size_t BANK_N,
					const size_t TILE_N) {
	Frame frame;
	frameInit(&frame);

	const size_t BANK = (BANK_N * 0x1000);

	uint8_t tile[16];
	memcpy(tile, CHR_ROM + (BANK + TILE_N * 16), 16);

	for( uint8_t y = 0; y < 8; ++y ) {
		uint8_t upper = tile[y];
		uint8_t lower = tile[y + 8];

		for( uint8_t x = 7; x != 255; --x ) {
			const uint8_t VALUE = (uint8_t)(((1 & upper) << 1) | (1 & lower));

			upper >>= 1;
			lower >>= 1;

			SDL_Color rgb = SYS_PAL[0x01];
			switch( VALUE ) {
				case 1:
					rgb = SYS_PAL[0x23];
					break;
				case 2:
					rgb = SYS_PAL[0x27];
					break;
				case 3:
					rgb = SYS_PAL[0x30];
					break;
			}

			frameSetPixel(&frame, x, y, rgb);
		}
	}

	return frame;
}
