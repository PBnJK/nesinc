#ifndef GUARD_NESINC_ERROR_H_
#define GUARD_NESINC_ERROR_H_

#include "common.h"

typedef enum {
	C_RED = 31,
	C_YELLOW = 33,
	C_GREEN = 32,
} OutColor;

void errPrint(const OutColor COLOR, const char *MSG, ...);

#endif	// GUARD_NESINC_ERROR_H_
