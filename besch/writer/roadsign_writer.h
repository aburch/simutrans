#ifndef ROADSIGN_WRITER_H
#define ROADSIGN_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"


class roadsign_writer_t : public obj_writer_t {
	private:
		static roadsign_writer_t the_instance;

		roadsign_writer_t() { register_writer(true); }

	protected:
		virtual cstring_t get_node_name(FILE* fp) const { return name_from_next_node(fp); }

	public:
		virtual void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj);

		virtual obj_type get_type() const { return obj_roadsign; }
		virtual const char* get_type_name() const { return "roadsign"; }
};

#endif
