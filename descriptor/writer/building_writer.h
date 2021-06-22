/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_BUILDING_WRITER_H
#define DESCRIPTOR_WRITER_BUILDING_WRITER_H


#include <string>
#include "obj_writer.h"
#include "../objversion.h"

template<class T> class slist_tpl;


class tile_writer_t : public obj_writer_t
{
private:
	static tile_writer_t the_instance;

	tile_writer_t() { register_writer(false); }

public:
	static tile_writer_t *instance() { return &the_instance; }

	virtual void write_obj(FILE* fp, obj_node_t &parent, int index, int seasons,
		slist_tpl<slist_tpl<slist_tpl<std::string> > >& backkeys,
		slist_tpl<slist_tpl<slist_tpl<std::string> > >& frontkeys
	);

protected:
	obj_type get_type() const OVERRIDE { return obj_tile; }
	const char *get_type_name() const OVERRIDE { return "tile"; }
};


class building_writer_t : public obj_writer_t
{
private:
	static building_writer_t the_instance;

	building_writer_t() { register_writer(true); }

public:
	std::string get_node_name(FILE* fp) const OVERRIDE { return name_from_next_node(fp); }

	static building_writer_t* instance() { return &the_instance; }

	virtual void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_building; }
	const char* get_type_name() const OVERRIDE { return "building"; }
};


#endif
