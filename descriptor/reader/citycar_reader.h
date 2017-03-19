/*
 *  Copyright (c) 1997 - 2002 by Volker Meyer & Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 */

#ifndef __CITYCAR_READER_H
#define __CITYCAR_READER_H

#include "obj_reader.h"


/*
 *  Autor:
 *      Volker Meyer
 */
class citycar_reader_t : public obj_reader_t {
	static citycar_reader_t the_instance;

	citycar_reader_t() { register_reader(); }
protected:
	void register_obj(obj_desc_t*&) OVERRIDE;
	bool successfully_loaded() const OVERRIDE;

public:
	static citycar_reader_t*instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_citycar; }
	char const* get_type_name() const OVERRIDE { return "citycar"; }
	obj_desc_t* read_node(FILE*, obj_node_info_t&) OVERRIDE;
};

#endif
