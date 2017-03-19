#ifndef __BUILDING_READER_H
#define __BUILDING_READER_H

#include "obj_reader.h"


class tile_reader_t : public obj_reader_t {
	static tile_reader_t the_instance;

	tile_reader_t() { register_reader(); }
public:
	static tile_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_tile; }
	char const* get_type_name() const OVERRIDE { return "tile"; }

	/* Read a node. Does version check and compatibility transformations.
	 * @author Hj. Malthaner
	 */
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;
};


class building_reader_t : public obj_reader_t {
	static building_reader_t the_instance;

	building_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;

public:
	static building_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_building; }
	char const* get_type_name() const OVERRIDE { return "building"; }

	/* Read a node. Does version check and compatibility transformations.
	 * @author Hj. Malthaner
	 */
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;

};

#endif
