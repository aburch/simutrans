/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_FREELIST_H
#define DATAOBJ_FREELIST_H


#include <cstddef>


/**
 * Helper class to organize small memory objects i.e. nodes for linked lists
 * and such.
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
