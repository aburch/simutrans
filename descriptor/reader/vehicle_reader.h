#ifndef __VEHICLE_READER_H
#define __VEHICLE_READER_H

#include "obj_reader.h"


class vehicle_reader_t : public obj_reader_t {
	static vehicle_reader_t the_instance;

	vehicle_reader_t() { register_reader(); }

protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;

public:
	static vehicle_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_vehicle; }
	char const* get_type_name() const OVERRIDE { return "vehicle"; }

	/* Read a node. Does version check and compatibility transformations.
	 * @author Hj. Malthaner
	 */
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;
};

#endif
