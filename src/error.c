#include "error.h"

#include <stdarg.h>
#include <stdio.h>

#define COLOR_RESET "\033[0m"

void errPrint(const OutColor COLOR, const char *MSG, ...) {
	va_list args;
	va_start(args, MSG);

	fprintf(stderr, "\033[0;%um", COLOR);
	vfprintf(stderr, MSG, args);
	fputs(COLOR_RESET "\n", stderr);

	va_end(args);
}
