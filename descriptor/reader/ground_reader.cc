#include <stdio.h>

#include "../ground_desc.h"
#include "ground_reader.h"


void ground_reader_t::register_obj(obj_desc_t *&data)
{
    ground_besch_t *desc = static_cast<ground_besch_t *>(data);

    ground_besch_t::register_desc(desc);
}


bool ground_reader_t::successfully_loaded() const
{
    return ground_besch_t::successfully_loaded();
}


obj_desc_t* ground_reader_t::read_node(FILE*, obj_node_info_t& info)
{
	return obj_reader_t::read_node<ground_besch_t>(info);
}
