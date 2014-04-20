/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../display/simimg.h"

#include "../simtools.h"
#include "../boden/grund.h"
#include "../dataobj/loadsave.h"

#include "simpeople.h"
#include "../besch/fussgaenger_besch.h"

static uint32 const strecke[] = { 6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000 };

static weighted_vector_tpl<const fussgaenger_besch_t*> liste;
stringhashtable_tpl<const fussgaenger_besch_t *> fussgaenger_t::table;


static bool compare_fussgaenger_besch(const fussgaenger_besch_t* a, const fussgaenger_besch_t* b)
{
	// sort pedestrian objects descriptors by their name
	return strcmp(a->get_name(), b->get_name())<0;
}


bool fussgaenger_t::register_besch(const fussgaenger_besch_t *besch)
{
	if(  table.remove(besch->get_name())  ) {
		dbg->warning( "fussgaenger_besch_t::register_besch()", "Object %s was overlaid by addon!", besch->get_name() );
	}
	table.put(besch->get_name(), besch);
	return true;
}


bool fussgaenger_t::alles_geladen()
{
	liste.resize(table.get_count());
	if (table.empty()) {
		DBG_MESSAGE("fussgaenger_t", "No pedestrians found - feature disabled");
	}
	else {
		vector_tpl<const fussgaenger_besch_t*> temp_liste(0);
		FOR(stringhashtable_tpl<fussgaenger_besch_t const*>, const& i, table) {
			// just entered them sorted
			temp_liste.insert_ordered(i.value, compare_fussgaenger_besch);
		}
		FOR(vector_tpl<fussgaenger_besch_t const*>, const i, temp_liste) {
			liste.append(i, i->get_gewichtung());
		}
	}
	return true;
}


fussgaenger_t::fussgaenger_t(loadsave_t *file)
 : verkehrsteilnehmer_t()
{
	rdwr(file);
	if(besch) {
		welt->sync_add(this);
	}
}


fussgaenger_t::fussgaenger_t(grund_t *gr) :
	verkehrsteilnehmer_t(gr, simrand(65535)),
	besch(pick_any_weighted(liste))
{
	time_to_life = pick_any(strecke);
	calc_bild();
}


fussgaenger_t::~fussgaenger_t()
{
	if(  time_to_life>0  ) {
		welt->sync_remove( this );
	}
}


void fussgaenger_t::calc_bild()
{
	set_bild(besch->get_bild_nr(ribi_t::get_dir(get_fahrtrichtung())));
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
		file->rdwr_str(s, lengthof(s));
		besch = table.get(s);
		// unknown pedestrian => create random new one
		if(besch == NULL  &&  !liste.empty()  ) {
			besch = pick_any_weighted(liste);
		}
	}

	if(file->get_version()<89004) {
		time_to_life = pick_any(strecke);
	}
}



// create a number (anzahl) of pedestrians (if possible)
void fussgaenger_t::erzeuge_fussgaenger_an(const koord3d k, int &anzahl)
{
	if (liste.empty()) {
		return;
	}

	grund_t* bd = welt->lookup(k);
	if (bd) {
		const weg_t* weg = bd->get_weg(road_wt);

		// we do not start on crossings (not overrunning pedestrians please
		if (weg && ribi_t::is_twoway(weg->get_ribi_unmasked())) {
			// we create maximal 4 pedestrians here for performance reasons
			for (int i = 0; i < 4 && anzahl > 0; i++) {
				fussgaenger_t* fg = new fussgaenger_t(bd);
				bool ok = bd->obj_add(fg) != 0;	// 256 limit reached
				if (ok) {
					if (i > 0) {
						// walk a little
						fg->sync_step( (i & 3) * 64 * 24);
					}
					welt->sync_add(fg);
					anzahl--;
				}
				else {
					// delete it, if we could not put it on the map
					fg->set_flag(obj_t::not_on_map);
					// do not try to delete it from sync-list
					fg->time_to_life = 0;
					delete fg;
					return; // it is pointless to try again
				}
			}
		}
	}
}


bool fussgaenger_t::sync_step(uint32 delta_t)
{
	time_to_life -= delta_t;

	weg_next += 128*delta_t;
	weg_next -= fahre_basis( weg_next );
	return time_to_life>0;
}


grund_t* fussgaenger_t::hop()
{
	grund_t *from = welt->lookup(pos_next);
	if(!from) {
		time_to_life = 0;
		return NULL;
	}

	// find the allowed directions
	const weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// no road anymore: destroy it
		time_to_life = 0;
		return NULL;
	}
	// new target
	grund_t *to = NULL;
	// ribi opposite to current direction
	ribi_t::ribi gegenrichtung = ribi_t::rueckwaerts( get_fahrtrichtung() );
	// all possible directions
	ribi_t::ribi ribi = weg->get_ribi_unmasked() & (~gegenrichtung);
	// randomized offset
	const uint8 offset = (ribi > 0  &&  ribi_t::ist_einfach(ribi)) ? 0 : simrand(4);

	for(uint r = 0; r < 4; r++) {
		ribi_t::ribi const test_ribi = ribi_t::nsow[ (r+offset) & 3];

		if(  (ribi & test_ribi)!=0  &&  from->get_neighbour(to, road_wt, test_ribi) )	{
			// this is our next target
			break;
		}
	}

	if (to) {
		pos_next = to->get_pos();
		fahrtrichtung = calc_set_richtung(get_pos(), pos_next);
	}
	else {
		// turn around
		fahrtrichtung = gegenrichtung;
		dx = -dx;
		dy = -dy;
		pos_next = get_pos();
		// .. but this looks ugly, so disappear
		time_to_life = 0;
	}

	verlasse_feld();
	set_pos(from->get_pos());
	calc_bild();
	// no need to call betrete_feld();
	from->obj_add(this);
	return from;
}
