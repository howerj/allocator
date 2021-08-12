#include <stdbool.h>

typedef long Align;		/* for alignment to long boundary */

typedef union header {		/* block header */
	struct {
		union header *ptr;	/* next block if on free list */
		size_t size;	/* size of this block */
	} s;

	Align x;		/* force alignment of blocks */

} Header;

static Header base = { 0 };	/* empty list to get started */

static Header *freeptr = NULL;	/* start of free list */

/* malloc: general-purpose storage allocator */
void *kr_malloc(size_t nbytes)
{
	Header *p;
	Header *prevptr;
	size_t nunits;
	void *result;
	bool is_allocating;

	nunits = (nbytes + sizeof(Header) - 1) / sizeof(Header) + 1;

	prevptr = freeptr;
	if (prevptr == NULL) {	/* no free list yet */
		base.s.ptr = &base;
		freeptr = &base;
		prevptr = &base;
		base.s.size = 0;
	}

	is_allocating = true;
	for (p = prevptr->s.ptr; is_allocating; p = p->s.ptr) {
		if (p->s.size >= nunits) {	/* big enough */
			if (p->s.size == nunits) {	/* exactly */
				prevptr->s.ptr = p->s.ptr;
			} else {	/* allocate tail end */
				p->s.size -= nunits;
				p += p->s.size;
				p->s.size = nunits;
			}

			freeptr = prevptr;
			result = p + 1;
			is_allocating = false;	/* we are done */
		}

		if (p == freeptr) {	/* wrapped around free list */
			p = morecore(nunits);
			if (p == NULL) {
				result = NULL;	/* none left */
				is_allocating = false;
			}
		}
		prevptr = p;
	}			/* for */

	return result;
}
