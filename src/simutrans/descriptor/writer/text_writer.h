/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_WRITER_TEXT_WRITER_H
#define DESCRIPTOR_WRITER_TEXT_WRITER_H


#include <stdio.h>
#include "obj_writer.h"
#include "../objversion.h"


class obj_node_t;


class text_writer_t : public obj_writer_t
{
private:
	static text_writer_t the_instance;

	text_writer_t() { register_writer(false); }

public:
	static text_writer_t* instance() { return &the_instance; }

	obj_type get_type() const OVERRIDE { return obj_text; }
	const char* get_type_name() const OVERRIDE { return "text"; }

	void dump_node(FILE* infp, const obj_node_info_t& node) OVERRIDE;
	void write_obj(FILE* fp, obj_node_t& parent, const char* text);
};


#endif
