/*
 * simmem.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

// use this for stress test; but it will return less than 32000 handles ...
//#define HARD_DEBUG

#if defined(WIN32)  && defined(HARD_DEBUG)

#include <assert.h>

#include "simdebug.h"
// use the ct routines for debugging
#include "dbgheap.cpp"


extern "C" {
void * guarded_malloc(int size)
{
	assert(size!=0);
	void *r = malloc(size);
	// sometime retries helps
	assert(r!=0);
	return r;
}

void * guarded_realloc(void *old, int newsize)
{
	assert(newsize!=0);
	assert(old!=0);
	void *r = realloc(old,newsize);
	assert(r!=0);
	return r;
}

void guarded_free(void *p)
{
	assert(p!=0);
	free(p);
}
}

#else

#include <stdio.h>
#include <stdlib.h>

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


extern "C" {



void * guarded_malloc(int size)
{
#ifdef USE_KEYLOCK
    unsigned char *base = (unsigned char *)malloc(size + sizeof(int)*3);
    int  *p = (int *)base;

    // fprintf(stderr, "Message: guarded_alloc(): allocating %d bytes, sig=%d, at %p\n", size, count, base);

    // write sig
    p[0] = count;

    count = (count + 1) & 0x7FFFFFF;


    // write length
    p[1] = size;


    // write end sig
    base[size+8] = 0xde;
    base[size+9] = 0xad;
    base[size+10] = 0xde;
    base[size+11] = 0xad;



    // hand back pointer behind sig and count,
    // so that sig and count are hidden
    return base+8;

#else

#ifdef DO_STATS
    block_count ++;
    printf("Message: guarded_alloc(): allocating %d bytes, %ld block\n", size, block_count);
#endif


    return malloc(size);
#endif
}

void * guarded_realloc(void *old, int newsize)
{
#ifdef USE_KEYLOCK
    if(old != NULL) {
	unsigned char * base = ((unsigned char *)old) - 8;
	int *p = (int *)base;
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
	    dbg->fatal("guarded_realloc()",
		       "check is %d, valid range is 0..%d\n", check, count);
	}
	// check size
	if(size < 0) {
	    dbg->fatal("guarded_realloc()",
		       "size is %d, valid range is >= 0\n", size);
	}
	// check end marker
	if(dead != 0xdeaddead) {
	    dbg->fatal("guarded_realloc()",
		       "dead marker for %p (%d bytes) is %d, should be 0xdeaddead\n", base, size, dead);
	}
	base = (unsigned char *)realloc(base, newsize + sizeof(int)*3);

	// write length
	p[1] = newsize;

	// write end sig
	base[newsize+8] = 0xde;
	base[newsize+9] = 0xad;
	base[newsize+10] = 0xde;
	base[newsize+11] = 0xad;

	// hand back pointer behind sig and count,
	// so that sig and count are hidden
	return base+8;
    }
    else {
	return guarded_malloc(newsize);
    }
#else
//    printf("Message: guarded_alloc(): allocating %d bytes\n", size);

    return realloc(old, newsize);
#endif
}


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
	    dbg->fatal("guarded_free()",
		       "check is %d, valid range is 0..%d\n", check, count);
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


} // extern "C"

#include "dataobj/freelist.h"

#ifdef USE_KEYLOCK
void * operator new(size_t size)
{
	return guarded_malloc(size);
}

void operator delete(void *p)
{
	guarded_free(p);
}
#endif

#endif
