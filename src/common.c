/* Copyright (c) 2015 Fabian Schuiki */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void die_impl(const char *filename, unsigned line, const char *fmt, ...) {
	fprintf(stderr, "%s.%d: ", __FILE__, __LINE__);

	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	fputc('\n', stderr);
	abort();
}
