/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_SIMVEHICLE_H
#define VEHICLE_SIMVEHICLE_H


#include <limits>
#include <string>
#include "../simtypes.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../halthandle_t.h"
#include "../convoihandle_t.h"
#include "../ifc/simtestdriver.h"
#include "../boden/grund.h"
#include "../descriptor/vehicle_desc.h"
#include "../vehicle/overtaker.h"
#include "../tpl/slist_tpl.h"
#include "../tpl/array_tpl.h"
#include "../dataobj/route.h"


#include "../tpl/fixed_list_tpl.h"

class convoi_t;
class schedule_t;
class signal_t;
class ware_t;
class schiene_t;
class strasse_t;
//class karte_ptr_t;

// for aircraft:
// length of the holding pattern.
#define HOLDING_PATTERN_LENGTH 16
// offset of end tile of the holding pattern before touchdown tile.
#define HOLDING_PATTERN_OFFSET 3
/*----------------------- Movables ------------------------------------*/

class traffic_vehicle_t
{
	private:
		sint64 time_at_last_hop; // ticks
		uint32 dist_travelled_since_last_hop; // yards
		virtual uint32 get_max_speed() { return 0; } // returns y/t
	protected:
		inline void reset_measurements()
		{
			dist_travelled_since_last_hop = 0; //yards
			time_at_last_hop = world()->get_ticks(); //ticks
		}
		inline void add_distance(uint32 distance) { dist_travelled_since_last_hop += distance; } // yards
		void flush_travel_times(strasse_t*); // calculate travel times, write to way, reset measurements
};

/**
 * Base class for all vehicles
 *
 * @author Hj. Malthaner
 */
class vehicle_base_t : public obj_t
{
	// BG, 15.02.2014: gr and weg are cached in enter_tile() and reset to NULL in leave_tile().
	grund_t* gr;
	weg_t* weg;
public:
	inline grund_t* get_grund() const
	{
		if (!gr)
			return welt->lookup(get_pos());
		return gr;
	}
	inline weg_t* get_weg() const
	{
		if (!weg)
		{
			// gr and weg are both initialized in enter_tile(). If there is a gr but no weg, then e.g. for ships there IS no way.
			if (!gr)
			{
				// get a local pointer only. Do not assign to instances gr that has to be done by enter_tile() only.
				grund_t* gr2 = get_grund();
				if (gr2)
					return gr2->get_weg(get_waytype());
			}
		}
		return weg;
	}
protected:
	// offsets for different directions
	static sint8 dxdy[16];

	// to make the length on diagonals configurable
	// Number of vehicle steps along a diagonal...
	// remember to subtract one when stepping down to 0
	static uint8 diagonal_vehicle_steps_per_tile;
	static uint8 old_diagonal_vehicle_steps_per_tile;
	static uint16 diagonal_multiplier;

	// [0]=xoff [1]=yoff
	static sint8 overtaking_base_offsets[8][2];

	/**
	 * Actual travel direction in screen coordinates
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi direction;

	// true on slope (make calc_height much faster)
	uint8 use_calc_height:1;

	// if true, use offsets to emulate driving on other side
	uint8 drives_on_left:1;

	/**
	* Thing is moving on this lane.
	* Possible values:
	* (Back)
	* 0 - sidewalk (going on the right side to w/sw/s)
	* 1 - road     (going on the right side to w/sw/s)
	* 2 - middle   (everything with waytype != road)
	* 3 - road     (going on the right side to se/e/../nw)
	* 4 - sidewalk (going on the right side to se/e/../nw)
	* (Front)
	*/
	uint8 disp_lane : 3;

	sint8 dx, dy;

	// number of steps in this tile (255 per tile)
	uint8 steps, steps_next;

	/**
	 * Next position on our path
	 * @author Hj. Malthaner
	 */
	koord3d pos_next;

	/**
	 * Offsets for uphill/downhill.
	 * Have to be multiplied with -TILE_HEIGHT_STEP/2.
	 * To obtain real z-offset, interpolate using steps, steps_next.
	 */
	uint8 zoff_start:4, zoff_end:4;

	// cached image
	image_id image;

	// The current livery of this vehicle.
	// @author: jamespetts, April 2011
	std::string current_livery;

	/**
	 * this vehicle will enter passing lane in the next tile -> 1
	 * this vehicle will enter traffic lane in the next tile -> -1
	 * Unclear -> 0
	 * @author THLeaderH
	 */
	sint8 next_lane;

	/**
	 * Vehicle movement: check whether this vehicle can enter the next tile (pos_next).
	 * @returns NULL if check fails, otherwise pointer to the next tile
	 */
	virtual grund_t* hop_check() = 0;

	/**
	 * Vehicle movement: change tiles, calls leave_tile and enter_tile.
	 * @param gr pointer to ground of new position (never NULL)
	 */
	virtual void hop(grund_t* gr) = 0;

	virtual void update_bookkeeping(uint32 steps) = 0;

	virtual void calc_image() = 0;

	// check for road vehicle, if next tile is free
	vehicle_base_t *no_cars_blocking( const grund_t *gr, const convoi_t *cnv, const uint8 current_direction, const uint8 next_direction, const uint8 next_90direction, const private_car_t *pcar, sint8 lane_on_the_tile );

	// If true, two vehicles might crash by lane crossing.
	bool judge_lane_crossing( const uint8 current_direction, const uint8 next_direction, const uint8 other_next_direction, const bool is_overtaking, const bool forced_to_change_lane ) const;

	// only needed for old way of moving vehicles to determine position at loading time
	bool is_about_to_hop( const sint8 neu_xoff, const sint8 neu_yoff ) const;

	// Players are able to reassign classes of accommodation in vehicles manually
	// during the game. Track these reassignments here with this array.
	uint8 *class_reassignments;

public:
	// only called during load time: set some offsets
	static void set_diagonal_multiplier( uint32 multiplier, uint32 old_multiplier );
	static uint16 get_diagonal_multiplier() { return diagonal_multiplier; }
	static uint8 get_diagonal_vehicle_steps_per_tile() { return diagonal_vehicle_steps_per_tile; }

	static void set_overtaking_offsets( bool driving_on_the_left );

	// if true, this convoi needs to restart for correct alignment
	bool need_realignment() const;

	virtual uint32 do_drive(uint32 dist);	// basis movement code

	inline void set_image( image_id b ) { image = b; }
	virtual image_id get_image() const {return image;}

	sint16 get_hoff(const sint16 raster_width = 1) const;
	uint8 get_steps() const {return steps;} // number of steps pass on the current tile.
	uint8 get_steps_next() const {return steps_next;} // total number of steps to pass on the current tile - 1. Mostly VEHICLE_STEPS_PER_TILE - 1 for straight route or diagonal_vehicle_steps_per_tile - 1 for a diagonal route.

	uint8 get_disp_lane() const { return disp_lane; }

	// to make smaller steps than the tile granularity, we have to calculate our offsets ourselves!
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const;

	/**
	 * Vehicle movement: calculates z-offset of vehicles on slopes,
	 * handles vehicles that are invisible in tunnels.
	 * @param gr vehicle is on this ground
	 * @note has to be called after loading to initialize z-offsets
	 */
	void calc_height(grund_t *gr = NULL);

	virtual void rotate90();

	template<class K1, class K2>
	static ribi_t::ribi calc_direction(const K1& from, const K2& to)
	{
		return ribi_type(from, to);
	}

	ribi_t::ribi calc_set_direction(const koord3d& start, const koord3d& ende);
	uint16 get_tile_steps(const koord &start, const koord &ende, /*out*/ ribi_t::ribi &direction) const;

	ribi_t::ribi get_direction() const {return direction;}

	ribi_t::ribi get_90direction() const {return ribi_type(get_pos(), get_pos_next());}

	koord3d get_pos_next() const {return pos_next;}

	virtual waytype_t get_waytype() const = 0;

	void set_class_reassignment(uint8 original_class, uint8 new_class);

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return true; }

	/**
	 * Vehicle movement: enter tile, add this to the ground.
	 * @pre position (obj_t::pos) needs to be updated prior to calling this functions
	 * @return pointer to ground (never NULL)
	 */
	virtual void enter_tile(grund_t*);

	/**
	 * Vehicle movement: leave tile, release reserved crossing, remove vehicle from the ground.
	 */
	virtual void leave_tile();

	virtual overtaker_t *get_overtaker() { return NULL; }
	virtual convoi_t* get_overtaker_cv() { return NULL; }

#ifdef INLINE_OBJ_TYPE
protected:
	vehicle_base_t(typ type);
	vehicle_base_t(typ type, koord3d pos);
#else
	vehicle_base_t();

	vehicle_base_t(koord3d pos);
#endif

	virtual bool is_flying() const { return false; }
};


template<> inline vehicle_base_t* obj_cast<vehicle_base_t>(obj_t* const d)
{
	return d->is_moving() ? static_cast<vehicle_base_t*>(d) : 0;
}


/**
 * Class for all vehicles with route
 *
 * @author Hj. Malthaner
 */

class vehicle_t : public vehicle_base_t, public test_driver_t
{
private:
	/**
	* Date of purchase in months
	* @author Hj. Malthaner
	*/
	sint32 purchase_time;

	/* For the more physical acceleration model friction is introduced
	* frictionforce = gamma*speed*weight
	* since the total weight is needed a lot of times, we save it
	* @author prissi
	* BG, 18.10.2011: in tons in simutrans standard, in kg in simutrans extended
	*/
	uint32 sum_weight;

	grund_t* hop_check();

	/**
	 * Calculate friction caused by slopes and curves.
	 */
	virtual void calc_drag_coefficient(const grund_t *gr);

	sint32 calc_modified_speed_limit(koord3d position, ribi_t::ribi current_direction, bool is_corner);

	bool load_freight_internal(halthandle_t halt, bool overcrowd, bool *skip_vehicles, bool use_lower_classes);

	// @author: jamespetts
	// Cornering settings.

	fixed_list_tpl<sint16, 192> pre_corner_direction;

	sint16 direction_steps;

	uint8 hill_up;
	uint8 hill_down;
	bool is_overweight;

	// Whether this individual vehicle is reversed.
	// @author: jamespetts
	bool reversed;

	//@author: jamespetts
	uint16 diagonal_costs;
	uint16 base_costs;

	/// This is the last tile on which this vehicle stopped: useful
	/// for logging traffic congestion
	koord3d last_stopped_tile;

protected:
	virtual void hop(grund_t*);

	virtual void update_bookkeeping(uint32 steps);

	// current limit (due to track etc.)
	sint32 speed_limit;

	ribi_t::ribi previous_direction;

	//uint16 target_speed[16];

	//const koord3d *lookahead[16];

	// for target reservation and search
	halthandle_t target_halt;

	/* The friction is calculated new every step, so we save it too
	* @author prissi
	*/
	sint16 current_friction;

	/**
	* Current index on the route
	* @author Hj. Malthaner
	*/
	uint16 route_index;

	uint16 total_freight;	// since the sum is needed quite often, it is cached (not differentiated by class)
	slist_tpl<ware_t> *fracht;   // list of goods being transported (array for each class)

	const vehicle_desc_t *desc;

	convoi_t *cnv;		// != NULL if the vehicle is part of a Convoi

	/**
	* Previous position on our path
	* @author Hj. Malthaner
	*/
	koord3d pos_prev;

	uint8 number_of_classes;

	bool leading:1;	// true, if vehicle is first vehicle of a convoi
	bool last:1;	// true, if vehicle is last vehicle of a convoi
	bool smoke:1;
	bool check_for_finish:1;		// true, if on the last tile
	bool has_driven:1;

	virtual void calc_image();

	bool check_access(const weg_t* way) const;

	/// Register this vehicle as having stopped on a tile, if it has not already done so.
	void log_congestion(strasse_t* road);

public:
	sint32 calc_speed_limit(const weg_t *weg, const weg_t *weg_previous, fixed_list_tpl<sint16, 192>* cornering_data, uint32 bridge_tiles, ribi_t::ribi current_direction, ribi_t::ribi previous_direction);

	virtual bool check_next_tile(const grund_t* ) const {return false;}

	bool check_way_constraints(const weg_t &way) const;

	uint8 hop_count;

//public:
	// the coordinates, where the vehicle was loaded the last time
	koord3d last_stop_pos;

	convoi_t *get_convoi() const { return cnv; }

	virtual void rotate90();

	ribi_t::ribi get_previous_direction() const { return previous_direction; }

	/**
	 * Method checks whether next tile is free to move on.
	 * Looks up next tile, and calls @ref can_enter_tile(const grund_t*, sint32&, uint8).
	 */
	bool can_enter_tile(sint32 &restart_speed, uint8 second_check_count);

	/**
	 * Method checks whether next tile is free to move on.
	 * @param gr_next next tile, must not be NULL
	 */
	virtual bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8 second_check_count) = 0;

	virtual void enter_tile(grund_t*);

	virtual void leave_tile();

	virtual waytype_t get_waytype() const = 0;

	/**
	* Determine the direction bits for this kind of vehicle.
	*
	* @author Hj. Malthaner, 04.01.01
	*/
	virtual ribi_t::ribi get_ribi(const grund_t* gr) const { return gr->get_weg_ribi(get_waytype()); }

	sint32 get_purchase_time() const {return purchase_time;}

	void get_smoke(bool yesno ) { smoke = yesno;}

	virtual route_t::route_result_t calc_route(koord3d start, koord3d ziel, sint32 max_speed_kmh, bool is_tall, route_t* route);
	uint16 get_route_index() const {return route_index;}
	void set_route_index(uint16 value) { route_index = value; }
	const koord3d get_pos_prev() const {return pos_prev;}

	virtual route_t::route_result_t reroute(const uint16 reroute_index, const koord3d &ziel);

	/**
	* Get the base image.
	* @author Hj. Malthaner
	*/
	image_id get_base_image() const { return desc->get_base_image(current_livery.c_str()); }

	/**
	 * @return image with base direction and freight image taken from loaded cargo
	 */
	image_id get_loaded_image() const;

	/**
	* @return vehicle description object
	* @author Hj. Malthaner
	*/
	const vehicle_desc_t *get_desc() const {return desc; }
	void set_desc(const vehicle_desc_t* value);

	/**
	* @return die running_cost in Cr/100Km
	* @author Hj. Malthaner
	*/
	int get_running_cost() const { return desc->get_running_cost(); }
	int get_running_cost(const karte_t* welt) const { return desc->get_running_cost(welt); }

	/**
	* @return fixed maintenance costs in Cr/100months
	* @author Bernd Gabriel
	*/
	uint32 get_fixed_cost() const { return desc->get_fixed_cost(); }
	uint32 get_fixed_cost(karte_t* welt) const { return desc->get_fixed_cost(welt); }

	/**
	* Play sound, when the vehicle is visible on screen
	* @author Hj. Malthaner
	*/
	void play_sound() const;

	/**
	 * Prepare vehicle for new ride.
	 * Sets route_index, pos_next, steps_next.
	 * If @p recalc is true this sets position and recalculates/resets movement parameters.
	 * @author Hj. Malthaner
	 */
	void initialise_journey( uint16 start_route_index, bool recalc );

	void set_direction_steps(sint16 value) { direction_steps = value; }

	void fix_class_accommodations();

	inline koord3d get_last_stop_pos() const { return last_stop_pos;  }

#ifdef INLINE_OBJ_TYPE
protected:
	vehicle_t(typ type);
	vehicle_t(typ type, koord3d pos, const vehicle_desc_t* desc, player_t* player);
public:
#else
	vehicle_t();
	vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player);
#endif

	~vehicle_t();

	/// Note that the original class is the accommodation index *not* the previously re-assigned class (if different)
	void set_class_reassignment(uint8 original_class, uint8 new_class);

	void make_smoke() const;

	void show_info();

	/* return friction constant: changes in hill and curves; may even negative downhill *
	* @author prissi
	*/
	inline sint16 get_frictionfactor() const { return current_friction; }

	/* Return total weight including freight (in kg!)
	* @author prissi
	*/
	inline uint32 get_total_weight() const { return sum_weight; }

	bool get_is_overweight() { return is_overweight; }

	// returns speedlimit of ways (and if convoi enters station etc)
	// the convoi takes care of the max_speed of the vehicle
	// In Extended this is mostly for entering stations etc.,
	// as the new physics engine handles ways
	sint32 get_speed_limit() const { return speed_limit; }
	static inline sint32 speed_unlimited() {return (std::numeric_limits<sint32>::max)(); }

	const slist_tpl<ware_t> & get_cargo(uint8 g_class) const { return fracht[g_class];}   // list of goods being transported (indexed by accommodation class)

	/**
	 * Rotate freight target coordinates, has to be called after rotating factories.
	 */
	void rotate90_freight_destinations(const sint16 y_size);

	/**
	* Calculate the total quantity of goods moved
	*/
	uint16 get_total_cargo() const { return total_freight; }

	uint16 get_total_cargo_by_class(uint8 g_class) const;

	uint16 get_reassigned_class(uint8 g_class) const;

	uint8 get_number_of_accommodation_classes() const;

	/**
	* Calculate transported cargo total weight in KG
	* @author Hj. Malthaner
	*/
	uint32 get_cargo_weight() const;

	const char * get_cargo_name() const;

	/**
	* get the type of cargo this vehicle can transport
	*/
	const goods_desc_t* get_cargo_type() const { return desc->get_freight_type(); }

	/**
	* Get the maximum capacity
	*/
	uint16 get_cargo_max() const {return desc->get_total_capacity(); }

	ribi_t::ribi get_next_90direction() const;

	const char * get_cargo_mass() const;

	/**
	* create an info text for the freight
	* e.g. to display in a info window
	* @author Hj. Malthaner
	*/
	void get_cargo_info(cbuffer_t & buf) const;

	// Check for straightness of way.
	//@author jamespetts

	enum direction_degrees {
		North = 360,
		Northeast = 45,
		East = 90,
		Southeast = 135,
		South = 180,
		Southwest = 225,
		West = 270,
		Northwest = 315,
	};

	direction_degrees get_direction_degrees(ribi_t::ribi) const;

	sint16 compare_directions(sint16 first_direction, sint16 second_direction) const;

	/**
	* Delete all vehicle load
	* @author Hj. Malthaner
	*/
	void discard_cargo();

	/**
	* Payment is done per hop. It iterates all goods and calculates
	* the income for the last hop. This method must be called upon
	* every stop.
	* @return income total for last hop
	* @author Hj. Malthaner
	*/
	//sint64  calc_gewinn(koord start, koord end, convoi_t* cnv) const;


	// sets or querey begin and end of convois
	void set_leading(bool value) {leading = value;}
	bool is_leading() const {return leading;}

	void set_last(bool value) {last = value;}
	bool is_last() const {return last;}

	// marks the vehicle as really used
	void set_driven() { has_driven = true; }

	virtual void set_convoi(convoi_t *c);

	/**
	 * Unload freight to halt
	 * @return sum of unloaded goods
	 */
	uint16 unload_cargo(halthandle_t halt, sint64 & revenue_from_unloading, array_tpl<sint64> & apportioned_revenues );

	/**
	 * Load freight from halt
	 * @return amount loaded
	 */
	uint16 load_cargo(halthandle_t halt)  { bool dummy; (void)dummy; return load_cargo(halt, false, &dummy, &dummy, true); }
	uint16 load_cargo(halthandle_t halt, bool overcrowd, bool *skip_convois, bool *skip_vehicles, bool use_lower_classes);

	/**
	* Remove freight that no longer can reach it's destination
	* i.e. because of a changed schedule
	* @author Hj. Malthaner
	*/
	void remove_stale_cargo();

	/**
	* Generate a matching schedule for the vehicle type
	* @author Hj. Malthaner
	*/
	virtual schedule_t *generate_new_schedule() const = 0;

	const char * is_deletable(const player_t *player);

	void rdwr(loadsave_t *file);
	virtual void rdwr_from_convoi(loadsave_t *file);

	uint32 calc_sale_value() const;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck();

	// this routine will display a tooltip for lost, on depot order, and stuck vehicles
#ifdef MULTI_THREAD
	virtual void display_overlay(int xpos, int ypos) const;
#else
	virtual void display_after(int xpos, int ypos, bool dirty) const;
#endif

	bool is_reversed() const { return reversed; }
	void set_reversed(bool value);

	// Gets modified direction, used for drawing
	// vehicles in reverse formation.
	ribi_t::ribi get_direction_of_travel() const;

	uint32 get_sum_weight() const { return sum_weight; }

	uint16 get_overcrowded_capacity(uint8 g_class) const;
	// @author: jamespetts
	uint16 get_overcrowding(uint8 g_class) const;

	// @author: jamespetts
	uint8 get_comfort(uint8 catering_level = 0, uint8 g_class = 0) const;

	uint16 get_accommodation_capacity(uint8 g_class, bool include_lower_classes = false) const;
	uint16 get_fare_capacity(uint8 g_class, bool include_lower_classes = false) const;

	// BG, 06.06.2009: update player's fixed maintenance
	void finish_rd();
	void before_delete();

	void set_current_livery(const char* liv) { current_livery = liv; }
	const char* get_current_livery() const { return current_livery.c_str(); }

	virtual sint32 get_takeoff_route_index() const { return INVALID_INDEX; }
	virtual sint32 get_touchdown_route_index() const { return INVALID_INDEX; }
};


template<> inline vehicle_t* obj_cast<vehicle_t>(obj_t* const d)
{
	return dynamic_cast<vehicle_t*>(d);
}


/**
 * A class for road vehicles. Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author Hj. Malthaner
 * @see vehicle_t
 */
class road_vehicle_t : public vehicle_t, public traffic_vehicle_t
{
private:
	// called internally only from can_enter_tile()
	// returns true on success
	bool choose_route(sint32 &restart_speed, ribi_t::ribi start_direction, uint16 index);

public:
	bool check_next_tile(const grund_t *bd) const OVERRIDE;

protected:
	bool is_checker;

public:
	virtual void enter_tile(grund_t*) OVERRIDE;
	virtual void hop(grund_t*) OVERRIDE;
	virtual uint32 do_drive(uint32 distance) OVERRIDE;

	virtual void rotate90() OVERRIDE;

	void calc_disp_lane();

	virtual waytype_t get_waytype() const OVERRIDE { return road_wt; }

	road_vehicle_t(loadsave_t *file, bool first, bool last);
	road_vehicle_t();
	road_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv); // start und schedule

	uint32 get_max_speed() override;

	virtual void set_convoi(convoi_t *c) OVERRIDE;

	// how expensive to go here (for way search)
	virtual int get_cost(const grund_t *, const sint32, koord) OVERRIDE;

	virtual route_t::route_result_t calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool is_tall, route_t* route) OVERRIDE;

	virtual bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8 second_check_count) OVERRIDE;

	// returns true for the way search to an unknown target.
	virtual bool  is_target(const grund_t *,const grund_t *) OVERRIDE;

	// since we must consider overtaking, we use this for offset calculation
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width, bool prev_based ) const;
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const OVERRIDE { get_screen_offset(xoff,yoff,raster_width,false); }

#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const { return road_vehicle; }
#endif

	schedule_t * generate_new_schedule() const OVERRIDE;

	virtual overtaker_t* get_overtaker() OVERRIDE;
	virtual convoi_t* get_overtaker_cv() OVERRIDE;

	virtual vehicle_base_t* other_lane_blocked(const bool only_search_top = false, sint8 offset = 0) const;
	virtual vehicle_base_t* other_lane_blocked_offset() const { return other_lane_blocked(false,1); }
};


/**
 * A class for rail vehicles (trains). Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author Hj. Malthaner
 * @see vehicle_t
 */
class rail_vehicle_t : public vehicle_t
{
protected:
	bool check_next_tile(const grund_t *bd) const;

	void enter_tile(grund_t*);

	sint32 activate_choose_signal(uint16 start_index, uint16 &next_signal_index, uint32 brake_steps, uint16 modified_sighting_distance_tiles, route_t* route, sint32 modified_route_index);

	working_method_t working_method;

public:
	virtual waytype_t get_waytype() const { return track_wt; }

	void rdwr_from_convoi(loadsave_t *file);

	// since we might need to unreserve previously used blocks, we must do this before calculation a new route
	route_t::route_result_t calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool is_tall, route_t* route);

	// how expensive to go here (for way search)
	virtual int get_cost(const grund_t *, const sint32, koord);

	virtual uint32 get_cost_upslope() const { return 75; } // Standard is 15

	// returns true for the way search to an unknown target.
	virtual bool  is_target(const grund_t *,const grund_t *);

	// handles all block stuff and route choosing ...
	virtual bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8);

	// reserves or un-reserves all blocks and returns the handle to the next block (if there)
	// returns true on successful reservation (the specific number being the number of blocks ahead clear,
	// needed for setting signal aspects in some cases).
	sint32 block_reserver(route_t *route, uint16 start_index, uint16 modified_sighting_distance_tiles, uint16 &next_signal, int signal_count, bool reserve, bool force_unreserve, bool is_choosing = false, bool is_from_token = false, bool is_from_starter = false, bool is_from_directional = false, uint32 brake_steps = 1, uint16 first_one_train_staff_index = INVALID_INDEX, bool from_call_on = false, bool *break_loop = NULL);

	// Finds the next signal without reserving any tiles.
	// Used for time interval (with and without telegraph) signals on plain track.
	void find_next_signal(route_t* route, uint16 start_index, uint16 &next_signal);

	void leave_tile();

	/// Unreserve behind the train using the current route
	void unreserve_in_rear();

	/// Unreserve behind the train (irrespective of route) all station tiles in rear
	void unreserve_station();

	void clear_token_reservation(signal_t* sig, rail_vehicle_t* w, schiene_t* sch);

#ifdef INLINE_OBJ_TYPE
protected:
	rail_vehicle_t(typ type, loadsave_t *file, bool is_leading, bool is_last);
	rail_vehicle_t(typ type, koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t *cnv); // start und schedule
	void init(loadsave_t *file, bool is_leading, bool is_last);
public:
#else
	typ get_typ() const { return rail_vehicle; }
#endif

	rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last);
	rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t *cnv); // start und schedule
	virtual ~rail_vehicle_t();

	virtual void set_convoi(convoi_t *c);

	virtual schedule_t * generate_new_schedule() const;

	working_method_t get_working_method() const { return working_method; }
	void set_working_method(working_method_t value);
};



/**
 * very similar to normal railroad, so we can implement it here completely ...
 * @author prissi
 * @see vehicle_t
 */
class monorail_rail_vehicle_t : public rail_vehicle_t
{
public:
	virtual waytype_t get_waytype() const { return monorail_wt; }

#ifdef INLINE_OBJ_TYPE
	// all handled by rail_vehicle_t
	monorail_rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) : rail_vehicle_t(monorail_vehicle, file,is_leading, is_last) {}
	monorail_rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(monorail_vehicle, pos, desc, player, cnv) {}
#else
	// all handled by rail_vehicle_t
	monorail_rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) : rail_vehicle_t(file,is_leading, is_last) {}
	monorail_rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(pos, desc, player, cnv) {}

	typ get_typ() const { return monorail_vehicle; }
#endif

	schedule_t * generate_new_schedule() const;
};



/**
 * very similar to normal railroad, so we can implement it here completely ...
 * @author prissi
 * @see vehicle_t
 */
class maglev_rail_vehicle_t : public rail_vehicle_t
{
public:
	virtual waytype_t get_waytype() const { return maglev_wt; }

#ifdef INLINE_OBJ_TYPE
	// all handled by rail_vehicle_t
	maglev_rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) : rail_vehicle_t(maglev_vehicle, file, is_leading, is_last) {}
	maglev_rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(maglev_vehicle, pos, desc, player, cnv) {}
#else
	// all handled by rail_vehicle_t
	maglev_rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) : rail_vehicle_t(file, is_leading, is_last) {}
	maglev_rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(pos, desc, player, cnv) {}

	typ get_typ() const { return maglev_vehicle; }
#endif

	schedule_t * generate_new_schedule() const;
};



/**
 * very similar to normal railroad, so we can implement it here completely ...
 * @author prissi
 * @see vehicle_t
 */
class narrowgauge_rail_vehicle_t : public rail_vehicle_t
{
public:
	virtual waytype_t get_waytype() const { return narrowgauge_wt; }

#ifdef INLINE_OBJ_TYPE
	// all handled by rail_vehicle_t
	narrowgauge_rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) : rail_vehicle_t(narrowgauge_vehicle, file, is_leading, is_last) {}
	narrowgauge_rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(narrowgauge_vehicle, pos, desc, player, cnv) {}
#else
	// all handled by rail_vehicle_t
	narrowgauge_rail_vehicle_t(loadsave_t *file, bool is_leading, bool is_last) : rail_vehicle_t(file, is_leading, is_last) {}
	narrowgauge_rail_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv) : rail_vehicle_t(pos, desc, player, cnv) {}

	typ get_typ() const { return narrowgauge_vehicle; }
#endif

	schedule_t * generate_new_schedule() const;
};



/**
 * A class for naval vehicles. Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author Hj. Malthaner
 * @see vehicle_t
 */
class water_vehicle_t : public vehicle_t
{
protected:
	// how expensive to go here (for way search)
	virtual int get_cost(const grund_t *, const sint32, koord) { return 1; }

	void calc_drag_coefficient(const grund_t *gr);

	bool check_next_tile(const grund_t *bd) const;

	void enter_tile(grund_t*);

public:
	waytype_t get_waytype() const { return water_wt; }

	virtual bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8);

	bool check_tile_occupancy(const grund_t* gr);

	// returns true for the way search to an unknown target.
	virtual bool  is_target(const grund_t *,const grund_t *) {return 0;}

	water_vehicle_t(loadsave_t *file, bool is_leading, bool is_last);
	water_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv);

#ifdef INLINE_OBJ_TYPE
#else
	obj_t::typ get_typ() const { return water_vehicle; }
#endif

	schedule_t * generate_new_schedule() const;

};


/**
 * A class for aircrafts. Manages the look of the vehicles
 * and the navigability of tiles.
 *
 * @author hsiegeln
 * @see vehicle_t
 */
class air_vehicle_t : public vehicle_t
{
public:
	enum flight_state { taxiing=0, departing=1, flying=2, landing=3, looking_for_parking=4, circling=5, taxiing_to_halt=6  };

private:

	// only used for  is_target() (do not need saving)
	ribi_t::ribi approach_dir;

	// Used to re-run the routing algorithm without
	// checking runway length in order to display
	// the correct error message.
	bool ignore_runway_length = false;

#ifdef USE_DIFFERENT_WIND
	static uint8 get_approach_ribi( koord3d start, koord3d ziel );
#endif
	//// only used for route search and approach vectors of get_ribi() (do not need saving)
	//koord3d search_start;
	//koord3d search_end;

	flight_state state;	// functions needed for the search without destination from find_route

	sint16 flying_height;
	sint16 target_height;
	uint32 search_for_stop, touchdown, takeoff;

	sint16 altitude_level; // for AFHP
	sint16 landing_distance; // for AFHP

	void calc_altitude_level(sint32 speed_limit_kmh){
		altitude_level = max(5, speed_limit_kmh/33);
		altitude_level = min(altitude_level, 30);
		landing_distance = altitude_level - 1;
	}
	// BG, 07.08.2012: extracted from calc_route()
	route_t::route_result_t calc_route_internal(
		karte_t *welt,
		const koord3d &start,
		const koord3d &ziel,
		sint32 max_speed,
		uint32 weight,
		air_vehicle_t::flight_state &state,
		sint16 &flying_height,
		sint16 &target_height,
		bool &runway_too_short,
		bool &airport_too_close_to_the_edge,
		uint32 &takeoff,
		uint32 &touchdown,
		uint32 &search_for_stop,
		route_t &route);

protected:
	// jumps to next tile and correct the height ...
	void hop(grund_t*) OVERRIDE;

	bool check_next_tile(const grund_t *bd) const OVERRIDE;

	void enter_tile(grund_t*) OVERRIDE;

	int block_reserver( uint32 start, uint32 end, bool reserve ) const;

	// find a route and reserve the stop position
	bool find_route_to_stop_position();

public:
	air_vehicle_t(loadsave_t *file, bool is_leading, bool is_last);
	air_vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player, convoi_t* cnv); // start and schedule

	// to shift the events around properly
	void get_event_index( flight_state &state_, uint32 &takeoff_, uint32 &stopsearch_, uint32 &landing_ ) { state_ = state; takeoff_ = takeoff; stopsearch_ = search_for_stop; landing_ = touchdown; }
	void set_event_index( flight_state state_, uint32 takeoff_, uint32 stopsearch_, uint32 landing_ ) { state = state_; takeoff = takeoff_; search_for_stop = stopsearch_; touchdown = landing_; }

	// since we are drawing ourselves, we must mark ourselves dirty during deletion
	virtual ~air_vehicle_t();

	waytype_t get_waytype() const OVERRIDE { return air_wt; }

	// returns true for the way search to an unknown target.
	bool  is_target(const grund_t *,const grund_t *) OVERRIDE;

	//bool can_takeoff_here(const grund_t *gr, ribi_t::ribi test_dir, uint8 len) const;

	// return valid direction
	ribi_t::ribi get_ribi(const grund_t* ) const OVERRIDE;

	// how expensive to go here (for way search)
	int get_cost(const grund_t *, const sint32, koord) OVERRIDE;

	bool can_enter_tile(const grund_t *gr_next, sint32 &restart_speed, uint8) OVERRIDE;

	void set_convoi(convoi_t *c) OVERRIDE;

	route_t::route_result_t calc_route(koord3d start, koord3d ziel, sint32 max_speed, bool is_tall, route_t* route) OVERRIDE;

	// BG, 08.08.2012: extracted from can_enter_tile()
	route_t::route_result_t reroute(const uint16 reroute_index, const koord3d &ziel) OVERRIDE;

#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return air_vehicle; }
#endif

	schedule_t *generate_new_schedule() const OVERRIDE;

	void rdwr_from_convoi(loadsave_t *file) OVERRIDE;

	int get_flyingheight() const {return flying_height-get_hoff()-2;}

	void force_land() { flying_height = 0; target_height = 0; state = taxiing_to_halt; }

	// image: when flying empty, on ground the plane
	virtual image_id get_image() const OVERRIDE {return !is_on_ground() ? IMG_EMPTY : image;}

	// image: when flying the shadow, on ground empty
	virtual image_id get_outline_image() const OVERRIDE {return !is_on_ground() ? image : IMG_EMPTY;}

	// shadow has black color (when flying)
	virtual PLAYER_COLOR_VAL get_outline_colour() const OVERRIDE {return !is_on_ground() ? TRANSPARENT75_FLAG | OUTLINE_FLAG | COL_BLACK : 0;}

#ifdef MULTI_THREAD
	// this draws the "real" aircrafts (when flying)
	virtual void display_after(int xpos, int ypos, const sint8 clip_num) const OVERRIDE;

	// this routine will display a tooltip for lost, on depot order, and stuck vehicles
	virtual void display_overlay(int xpos, int ypos) const OVERRIDE;
#else
	// this draws the "real" aircrafts (when flying)
	virtual void display_after(int xpos, int ypos, bool dirty) const OVERRIDE;
#endif

	// the drag calculation happens it calc_height
	void calc_drag_coefficient(const grund_t*) OVERRIDE {}

	bool is_on_ground() const { return flying_height==0  &&  !(state==circling  ||  state==flying); }

	// Used for running cost calculations
	bool is_using_full_power() const { return state != circling && state != taxiing; }

	const char * is_deletable(const player_t *player) OVERRIDE;

	virtual bool is_flying() const OVERRIDE { return !is_on_ground(); }

	bool runway_too_short;
	bool airport_too_close_to_the_edge;
	bool is_runway_too_short() {return runway_too_short; }
	bool is_airport_too_close_to_the_edge() { return airport_too_close_to_the_edge; }
	virtual sint32 get_takeoff_route_index() const OVERRIDE { return (sint32) takeoff; }
	virtual sint32 get_touchdown_route_index() const OVERRIDE { return (sint32) touchdown; }
};

sint16 get_friction_of_waytype(waytype_t waytype);

#endif
