/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_ROOT_READER_H
#define DESCRIPTOR_READER_ROOT_READER_H


#include "obj_reader.h"

class root_reader_t : public obj_reader_t {
	static root_reader_t the_instance;

	root_reader_t() { register_reader(); }
public:
	static root_reader_t*instance() { return &the_instance; }

	/// @copydoc obj_reader_t::read_node
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_root; }
	char const* get_type_name() const OVERRIDE { return "root"; }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
};

#endif
