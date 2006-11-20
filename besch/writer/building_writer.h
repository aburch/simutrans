#ifndef BUILDING_WRITER_H
#define BUILDING_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"

template<class T> class slist_tpl;


class tile_writer_t : public obj_writer_t {
	private:
		static tile_writer_t the_instance;

		tile_writer_t() { register_writer(false); }

	public:
		static tile_writer_t *instance() { return &the_instance; }

		virtual void write_obj(FILE* fp, obj_node_t &parent, int index, int seasons,
			slist_tpl<slist_tpl<slist_tpl<cstring_t> > >& backkeys,
			slist_tpl<slist_tpl<slist_tpl<cstring_t> > >& frontkeys
		);

	protected:
		virtual obj_type get_type() const { return obj_tile; }
		virtual const char *get_type_name() const { return "tile"; }
};


class building_writer_t : public obj_writer_t {
	private:
		static building_writer_t the_instance;

		building_writer_t() { register_writer(true); }

	public:
		virtual cstring_t get_node_name(FILE* fp) const { return name_from_next_node(fp); }

		static building_writer_t* instance() { return &the_instance; }

		virtual void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj);

		virtual obj_type get_type() const { return obj_building; }
		virtual const char* get_type_name() const { return "building"; }
};

#endif
