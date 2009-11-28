/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

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
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../gui/trafficlight_info.h"
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
		// if more than one state, we will switch direction and phase for traffic lights
		automatic = (besch->get_bild_anzahl()>4  &&  besch->get_wtyp()==road_wt);
	}
}



roadsign_t::roadsign_t(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch) :  ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = dir;
	bild = after_bild = IMG_LEER;
	zustand = 0;
	ticks_ns = ticks_ow = 16;
	set_besitzer( sp );
	// if more than one state, we will switch direction and phase for traffic lights
	automatic = (besch->get_bild_anzahl()>4  &&  besch->get_wtyp()==road_wt);
}



roadsign_t::~roadsign_t()
{
	const grund_t *gr = welt->lookup(get_pos());
	if(gr) {
		weg_t *weg = gr->get_weg((waytype_t)besch->get_wtyp());
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->set_ribi_maske(ribi_t::keine);
		}
		else {
			dbg->error("roadsign_t::~roadsign_t()","roadsign_t %p was deleted but ground was not a road!");
		}
	}
	if(automatic) {
		welt->sync_remove(this);
	}
}



void roadsign_t::set_dir(ribi_t::ribi dir)
{
	this->dir = dir;
	weg_t *weg = welt->lookup(get_pos())->get_weg((waytype_t)besch->get_wtyp());
	if(besch->get_wtyp()!=track_wt  &&   besch->get_wtyp()!=monorail_wt&&   besch->get_wtyp()!=maglev_wt&&   besch->get_wtyp()!=narrowgauge_wt) {
		weg->count_sign();
	}
	if(besch->is_single_way()  ||  besch->is_signal()  ||  besch->is_pre_signal()  ||  besch->is_longblock_signal()) {
		// set mask, if it is a signle way ...
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
	mark_image_dirty(after_bild,after_offset);

	bild = IMG_LEER;
	after_bild = IMG_LEER;
	calc_bild();
}


void roadsign_t::zeige_info()
{
	if(  automatic  ) {
		create_win(new trafficlight_info_t(this), w_info, (long)this );
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
	buf.append(translator::translate("Roadsign"));
	buf.append("\n");
	if(besch->is_single_way()) {
		buf.append(translator::translate("\nsingle way"));
	}
	if(besch->get_min_speed()!=0) {
		buf.append(translator::translate("\nminimum speed:"));
		buf.append(speed_to_kmh(besch->get_min_speed()));
	}
	buf.append(translator::translate("\ndirection:"));
	buf.append(dir);
	buf.append("\n");
	if(  automatic  ) {
		buf.append(translator::translate("\nSet phases:"));
		buf.append("\n");
		buf.append("\n");
	}
}



// coulb be still better aligned for drive_left settings ...
void roadsign_t::calc_bild()
{
	set_flag(ding_t::dirty);

	after_offset = 0;

	// vertical offset of the signal positions
	const grund_t *gr=welt->lookup(get_pos());
	if(gr==NULL) {
		return;
	}

	hang_t::typ hang = gr->get_weg_hang();
	if(hang==hang_t::flach) {
		set_yoff( -gr->get_weg_yoff() );
		after_offset = 0;
	}
	else {
		// since the places were switched
		if(besch->is_traffic_light() && !umgebung_t::drive_on_left) {
			if (hang==hang_t::nord || hang==hang_t::sued) {
				hang = ribi_t::rueckwaerts(hang);
			}
		}
		if(hang==hang_t::ost ||  hang==hang_t::nord) {
			set_yoff( -TILE_HEIGHT_STEP );
			after_offset = +TILE_HEIGHT_STEP;
		}
		else {
			set_yoff( 0 );
			after_offset = -TILE_HEIGHT_STEP;
		}
	}

	if(!automatic) {

		image_id tmp_bild=IMG_LEER;
		after_bild = IMG_LEER;

		if(dir&ribi_t::ost) {
			after_bild = besch->get_bild_nr(3+zustand*4);
		}

		if(dir&ribi_t::nord) {
			if(after_bild!=IMG_LEER) {
				tmp_bild = besch->get_bild_nr(0+zustand*4);
			}
			else {
				after_bild = besch->get_bild_nr(0+zustand*4);
			}
		}

		if(dir&ribi_t::west) {
			tmp_bild = besch->get_bild_nr(2+zustand*4);
		}

		if(dir&ribi_t::sued) {
			if(tmp_bild!=IMG_LEER) {
				after_bild = besch->get_bild_nr(1+zustand*4);
			}
			else {
				tmp_bild = besch->get_bild_nr(1+zustand*4);
			}
		}

		// some signs on roads must not have a background (but then they have only two rotations)
		if(besch->get_flags()&roadsign_besch_t::ONLY_BACKIMAGE) {
			if(after_bild!=IMG_LEER) {
				tmp_bild = after_bild;
			}
			after_bild = IMG_LEER;
		}

		set_bild( tmp_bild );
	}
	else {
		// traffic light
		weg_t *str=gr->get_weg(road_wt);
		if(str) {
			const uint8 weg_dir = str->get_ribi_unmasked();
			const uint8 direction = (dir&ribi_t::nord)!=0;

			// other front/back images for left side ...
			if(umgebung_t::drive_on_left) {

				// drive left
				if(weg_dir&ribi_t::nord) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->get_bild_nr(6+direction*8);
					}
					else {
						after_bild = besch->get_bild_nr(1+direction*8);
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->get_bild_nr(2+direction*8);
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::sued) {
						set_bild(besch->get_bild_nr(7+direction*8));
					}
					else {
						set_bild(besch->get_bild_nr(3+direction*8));
					}
				}
				else if(weg_dir&ribi_t::sued) {
					set_bild(besch->get_bild_nr(0+direction*8));
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
						set_bild(besch->get_bild_nr(5+direction*8));
					}
					else {
						set_bild(besch->get_bild_nr(3+direction*8));
					}
				}
				else if(weg_dir&ribi_t::nord) {
					set_bild(besch->get_bild_nr(1+direction*8));
				}
			}

		}
	}
}




// only used for traffic light: change the current state
bool roadsign_t::sync_step(long /*delta_t*/)
{
	// change every ~32s
	uint32 ticks = (welt->get_zeit_ms()>>10) % (ticks_ns+ticks_ow);

	uint8 new_zustand = welt->get_einstellungen()->get_rotation()&1 ? ticks>ticks_ow : ticks>ticks_ns ;
	if(zustand!=new_zustand) {
		zustand = new_zustand;
		dir = (new_zustand==0) ? ribi_t::nordsued : ribi_t::ostwest;
		calc_bild();
	}
	return true;
}



void roadsign_t::rotate90()
{
	ding_t::rotate90();
	// only meaningful for traffic lights
	if(automatic) {
		zustand = (zustand+1)&1;
	}
	dir = ribi_t::rotate90( dir );
	calc_bild();
}



// to correct offset on slopes
void roadsign_t::display_after(int xpos, int ypos, bool ) const
{
	if(after_bild!=IMG_LEER) {
		const int raster_width = get_tile_raster_width();
		ypos += tile_raster_scale_x(get_yoff()+after_offset, raster_width);
		xpos += tile_raster_scale_x(get_xoff(), raster_width);
		// draw with owner
		if(get_player_nr()!=-1) {
			display_color_img(after_bild, xpos, ypos, get_player_nr(), true, get_flag(ding_t::dirty) );
		}
		else {
			display_img(after_bild, xpos, ypos, get_flag(ding_t::dirty) );
		}
	}
}



void roadsign_t::rdwr(loadsave_t *file)
{
	xml_tag_t r( file, "roadsign_t" );

	ding_t::rdwr(file);

	uint8 dummy=0;
	if(  file->get_version()<103000  ) {
		file->rdwr_byte(dummy, " ");
		if(  file->is_loading()  ) {
			ticks_ns = ticks_ow = 16;
		}
	}
	else {
		file->rdwr_byte(ticks_ns, "" );
		file->rdwr_byte(ticks_ow, "" );
	}
	dummy = zustand;
	file->rdwr_byte(dummy, " ");
	zustand = dummy;
	dummy = dir;
	file->rdwr_byte(dummy, "\n");
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
		file->rdwr_str(bname, 128);

		besch = roadsign_t::table.get(bname);
		if(besch==NULL) {
			besch = roadsign_t::table.get(translator::compatibility_name(bname));
			if(besch==NULL) {
				dbg->warning("roadsign_t::rwdr", "description %s for roadsign/signal at %d,%d not found! (may be ignored)", bname, get_pos().x, get_pos().y);
			}
			else {
				dbg->warning("roadsign_t::rwdr", "roadsign/signal %s at %d,%d rpleaced by %s", bname, get_pos().x, get_pos().y, besch->get_name() );
			}
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
	if(gr==NULL  ||  !gr->hat_weg(besch->get_wtyp())) {
		dbg->error("roadsign_t::laden_abschliessen","roadsing: way/ground missing at %i,%i => ignore", get_pos().x, get_pos().y );
	}
	else {
		// after loading restore directions
		set_dir(dir);
		gr->get_weg(besch->get_wtyp())->count_sign();
	}
	// only traffic light need switches
	if(automatic) {
		welt->sync_add( this );
	}
}




// to sort compare_roadsign_besch for always the same menu order
static bool compare_roadsign_besch(const roadsign_besch_t* a, const roadsign_besch_t* b)
{
	int diff = a->get_wtyp() - b->get_wtyp();
	if (diff == 0) {
		if(a->is_free_route()) {
			diff += 120;
		}
		if(b->is_free_route()) {
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
		delete old_besch->get_builder();
		delete old_besch;
	}

	if(  besch->get_cursor()->get_bild_nr(1)!=IMG_LEER  ) {
		// add the tool
		wkz_roadsign_t *wkz = new wkz_roadsign_t();
		wkz->set_icon( besch->get_cursor()->get_bild_nr(1) );
		wkz->cursor = besch->get_cursor()->get_bild_nr(0);
		wkz->default_param = besch->get_name();
		wkz->id = werkzeug_t::general_tool.get_count()|GENERAL_TOOL;
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

	if(umgebung_t::drive_on_left  &&  besch->get_wtyp()==road_wt) {
		// correct for driving on left side
		if(besch->is_traffic_light()) {
			const int XOFF=(24*get_tile_raster_width())/64;
			const int YOFF=(16*get_tile_raster_width())/64;

			display_set_base_image_offset( besch->get_bild_nr(0), -XOFF, -YOFF );
			display_set_base_image_offset( besch->get_bild_nr(8), -XOFF, -YOFF );
			display_set_base_image_offset( besch->get_bild_nr(1), +XOFF, +YOFF );
			display_set_base_image_offset( besch->get_bild_nr(9), +XOFF, +YOFF );
			display_set_base_image_offset( besch->get_bild_nr(2), -XOFF, +YOFF );
			display_set_base_image_offset( besch->get_bild_nr(10), -XOFF, +YOFF );
			display_set_base_image_offset( besch->get_bild_nr(3), +XOFF, -YOFF );
			display_set_base_image_offset( besch->get_bild_nr(11), +XOFF, -YOFF );
		}
		else if(!besch->is_free_route()) {
			const int XOFF=(30*get_tile_raster_width())/64;
			const int YOFF=(14*get_tile_raster_width())/64;

			display_set_base_image_offset( besch->get_bild_nr(0), -XOFF, -YOFF );
			display_set_base_image_offset( besch->get_bild_nr(1), +XOFF, +YOFF );
			display_set_base_image_offset( besch->get_bild_nr(2), -XOFF, +YOFF );
			display_set_base_image_offset( besch->get_bild_nr(3), +XOFF, -YOFF );
		}
	}
	return true;
}




/**
 * Fill menu with icons of given signals/roadsings from the list
 * @author Hj. Malthaner
 */
void roadsign_t::fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, sint16 sound_ok, const karte_t *welt)
{
	const uint16 time = welt->get_timeline_year_month();

	stringhashtable_iterator_tpl<const roadsign_besch_t *>iter(table);
	vector_tpl<const roadsign_besch_t *>matching;

	while(  iter.next()  ) {
		const roadsign_besch_t* besch = iter.get_current_value();
		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			if(besch->get_builder()  &&  wtyp==besch->get_wtyp()) {
				// only add items with a cursor
				matching.insert_ordered( besch, compare_roadsign_besch );
			}
		}
	}
	for (vector_tpl<const roadsign_besch_t*>::const_iterator i = matching.begin(), end = matching.end(); i != end; ++i) {
		wzw->add_werkzeug( (*i)->get_builder() );
	}
}


/**
 * Finds a matching roadsing
 * @author prissi
 */
const roadsign_besch_t *roadsign_t::roadsign_search(uint8 flag,const waytype_t wt,const uint16 time)
{
	stringhashtable_iterator_tpl<const roadsign_besch_t *>iter(table);
	while(  iter.next()  ) {
		const roadsign_besch_t* besch = iter.get_current_value();
		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->get_wtyp()==wt  &&  besch->get_flags()==flag) {
				return besch;
		}
	}
	return NULL;
}
