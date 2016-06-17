/*
 * Copyright (c) 2005 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef obj_roadsign_h
#define obj_roadsign_h

#include "../simobj.h"
#include "../simtypes.h"
#include "../besch/roadsign_besch.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class tool_selector_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class roadsign_t : public obj_t, public sync_steppable
{
protected:
	image_id image;
	image_id after_bild;

	enum { SHOW_FONT=1, SHOW_BACK=2, SWITCH_AUTOMATIC=16 };

	uint8 state:4;	// counter for steps ...
	uint8 dir:4;
	bool ignore_choose:1; 
	uint8 automatic:1;
	bool preview:1;
	uint8 ticks_ns;
	uint8 ticks_ow;
	uint8 ticks_offset;

	sint8 after_yoffset, after_xoffset;

	const roadsign_besch_t *besch;

	ribi_t::ribi calc_mask() const { return ribi_t::ist_einfach(dir) ? dir : (ribi_t::ribi)ribi_t::keine; }

public:
	// Max. 16 (15 incl. 0)
	enum signal_aspects {danger = 0, clear = 1, caution = 2, preliminary_caution = 3, advance_caution = 4, clear_no_choose = 5, caution_no_choose = 6, preliminary_caution_no_choose = 7, advance_caution_no_choose = 8, call_on = 9 }; 

	/*
	 * return direction or the state of the traffic light
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi get_dir() const 	{ return dir; }

	/*
	* sets ribi mask of the sign
	* Caution: it will modify way ribis directly unless in preview mode!
	*/
	void set_dir(ribi_t::ribi dir);

	void set_state(signal_aspects s) {state = s; calc_image();}
	signal_aspects get_state() const { return (signal_aspects)state; }

#ifdef INLINE_OBJ_TYPE
protected:
	roadsign_t(typ type, loadsave_t *file);
	void init(loadsave_t *file);

	roadsign_t(typ type, player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t* besch, bool preview = false);
	void init(player_t *player, ribi_t::ribi dir, const roadsign_besch_t *besch, bool preview = false);
public:
#else
	typ get_typ() const { return roadsign; }
#endif
	const char* get_name() const { return "Roadsign"; }

	// assuming this is a private way sign
	uint16 get_player_mask() const { return (ticks_ow<<8)|ticks_ns; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return besch ? besch->get_wtyp() : invalid_wt; }

	roadsign_t(loadsave_t *file);
	roadsign_t(player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t* besch, bool preview = false);

	const roadsign_besch_t *get_besch() const {return besch;}

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	virtual ~roadsign_t();

	// since traffic lights need their own window
	void show_info();

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	virtual void info(cbuffer_t & buf, bool dummy = false) const;

	/**
	 * berechnet aktuelles image
	 */
	virtual void calc_image();

	// true, if a free route choose point (these are always single way the avoid recalculation of long return routes)
	bool is_free_route(uint8 check_dir) const { return besch->is_choose_sign() &&  check_dir == dir; }

	// changes the state of a traffic light
	bool sync_step(long);

	// change the phases of the traffic lights
	uint8 get_ticks_ns() const { return ticks_ns; }
	void set_ticks_ns(uint8 ns) { ticks_ns = ns; }
	uint8 get_ticks_ow() const { return ticks_ow; }
	void set_ticks_ow(uint8 ow) { ticks_ow = ow; }
	uint8 get_ticks_offset() const { return ticks_offset; }
	void set_ticks_offset(uint8 offset) { ticks_offset = offset; }

	inline void set_bild( image_id b ) { image = b; }
	image_id get_image() const { return image; }

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id get_front_image() const { return after_bild; }

	/**
	* draw the part overlapping the vehicles
	* (needed to get the right offset even on hills)
	* @author V. Meyer
	*/
#ifdef MULTI_THREAD
	void display_after(int xpos, int ypos, const sint8 clip_num) const;
#else
	void display_after(int xpos, int ypos, bool dirty) const;
#endif

	void rdwr(loadsave_t *file);

	void rotate90();

	// substracts cost
	void cleanup(player_t *player);

	void finish_rd();

	// static routines from here
private:
	static vector_tpl<roadsign_besch_t *> liste;
	static stringhashtable_tpl<const roadsign_besch_t *> table;

protected:
	static const roadsign_besch_t *default_signal;

public:
	static bool register_besch(roadsign_besch_t *besch);
	static bool alles_geladen();

	/**
	 * Fill menu with icons of given stops from the list
	 * @author Hj. Malthaner
	 */
	static void fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 sound_ok);

	static const roadsign_besch_t *roadsign_search(roadsign_besch_t::types flag, const waytype_t wt, const uint16 time);

	const roadsign_besch_t* find_best_upgrade(bool underground); 

	static const roadsign_besch_t *find_besch(const char *name) { return table.get(name); }

	static void set_scale(uint16 scale_factor);

	// Upgrades this sign or signal to another type.
	// Returns true if succeeds.
	bool upgrade(const roadsign_besch_t* new_besch); 
	bool upgrade(bool underground) { return upgrade(find_best_upgrade(underground)); } 

	static const char* get_working_method_name(working_method_t wm)
	{
		switch(wm)
		{
		case drive_by_sight:
			return "drive_by_sight";
		case time_interval:
			return "time_interval";
		case absolute_block:
			return "absolute_block";
		case token_block:
			return "token_block";
		case track_circuit_block:
			return "track_circuit_block";
		case cab_signalling:
			return "cab_signalling";
		case moving_block:
			return "moving_block";
		case one_train_staff:
			return "one_train_staff";
		case time_interval_with_telegraph:
			return "time_interval_with_telegraph";
		default:
			return "unknown";
		};
	}
};

#endif
