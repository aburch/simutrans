/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simunits.h"
#include "../simdebug.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../player/simplay.h"
#include "../simwerkz.h"
#include "../simworld.h"

#include "../besch/roadsign_besch.h"
#include "../besch/skin_besch.h"

#include "../boden/grund.h"
#include "../boden/wege/strasse.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/scenario.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../gui/trafficlight_info.h"
#include "../gui/privatesign_info.h"
#include "../gui/werkzeug_waehler.h"

#include "../tpl/stringhashtable_tpl.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "roadsign.h"


const roadsign_besch_t *roadsign_t::default_signal=NULL;

stringhashtable_tpl<const roadsign_besch_t *> roadsign_t::table;


roadsign_t::roadsign_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	bild = after_bild = IMG_LEER;
	rdwr(file);
	if(besch) {
		/* if more than one state, we will switch direction and phase for traffic lights
		 * however also gate signs need indications
		 */
		automatic = (besch->get_bild_anzahl()>4  &&  besch->get_wtyp()==road_wt)  ||  (besch->get_bild_anzahl()>2  &&  besch->is_private_way());
	}
	else {
		automatic = false;
	}
	// some sve had rather strange entries in zustand
	if(  !automatic  ||  besch==NULL  ) {
		zustand = 0;
	}
}


roadsign_t::roadsign_t(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch) : ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = dir;
	bild = after_bild = IMG_LEER;
	zustand = 0;
	ticks_ns = ticks_ow = 16;
	ticks_offset = 0;
	set_besitzer( sp );
	if(  besch->is_private_way()  ) {
		// init ownership of private ways
		ticks_ns = ticks_ow = 0;
		if(  sp->get_player_nr() >= 8  ) {
			ticks_ow = 1 << (sp->get_player_nr()-8);
		}
		else {
			ticks_ns = 1 << sp->get_player_nr();
		}
	}
	/* if more than one state, we will switch direction and phase for traffic lights
	 * however also gate signs need indications
	 */
	automatic = (besch->get_bild_anzahl()>4  &&  besch->get_wtyp()==road_wt)  ||  (besch->get_bild_anzahl()>2  &&  besch->is_private_way());
}


roadsign_t::~roadsign_t()
{
	if(  besch  &&  (besch->is_single_way()  ||  besch->is_signal_type())  ) {
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t *weg = gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
			if(weg) {
				// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
				weg->set_ribi_maske(ribi_t::keine);
			}
			else {
				dbg->error("roadsign_t::~roadsign_t()","roadsign_t %p was deleted but ground was not a road!");
			}
		}
	}
	if(automatic) {
		welt->sync_remove(this);
	}
}


void roadsign_t::set_dir(ribi_t::ribi dir)
{
	this->dir = dir;
	weg_t *weg = welt->lookup(get_pos())->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt);
	if(besch->get_wtyp()!=track_wt  &&   besch->get_wtyp()!=monorail_wt&&   besch->get_wtyp()!=maglev_wt&&   besch->get_wtyp()!=narrowgauge_wt) {
		weg->count_sign();
	}
	if(besch->is_single_way()  ||  besch->is_signal()  ||  besch->is_pre_signal()  ||  besch->is_longblock_signal()) {
		// set mask, if it is a single way ...
		weg->count_sign();
		if(ribi_t::ist_einfach(dir)) {
			weg->set_ribi_maske(dir);
		}
		else {
			weg->set_ribi_maske(ribi_t::keine);
		}
DBG_MESSAGE("roadsign_t::set_dir()","ribi %i",dir);
	}

	// force redraw
	mark_image_dirty(get_bild(),0);
	// some more magic to get left side images right ...
	sint8 old_x = get_xoff();
	set_xoff( after_xoffset );
	mark_image_dirty(after_bild,after_yoffset-get_yoff());
	set_xoff( old_x );

	bild = IMG_LEER;
	after_bild = IMG_LEER;
	calc_bild();
}


void roadsign_t::zeige_info()
{
	if(  besch->is_private_way()  ) {
		create_win(new privatesign_info_t(this), w_info, (ptrdiff_t)this );
	}
	else if(  automatic  ) {
		create_win(new trafficlight_info_t(this), w_info, (ptrdiff_t)this );
	}
	else {
		ding_t::zeige_info();
	}
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void roadsign_t::info(cbuffer_t & buf) const
{
	ding_t::info( buf );

	if(  besch->is_private_way()  ) {
		buf.append( "\n\n\n\n\n\n\n\n\n\n\n\n\n\n" );
	}
	else {
		buf.append(translator::translate("Roadsign"));
		buf.append("\n");
		if(besch->is_single_way()) {
			buf.append(translator::translate("\nsingle way"));
		}
		if(besch->get_min_speed()!=0) {
			buf.printf("%s%d", translator::translate("\nminimum speed:"), speed_to_kmh(besch->get_min_speed()));
		}
		buf.printf("%s%u\n", translator::translate("\ndirection:"), dir);
		if(  automatic  ) {
			buf.append(translator::translate("\nSet phases:"));
			buf.append("\n");
			buf.append("\n");
		}
	}
}


// could be still better aligned for drive_left settings ...
void roadsign_t::calc_bild()
{
	set_flag(ding_t::dirty);

	// vertical offset of the signal positions
	const grund_t *gr=welt->lookup(get_pos());
	if(gr==NULL) {
		return;
	}

	after_xoffset = 0;
	after_yoffset = 0;
	sint8 xoff = 0, yoff = 0;
	const bool left_offsets = (  besch->get_wtyp()==road_wt  &&  !besch->is_choose_sign()  &&  welt->get_settings().is_drive_left()  );

	// private way have also closed/open states
	if(  besch->is_private_way()  ) {
		uint8 image = 1-(dir&1);
		if(  (1<<welt->get_active_player_nr()) & get_player_mask()  ) {
			// gate open
			image += 2;
		}
		set_bild( besch->get_bild_nr(image) );
		set_yoff( 0 );
		if(  hang_t::typ hang = gr->get_weg_hang()  ) {
			if(hang==hang_t::west ||  hang==hang_t::nord) {
				set_yoff( -TILE_HEIGHT_STEP );
			}
		}
		else {
			set_yoff( -gr->get_weg_yoff() );
		}
		after_bild = IMG_LEER;
		return;
	}

	hang_t::typ hang = gr->get_weg_hang();
	if(  hang==hang_t::flach  ) {
		yoff = -gr->get_weg_yoff();
		after_yoffset = yoff;
	}
	else {
		// since the places were switched
		if(  left_offsets  ) {
			hang = ribi_t::rueckwaerts(hang);
		}
		if(hang==hang_t::ost ||  hang==hang_t::nord) {
			yoff = -TILE_HEIGHT_STEP;
			after_yoffset = 0;
		}
		else {
			yoff = 0;
			after_yoffset = -TILE_HEIGHT_STEP;
		}
	}

	image_id tmp_bild=IMG_LEER;
	if(!automatic) {
		assert( zustand==0 );

		after_bild = IMG_LEER;
		ribi_t::ribi temp_dir = dir;

		if(  gr->get_typ()==grund_t::tunnelboden  &&  gr->ist_karten_boden()  &&
			(grund_t::underground_mode==grund_t::ugm_none  ||  (grund_t::underground_mode==grund_t::ugm_level  &&  gr->get_hoehe()<grund_t::underground_level))   ) {
			// entering tunnel here: hide the image further in if not undergroud/sliced
			hang = gr->get_grund_hang();
			if(  hang==hang_t::ost  ||  hang==hang_t::nord  ) {
				temp_dir &= ~ribi_t::suedwest;
			}
			else {
				temp_dir &= ~ribi_t::nordost;
			}
		}

		// signs for left side need other offsets and other front/back order
		if(  left_offsets  ) {
			const sint16 XOFF = 24;
			const sint16 YOFF = 16;

			if(temp_dir&ribi_t::ost) {
				tmp_bild = besch->get_bild_nr(3);
				xoff += XOFF;
				yoff += -YOFF;
			}

			if(temp_dir&ribi_t::nord) {
				if(tmp_bild!=IMG_LEER) {
					after_bild = besch->get_bild_nr(0);
					after_xoffset += -XOFF;
					after_yoffset += -YOFF;
				}
				else {
					tmp_bild = besch->get_bild_nr(0);
					xoff += -XOFF;
					yoff += -YOFF;
				}
			}

			if(temp_dir&ribi_t::west) {
				after_bild = besch->get_bild_nr(2);
				after_xoffset += -XOFF;
				after_yoffset += YOFF;
			}

			if(temp_dir&ribi_t::sued) {
				if(after_bild!=IMG_LEER) {
					tmp_bild = besch->get_bild_nr(1);
					xoff += XOFF;
					yoff += YOFF;
				}
				else {
					after_bild = besch->get_bild_nr(1);
					after_xoffset += XOFF;
					after_yoffset += YOFF;
				}
			}
		}
		else {

			if(temp_dir&ribi_t::ost) {
				after_bild = besch->get_bild_nr(3);
			}

			if(temp_dir&ribi_t::nord) {
				if(after_bild!=IMG_LEER) {
					tmp_bild = besch->get_bild_nr(0);
				}
				else {
					after_bild = besch->get_bild_nr(0);
				}
			}

			if(temp_dir&ribi_t::west) {
				tmp_bild = besch->get_bild_nr(2);
			}

			if(temp_dir&ribi_t::sued) {
				if(tmp_bild!=IMG_LEER) {
					after_bild = besch->get_bild_nr(1);
				}
				else {
					tmp_bild = besch->get_bild_nr(1);
				}
			}
		}

		// some signs on roads must not have a background (but then they have only two rotations)
		if(  besch->get_flags()&roadsign_besch_t::ONLY_BACKIMAGE  ) {
			if(after_bild!=IMG_LEER) {
				tmp_bild = after_bild;
			}
			after_bild = IMG_LEER;
		}
	}
	else {
		// traffic light
		weg_t *str=gr->get_weg(road_wt);
		if(str) {
			const uint8 weg_dir = str->get_ribi_unmasked();
			const uint8 direction = (dir&ribi_t::nord)!=0;

			// other front/back images for left side ...
			if(  left_offsets  ) {
				const int XOFF=30;
				const int YOFF=14;

				if(weg_dir&ribi_t::nord) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->get_bild_nr(6+direction*8);
						after_xoffset += 0;
						after_yoffset += 0;
					}
					else {
						after_bild = besch->get_bild_nr(1+direction*8);
						after_xoffset += XOFF;
						after_yoffset += YOFF;
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->get_bild_nr(2+direction*8);
					after_xoffset += -XOFF;
					after_yoffset += YOFF;
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::sued) {
						tmp_bild = besch->get_bild_nr(7+direction*8);
						xoff += 0;
						yoff += 0;
					}
					else {
						tmp_bild = besch->get_bild_nr(3+direction*8);
						xoff += XOFF;
						yoff += -YOFF;
					}
				}
				else if(weg_dir&ribi_t::sued) {
					tmp_bild = besch->get_bild_nr(0+direction*8);
					xoff += -XOFF;
					yoff += -YOFF;
				}
			}
			else {
				// drive right ...
				if(weg_dir&ribi_t::sued) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->get_bild_nr(4+direction*8);
					}
					else {
						after_bild = besch->get_bild_nr(0+direction*8);
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->get_bild_nr(2+direction*8);
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::nord) {
						tmp_bild = besch->get_bild_nr(5+direction*8);
					}
					else {
						tmp_bild = besch->get_bild_nr(3+direction*8);
					}
				}
				else if(weg_dir&ribi_t::nord) {
					tmp_bild = besch->get_bild_nr(1+direction*8);
				}
			}

		}
	}
	// set image and offsets
	set_bild( tmp_bild );
	set_xoff( xoff );
	set_yoff( yoff );

}


// only used for traffic light: change the current state
bool roadsign_t::sync_step(long /*delta_t*/)
{
	if(  besch->is_private_way()  ) {
		uint8 image = 1-(dir&1);
		if(  (1<<welt->get_active_player_nr()) & get_player_mask()  ) {
			// gate open
			image += 2;
			// force redraw
			mark_image_dirty(get_bild(),0);
		}
		set_bild( besch->get_bild_nr(image) );
	}
	else {
		// change every ~32s
		uint32 ticks = ((welt->get_zeit_ms()>>10)+ticks_offset) % (ticks_ns+ticks_ow);

		uint8 new_zustand = (ticks >= ticks_ns) ^ (welt->get_settings().get_rotation() & 1);
		if(zustand!=new_zustand) {
			zustand = new_zustand;
			dir = (new_zustand==0) ? ribi_t::nordsued : ribi_t::ostwest;
			calc_bild();
		}
	}
	return true;
}


void roadsign_t::rotate90()
{
	// only meaningful for traffic lights
	ding_t::rotate90();
	if(automatic  &&  !besch->is_private_way()) {
		zustand = (zustand+1)&1;
		uint8 temp = ticks_ns;
		ticks_ns = ticks_ow;
		ticks_ow = temp;
	}
	dir = ribi_t::rotate90( dir );
}


// to correct offset on slopes
void roadsign_t::display_after(int xpos, int ypos, bool ) const
{
	if(after_bild!=IMG_LEER) {
		const int raster_width = get_current_tile_raster_width();
		xpos += tile_raster_scale_x(after_xoffset, raster_width);
		ypos += tile_raster_scale_y(after_yoffset, raster_width);
		// draw with owner
		if(get_player_nr()!=PLAYER_UNOWNED) {
			if(  ding_t::show_owner  ) {
				display_blend(after_bild, xpos, ypos, 0, (get_besitzer()->get_player_color1()+2) | OUTLINE_FLAG | TRANSPARENT75_FLAG, 0, dirty);
			}
			else {
				display_color(after_bild, xpos, ypos, get_player_nr(), true, get_flag(ding_t::dirty) );
			}
		}
		else {
			display_normal(after_bild, xpos, ypos, 0, true, get_flag(ding_t::dirty) );
		}
	}
}


void roadsign_t::rdwr(loadsave_t *file)
{
	xml_tag_t r( file, "roadsign_t" );

	ding_t::rdwr(file);

	uint8 dummy=0;
	if(  file->get_version()<=102002  ) {
		file->rdwr_byte(dummy);
		if(  file->is_loading()  ) {
			ticks_ns = ticks_ow = 16;
		}
	}
	else {
		file->rdwr_byte(ticks_ns);
		file->rdwr_byte(ticks_ow);
	}
	if(  file->get_version()>=110007  ) {
		file->rdwr_byte(ticks_offset);
	}
	else {
		if(  file->is_loading()  ) {
			ticks_offset = 0;
		}
	}

	dummy = zustand;
	file->rdwr_byte(dummy);
	zustand = dummy;
	dummy = dir;
	file->rdwr_byte(dummy);
	dir = dummy;
	if(file->get_version()<89000) {
		dir = ribi_t::rueckwaerts(dir);
	}

	if(file->is_saving()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

		besch = roadsign_t::table.get(bname);
		if(besch==NULL) {
			besch = roadsign_t::table.get(translator::compatibility_name(bname));
			if(  besch==NULL  ) {
				dbg->warning("roadsign_t::rwdr", "description %s for roadsign/signal at %d,%d not found! (may be ignored)", bname, get_pos().x, get_pos().y);
				welt->add_missing_paks( bname, karte_t::MISSING_SIGN );
			}
			else {
				dbg->warning("roadsign_t::rwdr", "roadsign/signal %s at %d,%d replaced by %s", bname, get_pos().x, get_pos().y, besch->get_name() );
			}
		}
		// init ownership of private ways signs
		if(  file->get_version()<110007  &&  besch  &&  besch->is_private_way()  ) {
			ticks_ns = 0xFD;
			ticks_ow = 0xFF;
		}
	}
}


void roadsign_t::entferne(spieler_t *sp)
{
	spieler_t::accounting(sp, -besch->get_preis(), get_pos().get_2d(), COST_CONSTRUCTION);
}


/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void roadsign_t::laden_abschliessen()
{
	grund_t *gr=welt->lookup(get_pos());
	if(gr==NULL  ||  !gr->hat_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt)) {
		dbg->error("roadsign_t::laden_abschliessen","roadsing: way/ground missing at %i,%i => ignore", get_pos().x, get_pos().y );
	}
	else {
		// after loading restore directions
		set_dir(dir);
		gr->get_weg(besch->get_wtyp()!=tram_wt ? besch->get_wtyp() : track_wt)->count_sign();
	}
	// only traffic light need switches
	if(automatic) {
		welt->sync_add_ts( this );
	}
}


// to sort compare_roadsign_besch for always the same menu order
static bool compare_roadsign_besch(const roadsign_besch_t* a, const roadsign_besch_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		if(a->is_choose_sign()) {
			diff += 120;
		}
		if(b->is_choose_sign()) {
			diff -= 120;
		}
		diff += (int)(a->get_flags() & ~roadsign_besch_t::SIGN_SIGNAL) - (int)(b->get_flags()  & ~roadsign_besch_t::SIGN_SIGNAL);
	}
	if (diff == 0) {
		/* Some type: sort by name */
		diff = strcmp(a->get_name(), b->get_name());
	}
	return diff < 0;
}


/* static stuff from here on ... */
bool roadsign_t::alles_geladen()
{
	if(table.empty()) {
		DBG_MESSAGE("roadsign_t", "No signs found - feature disabled");
	}
	return true;
}


bool roadsign_t::register_besch(roadsign_besch_t *besch)
{
	// avoid duplicates with same name
	const roadsign_besch_t *old_besch = table.get(besch->get_name());
	if(old_besch) {
		dbg->warning( "roadsign_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
		table.remove(besch->get_name());
		werkzeug_t::general_tool.remove( old_besch->get_builder() );
		delete old_besch->get_builder();
		delete old_besch;
	}

	if(  besch->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
		// add the tool
		wkz_roadsign_t *wkz = new wkz_roadsign_t();
		wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
		wkz->cursor = besch->get_cursor()->get_bild_nr(0);
		wkz->set_default_param(besch->get_name());
		werkzeug_t::general_tool.append( wkz );
		besch->set_builder( wkz );
	}
	else {
		besch->set_builder( NULL );
	}

	roadsign_t::table.put(besch->get_name(), besch);

	if(besch->get_wtyp()==track_wt  &&  besch->get_flags()==roadsign_besch_t::SIGN_SIGNAL) {
		default_signal = besch;
	}

	return true;
}


/**
 * Fill menu with icons of given signals/roadsings from the list
 * @author Hj. Malthaner
 */
void roadsign_t::fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, sint16 /*sound_ok*/, const karte_t *welt)
{
	// check if scenario forbids this
	if (!welt->get_scenario()->is_tool_allowed(welt->get_active_player(), WKZ_ROADSIGN | GENERAL_TOOL, wtyp)) {
		return;
	}

	const uint16 time = welt->get_timeline_year_month();

	vector_tpl<const roadsign_besch_t *>matching;

	FOR(stringhashtable_tpl<roadsign_besch_t const*>, const& i, table) {
		roadsign_besch_t const* const besch = i.value;
		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			if(besch->get_builder()  &&  wtyp==besch->get_wtyp()) {
				// only add items with a cursor
				matching.insert_ordered( besch, compare_roadsign_besch );
			}
		}
	}
	FOR(vector_tpl<roadsign_besch_t const*>, const i, matching) {
		wzw->add_werkzeug(i->get_builder());
	}
}


/**
 * Finds a matching roadsing
 * @author prissi
 */
const roadsign_besch_t *roadsign_t::roadsign_search(roadsign_besch_t::types const flag, waytype_t const wt, uint16 const time)
{
	FOR(stringhashtable_tpl<roadsign_besch_t const*>, const& i, table) {
		roadsign_besch_t const* const besch = i.value;
		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->get_wtyp()==wt  &&  besch->get_flags()==flag) {
				return besch;
		}
	}
	return NULL;
}
