/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../obj_desc.h"
#include "root_reader.h"


void root_reader_t::register_obj(obj_desc_t *&data)
{
	delete data;
	data = NULL;
}


obj_desc_t* root_reader_t::read_node(FILE*, obj_node_info_t& info)
{
	return obj_reader_t::read_node<obj_desc_t>(info);
}
