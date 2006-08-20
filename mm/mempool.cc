/*
 * Copyright (c) 2001 Hansjörg Malthaner
 *
 * Usage for Iso-Angband is granted. Usage in other projects is not
 * permitted without the agreement of the author.
 */

/* mempool.cc
 *
 * Speicherverwaltung mit Pools (Bloecken)
 * von Hj. Malthaner, 2000
 */

#include <stdlib.h>
#include <stdio.h>

#include "memblock.h"
#include "mempool.h"


mempool_t::mempool_t(int s)
{
  elsize = s;
  a_free_block = first_free = list = new memblock_t( elsize );
}

void *
mempool_t::alloc()
{
    void *p = NULL;

    if(a_free_block) {
	p = a_free_block->alloc_slot();

	if(a_free_block->get_free_entries() == 0) {
	    a_free_block = NULL;
	}

	first_free = list;
    }

    if(p == NULL) {
	// suche beim mutmaßlich ersten block mit freien slots beginnen

	memblock_t *t = first_free;

	while(t != NULL && t->get_free_entries() == 0) {
	    t = t->get_next();
	}

	if(t==NULL) {
	    //    printf("Allocating a new block\n");

	    t = new memblock_t(elsize);

	    if(t) {
		t->set_next(list);
		list = t;
	    } else {
		fprintf(stderr, "Error: mempool: new memblock failed!\n");
	    }
	}

	// alle vorigen blocks waren voll
	first_free = t;

	p = t->alloc_slot();

	if(p == NULL) {
	    fprintf(stderr, "Error: mempool out of memory!\n");
	}
    }

    return p;
}

void
mempool_t::free(void *p)
{
    memblock_t *t = list;

    while(t != NULL && !t->is_in_block(p)) {
	t = t->get_next();
    }
    // assert(t != NULL);

    if(t != NULL) {
	t->free_slot(p);

	// konservative abschätzung
	first_free = list;

	// aber wir wissen, das hier ein slot frei wurde
	a_free_block = t;

    } else {
	fprintf(stderr, "error: mempool_t::free %p is an invalid address!\n", p);

	first_free = list;
	a_free_block = NULL;

	abort();
    }
}
