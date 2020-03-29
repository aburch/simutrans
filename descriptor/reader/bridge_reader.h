/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_BRIDGE_READER_H
#define DESCRIPTOR_READER_BRIDGE_READER_H


#include "obj_reader.h"

class bridge_reader_t : public obj_reader_t {
	static bridge_reader_t the_instance;

	bridge_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;

public:
	static bridge_reader_t*instance() { return &the_instance; }

	/**
	 * Read a bridge info node. Does version check and
	 * compatibility transformations.
	 */
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_bridge; }
	char const* get_type_name() const OVERRIDE { return "bridge"; }
};

#endif
