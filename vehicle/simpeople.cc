/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../simimg.h"

#include "../simtools.h"
#include "../simmem.h"
#include "../boden/grund.h"
#include "../dataobj/loadsave.h"

#include "simpeople.h"
#include "../besch/fussgaenger_besch.h"

int fussgaenger_t::count = 0;

uint32 fussgaenger_t::strecke[] = {6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000};

static weighted_vector_tpl<const fussgaenger_besch_t*> liste;
stringhashtable_tpl<const fussgaenger_besch_t *> fussgaenger_t::table;


bool fussgaenger_t::register_besch(const fussgaenger_besch_t *besch)
{
	liste.append(besch, besch->get_gewichtung(), 1);
	table.put(besch->get_name(), besch);

	return true;
}

bool fussgaenger_t::laden_erfolgreich()
{
	if (liste.empty()) {
		DBG_MESSAGE("fussgaenger_t", "No pedestrians found - feature disabled");
	}
	return true ;
}


fussgaenger_t::fussgaenger_t(karte_t *welt, loadsave_t *file)
 : verkehrsteilnehmer_t(welt)
{
	rdwr(file);
	count ++;
	if(besch) {
		welt->sync_add(this);
	}
}


fussgaenger_t::fussgaenger_t(karte_t *welt, koord3d pos)
 : verkehrsteilnehmer_t(welt, pos)
{
	besch = liste.at_weight(simrand(liste.get_sum_weight()));
	time_to_life = strecke[count & 7];
	count ++;
	calc_bild();
}



void
fussgaenger_t::calc_bild()
{
	if(welt->lookup(get_pos())->ist_im_tunnel()) {
		set_bild(IMG_LEER);
	}
	else {
		set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung())));
	}
}



void fussgaenger_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fussgaenger_t" );

	verkehrsteilnehmer_t::rdwr(file);

	if(!file->is_loading()) {
		const char *s = besch->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s,256);
		besch = table.get(s);
		// unknow pedestrian => create random new one
		if(besch == NULL) {
			besch = liste.at_weight(simrand(liste.get_sum_weight()));
		}
	}

	if(file->get_version()<89004) {
		time_to_life = strecke[count & 7];
	}
}



// create anzahl pedestrains (if possible)
void fussgaenger_t::erzeuge_fussgaenger_an(karte_t *welt, const koord3d k, int &anzahl)
{
	if (liste.empty()) return;

	const grund_t* bd = welt->lookup(k);
	if (bd) {
		const weg_t* weg = bd->get_weg(road_wt);

		// we do not start on crossings (not overrunning pedestrians please
		if (weg && ribi_t::is_twoway(weg->get_ribi_unmasked())) {
			for (int i = 0; i < 4 && anzahl > 0; i++) {
				fussgaenger_t* fg = new fussgaenger_t(welt, k);
				bool ok = welt->lookup(k)->obj_add(fg) != 0;	// 256 limit reached
				if (ok) {
					for (int i = 0; i < (fussgaenger_t::count & 3); i++) {
						fg->sync_step(64 * 24);
					}
					welt->sync_add(fg);
					anzahl--;
				} else {
					// delete it, if we could not put them on the map
					fg->set_flag(ding_t::not_on_map);
					delete fg;
				}
			}
		}
	}
}



bool
fussgaenger_t::sync_step(long delta_t)
{
	if(time_to_life<0) {
		// remove obj
//DBG_MESSAGE("verkehrsteilnehmer_t::sync_step()","stopped");
  		return false;
	}

	time_to_life -= delta_t;

	weg_next += 128*delta_t;
	weg_next -= fahre_basis( weg_next );
	return true;
}
