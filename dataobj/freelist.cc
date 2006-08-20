
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "../tpl/debug_helper.h"

#include "../simmem.h"
#include "../simmesg.h"	// to get the right size of a message

#include "freelist.h"


#ifdef DEBUG
//#define DEBUG_MEM
#endif

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

// list for nodes size 8...64
#define NUM_LIST ((64/4)+1)

static nodelist_node_t *all_lists[NUM_LIST] = {
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL,
	NULL
};


void *
freelist_t::gimme_node(int size)
{
	nodelist_node_t ** list = NULL;
	if(size==0) {
		return NULL;
	}

	if(size>64) {
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
			default:
				ERROR("freelist_t::gimme_node()","No list with size %i! (only up to 64 and %i, 1220, 1624)", size, message_node_size );
				trap();
		}
	}
	else {
		list = &(all_lists[(size+3)/4]);
	}
	// need new memory?
	if(*list==NULL) {
		int num_elements = 32764/size;
		char *p = (char *)guarded_malloc( num_elements*size+sizeof(p) );
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
	nodelist_node_t *tmp=*list;
	*list = tmp->next;
	tmp->next = NULL;	// paranoia / respective for recycling in other parts
	return (void *)tmp;
}


// put back a node and checks for consitency
void
freelist_t::putback_check_node(nodelist_node_t ** list,nodelist_node_t *p)
{
	if(*list<p) {
		if(p==*list) {
			ERROR("freelist_t::putback_check_node()","node %p already freeded!",p);
			trap();
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
			ERROR("freelist_t::putback_check_node()","node %p already freeded!",p);
			trap();
		}
		p->next = tmp->next;
		tmp->next = p;
	}
}



void
freelist_t::putback_node(int size,void *p)
{
	nodelist_node_t ** list = NULL;
	if(size==0  ||  p==NULL) {
		return;
	}
	if(size>64) {
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
			default:
				ERROR("freelist_t::gimme_node()","No list with size %i! (only up to 64 and %i, 1220, 1624)", size, message_node_size );
				trap();
		}
	}
	else {
		list = &(all_lists[(size+3)/4]);
	}
#ifdef DEBUG_MEN
	putback_check_node(list,(nodelist_node_t *)p);
#else
	// putback to first node
	nodelist_node_t *tmp = (nodelist_node_t *)p;
	tmp->next = *list;
	*list = tmp;
#endif
}




void
freelist_t::putback_nodes(int size,void *p)
{
	if(size==0  ||  p==NULL) {
		return;
	}
	nodelist_node_t ** list = NULL;
	if(size>64) {
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
			default:
				ERROR("freelist_t::gimme_node()","No list with size %i! (only up to 64 and %i, 1220, 1624)", size, message_node_size );
				trap();
		}
	}
	else {
		list = &(all_lists[(size+3)/4]);
	}
#ifdef DEBUG_MEN
//MESSAGE("freelist_t::putback_nodes()","at %p",p);
	// goto end of nodes to append
	nodelist_node_t *tmp1 = (nodelist_node_t *)p;
	while(tmp1->next!=NULL) {
		nodelist_node_t *tmp = tmp1->next;
		putback_check_node(list,tmp1);
		tmp1 = tmp;
	}
	putback_check_node(list,tmp1);
#else
	// goto end of nodes to append
	nodelist_node_t *tmp = (nodelist_node_t *)p;
	while(tmp->next!=NULL) {
		tmp = tmp->next;
	}
	// putback list to first node
	tmp->next = *list;
	*list = (nodelist_node_t *)p;
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
}
