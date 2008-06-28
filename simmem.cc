/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */
#include "simmem.h"

#undef guarded_free

// use this for stress test; but it will return less than 32000 handles ...
//#define HARD_DEBUG

#if defined(WIN32)  && defined(HARD_DEBUG)

#include <assert.h>

#include "simdebug.h"
// use the ct routines for debugging
#include "dbgheap.cpp"


void guarded_free(void *p)
{
	assert(p!=0);
	free(p);
}

#else

#include <stdio.h>
#include <stdlib.h>

#include "simdebug.h"

/*
 * Define this to use a key-lock mechanism for sanity checks.
 * This can detect some kinds of overruns, underruns and double free's of
 * the same memory area.
 * @author Hj. Malthaner
 */
//#define USE_KEYLOCK

#ifdef USE_KEYLOCK

#include "simdebug.h"

static long count=0;
static long low_mark=0;
#endif


//#define DO_STATS


#ifdef DO_STATS

static long block_count = 0;

#endif


void guarded_free(void *p)
{
#ifdef USE_KEYLOCK
    if(p != NULL) {
        unsigned char * base = ((unsigned char *) p) - 8;
	int * p = (int *)base;
	const int check = *p;
	const int size = *(p+1);
	const unsigned int dead =
	  (base[size+8] << 24) +
	  (base[size+9] << 16) +
	  (base[size+10] << 8) +
	  (base[size+11] << 0);


	// fprintf(stderr, "Message: guarded_free(): freeing %d bytes at %p, check %d, dead %x\n", size, base, check, dead);

	// check sig
	if(check < low_mark || check >= count) {
		printf("guarded_free(): check is %d, valid range is 0..%d\n", check, count);
		exit(0);
//	    dbg->fatal("guarded_free()","check is %d, valid range is 0..%d\n", check, count);
	}

	// destroy sig, set to invalid range
	*p = -1;


	// check size
	if(size < 0) {
	    dbg->fatal("guarded_free()",
		       "size is %d, valid range is >= 0\n", size);
	}

	// destroy size, set to invalid range
	*(p+1) = -1;


	if(check == low_mark) {
	    low_mark ++;
	}


	// check end marker

	if(dead != 0xdeaddead) {
	    dbg->fatal("guarded_free()",
		       "dead marker for %p (%d bytes) is %d, should be 0xdeaddead\n", base, size, dead);
	}

	// hand back mem to os
	free( base );
    }
#else

#ifdef DO_STATS
    block_count --;
#endif

    free( p );
#endif
}


#ifdef USE_KEYLOCK
#error "operator new [] missing!"

void* operator new(size_t const size)
{
	return xmalloc(size);
}

void operator delete(void *p)
{
	guarded_free(p);
}
#endif

#endif


void* xmalloc(size_t const size)
{
#if defined USE_KEYLOCK
	void* const p = malloc(size + sizeof(int) * 3);
#else
#	if defined DO_STATS
	++block_count;
	printf("Message: xmalloc(): allocating %d bytes, %ld block\n", size, block_count);
#	endif

	void* const p = malloc(size);
#endif
	if (!p) {
		// use unified error handler instead, since BeOS need this as C style file!
		dbg->fatal("xmalloc()", "Could not alloc %li bytes.", (long)size );
	}

#if defined USE_KEYLOCK
	((int*)p)[0] = count; // write sig
	count = (count + 1) & 0x7FFFFFF;

	((int*)p)[1] = size; // write size

	// write end sig
	((unsigned char*)p)[size +  8] = 0xde;
	((unsigned char*)p)[size +  9] = 0xad;
	((unsigned char*)p)[size + 10] = 0xde;
	((unsigned char*)p)[size + 11] = 0xad;

	// hand back pointer behind sig and count, so that sig and count are hidden
	return (unsigned char*)p + 8;
#else
	return p;
#endif
}


void* xrealloc(void* const ptr, size_t const size)
{
#ifdef USE_KEYLOCK
	if (ptr != NULL) {
		unsigned char* const base  = ((unsigned char*)ptr) - 8;
		int            const check = ((int*)p)[0];
		int            const size  = ((int*)p)[1];
		unsigned int   const dead  =
			(base[size +  8] << 24) +
			(base[size +  9] << 16) +
			(base[size + 10] <<  8) +
			(base[size + 11] <<  0);

		if (check < low_mark || check >= count) { // check sig
			dbg->fatal("xrealloc()", "check is %d, valid range is 0..%d\n", check, count);
		}
		if (size < 0) { // check size
			dbg->fatal("xrealloc()", "size is %d, valid range is >= 0\n", size);
		}
		if (dead != 0xdeaddead) { // check end marker
			dbg->fatal("xrealloc()", "dead marker for %p (%d bytes) is %d, should be 0xdeaddead\n", base, size, dead);
		}
	}

	void* const p = realloc(ptr, size + sizeof(int) * 3);
#else
	void* const p = realloc(ptr, size);
#endif
	if (!p) {
		// use unified error handler instead, since BeOS need this as C style file!
		dbg->fatal("realloc()", "Could not alloc %li bytes.", (long)size );
	}

#if defined USE_KEYLOCK
	if (ptr == NULL) {
		((int*)p)[0] = count; // write sig
		count = (count + 1) & 0x7FFFFFF;
	}

	((int*)p)[1] = size; // write size

	// write end sig
	((unsigned char*)p)[size +  8] = 0xde;
	((unsigned char*)p)[size +  9] = 0xad;
	((unsigned char*)p)[size + 10] = 0xde;
	((unsigned char*)p)[size + 11] = 0xad;

	// hand back pointer behind sig and count, so that sig and count are hidden
	return (unsigned char*)p + 8;
#else
	return p;
#endif
}
