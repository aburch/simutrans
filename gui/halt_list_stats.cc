/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 * Written (w) 2001 Markus Weber
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "halt_list_stats.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simplay.h"
#include "../simevent.h"
#include "../simworld.h"
#include "../simimg.h"

#include "../dataobj/translator.h"

#include "../besch/skin_besch.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "components/list_button.h"

/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void halt_list_stats_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTRELEASE(ev)) {
		if (event_get_last_control_shift() != 2) {
			halt->zeige_info();
		}
	}
	if(IS_RIGHTRELEASE(ev)) {
		halt->get_welt()->setze_ij_off(halt->gib_basis_pos3d());
	}
}


/**
 * Zeichnet die Komponente
 * @author Markus Weber
 */
void halt_list_stats_t::zeichnen(koord offset)
{
	int halttype;       // 01-June-02    Markus Weber    Added

	clip_dimension clip = display_gib_clip_wh();

	if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) && halt.is_bound()) {

		static cbuffer_t buf(8192);  // Hajo: changed from 128 to 8192 because get_short_freight_info requires that a large buffer

		// status now in front
		display_fillbox_wh_clip(pos.x+offset.x+2, pos.y+offset.y+6, 26, INDICATOR_HEIGHT, halt->gib_status_farbe(), true);

		// name
		int left = pos.x + offset.x + 32 + display_proportional_clip(pos.x + offset.x + 32, pos.y + offset.y + 2, translator::translate(halt->gib_name()), ALIGN_LEFT, COL_BLACK, true);

		// what kind of stop
		halttype = halt->get_station_type();
		int pos_y = pos.y+offset.y-41;
		if (halttype & haltestelle_t::railstation) {
			display_color_img(skinverwaltung_t::zughaltsymbol->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::loadingbay) {
			display_color_img(skinverwaltung_t::autohaltsymbol->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::busstop) {
			display_color_img(skinverwaltung_t::bushaltsymbol->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::dock) {
			display_color_img(skinverwaltung_t::schiffshaltsymbol->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::airstop) {
			display_color_img(skinverwaltung_t::airhaltsymbol->gib_bild_nr(0), pos.x+left, pos_y, 0, false, true);
			left += 23;
		}
		if (halttype & haltestelle_t::monorailstop) {
			display_color_img(skinverwaltung_t::monorailhaltsymbol->gib_bild_nr(0), pos.x+left, pos_y, 0, false, true);
		}

		// now what do we accept here?
		pos_y = pos.y+offset.y+14;
		left = pos.x+offset.x;
		if (halt->get_pax_enabled()) {
			display_color_img(skinverwaltung_t::passagiere->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 10;
		}
		if (halt->get_post_enabled()) {
			display_color_img(skinverwaltung_t::post->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 10;
		}
		if (halt->get_ware_enabled()) {
			display_color_img(skinverwaltung_t::waren->gib_bild_nr(0), left, pos_y, 0, false, true);
			left += 10;
		}

		buf.clear();
		halt->get_short_freight_info(buf);
		display_proportional_clip(pos.x+offset.x+32, pos_y, buf, ALIGN_LEFT, COL_BLACK, true);
	}
}
