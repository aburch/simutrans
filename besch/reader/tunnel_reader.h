#ifndef __TUNNEL_READER_H
#define __TUNNEL_READER_H

#include "obj_reader.h"

class tunnel_besch_t;

class tunnel_reader_t : public obj_reader_t {
	static tunnel_reader_t the_instance;

	tunnel_reader_t() { register_reader(); }

	static void convert_old_tunnel(tunnel_besch_t *besch);

protected:
	void register_obj(obj_besch_t*&) OVERRIDE;

public:
	static tunnel_reader_t*instance() { return &the_instance; }

	obj_besch_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_tunnel; }
	char const* get_type_name() const OVERRIDE { return "tunnel"; }
};

#endif
