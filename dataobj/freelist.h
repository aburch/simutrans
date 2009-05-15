#ifndef freelist_t_h
#define freelist_t_h

/**
 * Helper class to organize small memory objects i.e. nodes for linked lists
 * and such.
 *
 * @author Hanjsjörg Malthaner
 */
class freelist_t
{
public:
	static void *gimme_node( size_t size );
	static void putback_node( size_t size, void *p );

	// clears all list memories
	static void free_all_nodes();
};

#endif
