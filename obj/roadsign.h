/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_ROADSIGN_H
#define OBJ_ROADSIGN_H


#include "../simobj.h"
#include "../simtypes.h"
#include "../descriptor/roadsign_desc.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"
#include "../simunits.h"

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

	uint8 state:4;	// counter for steps ...
	uint8 dir:4;
	bool ignore_choose:1;
	uint8 automatic:1;
	bool preview:1;
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
	// Max. 16 (15 incl. 0)
	enum signal_aspects
	{
		danger = 0,
		clear = 1,
		caution = 2,
		preliminary_caution = 3,
		advance_caution = 4,
		clear_no_choose = 5,
		caution_no_choose = 6,
		preliminary_caution_no_choose = 7,
		advance_caution_no_choose = 8,
		call_on = 9
	};

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

	void set_state(signal_aspects s) { state = s; calc_image(); }
	signal_aspects get_state() const { return (signal_aspects)state; }

#ifdef INLINE_OBJ_TYPE
protected:
	roadsign_t(typ type, loadsave_t *file);
	void init(loadsave_t *file);

	roadsign_t(typ type, player_t *player, koord3d pos, ribi_t::ribi dir, const roadsign_desc_t* desc, bool preview = false);
	void init(player_t *player, ribi_t::ribi dir, const roadsign_desc_t *desc, bool preview = false);
public:
#else
	typ get_typ() const { return roadsign; }
#endif
	const char* get_name() const OVERRIDE { return desc->get_name(); }

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
	virtual ~roadsign_t();

	// since traffic lights need their own window
	void show_info() OVERRIDE;

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	 * Calculate actual image
	 */
	virtual void calc_image() OVERRIDE;

	// true, if a free route choose point (these are always single way the avoid recalculation of long return routes)
	bool is_free_route(uint8 check_dir) const { return desc->is_choose_sign() &&  check_dir == dir; }

	// changes the state of a traffic light
	sync_result sync_step(uint32) OVERRIDE;

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

	uint8 get_lane_affinity() const { return lane_affinity; }
	void set_lane_affinity(uint8 lf) { lane_affinity = lf; }
	const koord3d get_intersection() const;

	uint8 get_open_direction() const { return open_direction; }
	void set_open_direction(uint8 dir) { open_direction = dir; }

	inline void set_image( image_id b ) { image = b; }
	image_id get_image() const OVERRIDE { return image; }

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id get_front_image() const OVERRIDE { return foreground_image; }

	/**
	* draw the part overlapping the vehicles
	* (needed to get the right offset even on hills)
	* @author V. Meyer
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
	static vector_tpl<roadsign_desc_t *> list;
	static stringhashtable_tpl<roadsign_desc_t *> table;

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

	const roadsign_desc_t* find_best_upgrade(bool underground);

	static const roadsign_desc_t *find_desc(const char *name) { return table.get(name); }

	static void set_scale(uint16 scale_factor);

	// Upgrades this sign or signal to another type.
	// Returns true if succeeds.
	bool upgrade(const roadsign_desc_t* new_desc);
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

	/* In order to allow for Swedish and Czeck translations (and possibly other translations as well), the type of signal showing the aspect need to be identified by the aspect name.
	Also, whether it is a time interval signal needs to be identified from the aspect name, as "CLEAR" or "CAUTION" on a three aspect signal in this case do not refer to
	the forthcomming signal (however, it does on the presignal!).
	Choose signals have their own namelist as well.
	 clearpre = presignal
	 clear2 = Two aspect signal (stop signal)
	 clear3 = three aspect signal
	 ...
	 cleartimepre = Time interval presignal
	 cleartime3 = Time interval three aspect signal (as there is no two aspect signal)
	 etc...
	*/

	// Presignals:
	static const char* get_pre_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clearpre";
		case caution:
			return "cautionpre";
		default:
			return "unknown";
		};
	}
	// Two aspect signals. This also holds the station signals:
	static const char* get_2_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear2";
		case caution:
			return "caution2";
		case clear_no_choose:
			return "clear2";
		case caution_no_choose:
			return "caution2";
		case call_on:
			return "call_on";
		default:
			return "unknown";
		};
	}
	// Three aspect signals:
	static const char* get_3_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear3";
		case caution:
			return "caution3";
		case clear_no_choose:
			return "clear3";
		case caution_no_choose:
			return "caution3";
		case call_on:
			return "call_on";
		default:
			return "unknown";
		};
	}
	// Four aspect signals:
	static const char* get_4_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear4";
		case caution:
			return "caution4";
		case preliminary_caution:
			return "preliminary_caution4";
		case clear_no_choose:
			return "clear4";
		case caution_no_choose:
			return "caution4";
		case preliminary_caution_no_choose:
			return "preliminary_caution4";
		case call_on:
			return "call_on";
		default:
			return "unknown";
		};
	}
	// Five aspect signals:
	static const char* get_5_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear5";
		case caution:
			return "caution5";
		case preliminary_caution:
			return "preliminary_caution5";
		case advance_caution:
			return "advanced_caution5";
		case clear_no_choose:
			return "clear5";
		case caution_no_choose:
			return "caution5";
		case preliminary_caution_no_choose:
			return "preliminary_caution5";
		case advance_caution_no_choose:
			return "advanced_caution5";
		case call_on:
			return "call_on";
		default:
			return "unknown";
		};
	}
	// Now the same for Choose signals!
	// Two aspect choose signals:
	static const char* get_2_choose_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear2_alternate";
		case clear_no_choose:
			return "clear2_main";
		case call_on:
			return "call_on_choose";
		default:
			return "unknown";
		};
	}
	// Three aspect choose signals:
	static const char* get_3_choose_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear3_alternate";
		case caution:
			return "caution3_alternate";
		case clear_no_choose:
			return "clear3_main";
		case caution_no_choose:
			return "caution3_main";
		case call_on:
			return "call_on_choose";
		default:
			return "unknown";
		};
	}
	// Four aspect choose signals:
	static const char* get_4_choose_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear4_alternate";
		case caution:
			return "caution4_alternate";
		case preliminary_caution:
			return "preliminary_caution4_alternate";
		case clear_no_choose:
			return "clear4_main";
		case caution_no_choose:
			return "caution4_main";
		case preliminary_caution_no_choose:
			return "preliminary_caution4_main";
		case call_on:
			return "call_on_choose";
		default:
			return "unknown";
		};
	}
	// Five aspect choose signals:
	static const char* get_5_choose_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "clear5_alternate";
		case caution:
			return "caution5_alternate";
		case preliminary_caution:
			return "preliminary_caution5_alternate";
		case advance_caution:
			return "advanced_caution5_alternate";
		case clear_no_choose:
			return "clear5_main";
		case caution_no_choose:
			return "caution5_main";
		case preliminary_caution_no_choose:
			return "preliminary_caution5_main";
		case advance_caution_no_choose:
			return "advanced_caution5_main";
		case call_on:
			return "call_on_choose";
		default:
			return "unknown";
		};
	}
	// Now some time interval signals.
	// Time interval three aspect signals (There is no two aspect signal). Presignal use the standard presignal:
	static const char* get_time_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "cleartime";
		case caution:
			return "cautiontime";
		case clear_no_choose:
			return "cleartime";
		case caution_no_choose:
			return "cautiontime";
		case call_on:
			return "call_ontime";
		default:
			return "unknown";
		};
	}
	// Time interval choose signals:
	static const char* get_time_choose_signal_aspects_name(signal_aspects aspect)
	{
		switch (aspect)
		{
		case clear:
			return "cleartime_alternate";
		case caution:
			return "cautiontime_alternate";
		case clear_no_choose:
			return "cleartime_main";
		case caution_no_choose:
			return "cautiontime_main";
		case call_on:
			return "call_ontime_choose";
		default:
			return "unknown";
		};
	}
	static const char* get_directions_name(ribi_t::ribi r)
	{
		switch (r)
		{
		case 1:
			return "south";
		case 2:
			return "west";
		case 3:
			return "north_and_east";
		case 4:
			return "north";
		case 5:
			return "north_and_south";
		case 6:
			return "south_and_east";
		case 8:
			return "east";
		case 9:
			return "north_and_west";
		case 10:
			return "east_and_west";
		case 12:
			return "south_and_west";
		default:
			return "unknown";
		};
	}
};

#endif
