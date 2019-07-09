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
class test_driver_t;
class grund_t;

/**
 * Route, e.g. for vehicles
 *
 * @author Hj. Malthaner
 * @date 15.01.00
 */
class route_t
{
private:
	/**
	 * The actual route search
	 * @author Hj. Malthaner
	 */
	bool intern_calc_route(karte_t *w, koord3d start, koord3d ziel, test_driver_t *tdriver, const sint32 max_kmh, const uint32 max_cost);

	koord3d_vector_t route;           // The coordinates for the vehicle route

	void postprocess_water_route(karte_t *welt);

	static inline uint32 calc_distance( const koord3d &p1, const koord3d &target )
	{
		return koord_distance(p1, target);
	}

public:
	typedef enum { no_route=0, valid_route=1, valid_route_halt_too_short=3 } route_result_t;

	/**
	 * Nodes for A* or breadth-first search
	 */
	class ANode {
	public:
		ANode * parent;
		const grund_t* gr;
		uint32 f;        ///< heuristic for cost to reach target
		uint32 g;        ///< cost to reach this tile
		uint8 dir;       ///< driving direction
		uint8 ribi_from; ///< we came from this direction
		uint16 count;    ///< length of route up to here
		uint8 jps_ribi;  ///< extra ribi mask for jump-point search

		/// sort nodes first with respect to f, then with respect to g
		inline bool operator <= (const ANode &k) const { return f==k.f ? g<=k.g : f<=k.f; }
	};

	static ANode *nodes;
	static uint32 MAX_STEP;
#ifdef DEBUG
	// a semaphore, since we only have a single version of the array in memory
	static bool node_in_use;
	static void GET_NODE() {if(node_in_use){ dbg->fatal("GET_NODE","called while list in use");} node_in_use =1; }
	static void RELEASE_NODE() {if(!node_in_use){ dbg->fatal("RELEASE_NODE","called while list free");} node_in_use =0; }
#else
	static void GET_NODE() {}
	static void RELEASE_NODE() {}
#endif

	const koord3d_vector_t &get_route() const { return route; }

	void rotate90( sint16 y_size ) { route.rotate90( y_size ); };


	bool is_contained(const koord3d &k) const { return route.is_contained(k); }

	uint32 index_of(const koord3d &k) const { return (uint32)(route.index_of(k)); }

	/**
	 * @return Coordinate at index @p n.
	 * @author Hj. Malthaner
	 */
	const koord3d& at(const uint16 n) const { return route[n]; }

	koord3d const& front() const { return route.front(); }

	koord3d const& back() const { return route.back(); }

	uint32 get_count() const { return route.get_count(); }

	bool empty() const { return route.get_count()<2; }

	/**
	 * Appends the other route to ours.
	 * @author prissi
	 */
	void append(const route_t *route);

	/**
	 * Inserts @p k at position 0.
	 * @author Hj. Malthaner
	 */
	void insert(koord3d k);

	/**
	 * Appends position @p k.
	 * @author prissi
	 */
	inline void append(koord3d k) { route.append(k); }

	/**
	 * removes all tiles from the route
	 * @author prissi
	 */
	void clear() { route.clear(); }

	/**
	 * Removes all tiles at indices >@p i.
	 * @author prissi
	 */
	void remove_koord_from(uint32 i);

	/**
	 * Appends a straight line to the @p target.
	 * Will return fals if fails
	 * @author prissi
	 */
	bool append_straight_route( karte_t *w, koord3d target);

	/**
	 * Finds route to a location, where @p tdriver->is_target becomes true.
	 * @param max_depth is the maximum length of a route
	 * @author prissi
	 */
	bool find_route(karte_t *w, const koord3d start, test_driver_t *tdriver, const uint32 max_khm, uint8 start_dir, uint32 max_depth, bool coupling = false);

	/**
	 * Calculates the route from @p start to @p target
	 * @author Hj. Malthaner
	 */
	route_result_t calc_route(karte_t *welt, koord3d start, koord3d target, test_driver_t *tdriver, const sint32 max_speed_kmh, sint32 max_tile_len );

	/**
	 * Load/Save of the route.
	 * @author V. Meyer
	 */
	void rdwr(loadsave_t *file);
};

#endif
