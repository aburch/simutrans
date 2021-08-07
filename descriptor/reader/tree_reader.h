/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_TREE_READER_H
#define DESCRIPTOR_READER_TREE_READER_H


#include "obj_reader.h"


class tree_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(tree_reader_t, obj_tree, "tree");

protected:
	bool successfully_loaded() const OVERRIDE;
	void register_obj(obj_desc_t*&) OVERRIDE;

public:
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


#endif
