/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DESCRIPTOR_READER_TEXT_READER_H
#define DESCRIPTOR_READER_TEXT_READER_H


#include "obj_reader.h"


class text_reader_t : public obj_reader_t
{
	OBJ_READER_DEF(text_reader_t, obj_text, "text");

public:
	/// @copydoc obj_reader_t::register_obj
	obj_desc_t *read_node(FILE *fp, obj_node_info_t &node) OVERRIDE;
};


#endif
