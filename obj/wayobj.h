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
#include "../dataobj/ribi.h"
#include "../besch/way_obj_besch.h"
#include "../tpl/vector_tpl.h"
#include "../tpl/stringhashtable_tpl.h"

class spieler_t;
class karte_t;
class koord;
class grund_t;
class werkzeug_waehler_t;

/**
 * Overhead powelines for elctrifed tracks.
 *
 * @author Hj. Malthaner
 */
class wayobj_t : public obj_no_info_t
{
private:
	const way_obj_besch_t *besch;

	uint8 diagonal:1;
	uint8 hang:7;

	// direction of this wayobj
	ribi_t::ribi dir;

	ribi_t::ribi find_next_ribi(const grund_t *start, const ribi_t::ribi dir, const waytype_t wt) const;


public:
	wayobj_t(koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch);

	wayobj_t(loadsave_t *file);

	virtual ~wayobj_t();

	const way_obj_besch_t *get_besch() const {return besch;}

	void rotate90();

	/**
	* the front image, drawn before vehicles
	* @author V. Meyer
	*/
	image_id get_bild() const {
		return hang ? besch->get_back_slope_image_id(hang) :
			(diagonal ? besch->get_back_diagonal_image_id(dir) : besch->get_back_image_id(dir));
	}

	/**
	* the front image, drawn after everything else
	* @author V. Meyer
	*/
	image_id get_after_bild() const {
		return hang ? besch->get_front_slope_image_id(hang) :
			diagonal ? besch->get_front_diagonal_image_id(dir) : besch->get_front_image_id(dir);
	}

	/**
	* 'Jedes Ding braucht einen Typ.'
	* @return Gibt den typ des Objekts zurück.
	* @author Hj. Malthaner
	*/
	typ get_typ() const { return wayobj; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return besch ? besch->get_wtyp() : invalid_wt; }

	void calc_bild();

	/**
	* Speichert den Zustand des Objekts.
	*
	* @param file Zeigt auf die Datei, in die das Objekt geschrieben werden
	* soll.
	* @author Hj. Malthaner
	*/
	void rdwr(loadsave_t *file);

	// substracts cost
	void entferne(spieler_t *sp);

	const char* ist_entfernbar(const spieler_t *sp) OVERRIDE;

	/**
	* calculate image after loading
	* @author prissi
	*/
	void laden_abschliessen();

	// specific for wayobj
	void set_dir(ribi_t::ribi d) { dir = d; calc_bild(); }
	ribi_t::ribi get_dir() const { return dir; }

	/* the static routines */
private:
	static vector_tpl<const way_obj_besch_t *> liste;
	static stringhashtable_tpl<const way_obj_besch_t *> table;

public:
	static const way_obj_besch_t *default_oberleitung;

	// use this constructor; it will extend a matching existing wayobj
	static void extend_wayobj_t(koord3d pos, spieler_t *besitzer, ribi_t::ribi dir, const way_obj_besch_t *besch);

	static bool register_besch(way_obj_besch_t *besch);
	static bool alles_geladen();

	// search an object (currently only used by AI for caternary)
	static const way_obj_besch_t *wayobj_search(waytype_t wt,waytype_t own,uint16 time);

	static const way_obj_besch_t *find_besch(const char *);

	/**
	 * Fill menu with icons of given stops from the list
	 * @author Hj. Malthaner
	 */
	static void fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, sint16 sound_ok);
};

#endif
