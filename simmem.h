/*
 * simmem.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#define guarded_malloc	malloc
#define guarded_realloc	realloc
#define guarded_free	free
#else
void * guarded_malloc(int size);
void *guarded_realloc(void *old, int newsize);
void guarded_free(void *p);
#endif

#ifdef __cplusplus
}
#endif
