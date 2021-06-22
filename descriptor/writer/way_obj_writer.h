/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_WAY_OBJ_WRITER_H
#define DESCRIPTOR_WRITER_WAY_OBJ_WRITER_H


#include <string>
#include "obj_writer.h"
#include "../objversion.h"


class way_obj_writer_t : public obj_writer_t
{
private:
	static way_obj_writer_t the_instance;

	way_obj_writer_t() { register_writer(true); }

protected:
	std::string get_node_name(FILE *fp) const OVERRIDE { return name_from_next_node(fp); }

public:
	/// Write a way-object description node
	void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_way_obj; }

	const char *get_type_name() const OVERRIDE { return "way-object"; }
};


#endif
