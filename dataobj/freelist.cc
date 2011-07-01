#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../simtypes.h"
#include "../simmem.h"
#include "freelist.h"


#ifdef DEBUG
#define DEBUG_MEM
#endif

#undef DEBUG_MEM // XXX deactivate because it is broken


struct nodelist_node_t
{
	nodelist_node_t* next;
};


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
	if(size==0) {
		return NULL;
	}

//#ifdef _64BIT
//	// all sizes should be divisible by 8
//	size = ((size+3)>>2)<<2;
//	if(size == 4)
//	{
//		size = 8;
//	}
//#else
//	// all sizes should be divisible by 4
//	size = ((size+3)>>2)<<2;
//#endif

	// all sizes should be divisible by 4 and at least as large as a pointer
	size = max( min_size, size );
	size = (size+3)>>2;
	size <<= 2;

	// hold return value
	nodelist_node_t *tmp;
	if(size>MAX_LIST_INDEX) {
		return xmalloc(size);
	}
	else {
		list = &(all_lists[size/4]);
	}

	// need new memory?
	if(*list==NULL) {
		int num_elements = 32764/(int)size;
		char* p = (char*)xmalloc(num_elements * size + sizeof(p));
		// put the memory into the chunklist for free it
		nodelist_node_t *chunk = (nodelist_node_t *)p;
		chunk->next = chunk_list;
		chunk_list = chunk;
		p += sizeof(p);
		// then enter nodes into nodelist
		for( int i=0;  i<num_elements;  i++ ) {
			nodelist_node_t *tmp = (nodelist_node_t *)(p+i*size);
			tmp->next = *list;
			*list = tmp;
		}
	}
	// return first node
	tmp = *list;
	*list = tmp->next;
	return (void *)tmp;
}


#ifdef DEBUG_MEM
// put back a node and check for consistency
static void putback_check_node(nodelist_node_t** list, nodelist_node_t* p)
{
	if(*list<p) {
		if(p==*list) {
			dbg->fatal("freelist_t::putback_check_node()","node %p already freeded!",p);
		}
		p->next = *list;
		*list = p;
	}
	else {
		nodelist_node_t *tmp = *list;
		while(tmp->next>p) {
			tmp = tmp->next;
		}
		if(p==tmp->next) {
			dbg->fatal("freelist_t::putback_check_node()","node %p already freeded!",p);
		}
		p->next = tmp->next;
		tmp->next = p;
	}
}
#endif


void freelist_t::putback_node( size_t size, void *p )
{
	nodelist_node_t ** list = NULL;
	if(size==0  ||  p==NULL) {
		return;
	}

//#ifdef _64BIT
//	// all sizes should be divisible by 8
//	size = ((size+3)>>2);
//	if(size == 1)
//	{
//		size = 2;
//	}
//#else
//	// all sizes should be divisible by 4
//	size = ((size+3)>>2);
//#endif
	
	// all sizes should be dividable by 4
	size = max( min_size, size );
	size = ((size+3)>>2);
	size <<= 2;

	if(size>MAX_LIST_INDEX) {
		free(p);
		return;
	}
	else {
		list = &(all_lists[size/4]);
	}
#ifdef DEBUG_MEM
	putback_check_node(list,(nodelist_node_t *)p);
#else
	// putback to first node
	nodelist_node_t *tmp = (nodelist_node_t *)p;
	tmp->next = *list;
	*list = tmp;
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
		guarded_free( p );
	}
	printf("freelist_t::free_all_nodes(): zeroing\n");
	for( int i=0;  i<NUM_LIST;  i++  ) {
		all_lists[i] = NULL;
	}
	printf("freelist_t::free_all_nodes(): ok\n");
}
