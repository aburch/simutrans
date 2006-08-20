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

#include "../tpl/vector_tpl.h"
#include "../simdings.h"

class KNode;
class karte_t;
class Stack;
class route_block_tester_t;

template <class T> class vector_tpl;

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
    bool intern_calc_route(koord3d start, koord3d ziel, fahrer_t *fahr, const uint32 max_kmh);


    vector_tpl <koord3d> route;           // Die Koordinaten fuer die Fahrtroute

    karte_t *welt;

#if 0
    /**
     * Hajo: 22-Jun-03 route_t was expanded to be able to check
     * blockings (i.e. red signals) while searching a path. This flag
     * triggers check for blockings. Also this flag modifies teh success
     * criteria: if the destination is a station, it is suffcient to find
     * any path to that station, not the precise coordinate.
     */
    // only used by drivables
    route_block_tester_t *block_tester;
#endif

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
    const koord3d & position_bei(const unsigned int n) const { return route.get(n); };


    /**
     * @return letzer index in der Koordinatenliste
     * @author Hj. Malthaner
     */
    int gib_max_n() const { return ((signed int)route.get_count())-1; };



    /**
     * kopiert positionen und hoehen von einer anderen route
     * @author Hj. Malthaner
     */
    void kopiere(const route_t *route);


    /**
     * kopiert positionen und hoehen von einer anderen route
     * @author prissi
     */
    void append(const route_t *route);


    /**
     * fügt k vorne in die route ein
     * @author Hj. Malthaner
     */
    void insert(koord3d k);

    /**
     * fügt k hinten in die route ein
     * @author prissi
     */
    void append(koord3d k);

    /**
     * removes all tiles from the route
     * @author prissi
     */
    void clear() { route.clear(); };

    /**
     * removes all tiles behind this position
     * @author prissi
     */
    void remove_koord_from(int);

	/* find the route to an unknow location (where tile_found becomes true)
	 * the max_depth is the maximum length of a route
	 * @author prissi
	 */
	bool find_route(karte_t *w, const koord3d start, fahrer_t *fahr, const uint32 max_khm, uint8 start_dir, uint32 max_depth );

    /**
     * berechnet eine route von start nach ziel.
     * @author Hj. Malthaner
     */
    bool calc_route(karte_t *welt, koord3d start, koord3d ziel, fahrer_t *fahr, const uint32 max_speed_kmh);

#if 0
    // only used by drivables
    bool calc_unblocked_route(karte_t *w,
			      const koord3d ziel,
			      const koord3d start,
			      fahrer_t *fahr,
			      route_block_tester_t *tester);

	bool find_path(karte_t * welt, const koord3d start, fahrer_t * fahr, ding_t::typ typ);
#endif

    /**
     * Lädt/speichert eine Route
     * @author V. Meyer
     */
    void rdwr(loadsave_t *file);

	bool is_ding_there(karte_t * welt, const koord3d pos, ding_t::typ typ);
};

#endif
