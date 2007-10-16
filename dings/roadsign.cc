/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simplay.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"

#include "../besch/roadsign_besch.h"

#include "../boden/wege/strasse.h"
#include "../boden/grund.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/umgebung.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../tpl/stringhashtable_tpl.h"

// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"

#include "../simvehikel.h"	// for kmh/speed conversion

#include "roadsign.h"



const roadsign_besch_t *roadsign_t::default_signal=NULL;




slist_tpl<const roadsign_besch_t *> roadsign_t::liste;
stringhashtable_tpl<const roadsign_besch_t *> roadsign_t::table;


roadsign_t::roadsign_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	bild = after_bild = IMG_LEER;
	rdwr(file);
	if(besch) {
		// if more than one state, we will switch direction and phase for traffic lights
		automatic = (besch->gib_bild_anzahl()>4  &&  besch->gib_wtyp()==road_wt);
	}
	last_switch = 0;
}



roadsign_t::roadsign_t(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch) :  ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = dir;
	bild = after_bild = IMG_LEER;
	zustand = 0;
	last_switch = 0;
	setze_besitzer( sp );
	// if more than one state, we will switch direction and phase for traffic lights
	automatic = (besch->gib_bild_anzahl()>4  &&  besch->gib_wtyp()==road_wt);
}



roadsign_t::~roadsign_t()
{
	const grund_t *gr = welt->lookup(gib_pos());
	if(gr) {
		weg_t *weg = gr->gib_weg((waytype_t)besch->gib_wtyp());
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->setze_ribi_maske(ribi_t::keine);
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
	weg_t *weg = welt->lookup(gib_pos())->gib_weg((waytype_t)besch->gib_wtyp());
	if(besch->gib_wtyp()!=track_wt  &&   besch->gib_wtyp()!=monorail_wt) {
		weg->count_sign();
	}
	if(besch->is_single_way()  ||  besch->is_signal()  ||  besch->is_pre_signal()  ||  besch->is_longblock_signal()) {
		// set mask, if it is a signle way ...
		weg->count_sign();
		if(ribi_t::ist_einfach(dir)) {
			weg->setze_ribi_maske(dir);
		}
		else {
			weg->setze_ribi_maske(ribi_t::keine);
		}
DBG_MESSAGE("roadsign_t::set_dir()","ribi %i",dir);
	}

	// force redraw
	mark_image_dirty(gib_bild(),0);
	mark_image_dirty(after_bild,after_offset);

	bild = IMG_LEER;
	after_bild = IMG_LEER;
	calc_bild();
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
	if(besch->gib_min_speed()!=0) {
		buf.append(translator::translate("\nminimum speed:"));
		buf.append(speed_to_kmh(besch->gib_min_speed()));
	}
	buf.append(translator::translate("\ndirection:"));
	buf.append(dir);
	buf.append("\n");
}



// coulb be still better aligned for drive_left settings ...
void roadsign_t::calc_bild()
{
	set_flag(ding_t::dirty);

	after_offset = 0;

	// vertical offset of the signal positions
	const grund_t *gr=welt->lookup(gib_pos());
	if(gr==NULL) {
		return;
	}

	hang_t::typ hang = gr->gib_weg_hang();
	if(hang==hang_t::flach) {
		setze_yoff( -gr->gib_weg_yoff() );
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
			setze_yoff( -TILE_HEIGHT_STEP );
			after_offset = +TILE_HEIGHT_STEP;
		}
		else {
			setze_yoff( 0 );
			after_offset = -TILE_HEIGHT_STEP;
		}
	}

	if(!automatic) {

		image_id tmp_bild=IMG_LEER;
		after_bild = IMG_LEER;

		if(dir&ribi_t::ost) {
			after_bild = besch->gib_bild_nr(3+zustand*4);
		}

		if(dir&ribi_t::nord) {
			if(after_bild!=IMG_LEER) {
				tmp_bild = besch->gib_bild_nr(0+zustand*4);
			}
			else {
				after_bild = besch->gib_bild_nr(0+zustand*4);
			}
		}

		if(dir&ribi_t::west) {
			tmp_bild = besch->gib_bild_nr(2+zustand*4);
		}

		if(dir&ribi_t::sued) {
			if(tmp_bild!=IMG_LEER) {
				after_bild = besch->gib_bild_nr(1+zustand*4);
			}
			else {
				tmp_bild = besch->gib_bild_nr(1+zustand*4);
			}
		}

		// some signs on roads must not have a background (but then they have only two rotations)
		if(besch->get_flags()&roadsign_besch_t::ONLY_BACKIMAGE) {
			if(after_bild!=IMG_LEER) {
				tmp_bild = after_bild;
			}
			after_bild = IMG_LEER;
		}

		setze_bild( tmp_bild );
	}
	else {
		// traffic light
		weg_t *str=gr->gib_weg(road_wt);
		if(str) {
			const uint8 weg_dir = str->gib_ribi_unmasked();
			const uint8 direction = (dir&ribi_t::nord)!=0;

			// other front/back images for left side ...
			if(umgebung_t::drive_on_left) {

				// drive left
				if(weg_dir&ribi_t::nord) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->gib_bild_nr(6+direction*8);
					}
					else {
						after_bild = besch->gib_bild_nr(1+direction*8);
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->gib_bild_nr(2+direction*8);
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::sued) {
						setze_bild(besch->gib_bild_nr(7+direction*8));
					}
					else {
						setze_bild(besch->gib_bild_nr(3+direction*8));
					}
				}
				else if(weg_dir&ribi_t::sued) {
					setze_bild(besch->gib_bild_nr(0+direction*8));
				}
			}
			else {
				// drive right ...
				if(weg_dir&ribi_t::sued) {
					if(weg_dir&ribi_t::ost) {
						after_bild = besch->gib_bild_nr(4+direction*8);
					}
					else {
						after_bild = besch->gib_bild_nr(0+direction*8);
					}
				}
				else if(weg_dir&ribi_t::ost) {
					after_bild = besch->gib_bild_nr(2+direction*8);
				}

				if(weg_dir&ribi_t::west) {
					if(weg_dir&ribi_t::nord) {
						setze_bild(besch->gib_bild_nr(5+direction*8));
					}
					else {
						setze_bild(besch->gib_bild_nr(3+direction*8));
					}
				}
				else if(weg_dir&ribi_t::nord) {
					setze_bild(besch->gib_bild_nr(1+direction*8));
				}
			}

		}
	}
}




// only used for traffic light: change the current state
bool
roadsign_t::sync_step(long delta_t)
{
	// change every 24 hours in normal speed = (1<<18)/24
	last_switch += delta_t;
	if(last_switch > 10922) {
		last_switch -= 10922;
		zustand = (zustand+1)&1;
		dir = (zustand==0) ? ribi_t::nordsued : ribi_t::ostwest;
		calc_bild();
	}
	return true;
}



void
roadsign_t::rotate90()
{
	// only meaningful for traffic lights
	if(automatic) {
		zustand = (zustand+1)&1;
	}
}



// to correct offset on slopes
void
roadsign_t::display_after(int xpos, int ypos, bool ) const
{
	if(after_bild!=IMG_LEER) {
		const int raster_width = get_tile_raster_width();
		ypos += tile_raster_scale_x(gib_yoff()+after_offset, raster_width);
		xpos += tile_raster_scale_x(gib_xoff(), raster_width);
		// draw with owner
		if(get_player_nr()!=-1) {
			display_color_img(after_bild, xpos, ypos, get_player_nr(), true, get_flag(ding_t::dirty) );
		}
		else {
			display_img(after_bild, xpos, ypos, get_flag(ding_t::dirty) );
		}
	}
}



void
roadsign_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	uint8 dummy=0;
	file->rdwr_byte(dummy, " ");
	file->rdwr_byte(zustand, " ");
	file->rdwr_byte(dir, "\n");
	if(file->get_version()<89000) {
		dir = ribi_t::rueckwaerts(dir);
	}

	if(file->is_saving()) {
		const char *s = besch->gib_name();
		file->rdwr_str(s, "N");
	}

	if(file->is_loading()) {
		char bname[128];
		file->rd_str_into(bname, "N");

		besch = roadsign_t::table.get(bname);
		if(besch==NULL) {
			besch = roadsign_t::table.get(translator::compatibility_name(bname));
			if(besch==NULL) {
				dbg->warning("roadsign_t::rwdr", "description %s for roadsign/signal at %d,%d not found! (may be ignored)", bname, gib_pos().x, gib_pos().y);
			}
			else {
				dbg->warning("roadsign_t::rwdr", "roadsign/signal %s at %d,%d rpleaced by %s", bname, gib_pos().x, gib_pos().y, besch->gib_name() );
			}
		}
	}
}




void
roadsign_t::entferne(spieler_t *sp)
{
	sp->buche(-besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
}



/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void roadsign_t::laden_abschliessen()
{
	grund_t *gr=welt->lookup(gib_pos());
	if(gr==NULL  ||  !gr->hat_weg(besch->gib_wtyp())) {
		dbg->error("roadsign_t::laden_abschliessen","roadsing: way/ground missing at %i,%i => ignore", gib_pos().x, gib_pos().y );
	}
	else {
		// after loading restore directions
		set_dir(dir);
		gr->gib_weg(besch->gib_wtyp())->count_sign();
	}
	// only traffic light need switches
	if(automatic) {
		welt->sync_add( this );
	}
}




/* static stuff from here on ... */
bool roadsign_t::alles_geladen()
{
	if (liste.empty()) {
		DBG_MESSAGE("roadsign_t", "No signs found - feature disabled");
	}
	return true;
}



bool roadsign_t::register_besch(roadsign_besch_t *besch)
{
	roadsign_t::table.put(besch->gib_name(), besch);
	roadsign_t::liste.append(besch);
	if(besch->gib_wtyp()==track_wt  &&  besch->get_flags()==roadsign_besch_t::SIGN_SIGNAL) {
		default_signal = besch;
	}
	if(umgebung_t::drive_on_left  &&  besch->gib_wtyp()==road_wt) {
		// correct for driving on left side
		if(besch->is_traffic_light()) {
			const int XOFF=(24*get_tile_raster_width())/64;
			const int YOFF=(16*get_tile_raster_width())/64;

			display_set_image_offset( besch->gib_bild_nr(0), -XOFF, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(8), -XOFF, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(1), +XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(9), +XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(2), -XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(10), -XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(3), +XOFF, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(11), +XOFF, -YOFF );
		}
		else if(!besch->is_free_route()) {
			const int XOFF=(30*get_tile_raster_width())/64;
			const int YOFF=(14*get_tile_raster_width())/64;

			display_set_image_offset( besch->gib_bild_nr(0), -XOFF, -YOFF );
			display_set_image_offset( besch->gib_bild_nr(1), +XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(2), -XOFF, +YOFF );
			display_set_image_offset( besch->gib_bild_nr(3), +XOFF, -YOFF );
		}
	}
DBG_DEBUG( "roadsign_t::register_besch()","%s", besch->gib_name() );
	return true;
}




/**
 * Fill menu with icons of given stops from the list
 * @author Hj. Malthaner
 */
void roadsign_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
	waytype_t wtyp,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	int sound_ok,
	int sound_ko,
  const karte_t *welt)
{
	const uint16 time = welt->get_timeline_year_month();
DBG_DEBUG("roadsign_t::fill_menu()","maximum %i",roadsign_t::liste.count());
	for (slist_iterator_tpl<const roadsign_besch_t*> i(roadsign_t::liste); i.next();) {
		const roadsign_besch_t* besch = i.get_current();
		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

			DBG_DEBUG("roadsign_t::fill_menu()", "try to add %s(%p)", besch->gib_name(), besch);
			if(besch->gib_cursor()->gib_bild_nr(1)!=IMG_LEER  &&  wtyp==besch->gib_wtyp()) {
				// only add items with a cursor
				DBG_DEBUG("roadsign_t::fill_menu()", "add %s", besch->gib_name());
				char buf[128];
				int n=sprintf(buf, "%s ",translator::translate(besch->gib_name()));
				money_to_string(buf+n, besch->gib_preis()/100.0);

				wzw->add_param_tool(werkzeug,
				  (const void *)besch,
				  karte_t::Z_PLAN,
				  sound_ok,
				  sound_ko,
				  besch->gib_cursor()->gib_bild_nr(1),
				  besch->gib_cursor()->gib_bild_nr(0),
				  buf );
			}
		}
	}
}


/**
 * Finds a matching roadsing
 * @author prissi
 */
const roadsign_besch_t *
roadsign_t::roadsign_search(uint8 flag,const waytype_t wt,const uint16 time)
{
	for (slist_iterator_tpl<const roadsign_besch_t*> i(roadsign_t::liste); i.next();) {
		const roadsign_besch_t* besch = i.get_current();
		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->gib_wtyp()==wt  &&  besch->get_flags()==flag) {
				return besch;
		}
	}
	return NULL;
}
