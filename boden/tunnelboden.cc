
#include "tunnelboden.h"

#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"

#include "../besch/grund_besch.h"



tunnelboden_t::tunnelboden_t(karte_t *welt, loadsave_t *file) : boden_t(welt, koord3d(0,0,0))
{
    rdwr(file);
    set_flag(grund_t::is_tunnel);
}


tunnelboden_t::tunnelboden_t(karte_t *welt, koord3d pos, hang_t::typ hang_typ) : boden_t(welt, pos)
{
    this->hang_typ = hang_typ;
    set_flag(grund_t::is_tunnel);
    set_flag(grund_t::is_in_tunnel);
}


void tunnelboden_t::calc_bild()
{
    setze_weg_bild(-1);
    if(ist_karten_boden()) {
      // setze_bild(grund_besch_t::boden->gib_bild(gib_grund_hang()));

      boden_t::calc_bild();
      setze_weg_bild(-1);
      set_flag(grund_t::is_tunnel);
      clear_flag(grund_t::is_in_tunnel);

    } else {
        setze_bild(-1);
        set_flag(grund_t::is_in_tunnel);
    }
}

void
tunnelboden_t::rdwr(loadsave_t *file)
{
    grund_t::rdwr(file);

    int int_hang = hang_typ;

    file->rdwr_long(int_hang, "\n");

    hang_typ = int_hang;
}



void * tunnelboden_t::operator new(size_t /*s*/)
{
	return (tunnelboden_t *)freelist_t::gimme_node(sizeof(tunnelboden_t));
}


void tunnelboden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(tunnelboden_t),p);
}
