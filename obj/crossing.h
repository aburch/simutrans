/*
 * Copyright (c) 2007 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef obj_crossing_h
#define obj_crossing_h

#include "../simtypes.h"
#include "../display/simimg.h"
#include "../descriptor/crossing_desc.h"
#include "../dataobj/crossing_logic.h"

class vehicle_base_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class crossing_t : public obj_no_info_t
{
protected:
	image_id foreground_image, image;
	uint8 ns;				// direction
	uint8 state;	// only needed for loading ...
	crossing_logic_t *logic;
	const crossing_desc_t *desc;

public:
	typ get_typ() const OVERRIDE { return crossing; }
	const char* get_name() const OVERRIDE { return "Kreuzung"; }

	/**
	 * waytype associated with this object
	 * for crossings: return invalid_wt since they do not need a way
	 * if the way is deleted the crossing will be deleted, too
	 */
	waytype_t get_waytype() const OVERRIDE { return invalid_wt; }

	crossing_t(loadsave_t *file);
	crossing_t(player_t *player, koord3d pos, const crossing_desc_t *desc, uint8 ns = 0);

	/**
	 * crossing logic is removed here
	 * @author prissi
	 */
	virtual ~crossing_t();

	void rotate90() OVERRIDE;

	const crossing_desc_t *get_desc() const { return desc; }

	/**
	 * @return string (only used for debug at the moment)
	 * @author prissi
	 */
	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	 * @return NULL when OK, otherwise an error message
	 * @author Hj. Malthaner
	 */
	const char *is_deletable(const player_t *player) OVERRIDE;

	// returns true, if the crossing can be passed by this vehicle
	bool request_crossing( const vehicle_base_t *v ) { return logic->request_crossing( v ); }

	// adds to crossing
	void add_to_crossing( const vehicle_base_t *v ) { return logic->add_to_crossing( v ); }

	// removes the vehicle from the crossing
	void release_crossing( const vehicle_base_t *v ) { return logic->release_crossing( v ); }

	crossing_logic_t::crossing_state_t get_state() { return logic->get_state(); }

	// called from the logic directly
	void state_changed();

	uint8 get_dir() { return ns; }

	void set_logic( crossing_logic_t *l ) { logic = l; }
	crossing_logic_t *get_logic() { return logic; }

	/**
	 * Dient zur Neuberechnung des Bildes
	 * @author Hj. Malthaner
	 */
	void calc_image() OVERRIDE;

	/**
	* Called whenever the season or snowline height changes
	* return false and the obj_t will be deleted
	*/
	bool check_season(const bool calc_only_season_change) OVERRIDE { if(  !calc_only_season_change  ) { calc_image(); } return true; }  // depends on snowline only

	// changes the state of a traffic light
	image_id get_image() const OVERRIDE { return image; }

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id get_front_image() const OVERRIDE { return foreground_image; }

	void rdwr(loadsave_t *file) OVERRIDE;

	void finish_rd() OVERRIDE;
};

#endif
