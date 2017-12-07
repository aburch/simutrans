#ifndef SIMROADTRAFFIC_H
#define SIMROADTRAFFIC_H

/**
 * Moving objects for Simutrans.
 * Transport vehicles are defined in simvehicle.h, because they greatly
 * differ from the vehicles defined herein for the individual traffic
 * (pedestrians, citycars, movingobj aka flock of sheep).
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "simvehicle.h"
#include "overtaker.h"

#include "../tpl/stringhashtable_tpl.h"
#include "../ifc/sync_steppable.h"

class citycar_desc_t;
class world_t;

/**
 * Base class for traffic participants with random movement
 * @author Hj. Malthaner
 */
class road_user_t : public vehicle_base_t, public sync_steppable
{
protected:
	/**
	 * Distance count
	 * @author Hj. Malthaner
	 */
	uint32 weg_next;

	/* ms until destruction
	 */
	sint32 time_to_life;

protected:
	virtual waytype_t get_waytype() const { return road_wt; }

	road_user_t();

	/**
	 * Creates thing at position given by @p gr.
	 * Does not add it to the tile!
	 * @param random number to compute initial direction.
	 */
	road_user_t(grund_t* gr, uint16 random);

public:
	virtual ~road_user_t();

	const char *get_name() const = 0;
	typ get_typ() const  = 0;

	/**
	 * Open a new observation window for the object.
	 * @author Hj. Malthaner
	 */
	virtual void show_info();

	void rdwr(loadsave_t *file);

	// finalizes direction
	void finish_rd();

	// we allow to remove all cars etc.
	const char *is_deletable(const player_t *) { return NULL; }
};


class private_car_t : public road_user_t, public overtaker_t
{
private:
	static stringhashtable_tpl<const citycar_desc_t *> table;

	const citycar_desc_t *desc;

	// prissi: time to life in blocks
#ifdef DESTINATION_CITYCARS
	koord target;
#endif
	koord3d pos_next_next;

	/**
	 * Actual speed
	 * @author Hj. Malthaner
	 */
	uint16 current_speed;

	uint32 ms_traffic_jam;

	grund_t* hop_check();

	void calc_disp_lane();

protected:
	void rdwr(loadsave_t *file);

	void calc_image();

public:
	private_car_t(loadsave_t *file);

	/**
	 * Creates citycar at position given by @p gr.
	 * Does not add car to the tile!
	 */
	private_car_t(grund_t* gr, koord target);

	virtual ~private_car_t();

	virtual void rotate90();

	const citycar_desc_t *get_desc() const { return desc; }

	sync_result sync_step(uint32 delta_t);

	void hop(grund_t *gr);
	bool ist_weg_frei(grund_t *gr);

	void enter_tile(grund_t* gr);

	void calc_current_speed(grund_t*);
	uint16 get_current_speed() const {return current_speed;}

	const char *get_name() const {return "Verkehrsteilnehmer";}
	typ get_typ() const { return road_user; }

	/**
	 * @return a description string for the object
	 * e.g. for the observation window/dialog
	 * @author Hj. Malthaner
	 * @see simwin
	 */
	virtual void info(cbuffer_t & buf) const;

	// true, if this vehicle did not moved for some time
	virtual bool is_stuck() { return current_speed==0;}

	/* this function builds the list of the allowed citycars
	 * it should be called every month and in the beginning of a new game
	 * @author prissi
	 */
	static void build_timeline_list(world_t *welt);
	static bool list_empty();

	static bool register_desc(const citycar_desc_t *desc);
	static bool successfully_loaded();

	// since we must consider overtaking, we use this for offset calculation
	virtual void get_screen_offset( int &xoff, int &yoff, const sint16 raster_width ) const;

	virtual overtaker_t *get_overtaker() { return this; }

	// Overtaking for city cars
	virtual bool can_overtake(overtaker_t *other_overtaker, sint32 other_speed, sint16 steps_other);
};

#endif
