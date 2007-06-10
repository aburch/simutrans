/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 *  This file is part of the Simutrans project and may not be used in other
 *  projects without written permission of the authors.
 */
#ifndef PEDESTRIAN_WRITER_H
#define PEDESTRIAN_WRITER_H

#include "obj_writer.h"
#include "../objversion.h"


/*
 *  Autor:
 *      Volker Meyer
 *
 *  Beschreibung:
 *      Liest Beschreibunnen der automatisch generierten Fussgänger
 */
class pedestrian_writer_t : public obj_writer_t {
	private:
		static pedestrian_writer_t the_instance;

		pedestrian_writer_t() { register_writer(true); }
		protected:
		virtual cstring_t get_node_name(FILE* fp) const { return name_from_next_node(fp); }

	public:
		virtual void write_obj(FILE* fp, obj_node_t& parent, tabfileobj_t& obj);

		virtual obj_type get_type() const { return obj_pedestrian; }
		virtual const char* get_type_name() const { return "pedestrian"; }
};

#endif
