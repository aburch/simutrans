/*
 * route.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#ifndef route_h
#define route_h

#ifndef koord3d_h
#include "koord3d.h"
#endif

#ifndef simdebug_h
#include "../simdebug.h"
#endif

#ifndef fahrer_h
#include "../ifc/fahrer.h"
#endif

#include "../simdings.h"

class KNode;
class karte_t;
class Stack;
class route_block_tester_t;

/**
 * Routen, zB für Fahrzeuge
 *
 * @author Hj. Malthaner
 * @date 15.01.00
 */
class route_t
{
private:

    /**
     * Die eigentliche Routensuche
     * @author Hj. Malthaner
     */
    bool intern_calc_route(koord3d start, koord3d ziel, fahrer_t *fahr);


    /**
     * Während der Routensuche werden Felder markiert. Diese Methode
     * entfernt die markierungen wieder
     * @author Hj. Malthaner
     */
    void entferne_markierungen();


    koord3d route[512];           // Die Koordinaten fuer die Fahrtroute

    int max_n;

    karte_t *welt;


    /**
     * Hajo: 22-Jun-03 route_t was expanded to be able to check
     * blockings (i.e. red signals) while searching a path. This flag
     * triggers check for blockings. Also this flag modifies teh success
     * criteria: if the destination is a station, it is suffcient to find
     * any path to that station, not the precise coordinate.
     */
    route_block_tester_t *block_tester;

public:


    /**
     * Konstruktor, legt eine leere Route an.
     * @author Hj. Malthaner
     */
    route_t();


    /**
     * @return Koordinate an index n
     * @author Hj. Malthaner
     */
    const koord3d & position_bei(const unsigned int n) const {
	if(max_n < (signed int)n) {
	    dbg->warning("route_t::position_bei()", "n (%d) > max_n (%d)", n, max_n);
	    return route[max_n > 0 ? max_n : 0];
	}
	return route[n];
    };


    /**
     * @return letzer index in der Koordinatenliste
     * @author Hj. Malthaner
     */
    inline int gib_max_n() const {return max_n;};


    /**
     * kopiert positionen und hoehen von einer anderen route
     * @author Hj. Malthaner
     */
    void kopiere(const route_t *route);


    /**
     * fügt k vorne in die route ein
     * @author Hj. Malthaner
     */
    void insert(koord3d k);


    /**
     * berechnet eine route von start nach ziel.
     * @author Hj. Malthaner
     */
    bool calc_route(karte_t *welt, koord3d start, koord3d ziel, fahrer_t *fahr);

    bool calc_unblocked_route(karte_t *w,
			      const koord3d ziel,
			      const koord3d start,
			      fahrer_t *fahr,
			      route_block_tester_t *tester);

    /**
     * Lädt/speichert eine Route
     * @author V. Meyer
     */
    void rdwr(loadsave_t *file);

		bool is_ding_there(karte_t * welt, const koord3d pos, ding_t::typ typ);

		bool find_path(karte_t * welt, const koord3d start, fahrer_t * fahr, ding_t::typ typ);
};

#endif
