/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#ifndef SIMMEM_H
#define SIMMEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#	include <malloc.h>
#	define guarded_free free
#else
void guarded_free(void* ptr);
#endif

void* xmalloc(size_t size);             // Throws std::bad_alloc on failure
void* xrealloc(void * const ptr, size_t size); // Throws std::bad_alloc on failure

#define MALLOC(type)     ((type*)xmalloc(sizeof(type)))       // Allocate an object of a certain type
#define MALLOCN(type, n) ((type*)xmalloc(sizeof(type) * (n))) // Allocate n objects of a certain type

#define REALLOC(ptr, type, n) (type*)xrealloc((void * const)ptr, sizeof(type) * (n)) // Reallocate n objects of a certain type

#ifdef __cplusplus
}
#endif

#endif
