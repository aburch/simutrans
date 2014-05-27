#include "root_reader.h"


void root_reader_t::register_obj(obj_besch_t *&data)
{
    delete data;
    data = NULL;
}
