/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_CROSSING_READER_H
#define DESCRIPTOR_READER_CROSSING_READER_H


#include "obj_reader.h"


class crossing_reader_t : public obj_reader_t {
	static crossing_reader_t the_instance;

	crossing_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;

public:
	static crossing_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_crossing; }
	char const* get_type_name() const OVERRIDE { return "crossing"; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};

#endif
