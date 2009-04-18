/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Helper for all AIs
 */

#ifndef _AI_H
#define _AI_H

#include "simplay.h"

#include "../sucher/bauplatz_sucher.h"

class karte_t;
class ware_besch_t;

/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class ai_bauplatz_mit_strasse_sucher_t : public bauplatz_sucher_t  {
public:
	ai_bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t(welt) {}
	bool strasse_bei(sint16 x, sint16 y) const;
	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const;
};


// AI helper functions
class ai_t : public spieler_t
{
public:
	ai_t(karte_t *wl, uint8 nr) : spieler_t( wl, nr ) {}

	// return true, if there is already a connection
	bool is_connected(const koord star_pos, const koord end_pos, const ware_besch_t *wtyp) const;

	// prepares a general tool just like a human player work do
	bool init_general_tool( int tool, const char *param );

	// calls a general tool just like a human player work do
	bool call_general_tool( int tool, koord k, const char *param );

	// find space for stations
	bool suche_platz(koord pos, koord &size, koord *dirs) const;
	bool suche_platz(koord &start, koord &size, koord target, koord off);

	// removes building markers
	void clean_marker( koord place, koord size );
	void set_marker( koord place, koord size );

	/**
	 * Find the first water tile using line algorithm von Hajo
	 * start MUST be on land!
	 **/
	koord find_shore(koord start, koord end) const;
	bool find_harbour(koord &start, koord &size, koord target);

	bool built_update_headquarter();

	// builds a round between those two places or returns false
	bool create_simple_road_transport(koord platz1, koord size1, koord platz2, koord size2, const weg_besch_t *road );
};

#endif
