/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef route_h
#define route_h

#include "../simdebug.h"

#include "../dataobj/koord3d.h"

#include "../tpl/vector_tpl.h"

class karte_t;
class fahrer_t;
class grund_t;

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
	bool intern_calc_route(karte_t *w, koord3d start, koord3d ziel, fahrer_t *fahr, const sint32 max_kmh, const uint32 max_cost, const uint32 axle_load, const uint32 convoy_weight, const sint32 tile_length);

protected:
	koord3d_vector_t route;           // Die Koordinaten fuer die Fahrtroute - "The coordinates for the route" (Google)

private:

	// Bernd Gabriel, Mar 10, 2010: weight limit info
	uint32 max_axle_load;
	uint32 max_convoy_weight;
public:
	typedef enum { no_route=0, valid_route=1, valid_route_halt_too_short=3 } route_result_t;

	// this class saves the nodes during route searches
	class ANode {
	public:
		ANode * parent;
		const grund_t* gr;
		uint32  f, g;
		uint8 dir;
		uint8 ribi_from; /// we came from this direction
		uint16 count;

		// Constructor
		route_t() : max_axle_load(999), max_convoy_weight(999) {};

		inline bool operator <= (const ANode &k) const { return f==k.f ? g<=k.g : f<=k.f; }
#if defined(tpl_sorted_heap_tpl_h)
		inline bool operator == (const ANode &k) const { return f==k.f  &&  g==k.g; }
#endif
#if defined(tpl_HOT_queue_tpl_h)
		inline bool is_matching(const ANode &l) const { return gr==l.gr; }
		inline uint32 get_distance() const { return f; }
#endif
	};

// These will need to be made non-static if this is ever to be threaded.
private:
	static const uint8 MAX_NODES_ARRAY = 2;
	static ANode *_nodes[MAX_NODES_ARRAY]; 
	static bool _nodes_in_use[MAX_NODES_ARRAY]; // semaphores, since we only have few nodes arrays in memory
public:
	static uint32 MAX_STEP;
	static uint32 max_used_steps;
	static void INIT_NODES(uint32 max_route_steps, const koord &world_size);
	static uint8 GET_NODES(ANode **nodes); 
	static void RELEASE_NODES(uint8 nodes_index);
	static void TERM_NODES();

	static inline uint32 calc_distance( const koord3d &p1, const koord3d &p2 )
	{
		return (abs(p1.x-p2.x)+abs(p1.y-p2.y)+abs(p1.z-p2.z)/16);
	}

	const koord3d_vector_t &get_route() const { return route; }

	uint32 get_max_axle_load() const { return max_axle_load; }

	void rotate90( sint16 y_size ) { route.rotate90( y_size ); };

	void concatenate_routes(route_t* tail_route);

	bool is_contained(const koord3d &k) const { return route.is_contained(k); }

	uint32 index_of(const koord3d &k) const { return (uint32)(route.index_of(k)); }

	/**
	 * @return Koordinate an index n
	 * @author Hj. Malthaner
	 */
	const koord3d& position_bei(const uint16 n) const { return route[n]; }

	koord3d const& front() const { return route.front(); }

	koord3d const& back() const { return route.back(); }

	uint32 get_count() const { return route.get_count(); }

	bool empty() const { return route.get_count()<2; }

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
	inline void append(koord3d k) { route.append(k); }

	/**
	 * removes all tiles from the route
	 * @author prissi
	 */
	void clear() { route.clear(); }

	/**
	 * removes all tiles behind this position
	 * @author prissi
	 */
	void remove_koord_from(uint32);

	/**
	 * Appends a straight line from the last koord3d in route to the desired target.
	 * Will return false if fails
	 * @author prissi
	 */
	bool append_straight_route( karte_t *w, koord3d );

	/* find the route to an unknown location (where tile_found becomes true)
	* the max_depth is the maximum length of a route
	* @author prissi
	*/
	bool find_route(karte_t *w, const koord3d start, fahrer_t *fahr, const uint32 max_khm, uint8 start_dir, uint32 weight, uint32 max_depth, bool private_car_checker = false);

	/**
	 * berechnet eine route von start nach ziel.
	 * @author Hj. Malthaner
	 */
	route_result_t calc_route(karte_t *welt, koord3d start, koord3d ziel, fahrer_t *fahr, const sint32 max_speed_kmh, const uint32 axle_load, sint32 max_tile_len, const uint32 max_cost=0xFFFFFFFF, const uint32 convoy_weight = 0);

	/**
	 * Lädt/speichert eine Route
	 * @author V. Meyer
	 */
	void rdwr(loadsave_t *file);
};

#endif
