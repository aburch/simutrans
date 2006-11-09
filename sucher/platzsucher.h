/*
 * platzsucher.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Author: V. Meyer
 */

#ifndef __PLATZSUCHER_H
#define __PLATZSUCHER_H

#if _MSC_VER > 1000
#pragma once
#endif

#include "../dataobj/koord.h"
class karte_t;

/**
 * pos_liste_t:
 *
 * Liefert nach wachsender Entfernung von (0, 0) sortiert alle Koordinaten
 * mit x und y im Bereich [-max_xy;max_xy] außer (0, 0) selber.
 * (0, 0) wird als Endekenzeichen verwendet.
 *
 * @author V. Meyer
 */
class pos_liste_t {
	sint16 max_radius;
	sint16 *spalten;
	sint16 radius;
	sint16 reihe;
	sint16 quadrant;

	sint16 suche_beste_reihe();

public:
	/**
	* constructor:
	*
	* @param max_xy (Maximalwert für x und y-Position)
	*
	* @author V. Meyer
	*/
	pos_liste_t(sint16 max_xy);
	virtual ~pos_liste_t();

	void neu_starten();
	bool gib_pos(koord &k);
	virtual bool gib_naechste_pos(koord &k);
};


/**
 * pos_liste_wh_t:
 *
 * Erweiterte Version von pos_liste_t. Liefert die umliegenden Positionen für
 * einen Bereich der Größe h mal w.
 * (0, 0) wird wieder als Endekenzeichen verwendet.
 *
 * @author V. Meyer
 */
class pos_liste_wh_t : public pos_liste_t {
	sint16 b;
	sint16 h;

	sint16 dx;
	sint16 dy;
public:
	pos_liste_wh_t(sint16 max_radius, sint16 b, sint16 h);
	~pos_liste_wh_t() {}

	void neu_starten(sint16 b, sint16 h);
	void neu_starten() { pos_liste_t::neu_starten(); }

	bool gib_naechste_pos(koord &k);
};

/**
 * platzsucher_t:
 *
 * ...
 *
 * @author V. Meyer
 */
class platzsucher_t {
protected:
	karte_t *welt;
	sint16 b;
	sint16 h;
	climate_bits cl;

	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const;

	virtual bool ist_feld_ok(koord pos, koord d, climate_bits cl) const;

	bool ist_randfeld(koord d) const;
	platzsucher_t(karte_t *welt) { this->welt = welt; }
	virtual ~platzsucher_t() {}
public:
	koord suche_platz(koord start, sint16 b, sint16 h, climate_bits cl, bool *r = NULL);
};

#endif
