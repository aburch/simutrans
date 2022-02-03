/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_ROOT_READER_H
#define DESCRIPTOR_READER_ROOT_READER_H


#include "obj_reader.h"


class root_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(root_reader_t, obj_root, "root");

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

protected:
	/// @copydoc obj_reader_t::register_obj
	void register_obj(obj_desc_t *&desc) OVERRIDE;
};


#endif
