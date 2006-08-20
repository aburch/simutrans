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
#include "../simcosts.h"
#include "../simworld.h"
#include "../simdings.h"
#include "../boden/wege/strasse.h"
#include "../boden/grund.h"
#include "../simimg.h"
#include "../dataobj/loadsave.h"
#include "../besch/roadsign_besch.h"
#include "../utils/cbuffer_t.h"

#include "../tpl/stringhashtable_tpl.h"

// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"

#include "roadsign.h"

slist_tpl<const roadsign_besch_t *> liste;
stringhashtable_tpl<const roadsign_besch_t *> table;


roadsign_t::roadsign_t(karte_t *welt, loadsave_t *file) : ding_t (welt)
{
  rdwr(file);
  step_frequency = 0;
  set_dir(dir);
}

roadsign_t::roadsign_t(karte_t *welt, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t *besch) :  ding_t(welt, pos)
{
DBG_MESSAGE("roadsign_t::roadsign_t()","at (%i,%i,%i) with dir=%i and min=%i",pos.x,pos.y,pos.z,dir,besch->gib_min_speed());
	this->besch = besch;
	zustand = 0;
	blockend = false;
	step_frequency = 0;
	set_dir(dir);
}


roadsign_t::~roadsign_t()
{
	weg_t *weg = welt->lookup(gib_pos())->gib_weg(weg_t::strasse);
	if(weg  &&  besch->is_single_way()) {
		// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
		dynamic_cast<strasse_t *>(weg)->setze_ribi_maske(ribi_t::keine);
	} else {
		DBG_MESSAGE("roadsign_t::~roadsign_t()","roadsign_t %p was deleted but ground was not a road!");
	}
}



void roadsign_t::set_dir(ribi_t::ribi dir)
{
	this->dir = dir;
	if(besch->is_single_way()) {
		weg_t *weg = welt->lookup(gib_pos())->gib_weg(weg_t::strasse);
		if(weg) {
			// Weg wieder freigeben, wenn das Signal nicht mehr da ist.
			weg->setze_ribi_maske(dir);
DBG_MESSAGE("roadsign_t::set_dir()","ribi %i",dir);
		}
	}
	calc_bild();
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void roadsign_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);
}


void roadsign_t::calc_bild()
{
	setze_bild(0,-1);
	after_bild = -1;

	if(dir&ribi_t::nord) {
		after_bild = besch->gib_bild_nr(1+zustand*4);
	}

	if(dir&ribi_t::sued) {
		setze_bild(0, besch->gib_bild_nr(0+zustand*4));
	}

	if(dir&ribi_t::ost) {
		setze_bild(0, besch->gib_bild_nr(2+zustand*4));
	}

	if(dir&ribi_t::west) {
		after_bild = besch->gib_bild_nr(3+zustand*4);
	}
}


void
roadsign_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);

	file->rdwr_char(blockend, " ");
	file->rdwr_char(zustand, " ");
	file->rdwr_char(dir, "\n");

	if(file->is_saving()) {
		const char *s = besch->gib_name();
		file->rdwr_str(s, "N");
	}

	if(file->is_loading()) {
		char bname[128];
		file->rd_str_into(bname, "N");

		besch = (const roadsign_besch_t *)table.get(bname);
		if(!besch) {
			DBG_MESSAGE("roadsign_t::rwdr", "description %s for tree at %d,%d not found, will be removed!", bname, gib_pos().x, gib_pos().y);
		}
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
	calc_bild();
}

bool roadsign_t::alles_geladen()
{
	if(liste.count() == 0) {
		DBG_MESSAGE("roadsign_t", "No signs found - feature disabled");
	}
	return true;
}

bool roadsign_t::register_besch(roadsign_besch_t *besch)
{
	table.put(besch->gib_name(), besch);
	liste.append(besch);
DBG_DEBUG( "roadsign_t::register_besch()","%s", besch->gib_name() );
	return true;
}


/**
 * Fill menu with icons of given stops from the list
 * @author Hj. Malthaner
 */
void roadsign_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
	int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
	int sound_ok,
	int sound_ko,
	int cost)
{
DBG_DEBUG("roadsign_t::fill_menu()","maximum %i",liste.count());
	for(  int i=0;  i<liste.count();  i++  ) {
		char buf[128];
		const roadsign_besch_t *besch=liste.at(i);

DBG_DEBUG("roadsign_t::fill_menu()","try at pos %i to add %s(%p)",i,besch->gib_name(),besch);
		if(besch->gib_cursor()->gib_bild_nr(1) != -1) {
			// only add items with a cursor
DBG_DEBUG("roadsign_t::fill_menu()","at pos %i add %s",i,besch->gib_name());
			sprintf(buf, "%s, %d$",translator::translate(besch->gib_name()),CST_ROADSIGN);

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
