/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_VEHICLE_H
#define VEHICLE_VEHICLE_H


#include "vehicle_base.h"

#include "../ground/grund.h"
#include "../descriptor/vehicle_desc.h"
#include "../vehicle/simtestdriver.h"
#include "../tpl/slist_tpl.h"
#include "../dataobj/route.h"


class schedule_t;
class signal_t;
class ware_t;
class route_t;
class goods_desc_t;


/**
 * Class for all vehicles with route
 */
class vehicle_t : public vehicle_base_t, public test_driver_t
{
private:
	/**
	* Date of purchase in months
	*/
	sint32 purchase_time;

	/**
	* For the more physical acceleration model friction is introduced
	* frictionforce = gamma*speed*weight
	* since the total weight is needed a lot of times, we save it
	*/
	uint32 sum_weight;

	grund_t* hop_check() OVERRIDE;

	/**
	 * Calculate friction caused by slopes and curves.
	 */
	virtual void calc_friction(const grund_t *gr);

protected:
	void hop(grund_t*) OVERRIDE;

	// current limit (due to track etc.)
	sint32 speed_limit;

	ribi_t::ribi previous_direction;

	// for target reservation and search
	halthandle_t target_halt;

	/** The friction is calculated new every step, so we save it too
	*/
	sint16 current_friction;

	/// Current index on the route
	route_t::index_t route_index;

	uint16 total_freight; // since the sum is needed quite often, it is cached
	slist_tpl<ware_t> fracht;   // list of goods being transported

	const vehicle_desc_t *desc;

	convoi_t *cnv;  // != NULL if the vehicle is part of a Convoi

	bool leading:1; // true, if vehicle is first vehicle of a convoi
	bool last:1;    // true, if vehicle is last vehicle of a convoi
	bool smoke:1;
	bool check_for_finish:1; // true, if on the last tile
	bool has_driven:1;

	bool check_next_tile(const grund_t* ) const OVERRIDE {return false;}

public:
	void calc_image() OVERRIDE;

	// the coordinates, where the vehicle was loaded the last time
	koord3d last_stop_pos;

	convoi_t *get_convoi() const { return cnv; }

	void rotate90() OVERRIDE;


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

	void enter_tile(grund_t*) OVERRIDE;

	void leave_tile() OVERRIDE;

	waytype_t get_waytype() const OVERRIDE = 0;

	/**
	* Determine the direction bits for this kind of vehicle.
	*/
	ribi_t::ribi get_ribi(const grund_t* gr) const OVERRIDE { return gr->get_weg_ribi(get_waytype()); }

	sint32 get_purchase_time() const {return purchase_time;}

	void get_smoke(bool yesno ) { smoke = yesno;}

	virtual bool calc_route(koord3d start, koord3d ziel, sint32 max_speed, route_t* route);
	route_t::index_t get_route_index() const { return route_index; }

	/**
	* Get the base image.
	*/
	image_id get_base_image() const { return desc->get_base_image(); }

	/**
	 * @return image with base direction and freight image taken from loaded cargo
	 */
	image_id get_loaded_image() const;

	/**
	* @return vehicle description object
	*/
	const vehicle_desc_t *get_desc() const {return desc; }

	/**
	* @return die running_cost in Cr/100Km
	*/
	int get_operating_cost() const { return desc->get_running_cost(); }

	/**
	* Play sound, when the vehicle is visible on screen
	*/
	void play_sound() const;

	/**
	 * Prepare vehicle for new ride.
	 * Sets route_index, pos_next, steps_next.
	 * If @p recalc is true this sets position and recalculates/resets movement parameters.
	 */
	void initialise_journey( route_t::index_t start_route_index, bool recalc );

	vehicle_t();
	vehicle_t(koord3d pos, const vehicle_desc_t* desc, player_t* player);

	~vehicle_t();

	void make_smoke() const;

	void show_info() OVERRIDE;

	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	 * return friction constant: changes in hill and curves; may even negative downhill *
	 */
	inline sint16 get_frictionfactor() const { return current_friction; }

	/**
	 * Return total weight including freight (in kg!)
	 */
	inline uint32 get_total_weight() const { return sum_weight; }

	// returns speedlimit of ways (and if convoi enters station etc)
	// the convoi takes care of the max_speed of the vehicle
	sint32 get_speed_limit() const { return speed_limit; }

	const slist_tpl<ware_t> & get_cargo() const { return fracht;}   // list of goods being transported

	/**
	 * Rotate freight target coordinates, has to be called after rotating factories.
	 */
	void rotate90_freight_destinations(const sint16 y_size);

	/**
	* Calculate the total quantity of goods moved
	*/
	uint16 get_total_cargo() const { return total_freight; }

	/**
	* Calculate transported cargo total weight in KG
	*/
	uint32 get_cargo_weight() const;

	/**
	* get the type of cargo this vehicle can transport
	*/
	const goods_desc_t* get_cargo_type() const { return desc->get_freight_type(); }

	/**
	* Get the maximum capacity
	*/
	uint16 get_cargo_max() const {return desc->get_capacity(); }

	const char * get_cargo_mass() const;

	/**
	* create an info text for the freight
	* e.g. to display in a info window
	*/
	void get_cargo_info(cbuffer_t & buf) const;

	/**
	* Delete all vehicle load
	*/
	void discard_cargo();

	/**
	* Payment is done per hop. It iterates all goods and calculates
	* the income for the last hop. This method must be called upon
	* every stop.
	* @return income total for last hop
	*/
	sint64 calc_revenue(const koord3d& start, const koord3d& end) const;

	// sets or query begin and end of convois
	void set_leading(bool janein) {leading = janein;}
	bool is_leading() {return leading;}

	void set_last(bool janein) {last = janein;}
	bool is_last() {return last;}

	// marks the vehicle as really used
	void set_driven() { has_driven = true; }

	virtual void set_convoi(convoi_t *c);

	/**
	 * Unload freight to halt
	 * @return sum of unloaded goods
	 */
	uint16 unload_cargo(halthandle_t halt, bool all, uint16 max_amount );

	/**
	 * Load freight from halt
	 * @return amount loaded
	 */
	uint16 load_cargo(halthandle_t halt, const vector_tpl<halthandle_t>& destination_halts, uint16 max_amount );

	/**
	* Remove freight that no longer can reach it's destination
	* i.e. because of a changed schedule
	*/
	void remove_stale_cargo();

	/**
	* Generate a matching schedule for the vehicle type
	*/
	virtual schedule_t *generate_new_schedule() const = 0;

	const char *is_deletable(const player_t *player) OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;
	virtual void rdwr_from_convoi(loadsave_t *file);

	uint32 calc_sale_value() const;

	// true, if this vehicle did not moved for some time
	bool is_stuck() OVERRIDE;

	// this routine will display a tooltip for lost, on depot order, and stuck vehicles
#ifdef MULTI_THREAD
	void display_overlay(int xpos, int ypos) const OVERRIDE;
#else
	void display_after(int xpos, int ypos, bool dirty) const OVERRIDE;
#endif
};


template<> inline vehicle_t* obj_cast<vehicle_t>(obj_t* const d)
{
	return dynamic_cast<vehicle_t*>(d);
}


#endif
