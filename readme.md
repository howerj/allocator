* Project: C Memory allocator for embedded use
* Author: Richard James Howe
* License: Public Domain
* Email: howe.r.j.89@gmail.com
* Repo: https://github.com/howerj/allocator

This is a (small) library for creating arenas to allocate within, which will
allow you to limit the amount of memory to a library (assuming the library
handles out of memory conditions correctly) or module within an embedded
system.

The users of this library should not use *malloc*, *calloc* or *realloc*, but
instead should use a function with the following type:

	typedef void *(*allocator_fn)(void *arena, void *ptr, size_t oldsz, size_t newsz);

Provided by:

	void *allocator(void *arena, void *ptr, size_t oldsz, size_t newsz);

In this library.

The single function is capable of allocating, reallocating, and freeing memory.
Your library should accept an *allocator\_fn* as a callback with a pointer
to an arena.

This allocator library also provides more details than the standard allocation
routines in C.

Libraries of mine that use the *allocator\_fn* are:

* <https://github.com/howerj/pickle>
* <https://github.com/howerj/httpc>
* <https://github.com/howerj/cdb>

When allocation is used within a library you should try to avoid using the
built in allocation routines, this will in part allow the library to be used on
embedded devices, or make it more likely to be (or better still, allocate
everything upfront, allow the library user to specify where the allocation
takes place, or just eliminate as much dynamic allocation as possible).

