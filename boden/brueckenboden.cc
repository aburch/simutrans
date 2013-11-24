#include "../simdebug.h"
#include "../simworld.h"

#include "../obj/bruecke.h"
#include "../bauer/brueckenbauer.h"

#include "../besch/grund_besch.h"
#include "../besch/bruecke_besch.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"

#include "brueckenboden.h"
#include "wege/weg.h"


brueckenboden_t::brueckenboden_t(koord3d pos, int grund_hang, int weg_hang) : grund_t(pos)
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
			set_bild( grund_besch_t::get_ground_tile(this) );
			grund_t::calc_back_bild(get_pos().z,slope);
			set_flag(draw_as_obj);
			if(  (get_grund_hang()==hang_t::west  &&  abs(back_bild_nr)>11)  ||  (get_grund_hang()==hang_t::nord  &&  get_back_bild(0)!=IMG_LEER)  ) {
				// must draw as obj, since there is a slop here nearby
				koord pos = get_pos().get_2d()+koord(get_grund_hang());
				grund_t *gr = welt->lookup_kartenboden(pos);
				gr->set_flag(grund_t::draw_as_obj);
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
	if(  file->is_saving()  &&  file->get_version() < 112007  ) {
		// truncate double weg_hang to single weg_hang, better than nothing
		uint8 sl = min( corner1(weg_hang), 1 ) + min( corner2(weg_hang), 1 ) * 2 + min( corner3(weg_hang), 1 ) * 4 + min( corner4(weg_hang), 1 ) * 8;
		file->rdwr_byte(sl);
	}
	else {
		file->rdwr_byte(weg_hang);
	}

	if(  file->is_loading()  &&  file->get_version() < 112007  ) {
		// convert slopes from old single height saved game
		weg_hang = (scorner1(weg_hang) + scorner2(weg_hang) * 3 + scorner3(weg_hang) * 9 + scorner4(weg_hang) * 27) * env_t::pak_height_conversion_factor;
	}

	if(!find<bruecke_t>()) {
		dbg->error( "brueckenboden_t::rdwr()","no bridge on bridge ground at (%s); try replacement", pos.get_str() );
		weg_t *w = get_weg_nr(0);
		if(w) {
			const bruecke_besch_t *br_besch = brueckenbauer_t::find_bridge( w->get_waytype(), w->get_max_speed(), 0 );
			const grund_t *kb = welt->lookup_kartenboden(get_pos().get_2d());
			int height = 1;
			if(  kb && get_pos().z - kb->get_pos().z > 1 ) {
				height = 2;
			}
			bruecke_t *br = new bruecke_t( get_pos(), welt->get_spieler(1), br_besch, ist_karten_boden() ? br_besch->get_end( slope, get_grund_hang(), get_weg_hang() ) : br_besch->get_simple( w->get_ribi_unmasked(), height ) );
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
	if(  ist_karten_boden()  &&  weg_hang == 0  ) {
		// we want to find maximum height of slope corner shortcut as we know this is n, s, e or w and single heights are not integer multiples of 8
		return TILE_HEIGHT_STEP * ((slope & 7) ? 1 : 2);
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
