#include "allocator.h"
#include <assert.h>
#include <stdio.h>

int allocator_trace(void *param, const char *fmt, va_list ap) {
	assert(param);
	assert(fmt);
	va_list nap;
	va_copy(nap, ap);
	return vfprintf((FILE*)param, fmt, ap);
}

int main(void) {
	FILE *out = stdout;

	fprintf(out, "Allocator Library\nRichard James Howe / howe.r.j.89@gmail.com / Public Domain\n");
	fprintf(out, "version=%s\n", ALLOCATOR_VERSION);

	if (allocator_test() < 0) {
		fprintf(out, "Internal tests failed\n");
		return 1;
	}

	fprintf(out, "tests passed\n");
	return 0;
}
