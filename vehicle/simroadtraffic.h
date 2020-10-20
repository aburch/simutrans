/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_SIMROADTRAFFIC_H
#define VEHICLE_SIMROADTRAFFIC_H


#include "simvehicle.h"
#include "overtaker.h"

#include "../tpl/slist_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../ifc/sync_steppable.h"

class citycar_desc_t;
class karte_t;


/**
 * Base class for traffic participants with random movement
 *
 * Transport vehicles are defined in simvehicle.h, because they greatly
 * differ from the vehicles defined herein for the individual traffic
 * (pedestrians, citycars, movingobj aka flock of sheep).
 */
class road_user_t : public vehicle_base_t, public sync_steppable
{
protected:
	/**
	 * Distance count
	 */
	uint32 weg_next;

	/* ms until destruction
	 */
	sint32 time_to_life;

	/** Necessary to keep track of the
	 * distance travelled sine the last
	 * time that this vehicle paid
	 * a toll for using a player's way
	 */
	uint8 tiles_since_last_increment;

protected:
	virtual waytype_t get_waytype() const OVERRIDE { return road_wt; }

	void hop(grund_t *gr) OVERRIDE;
	virtual void update_bookkeeping(uint32) OVERRIDE {};

#ifdef INLINE_OBJ_TYPE
	road_user_t(typ type);

	/**
	 * Creates thing at position given by @p gr.
	 * Does not add it to the tile!
	 * @param random number to compute initial direction.
	 */
	road_user_t(typ type, grund_t* gr, uint16 random);
#else
	road_user_t();

	/**
	 * Creates thing at position given by @p gr.
	 * Does not add it to the tile!
	 * @param random number to compute initial direction.
	 */
	road_user_t(grund_t* gr, uint16 random);
#endif

public:
	virtual ~road_user_t();

	/**
	 * Open a new observation window for the object.
	 */
	virtual void show_info() OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	// finalizes direction
	void finish_rd() OVERRIDE;

	void set_time_to_life(uint32 value) { time_to_life = value; }

	// we allow to remove all cars etc.
	const char * is_deletable(const player_t *) OVERRIDE { return NULL; }
};


class private_car_t : public road_user_t, public overtaker_t, public traffic_vehicle_t
{
private:

	koord origin;
	const citycar_desc_t *desc;

	// time to life in blocks
	koord target;
	koord3d pos_next_next;

	/**
	 * Actual speed
	 */
	uint16 current_speed;

	uint32 ms_traffic_jam;

	koord3d last_tile_marked_as_stopped;

	grund_t* hop_check() OVERRIDE;

	void calc_disp_lane();

protected:
	void rdwr(loadsave_t *file) OVERRIDE;

	void calc_image() OVERRIDE;

public:
	private_car_t(loadsave_t *file);

	/**
	 * Creates citycar at position given by @p gr.
	 * Does not add car to the tile!
	 */
	private_car_t(grund_t* gr, koord target);

	virtual ~private_car_t();

	void rotate90() OVERRIDE;

	static stringhashtable_tpl<const citycar_desc_t *> table;

	const citycar_desc_t *get_desc() const { return desc; }

	sync_result sync_step(uint32 delta_t) OVERRIDE;

	void hop(grund_t *gr) OVERRIDE;
	bool can_enter_tile(grund_t *gr);

	void enter_tile(grund_t* gr) OVERRIDE;

	void calc_current_speed(grund_t*);
	uint16 get_current_speed() const {return current_speed;}

	uint32 get_max_speed() override;
	inline sint32 get_max_power_speed() OVERRIDE {return get_max_speed();}

	const char *get_name() const OVERRIDE {return "Verkehrsteilnehmer";}
	//typ get_typ() const { return road_user; }

	/**
	 * @return a description string for the object
	 * e.g. for the observation window/dialog
	 * @see simwin
	 */
	virtual void info(cbuffer_t & buf) const OVERRIDE;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() OVERRIDE { return current_speed==0;}

	/** this function builds the list of the allowed citycars
	 * it should be called every month and in the beginning of a new game
	 */
	static void build_timeline_list(karte_t *welt);
	static bool list_empty();

	static bool register_desc(const citycar_desc_t *desc);
	static bool successfully_loaded();

	// since we must consider overtaking, we use this for offset calculation
	void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width, bool prev_based ) const;
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const OVERRIDE { get_screen_offset(xoff,yoff,raster_width,false); }

	virtual overtaker_t *get_overtaker() OVERRIDE { return this; }

	// Overtaking for city cars
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other) OVERRIDE;

	virtual vehicle_base_t* other_lane_blocked(const bool only_search_top) const;
	vehicle_base_t* is_there_car(grund_t *gr) const; // This is a helper function of other_lane_blocked

	virtual void reflesh(sint8,sint8) OVERRIDE;

	void * operator new(size_t s);
	void operator delete(void *p);
};

#endif
