/*
 * simbridge.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef __simbridge_h
#define __simbridge_h

#include "../simtypes.h"
#include "../dataobj/koord.h"
#include "../dataobj/koord3d.h"
#include "../boden/wege/weg.h"

// Hajo: these are needed to build the menu entries
#include "../gui/werkzeug_parameter_waehler.h"
#include "../besch/skin_besch.h"
#include "../dataobj/translator.h"

class bruecke_besch_t;
class tabfileobj_t;
class karte_t;                 // Hajo: 22-Nov-01: Added forward declaration
class spieler_t;               // Hajo: 22-Nov-01: Added forward declaration
class grund_t;

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

    /*
     * Grund bestimmen, auf dem die Brücke enden soll.
     *
     * @author V. Meyer
     */
    static koord3d finde_ende(karte_t *welt, koord3d pos, koord zv, weg_t::typ wegtyp);

    /*
     * Brückenendpunkte bei Rampen werden auf flachem Grund gebaut und müssen daher genauer
     * auf störende vorhandene Bauten überprüft werden.
     *
     * @author V. Meyer
     */
    static bool ist_ende_ok(spieler_t *sp, const grund_t *gr);


    brueckenbauer_t() {} // private -> no instance please


public:

    /*
     * Baut Anfang oder Ende der Brücke.
     * (also used for monorail ramps ...)
     * @author V. Meyer
     */
    static void baue_auffahrt(karte_t *welt, spieler_t *sp, koord3d pos, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch);


    /*
     * Baut die Brücke wirklich, vorher sollte alles gerprüft sein.
     *
     * @author V. Meyer
     */
    static bool baue_bruecke(karte_t *welt, spieler_t *sp, koord3d pos, koord3d end, koord zv, const bruecke_besch_t *besch, const weg_besch_t *weg_besch);


    /**
     * Registers a new bridge type
     * @author V. Meyer, Hj. Malthaner
     */
    static void register_besch(const bruecke_besch_t *besch);


    static bool laden_erfolgreich();


    static const bruecke_besch_t *gib_besch(const char *name);


    /*
     * Brückenbau-Funktion - Prototyp passend für den werkzeugwaehler_t.
     * param ist ein Zeiger auf eine Brückenbeshreibung (besch_t).
     *
     * @author V. Meyer
     */
    static int baue(spieler_t *sp, karte_t *welt, koord pos, value_t param);


    /*
     * Brückenbau für die KI - chosse chepest fast enough
     * @author V. Meyer
     */
    static int baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp,int top_speed);

    /*
     * Brückenbau für die KI - der ist der Brückenstil egal.
     * Vorerst greifen wir den ersten passenden Stil - Zufallswahl könnte man ergänzen.
     *
     * @author V. Meyer
     */
    static int baue(spieler_t *sp, karte_t *welt, koord pos, weg_t::typ wegtyp);

    /*
     * Brückenlösch-Funktion
     *
     * @author V. Meyer
     */
    static const char *remove(karte_t *welt, spieler_t *sp, koord3d pos, weg_t::typ wegtyp);


	/**
	 * Find a matching bridge
	 * @author prissi
	 */
	static const bruecke_besch_t *
	find_bridge(const weg_t::typ wtyp, const uint32 min_speed,const uint16 time);

    /**
     * Fill menu with icons of given waytype
     * @author priss
     */
    static void fill_menu(werkzeug_parameter_waehler_t *wzw,
        const weg_t::typ wtyp,
        const int sound_ok, const int sound_ko,const uint16 time);
};

#endif
