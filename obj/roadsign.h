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
#include "../descriptor/roadsign_desc.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/stringhashtable_tpl.h"

template<class T> class vector_tpl;
class tool_selector_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class roadsign_t : public obj_t, public sync_steppable
{
protected:
	image_id image;
	image_id foreground_image;

	enum { SHOW_FONT=1, SHOW_BACK=2, SWITCH_AUTOMATIC=16 };

	uint8 state:2;	// counter for steps ...
	uint8 dir:4;

	uint8 automatic:1;
	uint8 preview:1;
	uint8 ticks_ns;
	uint8 ticks_ow;
	uint8 ticks_offset;
	uint8 open_direction;

	sint8 after_yoffset, after_xoffset;

	// 0 = not fixed, 1 = only fix left lane, 2 = only fix right lane, 3 = fix both lane, 4 = not applied
	uint8 lane_affinity;
	koord3d intersection_pos;

	const roadsign_desc_t *desc;

	ribi_t::ribi calc_mask() const { return ribi_t::is_single(dir) ? dir : (ribi_t::ribi)ribi_t::none; }
public:
	enum signalstate {rot=0, gruen=1, naechste_rot=2 };

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

	void set_state(signalstate z) {state = z; calc_image();}
	signalstate get_state() { return (signalstate)state; }

	typ get_typ() const { return roadsign; }
	const char* get_name() const { return "Roadsign"; }

	// assuming this is a private way sign
	uint16 get_player_mask() const { return (ticks_ow<<8)|ticks_ns; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return desc ? desc->get_wtyp() : invalid_wt; }

	roadsign_t(loadsave_t *file);
	roadsign_t(player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_desc_t* desc, bool preview = false);

	const roadsign_desc_t *get_desc() const {return desc;}

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	~roadsign_t();

	// since traffic lights need their own window
	void show_info();

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	virtual void info(cbuffer_t & buf) const;

	/**
	 * Calculate actual image
	 */
	virtual void calc_image();

	// true, if a free route choose point (these are always single way the avoid recalculation of long return routes)
	bool is_free_route(uint8 check_dir) const { return desc->is_choose_sign() &&  check_dir == dir; }

	// changes the state of a traffic light
	sync_result sync_step(uint32);

	// change the phases of the traffic lights
	uint8 get_ticks_ns() const { return ticks_ns; }
	void set_ticks_ns(uint8 ns) {
		ticks_ns = ns;
		// To prevent overflow in ticks_offset when rotating
		if (ticks_ow > 256-ticks_ns) {
			ticks_ow = 256-ticks_ns;
		}
	}
	uint8 get_ticks_ow() const { return ticks_ow; }
	void set_ticks_ow(uint8 ow) {
		ticks_ow = ow;
		// To prevent overflow in ticks_offset when rotating
		if (ticks_ns > 256-ticks_ow) {
			ticks_ns = 256-ticks_ow;
		}
	}
	uint8 get_ticks_offset() const { return ticks_offset; }
	void set_ticks_offset(uint8 offset) { ticks_offset = offset; }

	uint8 get_open_direction() const { return open_direction; }
	void set_open_direction(uint8 dir) { open_direction = dir; }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const { return image; }

	uint8 get_lane_affinity() const { return lane_affinity; }
	void set_lane_affinity(uint8 lf) { lane_affinity = lf; }
	const koord3d get_intersection() const;

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id get_front_image() const { return foreground_image; }

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

	// subtracts cost
	void cleanup(player_t *player);

	void finish_rd();

	// static routines from here
private:
	static stringhashtable_tpl<const roadsign_desc_t *> table;

protected:
	static const roadsign_desc_t *default_signal;

public:
	static bool register_desc(roadsign_desc_t *desc);
	static bool successfully_loaded();

	/**
	 * Fill menu with icons of given stops from the list
	 * @author Hj. Malthaner
	 */
	static void fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 sound_ok);

	static const roadsign_desc_t *roadsign_search(roadsign_desc_t::types flag, const waytype_t wt, const uint16 time);

	static const roadsign_desc_t *find_desc(const char *name) { return table.get(name); }

	static const vector_tpl<const roadsign_desc_t*>& get_available_signs(const waytype_t wt);
};

#endif
