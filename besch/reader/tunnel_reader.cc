#include <stdio.h>

#include "../../dataobj/ribi.h"

#include "../tunnel_besch.h"
#include "tunnel_reader.h"

#include "../../bauer/tunnelbauer.h"


void tunnel_reader_t::register_obj(obj_besch_t *&data)
{
    tunnel_besch_t *besch = static_cast<tunnel_besch_t *>(data);

    printf("tunnel_reader_t::register_obj(): Tunnel %s geladen\n", besch->gib_name());

    tunnelbauer_t::register_besch(besch);
}

bool tunnel_reader_t::successfully_loaded() const
{
    return tunnelbauer_t::laden_erfolgreich();
}
