#include <stdio.h>

#include "../grund_besch.h"
#include "ground_reader.h"


void ground_reader_t::register_obj(obj_desc_t *&data)
{
    ground_desc_t *desc = static_cast<ground_desc_t *>(data);

    ground_desc_t::register_desc(desc);
}


bool ground_reader_t::successfully_loaded() const
{
    return ground_desc_t::alles_geladen();
}


obj_desc_t* ground_reader_t::read_node(FILE*, obj_node_info_t& info)
{
	return obj_reader_t::read_node<ground_desc_t>(info);
}
