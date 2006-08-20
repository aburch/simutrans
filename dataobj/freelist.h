#ifndef freelist_t_h
#define freelist_t_h

#include "../tpl/array_tpl.h"


typedef struct ns{
	struct ns *next;
} nodelist_node_t;

/**
 * Helper class to organize small memory objects i.e. nodes for linked lists
 * and such.
 *
 * @author Hanjsjörg Malthaner
 */
class freelist_t
{
private:
/*
	static nodelist_node_t *node8 = NULL;
	static nodelist_node_t *node12 = NULL;
	static nodelist_node_t *node24 = NULL;
	static nodelist_node_t *node1624 = NULL;
	inline nodelist_node_t **gimme_list(int size);
*/

public:
	static void *gimme_node(int size);
	static void putback_node(int size,void *p);
	static void putback_nodes(int size,void *p);
};

#endif // node16_h
