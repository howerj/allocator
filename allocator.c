/* Richard James Howe, Email: howe.r.j.89@gmail.com, Public Domain, https:github.com/howerj/allocator */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "allocator.h"

/* TODO: Size checks, formatting, tracing options, algorithm selection, tests, version number, canaries,
 * examples (memory mapping/file backed/pickle TCL interpreter) */
/* TODO: Experiment with a different API, could just pass around "buf/len" instead of arena? Or just
 * buf once it has been setup as the length can be stored within buf. */

#ifndef ALLOCATOR_ALIGNMENT
#define ALLOCATOR_ALIGNMENT (16ull)
#endif

#define ALIGN_MASK (ALLOCATOR_ALIGNMENT - 1ull)
// TODO: Build bug/assert wrapper/implies

typedef struct {
	unsigned char *buf, *aligned, *arena;
	allocator_trace_fn trace;
	void *trace_param;
	size_t buf_len, arena_len;
	int error, type;
	size_t nofree;
} allocator_t;

static void arena_validate(void *arena) {
	assert(arena);
	allocator_t *a = arena;
	assert(a->buf);
	assert(a->aligned);
	assert(a->buf_len >= ((sizeof (*a) + ALLOCATOR_ALIGNMENT) * 2ull));
	assert(a->nofree <= a->arena_len);
	switch (a->type) {
	case ALLOCATOR_TYPE_LIST:
	case ALLOCATOR_TYPE_NO_FREE:
	case ALLOCATOR_TYPE_FAIL:
		break;
	default: assert(0);
	}
}

static uintptr_t alignup(uintptr_t u) {
	return (u & ~ALIGN_MASK) + (ALLOCATOR_ALIGNMENT & (uintptr_t)((intptr_t)(-!!(u & ALIGN_MASK))));
}

static int alogger(void *arena, int fatal, const char *func, int line, const char *fmt, ...) {
	assert(arena);
	assert(fmt);
	allocator_t *a = arena;
	if (a->error)
		return -1;
	if (fatal)
		a->error = -line;
	if (a->trace == NULL)
		return 0;
	/* TODO: Trace fatal/func/line */
	va_list ap;
	va_start(ap, fmt);
	const int r1 = a->trace(a->trace_param, fmt, ap);
	va_end(ap);
	return r1;
fail:
	a->error = -1;
	return -1;
}

#define alog(ARENA, FMT, ...) alogger((ARENA), 0, __func__, __LINE__, (FMT), ##__VA_ARGS__)
#define adie(ARENA, FMT, ...) alogger((ARENA), 1, __func__, __LINE__, (FMT), ##__VA_ARGS__)

int allocator_format(void **arena, int type, unsigned char *buf, size_t len) {
	assert(arena);
	assert(buf);
	*arena = NULL;
	memset(buf, 0, len);
	unsigned char *aligned = (unsigned char*)alignup((uintptr_t)buf);
	type = type == ALLOCATOR_TYPE_DEFAULT ? ALLOCATOR_TYPE_LIST : type;
	allocator_t a = {
		.trace = NULL,
		.buf = buf,
		.buf_len = len,
		.aligned = aligned,
		.error = 0,
		.type = type,
	};
	switch (type) {
	case ALLOCATOR_TYPE_LIST:
	case ALLOCATOR_TYPE_NO_FREE:
	case ALLOCATOR_TYPE_FAIL:
		break;
	default:
		return -1;
	}

	if (len < ((sizeof (a) + ALLOCATOR_ALIGNMENT) * 2ull))
		return -1;
	a.arena = (unsigned char*)alignup((uintptr_t)aligned + sizeof (a));
	a.arena_len = (buf + len) - a.arena;
	memcpy(aligned, &a, sizeof a);
	*arena = (void*)aligned;
	return 0;
}

int allocator_reformat(void *arena, int type) {
	arena_validate(arena);
	allocator_t *a = arena;
	void *newarena = arena;
	const int r = allocator_format(&newarena, type, a->buf, a->buf_len);
	if (r >= 0)
		assert(newarena == arena);
	return r;
}

int allocator_is_ptr_valid(void *arena, void *ptr) {
	arena_validate(arena);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	unsigned char *end = a->arena + a->arena_len, *p = ptr;
	if (p < a->arena || p > end)
		return 0;
	switch (a->type) {
	case ALLOCATOR_TYPE_LIST:
	case ALLOCATOR_TYPE_NO_FREE:
	case ALLOCATOR_TYPE_FAIL:
	default:
		return -1;
	}

	return 1;
}

int allocator_is_ptr_allocated(void *arena, void *ptr) {
	arena_validate(arena);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	const int valid = allocator_is_ptr_valid(arena, ptr);
	if (valid < 0)
		return -1;
	if (valid == 0)
		return 0;

	return 1;
}

int allocator_get_max_allocatable(void *arena, size_t *size) {
	arena_validate(arena);
	assert(size);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	*size = 0;
	switch (a->type) {
	case ALLOCATOR_TYPE_NO_FREE: *size = a->arena_len - a->nofree; return 0;
	case ALLOCATOR_TYPE_FAIL:  return 0;
	case ALLOCATOR_TYPE_LIST: break;
	}
	return -1;
}

int allocator_get_overhead(void *arena, size_t *size) {
	arena_validate(arena);
	assert(size);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	*size = 0;
	return -1;
}

int allocator_get_free(void *arena, size_t *size) {
	arena_validate(arena);
	assert(size);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	*size = 0;
	return -1;
}

int allocator_get_total(void *arena, size_t *size) {
	arena_validate(arena);
	assert(size);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	*size = 0;
	return -1;
}

int allocator_set_trace(void *arena, allocator_trace_fn trace, void *param) {
	arena_validate(arena);
	allocator_t *a = arena;
	if (a->error < 0)
		return a->error;
	a->trace = trace;
	a->trace_param = param;
	return 0;
}

void *allocator(void *arena, void *ptr, size_t oldsz, size_t newsz) {
	arena_validate(arena);
	allocator_t *a = arena;
	if (a->error < 0)
		return NULL;
	if (a->type == ALLOCATOR_TYPE_FAIL) /* always fails, might want to do a probabilistic one */
		return NULL;
	return NULL;
}

int allocator_test(void) {
	if (alignup(0) != 0) return -1;
	if (alignup(1) != ALLOCATOR_ALIGNMENT) return -1;
	if (alignup(ALLOCATOR_ALIGNMENT - 1ull) != ALLOCATOR_ALIGNMENT) return -1;
	if (alignup(ALLOCATOR_ALIGNMENT) != ALLOCATOR_ALIGNMENT) return -1;
	if (alignup(ALLOCATOR_ALIGNMENT + 1ull) != (2ull * ALLOCATOR_ALIGNMENT)) return -1;
	return 0;
}

