#ifndef __WAY_READER_H
#define __WAY_READER_H

#include "obj_reader.h"


class way_reader_t : public obj_reader_t {
	static way_reader_t the_instance;

	way_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;

	bool successfully_loaded() const OVERRIDE;
public:
	static way_reader_t*instance() { return &the_instance; }

	/**
	 * Read a way info node. Does version check and
	 * compatibility transformations.
	 * @author Hj. Malthaner
	 */
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_way; }
	char const* get_type_name() const OVERRIDE { return "way"; }
};

#endif
