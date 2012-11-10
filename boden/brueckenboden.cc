#include "../simdebug.h"
#include "../simworld.h"

#include "../dings/bruecke.h"
#include "../bauer/brueckenbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/bruecke_besch.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"

#include "brueckenboden.h"
#include "wege/weg.h"


brueckenboden_t::brueckenboden_t(karte_t *welt, koord3d pos, int grund_hang, int weg_hang) : grund_t(welt, pos)
{
	slope = grund_hang;
	this->weg_hang = weg_hang;
}


void brueckenboden_t::calc_bild_internal()
{
	if(!is_visible()) {
		if (ist_karten_boden()) {
			grund_t::calc_back_bild(get_disp_height(), 0);
		}
		else {
			clear_back_bild();
		}
		set_bild(IMG_LEER);
	}
	else {
		if(ist_karten_boden()) {
			set_bild( grund_besch_t::get_ground_tile(slope,get_pos().z) );
			grund_t::calc_back_bild(get_pos().z,slope);
			set_flag(draw_as_ding);
			if(  (get_grund_hang()==hang_t::west  &&  abs(back_bild_nr)>11)  ||  (get_grund_hang()==hang_t::nord  &&  get_back_bild(0)!=IMG_LEER)  ) {
				// must draw as ding, since there is a slop here nearby
				koord pos = get_pos().get_2d()+koord(get_grund_hang());
				grund_t *gr = welt->lookup_kartenboden(pos);
				gr->set_flag(grund_t::draw_as_ding);
			}
		}
		else {
			clear_back_bild();
			set_bild( IMG_LEER );
		}
	}
}


void brueckenboden_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "brueckenboden_t" );

	grund_t::rdwr(file);

	if(file->get_version()<88009) {
		uint8 sl;
		file->rdwr_byte(sl);
		slope = sl;
	}
	file->rdwr_byte(weg_hang);

	if(!find<bruecke_t>()) {
		dbg->error( "brueckenboden_t::rdwr()","no bridge on bridge ground at (%s); try replacement", pos.get_str() );
		weg_t *w = get_weg_nr(0);
		if(w) {
			const bruecke_besch_t *br_besch = brueckenbauer_t::find_bridge( w->get_waytype(), w->get_max_speed(), 0 );
			bruecke_t *br = new bruecke_t(
				welt,
				get_pos(),
				welt->get_spieler(1),
				br_besch,
				ist_karten_boden() ?
					(slope==hang_t::flach ?
						br_besch->get_rampe(ribi_typ(get_weg_hang())) :
						br_besch->get_start(ribi_typ(get_grund_hang()))) :
					br_besch->get_simple(w->get_ribi_unmasked())
			);
			obj_add( br );
		}
	}
}


void brueckenboden_t::rotate90()
{
	weg_hang = hang_t::rotate90( weg_hang );
	grund_t::rotate90();
}


sint8 brueckenboden_t::get_weg_yoff() const
{
	if(ist_karten_boden() && weg_hang == 0) {
		return TILE_HEIGHT_STEP;
	}
	else {
		return 0;
	}
}

void brueckenboden_t::info(cbuffer_t & buf) const
{
	const bruecke_t *bridge = find<bruecke_t>();
	if(bridge  &&  bridge->get_besch()) {
		const bruecke_besch_t *besch = bridge->get_besch();
		buf.append(translator::translate(besch->get_name()));
		buf.append("\n");
	}
	grund_t::info(buf);
}
