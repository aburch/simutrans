/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

/* memblock.cc
 *
 * Verwaltung von Speicherbloecken fuer Memory Pools
 * von Hj. Malthaner, 2000
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "memblock.h"
#include "../simmem.h"

memblock_t::memblock_t(int size)
{
  elsize = size;

  base = guarded_malloc(size*BLOCKSIZE);

  memset(map, 0, BLOCKSIZE/8);

  free_entries = BLOCKSIZE;
  first_free = 0;

  next = NULL;
  a_free_slot = NULL;
}

void *
memblock_t::alloc_slot()
{
  void * p = NULL;

//  printf("alloc slot in block %p\n", this);


  if(free_entries == 0) {
    // no slot free

    fprintf(stderr, "Warning: alloc_slot from full memblock!\n");

    return NULL;
  } else {
    // cached slot ?
    if(a_free_slot) {
	p = a_free_slot;
	a_free_slot = 0;

	free_entries --;

	return p;
    }

    // find a free group

    for(int i=first_free; i<BLOCKSIZE/32; i++) {
      if(map[i] != 0xFFFFFFFF) {
	const unsigned int group_map = map[i];

	first_free = i;

	int slot = 0;

	if((group_map & 0xFF) == 0xFF) {
	    slot += 8;
	    if((group_map & 0xFFFF) == 0xFFFF) {
		slot += 8;
		if((group_map & 0xFFFFFF) == 0xFFFFFF) {
		    slot += 8;
		}
	    }
	}

	for(; slot<32 ; slot++) {
	  if((group_map & (1<<slot)) == 0) {
	    map[i] |= (1<<slot);

	    p = ((char *)base)+((i*32+slot)*elsize);

	    free_entries --;

	    // exit directly if a slot was found
	    return p;
	  }
	}
      }
    }
  }

  if(p == NULL) {
    fprintf(stderr, "Error: memblock out of memory!\n");
    fprintf(stderr, "Error: free entry counter is %d!\n", free_entries);
    fprintf(stderr, "Error: first free is %d!\n", first_free);
  }

  return p;
}

void
memblock_t::free_slot(void *p)
{
    if(a_free_slot == NULL) {
	// gerade kein slot im cache
	// dann cache updaten

	a_free_slot = p;

    } else {
	// free slot cache schon belegt
	// slot freigeben

	char *el = (char *)p;

	const int elnum = (el - ((char *)base))/elsize;

//	fprintf(stderr, "elnum %d (%p)\n", elnum, this);

	const int group = elnum/32;
	const int slot = elnum & 31;

	// sanity check
	if(a_free_slot == p) {
	    fprintf(stderr, "Error: free'd slot %p twice!\n", p);
	    return;
	}

	// sanity check
	if((map[group] & (1 << slot)) == 0) {
	    fprintf(stderr, "Error: free'd slot %p twice!\n", p);
	    return;
	}


	map[group] &= ~(1 << slot);

	// konservative abschätzung
	first_free = 0;

    }

    free_entries++;
}
