/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef SIMMEM_H
#define SIMMEM_H


#include <stddef.h>

void* xmalloc(size_t size);             // Throws std::bad_alloc on failure
void* xrealloc(void * const ptr, size_t size); // Throws std::bad_alloc on failure

#define MALLOC(type)             ((type*)xmalloc(sizeof(type)))       // Allocate an object of a certain type
#define MALLOCN(type, n)         ((type*)xmalloc(sizeof(type) * (n))) // Allocate n objects of a certain type
#define MALLOCF(type, member, n) ((type*)xmalloc(offsetof(type, member) + sizeof(*((type*)0)->member) * (n)))

#define REALLOC(ptr, type, n) (type*)xrealloc((void *)ptr, sizeof(type) * (n)) // Reallocate n objects of a certain type

#endif
