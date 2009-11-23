/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef __simbridge_h
#define __simbridge_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"

class bruecke_besch_t;
class grund_t;
class karte_t;                 // Hajo: 22-Nov-01: Added forward declaration
class spieler_t;               // Hajo: 22-Nov-01: Added forward declaration
class weg_besch_t;
class werkzeug_waehler_t;

/**
 * Baut Brücken. Brücken sollten nicht direct instanziiert werden
 * sondern immer vom brueckenbauer_t erzeugt werden.
 *
 * Es gibt keine Instanz - nur statische Methoden.
 *
 * @author V. Meyer
 */
class brueckenbauer_t {
private:

	brueckenbauer_t() {} // private -> no instance please


public:
	/*
	 * Grund bestimmen, auf dem die Brücke enden soll.
	 *
	 * @author V. Meyer
	 */
	static koord3d finde_ende(karte_t *welt, koord3d pos, koord zv, const bruecke_besch_t *besch, const char *&msg, bool ai_bridge=false );

	/*
	 * Brückenendpunkte bei Rampen werden auf flachem Grund gebaut und müssen daher genauer
	 * auf störende vorhandene Bauten überprüft werden.
	 *
	 * @author V. Meyer
	 */
	static bool ist_ende_ok(spieler_t *sp, const grund_t *gr);

	// built a ramp to change level
	static void baue_auffahrt(karte_t *welt, spieler_t *sp, koord3d pos, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch);

	// builds the bridge => checks should be done before
	static void baue_bruecke(karte_t *welt, spieler_t *sp, koord3d pos, koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch);

	/**
	 * Registers a new bridge type
	 * @author V. Meyer, Hj. Malthaner
	 */
	static void register_besch(bruecke_besch_t *besch);


	static bool laden_erfolgreich();


	static const bruecke_besch_t *get_besch(const char *name);

	// the main construction routine
	static const char *baue( karte_t *welt, spieler_t *sp, koord pos, const bruecke_besch_t *besch);

	/*
	 * Brückenlösch-Funktion
	 *
	 * @author V. Meyer
	 */
	static const char *remove(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t wegtyp);


	/**
	 * Find a matching bridge
	 * @author prissi
	 */
	static const bruecke_besch_t *find_bridge(const waytype_t wtyp, const uint32 min_speed,const uint16 time);

	/**
	 * Fill menu with icons of given waytype
	 * @author priss
	 */
	static void fill_menu(werkzeug_waehler_t *wzw, const waytype_t wtyp, sint16 sound_ok, const karte_t *welt);
};

#endif
