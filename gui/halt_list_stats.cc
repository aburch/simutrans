/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "halt_list_stats.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../player/simplay.h"
#include "../simworld.h"
#include "../display/simimg.h"

#include "../dataobj/translator.h"

#include "../besch/skin_besch.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "gui_frame.h"


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool halt_list_stats_t::infowin_event(const event_t *ev)
{
	if(halt.is_bound()) {
		if(IS_LEFTRELEASE(ev)) {
			if (event_get_last_control_shift() != 2) {
				halt->zeige_info();
			}
			return true;
		}
		if(IS_RIGHTRELEASE(ev)) {
			welt->get_viewport()->change_world_position(halt->get_basis_pos3d());
			return true;
		}
	}
	return false;
}


/**
 * Draw the component
 * @author Markus Weber
 */
void halt_list_stats_t::draw(scr_coord offset)
{
	clip_dimension clip = display_get_clip_wh();

	if(  !( (pos.y+offset.y)>clip.yy  ||  (pos.y+offset.y)<clip.y-32 )  &&  halt.is_bound()) {
		// status now in front
		display_fillbox_wh_clip(pos.x+offset.x+4, pos.y+offset.y+6, 26, D_INDICATOR_HEIGHT, halt->get_status_farbe(), true);

		// name
		int left = pos.x + offset.x + 32+2 + display_proportional_clip(pos.x + offset.x + 32+2, pos.y + offset.y + 2, translator::translate(halt->get_name()), ALIGN_LEFT, COL_BLACK, true);

		// what kind of stop
		haltestelle_t::stationtyp const halttype = halt->get_station_type();
		int pos_y = pos.y+offset.y-41;
		if (halttype & haltestelle_t::railstation) {
			display_color_img(skinverwaltung_t::zughaltsymbol->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::loadingbay) {
			display_color_img(skinverwaltung_t::autohaltsymbol->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::busstop) {
			display_color_img(skinverwaltung_t::bushaltsymbol->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::dock) {
			display_color_img(skinverwaltung_t::schiffshaltsymbol->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::airstop) {
			display_color_img(skinverwaltung_t::airhaltsymbol->get_bild_nr(0), pos.x+left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::monorailstop) {
			display_color_img(skinverwaltung_t::monorailhaltsymbol->get_bild_nr(0), pos.x+left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::tramstop) {
			display_color_img(skinverwaltung_t::tramhaltsymbol->get_bild_nr(0), pos.x+left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::maglevstop) {
			display_color_img(skinverwaltung_t::maglevhaltsymbol->get_bild_nr(0), pos.x+left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::narrowgaugestop) {
			display_color_img(skinverwaltung_t::narrowgaugehaltsymbol->get_bild_nr(0), pos.x+left, pos_y, 0, false, true);
			left += 23;
		}

		// now what do we accept here?
		pos_y = pos.y+offset.y+14;
		left = pos.x+offset.x+2;
		if (halt->get_pax_enabled()) {
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 10;
		}
		if (halt->get_post_enabled()) {
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 10;
		}
		if (halt->get_ware_enabled()) {
			display_color_img(skinverwaltung_t::waren->get_bild_nr(0), left, pos_y, 0, false, true);
			left += 10;
		}

		static cbuffer_t buf;
		buf.clear();
		halt->get_short_freight_info(buf);
		display_proportional_clip(pos.x+offset.x+32+2, pos_y, buf, ALIGN_LEFT, COL_BLACK, true);
	}
}
