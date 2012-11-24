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

#include "../sucher/bauplatz_sucher.h"

class karte_t;
class vehikel_besch_t;
class ware_besch_t;

/**
 * bauplatz_mit_strasse_sucher_t:
 *
 * Sucht einen freien Bauplatz mithilfe der Funktion suche_platz().
 *
 * @author V. Meyer
 */
class ai_bauplatz_mit_strasse_sucher_t : public bauplatz_sucher_t  {
public:
	ai_bauplatz_mit_strasse_sucher_t(karte_t *welt) : bauplatz_sucher_t(welt) {}
	bool strasse_bei(sint16 x, sint16 y) const;
	virtual bool ist_platz_ok(koord pos, sint16 b, sint16 h, climate_bits cl) const;
};


// AI helper functions
class ai_t : public spieler_t
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
	ai_t(karte_t *wl, uint8 nr) : spieler_t( wl, nr ) {
		road_transport = rail_transport = air_transport = ship_transport = false;
		construction_speed = 8000;
	}

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

	virtual void rdwr(loadsave_t *file);

	// return true, if there is already a connection
	bool is_connected(const koord star_pos, const koord end_pos, const ware_besch_t *wtyp) const;

	// prepares a general tool just like a human player work do
	bool init_general_tool( int tool, const char *param );

	// calls a general tool just like a human player work do
	bool call_general_tool( int tool, koord k, const char *param );

	/**
	 * Tells the player the result of tool-work commands
	 * If player is active then play sound, popup error msg etc
	 * AI players react upon this call and proceed
	 * @author Dwachs
	 */
	virtual void tell_tool_result(werkzeug_t *tool, koord3d pos, const char *err, bool local);

	// find space for stations
	bool suche_platz(koord pos, koord &size, koord *dirs) const;
	bool suche_platz(koord &start, koord &size, koord target, koord off);

	// removes building markers
	void clean_marker( koord place, koord size );
	void set_marker( koord place, koord size );

	/**
	 * Find the first water tile using line algorithm von Hajo
	 * start MUST be on land!
	 **/
	koord find_shore(koord start, koord end) const;
	bool find_harbour(koord &start, koord &size, koord target);

	bool built_update_headquarter();

	// builds a round between those two places or returns false
	bool create_simple_road_transport(koord platz1, koord size1, koord platz2, koord size2, const weg_besch_t *road );

	/// helper method to call vehikelbauer_t::vehikel_search and fill in time-line related parameters
	static const vehikel_besch_t *vehikel_search(waytype_t typ, const uint32 target_power, const sint32 target_speed, const ware_besch_t * target_freight, bool include_electric);
};

#endif
