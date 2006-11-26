/*
 * weg.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


/*
 * weg.cc
 *
 * Basisklasse für Wege in Simutrans.
 *
 * 14.06.00 getrennt von simgrund.cc
 * Überarbeitet Januar 2001
 *
 * von Hj. Malthaner
 */

#include <stdio.h>

#include "weg.h"

#include "schiene.h"
#include "strasse.h"
#include "monorail.h"
#include "kanal.h"
#include "runway.h"


#include "../grund.h"
#include "../../simworld.h"
#include "../../simimg.h"
#include "../../simhalt.h"
#include "../../simdings.h"
#include "../../simplay.h"
#include "../../dings/roadsign.h"
#include "../../dings/signal.h"
#include "../../utils/cbuffer_t.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/loadsave.h"
#include "../../besch/weg_besch.h"
#include "../../besch/roadsign_besch.h"

#include "../../tpl/slist_tpl.h"

/**
 * Alle instantiierten Wege
 * @author Hj. Malthaner
 */
slist_tpl <weg_t *> alle_wege;



/**
 * Get list of all ways
 * @author Hj. Malthaner
 */
const slist_tpl <weg_t *> & weg_t::gib_alle_wege()
{
	return alle_wege;
}



// returns a way with matchin waytype
weg_t*
weg_t::alloc(waytype_t wt)
{
	weg_t *weg = NULL;
	switch(wt) {
		case track_wt:
			weg = new schiene_t(welt);
			break;
		case monorail_wt:
			weg = new monorail_t(welt);
			break;
		case road_wt:
			weg = new strasse_t(welt);
			break;
		case water_wt:
			weg = new kanal_t(welt);
			break;
		case air_wt:
			weg = new runway_t(welt);
			break;
		default:
			// keep compiler happy; should never reach here anyway
			assert(0);
			break;
	}
	return weg;
}




/**
 * Setzt die erlaubte Höchstgeschwindigkeit
 * @author Hj. Malthaner
 */
void weg_t::setze_max_speed(unsigned int s)
{
	max_speed = s;
}


/**
 * Setzt neue Beschreibung. Ersetzt alte Höchstgeschwindigkeit
 * mit wert aus Beschreibung.
 * @author Hj. Malthaner
 */
void weg_t::setze_besch(const weg_besch_t *b)
{
	besch = b;
	max_speed = besch->gib_topspeed();
}


/**
 * initializes statistic array
 * @author hsiegeln
 */
void weg_t::init_statistics()
{
	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
			statistics[month][type] = 0;
		}
	}
}


/**
 * Initializes all member variables
 * @author Hj. Malthaner
 */
void weg_t::init()
{
	set_flag(ding_t::is_wayding);
	ribi = ribi_maske = ribi_t::keine;
	max_speed = 450;
	besch = 0;
	init_statistics();
	alle_wege.insert(this);
	flags = 0;
}



weg_t::~weg_t()
{
	alle_wege.remove(this);
	spieler_t *sp=gib_besitzer();
	if(sp) {
		sp->add_maintenance( -besch->gib_wartung() );
	}
}



void weg_t::rdwr(loadsave_t *file)
{
	if(file->is_saving()) {
		// reading has been done by grund_t!
		file->wr_obj_id( gib_waytype() );
	}

	uint8 dummy8 = ribi;
	file->rdwr_byte(dummy8, "\n");
	if(file->is_loading()) {
		ribi = dummy8 & 15;	// before: high bits was maske
		ribi_maske = 0;	// maske will be restored by signal/roadsing
	}

	uint16 dummy16=max_speed;
	file->rdwr_short(dummy16, "\n");
	max_speed=dummy16;

	if(file->get_version()>=89000) {
		dummy8 = flags;
		file->rdwr_byte(dummy8,"f");
		if(file->is_loading()) {
			// all other flags are restored afterwards
			flags = dummy8 & HAS_WALKWAY;
		}
	}

	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
			long w=statistics[month][type];
			file->rdwr_long(w, "\n");
			statistics[month][type] = (sint16)w;
			// DBG_DEBUG("weg_t::rdwr()", "statistics[%d][%d]=%d", month, type, statistics[month][type]);
		}
	}
}


/**
 * Info-text für diesen Weg
 * @author Hj. Malthaner
 */
void weg_t::info(cbuffer_t & buf) const
{
	buf.append("\n");
	buf.append(translator::translate("Max. speed:"));
	buf.append(" ");
	buf.append(max_speed);
	buf.append(translator::translate("km/h\n"));

	buf.append(translator::translate("\nRibi (unmasked)"));
	buf.append(gib_ribi_unmasked());

	buf.append(translator::translate("\nRibi (masked)"));
	buf.append(gib_ribi());
	buf.append("\n");

	if(has_sign()) {
		buf.append(translator::translate("\nwith sign/signal\n"));
	}

	if(is_electrified()) {
		buf.append(translator::translate("\nelektrified"));
	}
	else {
		buf.append(translator::translate("\nnot elektrified"));
	}

#if 1
	char buffer[256];
	sprintf(buffer,translator::translate("convoi passed last\nmonth %i\n"), statistics[1][1]);
	buf.append(buffer);
  // Debug - output stats
#else
  buf.append("\n");
  for (int type=0; type<MAX_WAY_STATISTICS; type++) {
    for (int month=0; month<MAX_WAY_STAT_MONTHS; month++) {
      buf.append(statistics[month][type]);
      buf.append(" ");
    }
    buf.append("\n");
  }
#endif
  buf.append("\n");
}


/**
 * counts signals on this tile;
 * It would be enough for the signals to register and unreigister themselves, but this is more secure ...
 * @author prissi
 */
void
weg_t::count_sign()
{
	flags &= ~HAS_SIGN;
	const grund_t *gr=welt->lookup(gib_pos());
	if(gr) {
		const ding_t::typ type=(gib_waytype()==track_wt  ||  gib_waytype()==monorail_wt) ? ding_t::signal : ding_t::roadsign;
		for( uint8 i=0;  i<gr->gib_top();  i++  ) {
			ding_t *d=gr->obj_bei(i);
			// sign for us?
			if(d  &&  d->gib_typ()==type  &&  ((roadsign_t*)d)->gib_besch()->gib_wtyp()==gib_besch()->gib_wtyp()) {
				// here is a sign ...
				flags |= HAS_SIGN;
				return;
			}
		}
	}
}



void
weg_t::calc_bild()
{
	// V.Meyer: weg_position_t changed to grund_t::get_neighbour()
	grund_t *from = welt->lookup(gib_pos());
	grund_t *to;

	if(from==NULL  ||  besch==NULL  ||  from->ist_tunnel()) {
		setze_bild(IMG_LEER);
		return;
	}

	hang_t::typ hang = from->gib_weg_hang();
	if(hang != hang_t::flach) {
		setze_bild(besch->gib_hang_bild_nr(hang));
		return;
	}

	const ribi_t::ribi ribi = gib_ribi_unmasked();

	if(ribi_t::ist_kurve(ribi)) {
		ribi_t::ribi r1 = ribi_t::keine, r2 = ribi_t::keine;

		bool diagonal = false;
		switch(ribi) {
			case ribi_t::nordost:
				if(from->get_neighbour(to, gib_waytype(), koord::ost))
					r1 = to->gib_weg_ribi_unmasked(gib_waytype());
				if(from->get_neighbour(to, gib_waytype(), koord::nord))
					r2 = to->gib_weg_ribi_unmasked(gib_waytype());
				diagonal =
					(r1 == ribi_t::suedwest || r2 == ribi_t::suedwest) &&
					r1 != ribi_t::nordwest &&
					r2 != ribi_t::suedost;
			break;

			case ribi_t::suedost:
				if(from->get_neighbour(to, gib_waytype(), koord::ost))
					r1 = to->gib_weg_ribi_unmasked(gib_waytype());
				if(from->get_neighbour(to, gib_waytype(), koord::sued))
					r2 = to->gib_weg_ribi_unmasked(gib_waytype());
				diagonal =
					(r1 == ribi_t::nordwest || r2 == ribi_t::nordwest) &&
					r1 != ribi_t::suedwest &&
					r2 != ribi_t::nordost;
			break;

			case ribi_t::nordwest:
				if(from->get_neighbour(to, gib_waytype(), koord::west))
					r1 = to->gib_weg_ribi_unmasked(gib_waytype());
				if(from->get_neighbour(to, gib_waytype(), koord::nord))
					r2 = to->gib_weg_ribi_unmasked(gib_waytype());
				diagonal =
					(r1 == ribi_t::suedost || r2 == ribi_t::suedost) &&
					r1 != ribi_t::nordost &&
					r2 != ribi_t::suedwest;
			break;

			case ribi_t::suedwest:
				if(from->get_neighbour(to, gib_waytype(), koord::west))
					r1 = to->gib_weg_ribi_unmasked(gib_waytype());
				if(from->get_neighbour(to, gib_waytype(), koord::sued))
					r2 = to->gib_weg_ribi_unmasked(gib_waytype());
				diagonal =
					(r1 == ribi_t::nordost || r2 == ribi_t::nordost) &&
					r1 != ribi_t::suedost &&
					r2 != ribi_t::nordwest;
				break;
		}

		if(diagonal) {
			static int rekursion = 0;

			if(rekursion == 0) {
				rekursion++;
				for(int r = 0; r < 4; r++) {
					if(from->get_neighbour(to, gib_waytype(), koord::nsow[r])) {
						to->calc_bild();
					}
				}
				rekursion--;
			}

			image_id bild = besch->gib_diagonal_bild_nr(ribi);
			if(bild != IMG_LEER) {
				setze_bild(bild);
				return;
			}
		}
	}

	setze_bild(besch->gib_bild_nr(ribi));
}



/**
 * new month
 * @author hsiegeln
 */
void weg_t::neuer_monat()
{
	for (int type=0; type<MAX_WAY_STATISTICS; type++) {
		for (int month=MAX_WAY_STAT_MONTHS-1; month>0; month--) {
			statistics[month][type] = statistics[month-1][type];
		}
		statistics[0][type] = 0;
	}
}



// correct speed and maitainace
void weg_t::laden_abschliessen()
{
	spieler_t *sp=gib_besitzer();
	if(sp  &&  besch) {
		sp->add_maintenance( besch->gib_wartung() );
	}
}
