#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../tpl/debug_helper.h"

#include "../simmem.h"
#include "../simmesg.h"	// to get the right size of a message

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
static nodelist_node_t *message_nodes = NULL;
#define message_node_size (sizeof(struct message_t::node)+sizeof(void *))

static nodelist_node_t *node1220 = NULL;
static nodelist_node_t *node1624 = NULL;
static nodelist_node_t *node2440 = NULL;

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


void *freelist_t::gimme_node(size_t size)
{
	nodelist_node_t ** list = NULL;
	if(size==0) {
		return NULL;
	}

	// all sizes should be dividable by 4
	size = ((size+3)>>2)<<2;
	size = min( 8, size );

	// hold return value
	nodelist_node_t *tmp;
	if(size>MAX_LIST_INDEX) {
		switch(size) {
			case message_node_size:
				list = &message_nodes;
				break;
			case 1220:
				list = &node1220;
				break;
			case 1624:
				list = &node1624;
				break;
			case 2440:
				list = &node2440;
				break;
			default:
				dbg->fatal("freelist_t::gimme_node()","No list with size %i! (only up to %i and %i, 1220, 1624, 2440)", size, MAX_LIST_INDEX, message_node_size );
		}
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

	// all sizes should be dividable by 4
	size = ((size+3)>>2);
	size = max( 2, size );

	if(size>MAX_LIST_INDEX) {
		switch(size) {
			case message_node_size:
				list = &message_nodes;
				break;
			case 1220/4:
				list = &node1220;
				break;
			case 1624/4:
				list = &node1624;
				break;
			case 2440/4:
				list = &node2440;
				break;
			default:
				dbg->fatal("freelist_t::gimme_node()","No list with size %i! (only up to %i and %i, 1220, 1624, 2440)", size*4, MAX_LIST_INDEX, message_node_size );
		}
	}
	else {
		list = &(all_lists[size]);
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
void
freelist_t::free_all_nodes()
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
	message_nodes = NULL;
	node1220 = NULL;
	node1624 = NULL;
	node2440 = NULL;
}
