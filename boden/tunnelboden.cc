
#include "tunnelboden.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../mm/mempool.h"
#include "../dataobj/loadsave.h"
#include "../besch/grund_besch.h"


mempool_t * tunnelboden_t::mempool = new mempool_t(sizeof(tunnelboden_t) );


tunnelboden_t::tunnelboden_t(karte_t *welt, loadsave_t *file) : boden_t(welt, koord3d(0,0,0))
{
    rdwr(file);
    set_flag(grund_t::is_tunnel);
}


tunnelboden_t::tunnelboden_t(karte_t *welt, koord3d pos, hang_t::typ hang_typ) : boden_t(welt, pos)
{
    this->hang_typ = hang_typ;
    set_flag(grund_t::is_tunnel);
}


void tunnelboden_t::calc_bild()
{
    setze_weg_bild(-1);
    if(ist_karten_boden()) {
      // setze_bild(grund_besch_t::boden->gib_bild(gib_grund_hang()));

      boden_t::calc_bild();
      setze_weg_bild(-1);

    } else {
        setze_bild(-1);
    }
}

void
tunnelboden_t::rdwr(loadsave_t *file)
{
    grund_t::rdwr(file);

    int int_hang = hang_typ;

    file->rdwr_int(int_hang, "\n");

    hang_typ = int_hang;
}


void * tunnelboden_t::operator new(size_t /*s*/)
{
    return mempool->alloc();
}


void tunnelboden_t::operator delete(void *p)
{
    mempool->free( p );
}
