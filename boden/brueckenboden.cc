
#include "../simdebug.h"
#include "brueckenboden.h"

#include "../gui/karte.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../besch/grund_besch.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"


brueckenboden_t::brueckenboden_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
    rdwr(file);
    set_flag(grund_t::is_bridge);
}


brueckenboden_t::brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang) : grund_t(welt, pos)
{
    this->grund_hang = grund_hang;
    this->weg_hang = weg_hang;
    set_flag(grund_t::is_bridge);
}


void brueckenboden_t::calc_bild()
{
    if(ist_karten_boden()) {
        reliefkarte_t::gib_karte()->recalc_relief_farbe(gib_pos().gib_2d());
	setze_weg_bild(-1);
	if(gib_hoehe() == welt->gib_grundwasser()) {
	    setze_bild(grund_besch_t::ufer->gib_bild(gib_grund_hang()));
	}
	else {
	    setze_bild(grund_besch_t::boden->gib_bild(gib_grund_hang()));
	}
    } else {
	setze_weg_bild(IMG_LEER);
	setze_bild(IMG_LEER);
    }
}


void
brueckenboden_t::rdwr(loadsave_t *file)
{
    grund_t::rdwr(file);

    if(file->get_version() <= 84004) {
      short v;
      v = grund_hang;
      file->rdwr_short(v, " ");
      grund_hang = v;

      v= weg_hang;
      file->rdwr_short(v, "\n");
      weg_hang = v;

    } else {
      file->rdwr_byte(grund_hang, " ");
      file->rdwr_byte(weg_hang, "\n");
    }
}


int brueckenboden_t::gib_weg_yoff() const
{
    if(ist_karten_boden() && weg_hang == 0) {
	return 16;
    } else {
	return 0;
    }
}


void * brueckenboden_t::operator new(size_t /*s*/)
{
	return (brueckenboden_t *)freelist_t::gimme_node(sizeof(brueckenboden_t));
}


void brueckenboden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(brueckenboden_t),p);
}
