#ifndef __CROSSING_READER_H
#define __CROSSING_READER_H

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
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;
};

#endif
