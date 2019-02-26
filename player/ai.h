/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Helper for all AIs
 */

#ifndef _AI_H
#define _AI_H

#include "simplay.h"

#include "../finder/building_placefinder.h"

class karte_t;
class vehicle_desc_t;
class goods_desc_t;

/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Search for a free location using the function find_place().
 *
 * @author V. Meyer
 */
class ai_building_place_with_road_finder : public building_placefinder_t  {
public:
	ai_building_place_with_road_finder(karte_t *welt) : building_placefinder_t(welt) {}
	bool is_road_at(sint16 x, sint16 y) const;
	bool is_area_ok(koord pos, sint16 w, sint16 h, climate_bits cl) const OVERRIDE;
};


// AI helper functions
class ai_t : public player_t
{
protected:
	// set the allowed modes of transport
	bool road_transport;
	bool rail_transport;
	bool ship_transport;
	bool air_transport;

	// the shorter the faster construction will occur
	sint32 construction_speed;

public:
	ai_t(uint8 nr);

	bool has_road_transport() const { return road_transport; }
	virtual void set_road_transport( bool yesno ) { road_transport = yesno; }

	bool has_rail_transport() const { return rail_transport; }
	virtual void set_rail_transport( bool yesno ) { rail_transport = yesno; }

	bool has_ship_transport() const { return ship_transport; }
	virtual void set_ship_transport( bool yesno ) { ship_transport = yesno; }

	bool has_air_transport() const { return air_transport; }
	virtual void set_air_transport( bool yesno ) { air_transport = yesno; }

	sint32 get_construction_speed() const { return construction_speed; }
	virtual void set_construction_speed( sint32 newspeed ) { construction_speed = newspeed; }

	void rdwr(loadsave_t *file) OVERRIDE;

	// return true, if there is already a connection
	bool is_connected(const koord star_pos, const koord end_pos, const goods_desc_t *wtyp) const;

	// calls a general tool just like a human player work do
	bool call_general_tool( int tool, koord k, const char *param );

	// find space for stations
	bool suche_platz(koord pos, koord &size, koord *dirs);
	bool suche_platz(koord &start, koord &size, koord target, koord off);

	// removes building markers
	void clean_marker( koord place, koord size );
	void set_marker( koord place, koord size );

	halthandle_t get_halt( const koord haltpos ) const;

	/**
	 * Find the first water tile using line algorithm
	 * start MUST be on land!
	 * @author Hajo
	 **/
	koord find_shore(koord start, koord end) const;
	bool find_harbour(koord &start, koord &size, koord target);

	bool built_update_headquarter();

	// builds a round between those two places or returns false
	bool create_simple_road_transport(koord platz1, koord size1, koord platz2, koord size2, const way_desc_t *road );

	/// helper method to call vehicle_builder_t::vehikel_search and fill in time-line related parameters
	static const vehicle_desc_t *vehikel_search(waytype_t typ, const uint32 target_power, const sint32 target_speed, const goods_desc_t * target_freight, bool include_electric);
};

#endif
