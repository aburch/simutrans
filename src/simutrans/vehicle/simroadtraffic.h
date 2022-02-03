/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef VEHICLE_SIMROADTRAFFIC_H
#define VEHICLE_SIMROADTRAFFIC_H


#include "vehicle_base.h"
#include "overtaker.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../obj/sync_steppable.h"

class citycar_desc_t;
class karte_t;

/**
 * Base class for traffic participants with random movement
 *
 * Transport vehicles are defined in vehicle.h, because they greatly
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

protected:
	waytype_t get_waytype() const OVERRIDE { return road_wt; }

	road_user_t();

	/**
	 * Creates thing at position given by @p gr.
	 * Does not add it to the tile!
	 * @param random number to compute initial direction.
	 */
	road_user_t(grund_t* gr, uint16 random);

public:
	virtual ~road_user_t();

	const char *get_name() const OVERRIDE = 0;
	typ get_typ() const OVERRIDE  = 0;

	/**
	 * Open a new observation window for the object.
	 */
	void show_info() OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	// finalizes direction
	void finish_rd() OVERRIDE;

	// we allow to remove all cars etc.
	const char *is_deletable(const player_t *) OVERRIDE { return NULL; }
};


class private_car_t : public road_user_t, public overtaker_t
{
private:
	static stringhashtable_tpl<const citycar_desc_t *> table;

	const citycar_desc_t *desc;

	// time to life in blocks
#ifdef DESTINATION_CITYCARS
	koord target;
#endif
	koord3d pos_next_next;

	/**
	 * Actual speed
	 */
	uint16 current_speed;

	uint32 ms_traffic_jam;

	grund_t* hop_check() OVERRIDE;

	void calc_disp_lane();

	bool can_enter_tile(grund_t *gr);
protected:
	void rdwr(loadsave_t *file) OVERRIDE;

	void calc_image() OVERRIDE;

public:
	private_car_t(loadsave_t *file);

	/**
	 * Creates citycar at position given by @p gr.
	 * Does not add car to the tile!
	 * If @p name == NULL then a random car is created.
	 */
	private_car_t(grund_t* gr, koord target, const char* name = NULL);

	virtual ~private_car_t();

	void rotate90() OVERRIDE;

	const citycar_desc_t *get_desc() const { return desc; }

	sync_result sync_step(uint32 delta_t) OVERRIDE;

	void hop(grund_t *gr) OVERRIDE;

	void enter_tile(grund_t* gr) OVERRIDE;

	void calc_current_speed(grund_t*);
	uint16 get_current_speed() const {return current_speed;}

	const char *get_name() const OVERRIDE {return "Verkehrsteilnehmer";}
	typ get_typ() const OVERRIDE { return road_user; }

	/**
	 * @param[out] buf a description string for the object
	 * e.g. for the observation window/dialog
	 * @see simwin
	 */
	void info(cbuffer_t & buf) const OVERRIDE;

	// true, if this vehicle did not moved for some time
	bool is_stuck() OVERRIDE { return current_speed==0;}

	/** this function builds the list of the allowed citycars
	 * it should be called every month and in the beginning of a new game
	 */
	static void build_timeline_list(karte_t *welt);
	static bool list_empty();

	static bool register_desc(const citycar_desc_t *desc);
	static bool successfully_loaded();

	// since we must consider overtaking, we use this for offset calculation
	void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const OVERRIDE;

	overtaker_t *get_overtaker() OVERRIDE { return this; }

	// Overtaking for city cars
	bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other) OVERRIDE;
};

#endif
