/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_ROUTE_H
#define DATAOBJ_ROUTE_H


#include "../simdebug.h"

#include "../dataobj/koord3d.h"

#include "../tpl/vector_tpl.h"


class karte_t;
class test_driver_t;
class grund_t;


/**
 * Route, e.g. for vehicles
 */
class route_t
{
public:
	typedef uint16 index_t;

	static const index_t INVALID_INDEX = 0xFFFA;

private:
	/**
	 * The actual route search
	 */
	bool intern_calc_route(karte_t *w, koord3d start, koord3d ziel, test_driver_t *tdriver, const sint32 max_kmh, const uint32 max_cost);

	koord3d_vector_t route;           // The coordinates for the vehicle route

	void postprocess_water_route(karte_t *welt);

	static inline uint32 calc_distance( const koord3d &p1, const koord3d &target )
	{
		return koord_distance(p1, target);
	}

public:
	enum route_result_t {
		no_route                   = 0,
		valid_route                = 1,
		valid_route_halt_too_short = 3
	};

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

	void rotate90( sint16 y_size ) { route.rotate90( y_size ); }


	bool is_contained(const koord3d &k) const { return route.is_contained(k); }

	uint32 index_of(const koord3d &k) const { return (uint32)(route.index_of(k)); }

	/**
	 * @return Coordinate at index @p n.
	 */
	const koord3d& at(const uint16 n) const { return route[n]; }

	koord3d const& front() const { return route.front(); }

	koord3d const& back() const { return route.back(); }

	uint32 get_count() const { return route.get_count(); }

	bool empty() const { return route.get_count()<2; }

	/**
	 * Appends the other route to ours.
	 */
	void append(const route_t *route);

	/**
	 * Inserts @p k at position 0.
	 */
	void insert(koord3d k);

	/**
	 * Appends position @p k.
	 */
	inline void append(koord3d k) { route.append(k); }

	/**
	 * removes all tiles from the route
	 */
	void clear() { route.clear(); }

	/**
	 * Removes all tiles at indices >@p i.
	 */
	void remove_koord_from(uint32 i);

	/**
	 * Appends a straight line to the @p target.
	 * Will return fals if fails
	 */
	bool append_straight_route( karte_t *w, koord3d target);

	/**
	 * Finds route to a location, where @p tdriver->is_target becomes true.
	 * @param max_depth is the maximum length of a route
	 */
	bool find_route(karte_t *w, const koord3d start, test_driver_t *tdriver, const uint32 max_khm, uint8 start_dir, uint32 max_depth );

	/**
	 * Calculates the route from @p start to @p target
	 */
	route_result_t calc_route(karte_t *welt, koord3d start, koord3d target, test_driver_t *tdriver, const sint32 max_speed_kmh, sint32 max_tile_len );

	/**
	 * Load/Save of the route.
	 */
	void rdwr(loadsave_t *file);
};

#endif
