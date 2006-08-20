#include <stdlib.h>

#include "../tpl/debug_helper.h"

#include "../simmem.h"

#include "freelist.h"


/* this module keeps account of the free nodes of list and recycles them.
 * nodes of the same size will be kept in the same list
 * to be more efficient, all nodes with sizes smaller than 16 will be used at size 16 (one cacheline)
 */


static nodelist_node_t *node276 = NULL;
static nodelist_node_t *node1220 = NULL;
static nodelist_node_t *node1624 = NULL;

// list for nodes size 8...64
static nodelist_node_t *all_lists[1+64/4] = {
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
	if(size>64) {
		switch(size) {
			case 276:
				list = &node276;
				break;
			case 1220:
				list = &node1220;
				break;
			case 1624:
				list = &node1624;
				break;
			default:
				ERROR("freelist_t::gimme_node()","No list with size %i!", size );
				abort();
		}
	}
	else {
		list = &(all_lists[(size+3)/4]);
	}
	// need new memory?
	if(*list==NULL) {
		int num_elements = 32768/size;
		char *p = (char *)guarded_malloc( num_elements*size );
		while( num_elements-->0 ) {
			nodelist_node_t *tmp = (nodelist_node_t *)(p+num_elements*size);
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



void
freelist_t::putback_node(int size,void *p)
{
	nodelist_node_t ** list = NULL;
	if(size>64) {
		switch(size) {
			case 276:
				list = &node276;
				break;
			case 1220:
				list = &node1220;
				break;
			case 1624:
				list = &node1624;
				break;
			default:
				ERROR("freelist_t::gimme_node()","No list with size %i!", size );
				abort();
		}
	}
	else {
		list = &(all_lists[(size+3)/4]);
	}
	// putback to first node
	nodelist_node_t *tmp = (nodelist_node_t *)p;
	tmp->next = *list;
	*list = tmp;
}




void
freelist_t::putback_nodes(int size,void *p)
{
	nodelist_node_t ** list = NULL;
	if(size>64) {
		switch(size) {
			case 276:
				list = &node276;
				break;
			case 1220:
				list = &node1220;
				break;
			case 1624:
				list = &node1624;
				break;
			default:
				ERROR("freelist_t::gimme_node()","No list with size %i!", size );
				abort();
		}
	}
	else {
		list = &(all_lists[(size+3)/4]);
	}
	// goto end of nodes to append
	nodelist_node_t *tmp = (nodelist_node_t *)p;
	while(tmp->next!=NULL) {
		tmp = tmp->next;
	}
	// putback list to first node
	tmp->next = *list;
	*list = (nodelist_node_t *)p;
}
