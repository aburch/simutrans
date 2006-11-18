#ifndef freelist_t_h
#define freelist_t_h


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
	// put back a node and checks for consitency
	static void putback_check_node(nodelist_node_t ** list,nodelist_node_t *p);

public:
	static void *gimme_node(int size);
	static void putback_node(int size,void *p);
	static void putback_nodes(int size,void *p);

	// clears all list memories
	static void free_all_nodes();
};

#endif
