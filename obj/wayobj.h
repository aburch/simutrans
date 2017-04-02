/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef wayobj_t_h
#define wayobj_t_h

#include "../simtypes.h"
#include "../display/simimg.h"
#include "../simobj.h"
#include "../simworld.h"
#include "../boden/grund.h"
#include "../dataobj/ribi.h"
#include "../descriptor/way_obj_desc.h"
#include "../tpl/stringhashtable_tpl.h"

class player_t;
class karte_t;
class koord;
class grund_t;
class tool_selector_t;

/**
 * Overhead powelines for electrified tracks.
 *
 * @author Hj. Malthaner
 */
class wayobj_t : public obj_no_info_t
{
private:
	const way_obj_desc_t *desc;

	uint8 diagonal:1;
	uint8 hang:7;

	// direction of this wayobj
	ribi_t::ribi dir;

	ribi_t::ribi find_next_ribi(const grund_t *start, const ribi_t::ribi dir, const waytype_t wt) const;


public:
	wayobj_t(koord3d pos, player_t *owner, ribi_t::ribi dir, const way_obj_desc_t *desc);

	wayobj_t(loadsave_t *file);

	virtual ~wayobj_t();

	const way_obj_desc_t *get_desc() const {return desc;}

	void rotate90();

	/**
	* the front image, drawn before vehicles
	* @author V. Meyer
	*/
	image_id get_image() const {
		return hang ? desc->get_back_slope_image_id(hang) :
			(diagonal ? desc->get_back_diagonal_image_id(dir) : desc->get_back_image_id(dir));
	}

	/**
	* the front image, drawn after everything else
	* @author V. Meyer
	*/
	image_id get_front_image() const {
		return hang ? desc->get_front_slope_image_id(hang) :
			diagonal ? desc->get_front_diagonal_image_id(dir) : desc->get_front_image_id(dir);
	}

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return the object type.
	* @author Hj. Malthaner
	*/

#ifdef INLINE_OBJ_TYPE
#else
	typ get_typ() const { return wayobj; }
#endif

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return desc ? desc->get_wtyp() : invalid_wt; }

	void calc_image();

	/**
	* Speichert den Zustand des Objekts.
	*
	* @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	* soll.
	* @author Hj. Malthaner
	*/
	void rdwr(loadsave_t *file);

	// subtracts cost
	void cleanup(player_t *player);

	const char*  is_deletable(const player_t *player) OVERRIDE;
	bool clashes_with_halt() {
		return get_desc()->is_noise_barrier();
	}

	/**
	* calculate image after loading
	* @author prissi
	*/
	void finish_rd();

	// specific for wayobj
	void set_dir(ribi_t::ribi d) { dir = d; calc_image(); }
	ribi_t::ribi get_dir() const { return dir; }

	/* the static routines */
private:
	static stringhashtable_tpl<way_obj_desc_t *> table;

public:
	static const way_obj_desc_t *default_oberleitung;

	// use this constructor; it will extend a matching existing wayobj
	static const char *extend_wayobj_t(koord3d pos, player_t *owner, ribi_t::ribi dir, const way_obj_desc_t *desc);

	static bool register_desc(way_obj_desc_t *desc);
	static bool successfully_loaded();

	// search an object (currently only used by AI for catenary)
	static const way_obj_desc_t *get_overhead_line(waytype_t wt,uint16 time);

	static const way_obj_desc_t *find_desc(const char *);

	/**
	 * Fill menu with icons of given stops from the list
	 * @author Hj. Malthaner
	 */
	static void fill_menu(tool_selector_t *tool_selector, waytype_t wtyp, sint16 sound_ok);

	static stringhashtable_tpl<way_obj_desc_t *>* get_all_wayobjects() { return &table; }
};

#endif
