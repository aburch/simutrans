#include <stdio.h>

#include "../../simdings.h"

#include "../grund_besch.h"
#include "ground_reader.h"


void ground_reader_t::register_obj(obj_besch_t *&data)
{
    grund_besch_t *besch = static_cast<grund_besch_t *>(data);

    grund_besch_t::register_besch(besch);
//    printf("...Grund %s geladen\n", besch->get_name());
}


bool ground_reader_t::successfully_loaded() const
{
    return grund_besch_t::alles_geladen();
}
