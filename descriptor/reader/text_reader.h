/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_TEXT_READER_H
#define DESCRIPTOR_READER_TEXT_READER_H


#include "obj_reader.h"

class text_reader_t : public obj_reader_t {
	static text_reader_t the_instance;

	text_reader_t() { register_reader(); }
public:
	static text_reader_t*instance() { return &the_instance; }

	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;

	obj_type get_type() const OVERRIDE { return obj_text; }
	char const* get_type_name() const OVERRIDE { return "text"; }
};

#endif
