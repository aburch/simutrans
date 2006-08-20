/*
 * signal.cc
 *
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





slist_tpl<const roadsign_besch_t *> roadsign_t::liste;
stringhashtable_tpl<const roadsign_besch_t *> roadsign_t::table;


roadsign_t::roadsign_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
	rdwr(file);
	step_frequency = 0;
	// if more than one state, we will switch direction and phase
	if(besch->gib_bild_anzahl()>4) {
		flags |= SWITCH_AUTOMATIC;
	}
	last_switch = 0;
}



roadsign_t::roadsign_t(karte_t *welt, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch) :  ding_t(welt, pos)
{
	this->besch = besch;
	this->dir = dir;
	zustand = 0;
	last_switch = 0;
	flags = 3;
	step_frequency = 0;
	// if more than one state, we will switch direction and phase
	if(besch->gib_bild_anzahl()>4) {
		flags |= SWITCH_AUTOMATIC;
	}
}



roadsign_t::~roadsign_t()
{
	weg_t *weg = welt->lookup(gib_pos())->gib_weg((weg_t::typ)besch->gib_wtyp());
	if(weg) {
		// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
		weg->setze_ribi_maske(ribi_t::keine);
	}
	else {
		dbg->error("roadsign_t::~roadsign_t()","roadsign_t %p was deleted but ground was not a road!");
	}
}



void roadsign_t::set_dir(ribi_t::ribi dir)
{
	this->dir = dir;
	if(besch->is_single_way()  ||  besch->is_signal()  ||  besch->is_pre_signal()) {
		weg_t *weg = welt->lookup(gib_pos())->gib_weg((weg_t::typ)besch->gib_wtyp());
		if(weg) {
			// set mask, if it is a signle way ...
			if(ribi_t::ist_einfach(dir)) {
				weg->setze_ribi_maske(dir);
			}
			else {
				weg->setze_ribi_maske(ribi_t::keine);
			}
			weg->count_sign();
DBG_MESSAGE("roadsign_t::set_dir()","ribi %i",dir);
		}
	}
	setze_bild(0,IMG_LEER);
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



void roadsign_t::calc_bild()
{
	if(step_frequency==0) {

		if(dir&ribi_t::nord) {
			after_bild = besch->gib_bild_nr(0+zustand*4);
		}

		if(dir&ribi_t::sued) {
			setze_bild(0, besch->gib_bild_nr(1+zustand*4));
		}

		if(dir&ribi_t::ost) {
			setze_bild(0, besch->gib_bild_nr(3+zustand*4));
		}

		if(dir&ribi_t::west) {
			after_bild = besch->gib_bild_nr(2+zustand*4);
		}
	}
	else {
		// traffic light
		weg_t *str= welt->lookup(gib_pos())->gib_weg(weg_t::strasse);
		if(str)
		{
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
						setze_bild(0, besch->gib_bild_nr(7+direction*8));
					}
					else {
						setze_bild(0, besch->gib_bild_nr(3+direction*8));
					}
				}
				else if(weg_dir&ribi_t::sued) {
					setze_bild(0, besch->gib_bild_nr(0+direction*8));
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
						setze_bild(0, besch->gib_bild_nr(5+direction*8));
					}
					else {
						setze_bild(0, besch->gib_bild_nr(3+direction*8));
					}
				}
				else if(weg_dir&ribi_t::nord) {
					setze_bild(0, besch->gib_bild_nr(1+direction*8));
				}
			}

		}
	}
}




// only used for traffic light: change the current state
bool roadsign_t::step(long delta_t)
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
roadsign_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	file->rdwr_byte(flags, " ");
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

		besch = (const roadsign_besch_t *)roadsign_t::table.get(bname);
		if(!besch) {
			besch = (const roadsign_besch_t *)roadsign_t::table.get(translator::compatibility_name(bname));
		}
		if(!besch) {
			DBG_MESSAGE("roadsign_t::rwdr", "description %s for ropadsign at %d,%d not found, will be removed!", bname, gib_pos().x, gib_pos().y);
		}
	}
}




void
roadsign_t::entferne(spieler_t *sp)
{
	if(sp!=NULL) {
		sp->buche(-besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION);
	}
}



/**
 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
 * um das Aussehen des Dings an Boden und Umgebung anzupassen
 *
 * @author Hj. Malthaner
 */
void roadsign_t::laden_abschliessen()
{
	// after loading restore directions
	set_dir(dir);
	if((flags&SWITCH_AUTOMATIC)==0) {
		step_frequency = 1;
	}
}




/* static stuff from here on ... */
bool roadsign_t::alles_geladen()
{
	if(liste.count() == 0) {
		DBG_MESSAGE("roadsign_t", "No signs found - feature disabled");
	}
	return true;
}

bool roadsign_t::register_besch(roadsign_besch_t *besch)
{
	roadsign_t::table.put(besch->gib_name(), besch);
	roadsign_t::liste.append(besch);
	if(umgebung_t::drive_on_left  &&  besch->gib_wtyp()==weg_t::strasse) {
		// correct for driving on left side
		if(besch->is_traffic_light()) {
			const int XOFF=(48*get_tile_raster_width())/64;
			const int YOFF=(26*get_tile_raster_width())/64;

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
	weg_t::typ wtyp,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	int sound_ok,
	int sound_ko,
	const uint16 time)
{
DBG_DEBUG("roadsign_t::fill_menu()","maximum %i",roadsign_t::liste.count());
	for( unsigned i=0;  i<roadsign_t::liste.count();  i++  ) {
		char buf[128];
		const roadsign_besch_t *besch=roadsign_t::liste.at(i);

		if(time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time)) {

DBG_DEBUG("roadsign_t::fill_menu()","try at pos %i to add %s(%p)",i,besch->gib_name(),besch);
			if(besch->gib_cursor()->gib_bild_nr(1)!=IMG_LEER  &&  wtyp==besch->gib_wtyp()) {
				// only add items with a cursor
DBG_DEBUG("roadsign_t::fill_menu()","at pos %i add %s",i,besch->gib_name());
				int n=sprintf(buf, "%s ",translator::translate(besch->gib_name()));
				money_to_string(buf+n, besch->gib_preis()/-100.0);

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
roadsign_t::roadsign_search(uint8 flag,const weg_t::typ wt,const uint16 time)
{
	for( unsigned i=0;  i<roadsign_t::liste.count();  i++  ) {
		const roadsign_besch_t *besch=roadsign_t::liste.at(i);

		if((time==0  ||  (besch->get_intro_year_month()<=time  &&  besch->get_retire_year_month()>time))
			&&  besch->gib_wtyp()==wt  &&  besch->get_flags()==flag) {
				return besch;
		}
	}
	return NULL;
}
