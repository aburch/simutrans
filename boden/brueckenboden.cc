#include "../simdebug.h"

#include "../dings/bruecke.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simwin.h"
#include "../simhalt.h"

#include "../bauer/brueckenbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/bruecke_besch.h"

#include "../dataobj/loadsave.h"

#include "../gui/ground_info.h"
#include "../tpl/ptrhashtable_tpl.h"

#include "brueckenboden.h"


brueckenboden_t::brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang) : grund_t(welt, pos)
{
	slope = grund_hang;
	this->weg_hang = weg_hang;
}


void brueckenboden_t::calc_bild_internal()
{
	if(ist_tunnel()) {
		clear_back_bild();
		setze_bild(IMG_LEER);
	}
	else {
		if(ist_karten_boden()) {
			setze_bild( grund_besch_t::gib_ground_tile(slope,gib_pos().z) );
			grund_t::calc_back_bild(gib_pos().z/Z_TILE_STEP,slope);
			set_flag(draw_as_ding);
			if(  (gib_grund_hang()==hang_t::west  &&  abs(back_bild_nr)>11)  ||  (gib_grund_hang()==hang_t::nord  &&  gib_back_bild(0)!=IMG_LEER)  ) {
				// must draw as ding, since there is a slop here nearby
				koord pos = gib_pos().gib_2d()+koord(gib_grund_hang());
				grund_t *gr = welt->lookup_kartenboden(pos);
				gr->set_flag(grund_t::draw_as_ding);
			}
		}
		else {
			clear_back_bild();
			setze_bild( IMG_LEER );
		}
	}
}


void
brueckenboden_t::rdwr(loadsave_t *file)
{
	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		uint8 sl;
		file->rdwr_byte( sl, " " );
		slope = sl;
	}
	file->rdwr_byte(weg_hang, "\n");

	if(!find<bruecke_t>()) {
		dbg->error( "brueckenboden_t::rdwr()","no bridge on bridgebround at (%s); try repalcement", pos.gib_str()  );
		weg_t *w = gib_weg_nr(0);
		const bruecke_besch_t *br_besch = brueckenbauer_t::find_bridge( w->gib_waytype(), w->gib_max_speed(), 0 );
		bruecke_t *br = new bruecke_t(
			welt,
			gib_pos(),
			welt->gib_spieler(1),
			br_besch,
			ist_karten_boden() ?
				(slope==hang_t::flach ?
					br_besch->gib_rampe(ribi_typ(gib_weg_hang())) :
					br_besch->gib_start(ribi_typ(gib_grund_hang()))) :
				br_besch->gib_simple(w->gib_ribi_unmasked())
		);
		obj_add( br );
	}
}


void brueckenboden_t::rotate90()
{
	weg_hang = hang_t::rotate90( weg_hang );
	grund_t::rotate90();
}


sint8 brueckenboden_t::gib_weg_yoff() const
{
	if(ist_karten_boden() && weg_hang == 0) {
		return TILE_HEIGHT_STEP;
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
		create_win(new grund_info_t(this), w_info, (long)this);
		return true;
	}
	return false;
}
