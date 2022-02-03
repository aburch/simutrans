/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simdebug.h"
#include "../world/simworld.h"

#include "../obj/bruecke.h"
#include "../builder/brueckenbauer.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/bridge_desc.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#include "../utils/cbuffer.h"

#include "brueckenboden.h"
#include "../obj/way/weg.h"

#include "../vehicle/vehicle_base.h"


brueckenboden_t::brueckenboden_t(koord3d pos, slope_t::type grund_hang, slope_t::type weg_hang) :
	grund_t(pos),
	weg_hang(weg_hang)
{
	slope = grund_hang;
}


void brueckenboden_t::calc_image_internal(const bool calc_only_snowline_change)
{
	if(  ist_karten_boden()  ) {

		set_image( ground_desc_t::get_ground_tile(this) );

		if(  !calc_only_snowline_change  ) {
			grund_t::calc_back_image( get_pos().z, slope );
			set_flag( draw_as_obj );
			if(  (get_grund_hang() == slope_t::west  &&  abs(back_imageid) > 11)  ||  (get_grund_hang() == slope_t::north  &&  get_back_image(0) != IMG_EMPTY)  ) {
				// must draw as obj, since there is a slop here nearby
				koord pos = get_pos().get_2d() + koord( get_grund_hang() );
				grund_t *gr = welt->lookup_kartenboden( pos );
				gr->set_flag( grund_t::draw_as_obj );
			}
		}
	}
	else {
		clear_back_image();
		set_image(IMG_EMPTY);
	}

}


void brueckenboden_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "brueckenboden_t" );

	grund_t::rdwr(file);

	if(file->is_version_less(88, 9)) {
		uint8 sl;
		file->rdwr_byte(sl);
		slope = sl;
	}
	if(  file->is_saving()  &&  file->is_version_less(112, 7)  ) {
		// truncate double weg_hang to single weg_hang, better than nothing
		uint8 sl = min( corner_sw(weg_hang), 1 ) + min( corner_se(weg_hang), 1 ) * 2 + min( corner_ne(weg_hang), 1 ) * 4 + min( corner_nw(weg_hang), 1 ) * 8;
		file->rdwr_byte(sl);
	}
	else {
		file->rdwr_byte(weg_hang);
	}

	if(  file->is_loading()  &&  file->is_version_less(112, 7)  ) {
		// convert slopes from old single height saved game
		weg_hang = (scorner_sw(weg_hang) + scorner_se(weg_hang) * 3 + scorner_ne(weg_hang) * 9 + scorner_nw(weg_hang) * 27) * env_t::pak_height_conversion_factor;
	}

	if(!find<bruecke_t>()) {
		dbg->error( "brueckenboden_t::rdwr()", "No bridge on bridge ground at (%s); try replacement", pos.get_str() );
		weg_t *w = get_weg_nr(0);

		if(w) {
			const bridge_desc_t *br_desc = bridge_builder_t::find_bridge( w->get_waytype(), w->get_max_speed(), 0 );
			if (!br_desc) {
				dbg->fatal("brueckenboden_t::rdwr", "No suitable bridge found for bridge ground at (%s)!", pos.get_str());
			}

			const grund_t *kb = welt->lookup_kartenboden(get_pos().get_2d());
			int height = 1;

			if(  kb && get_pos().z - kb->get_pos().z > 1 ) {
				height = 2;
			}

			bruecke_t *br = new bruecke_t( get_pos(), welt->get_public_player(), br_desc, ist_karten_boden() ? br_desc->get_end( slope, get_grund_hang(), get_weg_hang() ) : br_desc->get_straight( w->get_ribi_unmasked(), height ) );
			obj_add( br );
		}
	}
}


void brueckenboden_t::rotate90()
{
	if( sint8 way_offset = get_weg_yoff() ) {
		pos.rotate90( welt->get_size().y-1 );
		slope = slope_t::rotate90( slope );
		// since the y_off contains also the way height, we need to remove it before rotations and add it back afterwards
		for( uint8 i = 0; i < objlist.get_top(); i++ ) {
			obj_t * obj = obj_bei( i );
			if(  !dynamic_cast<vehicle_base_t*>(obj)  ) {
				obj->set_yoff( obj->get_yoff() + way_offset );
				obj->rotate90();
				obj->set_yoff( obj->get_yoff() - way_offset );
			}
			else {
				// vehicle corrects its offset themselves
				obj->rotate90();
			}
		}
	}
	else {
		weg_hang = slope_t::rotate90( weg_hang );
		grund_t::rotate90();
	}
}


sint8 brueckenboden_t::get_weg_yoff() const
{
	if(  ist_karten_boden()  &&  weg_hang == 0  ) {
		// we want to find maximum height of slope corner shortcut as we know this is n, s, e or w and single heights are not integer multiples of 8
		return TILE_HEIGHT_STEP * slope_t::max_diff(slope);
	}
	else {
		return 0;
	}
}

void brueckenboden_t::info(cbuffer_t & buf) const
{
	const bruecke_t *bridge = find<bruecke_t>();
	if(bridge  &&  bridge->get_desc()) {
		const bridge_desc_t *desc = bridge->get_desc();
		buf.append(translator::translate(desc->get_name()));
		buf.append("\n");
	}
	grund_t::info(buf);
}
