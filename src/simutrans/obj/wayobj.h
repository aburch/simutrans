/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_WAYOBJ_H
#define OBJ_WAYOBJ_H


//#include "../simtypes.h"
#include "simobj.h"
#include "../display/simimg.h"
#include "../dataobj/ribi.h"
#include "../descriptor/way_obj_desc.h"
#include "../tpl/stringhashtable_tpl.h"

class player_t;
class karte_t;
class koord;
class grund_t;
class tool_selector_t;

/* wayobj enable various functionality of ways, most prominent are overhead power lines */
class wayobj_t : public obj_no_info_t
{
private:
	const way_obj_desc_t *desc;

	uint8 close_diagonal:2;
	uint8 diagonal:1;
	uint8 hang:7;

	// direction of this wayobj
	uint8 nw:1;
	uint8 dir:7;

	image_id img;
	image_id after_img;

	static const uint8 dir_unknown = 127;

	ribi_t::ribi find_next_ribi(const grund_t *start, const ribi_t::ribi dir, const waytype_t wt) const;


public:
	wayobj_t(koord3d pos, player_t *owner, ribi_t::ribi dir, const way_obj_desc_t *desc);

	wayobj_t(loadsave_t *file);

	virtual ~wayobj_t();

	const way_obj_desc_t *get_desc() const {return desc;}

	void rotate90() OVERRIDE;

	/**
	* the back image, drawn before vehicles
	*/
	image_id get_image() const OVERRIDE { return img; }

	/**
	 * the front image, drawn after everything else
	 */
	image_id get_front_image() const OVERRIDE {	return after_img; }

	typ get_typ() const OVERRIDE { return wayobj; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const OVERRIDE { return desc ? desc->get_wtyp() : invalid_wt; }

	void calc_cached_image();

	void rdwr(loadsave_t *file) OVERRIDE;

	void cleanup(player_t *player) OVERRIDE;

	const char *get_removal_error(const player_t *player) OVERRIDE;

	bool finish_rd() OVERRIDE;

	// specific for wayobj
	void set_dir(ribi_t::ribi d) { dir = d; }
	ribi_t::ribi get_dir() const { return close_diagonal ? ribi_t::all : dir; }
	uint8 get_close_diagonal() const { return close_diagonal; }

	virtual void display(int xpos, int ypos  CLIP_NUM_DEF) const OVERRIDE;

#ifdef MULTI_THREAD
	virtual void display_after(int xpos, int ypos, const sint8 clip_num) const OVERRIDE;
#else
	virtual void display_after(int xpos, int ypos, bool is_global) const OVERRIDE;
#endif

	/* the static routines */
private:
	static stringhashtable_tpl<const way_obj_desc_t *> table;

public:
	static const way_obj_desc_t *default_oberleitung;

	// use this constructor; it will extend a matching existing wayobj
	static void extend_wayobj(koord3d pos, player_t *owner, ribi_t::ribi dir, const way_obj_desc_t *desc, bool keep_existing_faster_way);

	static bool register_desc(way_obj_desc_t *desc);
	static bool successfully_loaded();

	// search an object (currently only used by AI for catenary)
	static const way_obj_desc_t *get_overhead_line(waytype_t wt, uint16 time);

	static const way_obj_desc_t *find_desc(const char *);

	/**
	 * Fill menu with icons of given stops from the list
	 */
	static void fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 sound_ok);

	static const stringhashtable_tpl<const way_obj_desc_t *>& get_list() { return table; }
};

#endif
