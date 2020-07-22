/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef DATAOBJ_ROUTE_H
#define DATAOBJ_ROUTE_H


#include "../simdebug.h"

#include "../dataobj/koord3d.h"

#include "../tpl/vector_tpl.h"

#include "../utils/simthread.h"

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

	enum overweight_type { not_overweight, cannot_route, slowly_only };
public:
	typedef enum { no_route = 0, valid_route = 1, valid_route_halt_too_short = 3, route_too_complex = 4, no_control_tower = 5 } route_result_t;

	enum find_route_flags { none, private_car_checker, choose_signal, simple_cost };

private:

	/**
	 * The actual route search
	 * @author Hj. Malthaner
	 */
	route_result_t intern_calc_route(karte_t *w, koord3d start, koord3d ziel, test_driver_t* const tdriver, const sint32 max_kmh, const sint64 max_cost, const uint32 axle_load, const uint32 convoy_weight, bool is_tall, const sint32 tile_length, const koord3d avoid_tile, uint8 start_dir = ribi_t::all, find_route_flags flags = none);

protected:
	koord3d_vector_t route;           // The coordinates for the vehicle route

	// Bernd Gabriel, Mar 10, 2010: weight limit info
	uint32 max_axle_load;
	uint32 max_convoy_weight;

	void postprocess_water_route(karte_t *welt);

	static inline uint32 calc_distance( const koord3d &p1, const koord3d &target )
	{
		return shortest_distance(p1.get_2d(), target.get_2d());
	}

public:

	// Constructor: set axle load and convoy weight to maximum possible value
	route_t() : max_axle_load(0xFFFFFFFFl), max_convoy_weight(0xFFFFFFFFl) {};


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

private:
	static const uint8 MAX_NODES_ARRAY = 2;
	static thread_local ANode *_nodes[MAX_NODES_ARRAY];
	static thread_local bool _nodes_in_use[MAX_NODES_ARRAY]; // semaphores, since we only have few nodes arrays in memory
public:
	static thread_local uint32 MAX_STEP;
	static thread_local uint32 max_used_steps;
	static void INIT_NODES(uint32 max_route_steps, const koord &world_size);
	static uint8 GET_NODES(ANode **nodes);
	static void RELEASE_NODES(uint8 nodes_index);
	static void TERM_NODES(void* args = NULL);

	static bool suspend_private_car_routing;

	const koord3d_vector_t &get_route() const { return route; }

	uint32 get_max_axle_load() const { return max_axle_load; }

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
	 * Removes all tiles before
	 * this position.
	 */
	void remove_koord_to(uint32 i);

	/**
	 * Appends a straight line to the @p target.
	 * Will return false if fails
	 * @author prissi
	 */
	bool append_straight_route( karte_t *w, koord3d target);

	/**
	 * Finds route to a location, where @p tdriver-> is_target becomes true.
	 * @param max_depth is the maximum length of a route
	 * @author prissi
	 */
	bool find_route(karte_t *w, const koord3d start, test_driver_t *tdriver, const uint32 max_khm, uint8 start_dir, uint32 axle_load, sint32 max_tile_len, uint32 total_weight, uint32 max_depth, bool is_tall, find_route_flags flags = none);

	/**
	 * Calculates the route from @p start to @p target
	 * @author Hj. Malthaner
	 */
	route_result_t calc_route(karte_t *welt, koord3d start, koord3d ziel, test_driver_t* const tdriver, const sint32 max_speed_kmh, const uint32 axle_load, bool is_tall, sint32 max_tile_len, const sint64 max_cost = SINT64_MAX_VALUE, const uint32 convoy_weight = 0, const koord3d avoid_tile = koord3d::invalid, uint8 direction = ribi_t::all, find_route_flags flags = none);

	/**
	 * Load/Save of the route.
	 * @author V. Meyer
	 */
	void rdwr(loadsave_t *file);
};

#endif
