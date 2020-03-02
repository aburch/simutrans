/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../simtypes.h"
#include "../simmem.h"
#include "freelist.h"

// define USE_VALGRIND_MEMCHECK to make
// valgrind aware of the freelist memory pool
#ifdef USE_VALGRIND_MEMCHECK
#include <valgrind/memcheck.h>
#endif

struct nodelist_node_t
{
#ifdef DEBUG_FREELIST
	unsigned magic : 16;
	unsigned free : 1;
	unsigned size : 15;
#endif
	nodelist_node_t* next;
};

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t freelist_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

// list of all allocated memory
static nodelist_node_t *chunk_list = NULL;

/* this module keeps account of the free nodes of list and recycles them.
 * nodes of the same size will be kept in the same list
 * to be more efficient, all nodes with sizes smaller than 16 will be used at size 16 (one cacheline)
 */

// if additional fixed sizes are required, add them here
// (the few request for larger ones are satisfied with xmalloc otherwise)


// for 64 bit, set this to 128
#define MAX_LIST_INDEX (128)

// list for nodes size 8...64
#define NUM_LIST ((MAX_LIST_INDEX/4)+1)

static nodelist_node_t *all_lists[NUM_LIST] = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL
};


// to have this working, we need chunks at least the size of a pointer
const size_t min_size = sizeof(void *);


void *freelist_t::gimme_node(size_t size)
{
	nodelist_node_t ** list = NULL;
	if(  size == 0  ) {
		return NULL;
	}

	// all sizes should be dividable by 4 and at least as large as a pointer
#ifdef DEBUG_FREELIST
	size = max( min_size, size + min_size);
#else
	size = max( min_size, size );
#endif
	size = (size+3)>>2;
	size <<= 2;

#ifdef MULTI_THREAD
	pthread_mutex_lock( &freelist_mutex );
#endif

	// hold return value
	nodelist_node_t *tmp;
	if(  size > MAX_LIST_INDEX  ) {
		// too large: just use malloc anyway
		tmp = (nodelist_node_t *)xmalloc(size);
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &freelist_mutex );
#endif
#ifdef DEBUG_FREELIST
		tmp->magic = 0xAA;
		tmp->free = 0;
		tmp->size = size/4;
#endif
		return tmp;
	}


	list = &(all_lists[size/4]);
	// need new memory?
	if(  *list == NULL  ) {
		int num_elements = 32764/(int)size;
		char* p = (char*)xmalloc(num_elements * size + sizeof(p));

#ifdef USE_VALGRIND_MEMCHECK
		// tell valgrind that we still cannot access the pool p
		VALGRIND_MAKE_MEM_NOACCESS(p, num_elements * size + sizeof(p));
#endif

		// put the memory into the chunklist for free it
		nodelist_node_t *chunk = (nodelist_node_t *)p;

#ifdef USE_VALGRIND_MEMCHECK
		// tell valgrind that we reserved space for one nodelist_node_t
		VALGRIND_CREATE_MEMPOOL(chunk, 0, false);
		VALGRIND_MEMPOOL_ALLOC(chunk, chunk, sizeof(*chunk));
		VALGRIND_MAKE_MEM_UNDEFINED(chunk, sizeof(*chunk));
#endif

		chunk->next = chunk_list;
		chunk_list = chunk;
		p += sizeof(p);
		// then enter nodes into nodelist
		for(  int i=0;  i<num_elements;  i++  ) {
			nodelist_node_t *tmp = (nodelist_node_t *)(p+i*size);
#ifdef USE_VALGRIND_MEMCHECK
			// tell valgrind that we reserved space for one nodelist_node_t
			VALGRIND_CREATE_MEMPOOL(tmp, 0, false);
			VALGRIND_MEMPOOL_ALLOC(tmp, tmp, sizeof(*tmp));
			VALGRIND_MAKE_MEM_UNDEFINED(tmp, sizeof(*tmp));
#endif
			tmp->next = *list;
			*list = tmp;
		}
	}

	// return first node of list
	tmp = *list;
	*list = tmp->next;

#ifdef USE_VALGRIND_MEMCHECK
	// tell valgrind that we now have access to a chunk of size bytes
	VALGRIND_MEMPOOL_CHANGE(tmp, tmp, tmp, size);
	VALGRIND_MAKE_MEM_UNDEFINED(tmp, size);
#endif

#ifdef MULTI_THREAD
	pthread_mutex_unlock( &freelist_mutex );
#endif

#ifdef DEBUG_FREELIST
	tmp->magic = 0x5555;
	tmp->free = 0;
	tmp->size = size/4;
#endif
	return (void *)&(tmp->next);
}


void freelist_t::putback_node( size_t size, void *p )
{
	nodelist_node_t ** list = NULL;
	if(  size==0  ||  p==NULL  ) {
		return;
	}

	// all sizes should be dividable by 4
#ifdef DEBUG_FREELIST
	size = max( min_size, size + min_size );
#else
	size = max( min_size, size );
#endif
	size = ((size+3)>>2);
	size <<= 2;

#ifdef MULTI_THREAD
	pthread_mutex_lock( &freelist_mutex );
#endif

	if(  size > MAX_LIST_INDEX  ) {
		free(p);
#ifdef MULTI_THREAD
		pthread_mutex_unlock( &freelist_mutex );
#endif
		return;
	}

	list = &(all_lists[size/4]);

#ifdef USE_VALGRIND_MEMCHECK
	// tell valgrind that we keep access to a nodelist_node_t within the memory chunk
	VALGRIND_MEMPOOL_CHANGE(p, p, p, sizeof(nodelist_node_t));
	VALGRIND_MAKE_MEM_NOACCESS(p, size);
	VALGRIND_MAKE_MEM_UNDEFINED(p, sizeof(nodelist_node_t));
#endif

	// putback to first node
	nodelist_node_t *tmp = (nodelist_node_t *)p;
#ifdef DEBUG_FREELIST
	tmp = (nodelist_node_t *)((char *)p - min_size);
	assert(  tmp->magic == 0x5555  &&  tmp->free == 0  &&  tmp->size == size/4  );
	tmp->free = 1;
#endif
	tmp->next = *list;
	*list = tmp;

#ifdef MULTI_THREAD
	pthread_mutex_unlock( &freelist_mutex );
#endif
}


// clears all list memories
void freelist_t::free_all_nodes()
{
	printf("freelist_t::free_all_nodes(): frees all list memory\n" );
	while(chunk_list) {
		nodelist_node_t *p = chunk_list;
		printf("freelist_t::free_all_nodes(): free node %p (next %p)\n",p,chunk_list->next);
		chunk_list = chunk_list->next;

		// now release memory
#ifdef USE_VALGRIND_MEMCHECK
		VALGRIND_DESTROY_MEMPOOL( p );
#endif
		free( p );
	}
	printf("freelist_t::free_all_nodes(): zeroing\n");
	for( int i=0;  i<NUM_LIST;  i++  ) {
		all_lists[i] = NULL;
	}
	printf("freelist_t::free_all_nodes(): ok\n");
}
