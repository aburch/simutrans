/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_GROUNDOBJ_READER_H
#define DESCRIPTOR_READER_GROUNDOBJ_READER_H


#include "obj_reader.h"


class groundobj_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(groundobj_reader_t, obj_groundobj, "groundobj");

protected:
	/// @copydoc obj_reader_t::register_obj
	void register_obj(obj_desc_t*&) OVERRIDE;

	/// @copydoc obj_reader_t::successfully_loaded
	bool successfully_loaded() const OVERRIDE;

public:
	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


#endif
