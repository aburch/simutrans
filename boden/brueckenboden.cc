
#include "../simdebug.h"

#include "../gui/karte.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simtools.h"
#include "../simhalt.h"

#include "../besch/grund_besch.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/freelist.h"

#include "../gui/ground_info.h"

#include "brueckenboden.h"

brueckenboden_t::brueckenboden_t(karte_t *welt, loadsave_t *file) : grund_t(welt)
{
	rdwr(file);
	set_flag(grund_t::is_bridge);
}


brueckenboden_t::brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang) : grund_t(welt, pos)
{
	slope = grund_hang;
	this->weg_hang = weg_hang;
	set_flag(grund_t::is_bridge);
}


void brueckenboden_t::calc_bild()
{
	reliefkarte_t::gib_karte()->recalc_relief_farbe(gib_pos().gib_2d());
	if(ist_karten_boden()) {
		if(gib_hoehe()==welt->gib_grundwasser()) {
			setze_bild(grund_besch_t::ufer->gib_bild(slope));
		}
		else {
			setze_bild(grund_besch_t::boden->gib_bild(slope));
		}
		grund_t::calc_back_bild(gib_hoehe()/16,slope);
	}
	else {
		setze_bild(IMG_LEER);
		clear_back_bild();
	}
	if(gib_weg_nr(0)) {
		gib_weg_nr(0)->setze_bild(IMG_LEER);
	}
}


void
brueckenboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);
	if(file->get_version() <= 84004) {
		short v;
		v = slope;
		file->rdwr_short(v, " ");
		slope = v;

		v= weg_hang;
		file->rdwr_short(v, "\n");
		weg_hang = v;

	}
	else {
		if(file->get_version()<=88009) {
			file->rdwr_byte(slope, " ");
		}
		file->rdwr_byte(weg_hang, "\n");
	}
}


int brueckenboden_t::gib_weg_yoff() const
{
	if(ist_karten_boden() && weg_hang == 0) {
		return height_scaling(16);
	}
	else {
		return 0;
	}
}


bool
brueckenboden_t::zeige_info()
{
	if(gib_halt().is_bound()) {
		gib_halt()->zeige_info();
		return true;
	}
	else {
		if(hat_wege()) {	// if this is true, then all land info is shown
			// there is some info!
			if(!grund_infos->get(this)) {
				grund_infos->put(this, new grund_info_t(welt, this));
			}
			create_win(-1, -1, grund_infos->get(this), w_autodelete);
			return true;
		}
	}
	return false;
}


void * brueckenboden_t::operator new(size_t /*s*/)
{
	return (brueckenboden_t *)freelist_t::gimme_node(sizeof(brueckenboden_t));
}


void brueckenboden_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(brueckenboden_t),p);
}
