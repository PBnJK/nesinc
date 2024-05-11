#ifndef GUARD_NESINC_RECT_H_
#define GUARD_NESINC_RECT_H_

#include "common.h"

typedef struct _Rect {
	size_t x1;
	size_t y1;
	size_t x2;
	size_t y2;
} Rect;

#define RECT(X1, Y1, X2, Y2) ((Rect){X1, Y1, X2, Y2})

#endif	// GUARD_NESINC_RECT_H_
