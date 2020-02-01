/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */
#include <stdio.h>	// since BeOS needs size_t from there ...
#include <stdlib.h>

#include "simmem.h"
#include "simdebug.h"


void* xmalloc(size_t const size)
{
	void* const p = malloc(size);

	if (!p) {
		// use unified error handler instead, since BeOS need this as C style file!
		dbg->fatal("xmalloc()", "Could not alloc %li bytes.", (long)size );
	}
	return p;
}


void* xrealloc(void* const ptr, size_t const size)
{
	void* const p = realloc(ptr, size);

	if (!p) {
		// use unified error handler instead, since BeOS need this as C style file!
		dbg->fatal("realloc()", "Could not alloc %li bytes.", (long)size );
	}
	return p;
}
