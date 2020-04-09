/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_TUNNEL_READER_H
#define DESCRIPTOR_READER_TUNNEL_READER_H


#include "obj_reader.h"

class tunnel_desc_t;

class tunnel_reader_t : public obj_reader_t {
	static tunnel_reader_t the_instance;

	tunnel_reader_t() { register_reader(); }

	static void convert_old_tunnel(tunnel_desc_t *desc);

protected:
	void register_obj(obj_desc_t*&) OVERRIDE;

public:
	static tunnel_reader_t*instance() { return &the_instance; }

	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_tunnel; }
	char const* get_type_name() const OVERRIDE { return "tunnel"; }
};

#endif
