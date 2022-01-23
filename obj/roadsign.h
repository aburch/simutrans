/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_ROADSIGN_H
#define OBJ_ROADSIGN_H


#include "simobj.h"
#include "../simtypes.h"
#include "../descriptor/roadsign_desc.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/stringhashtable_tpl.h"

template<class T> class vector_tpl;
class tool_selector_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 */
class roadsign_t : public obj_t, public sync_steppable
{
protected:
	image_id image;
	image_id foreground_image;

	enum {
		SHOW_FONT        = 1,
		SHOW_BACK        = 2,
		SWITCH_AUTOMATIC = 16
	};

	uint8 state:2; // counter for steps ...
	uint8 dir:4;

	uint8 automatic:1;
	uint8 preview:1;
	uint8 ticks_ns;
	uint8 ticks_ow;
        uint8 ticks_yellow_ns, ticks_yellow_ow;
	uint8 ticks_offset;

	sint8 after_yoffset, after_xoffset;

	const roadsign_desc_t *desc;

	ribi_t::ribi calc_mask() const { return ribi_t::is_single(dir) ? dir : (ribi_t::ribi)ribi_t::none; }
public:
	enum signalstate {
		STATE_RED    = 0,
		STATE_GREEN  = 1,
		STATE_YELLOW = 2  // next state is red
	};

	/**
	 * return direction or the state of the traffic light
	 */
	ribi_t::ribi get_dir() const { return dir; }

	/*
	* sets ribi mask of the sign
	* Caution: it will modify way ribis directly unless in preview mode!
	*/
	void set_dir(ribi_t::ribi dir);

	void set_state(signalstate z) {state = z; calc_image();}
	signalstate get_state() { return (signalstate)state; }

	typ get_typ() const OVERRIDE { return roadsign; }
	const char* get_name() const OVERRIDE { return "Roadsign"; }

	// assuming this is a private way sign
	uint16 get_player_mask() const { return (ticks_ow<<8)|ticks_ns; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const OVERRIDE { return desc ? desc->get_wtyp() : invalid_wt; }

	roadsign_t(loadsave_t *file);
	roadsign_t(player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_desc_t* desc, bool preview = false);

	const roadsign_desc_t *get_desc() const {return desc;}

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	~roadsign_t();

	// since traffic lights need their own window
	void show_info() OVERRIDE;

	/// @copydoc obj_t::info
	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	 * Calculate actual image
	 */
	void calc_image() OVERRIDE;

	// true, if a free route choose point (these are always single way the avoid recalculation of long return routes)
	bool is_free_route(uint8 check_dir) const { return desc->is_choose_sign() &&  check_dir == dir; }

	// changes the state of a traffic light
	sync_result sync_step(uint32) OVERRIDE;

	// change the phases of the traffic lights
	uint8 get_ticks_ns() const { return ticks_ns; }
	void set_ticks_ns(uint8 ns) {
		ticks_ns = ns;
		// To prevent overflow in ticks_offset when rotating
		if (ticks_ow > 256-ticks_ns - ticks_yellow_ns - ticks_yellow_ow ) {
			ticks_ow = 256-ticks_ns-ticks_yellow_ns-ticks_yellow_ow;
		}
	}
	uint8 get_ticks_ow() const { return ticks_ow; }
	void set_ticks_ow(uint8 ow) {
		ticks_ow = ow;
		// To prevent overflow in ticks_offset when rotating
		if (ticks_ns > 256-ticks_ow - ticks_yellow_ns-ticks_yellow_ow ) {
			ticks_ns = 256-ticks_ow-ticks_yellow_ns-ticks_yellow_ow;
		}
	}
	uint8 get_ticks_yellow_ns() const { return ticks_yellow_ns; }
	void set_ticks_yellow_ns(uint8 yellow) {
		ticks_yellow_ns = yellow;
		// To prevent overflow in ticks_offset when rotating
		if (ticks_yellow_ns > 256-ticks_ns - ticks_ow - ticks_yellow_ow) {
		  ticks_yellow_ns = 256-ticks_ns-ticks_ow-ticks_yellow_ow;
		}
	}
	uint8 get_ticks_yellow_ow() const { return ticks_yellow_ow; }
	void set_ticks_yellow_ow(uint8 yellow) {
		ticks_yellow_ow = yellow;
		// To prevent overflow in ticks_offset when rotating
		if (ticks_yellow_ow > 256-ticks_ns - ticks_ow - ticks_yellow_ns) {
		  ticks_yellow_ow = 256-ticks_ns-ticks_ow-ticks_yellow_ns;
		}
	}
	uint8 get_ticks_offset() const { return ticks_offset; }
	void set_ticks_offset(uint8 offset) { ticks_offset = offset; }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const OVERRIDE { return image; }

	/**
	* For the front image hiding vehicles
	*/
	image_id get_front_image() const OVERRIDE { return foreground_image; }

	/**
	* draw the part overlapping the vehicles
	* (needed to get the right offset even on hills)
	*/

#ifdef MULTI_THREAD
	void display_after(int xpos, int ypos, const sint8 clip_num) const OVERRIDE;
#else
	void display_after(int xpos, int ypos, bool dirty) const OVERRIDE;
#endif

	void rdwr(loadsave_t *file) OVERRIDE;

	void rotate90() OVERRIDE;

	// subtracts cost
	void cleanup(player_t *player) OVERRIDE;

	void finish_rd() OVERRIDE;

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
	 */
	static void fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 sound_ok);

	static const roadsign_desc_t *roadsign_search(roadsign_desc_t::types flag, const waytype_t wt, const uint16 time);

	static const roadsign_desc_t *find_desc(const char *name) { return table.get(name); }

	static const vector_tpl<const roadsign_desc_t*>& get_available_signs(const waytype_t wt);
};

#endif
