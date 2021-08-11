/* Richard James Howe, Email: howe.r.j.89@gmail.com, Public Domain, https:github.com/howerj/allocator */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdarg.h>

#ifndef ALLOCATOR_FN
#define ALLOCATOR_FN
typedef void *(*allocator_fn)(void *arena, void *ptr, size_t oldsz, size_t newsz);
#endif

enum { ALLOCATOR_TYPE_DEFAULT, ALLOCATOR_TYPE_LIST, ALLOCATOR_TYPE_NO_FREE, ALLOCATOR_TYPE_FAIL, };

typedef int (*allocator_trace_fn)(void *param, const char *fmt, va_list ap);

int allocator_format(void **arena, int type, unsigned char *buf, size_t len);
int allocator_reformat(void *arena, int type);
int allocator_is_ptr_valid(void *arena, void *ptr);
int allocator_is_ptr_allocated(void *arena, void *ptr);
int allocator_set_trace(void *arena, allocator_trace_fn trace, void *param);
int allocator_get_max_allocatable(void *arena, size_t *size);
int allocator_get_overhead(void *arena, size_t *size);
int allocator_get_free(void *arena, size_t *size);
int allocator_get_total(void *arena, size_t *size);
int allocator_test(void);
void *allocator(void *arena, void *ptr, size_t oldsz, size_t newsz);


#ifdef __cplusplus
}
#endif
#endif
