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

class werkzeug_waehler_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class roadsign_t : public obj_t, public sync_steppable
{
protected:
	image_id bild;
	image_id after_bild;

	enum { SHOW_FONT=1, SHOW_BACK=2, SWITCH_AUTOMATIC=16 };

	uint8 zustand:2;	// counter for steps ...
	uint8 dir:4;

	uint8 automatic:1;
	uint8 ticks_ns;
	uint8 ticks_ow;
	uint8 ticks_offset;

	sint8 after_yoffset, after_xoffset;

	const roadsign_besch_t *besch;

public:
	enum signalzustand {rot=0, gruen=1, naechste_rot=2 };

	/*
	 * return direction or the state of the traffic light
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi get_dir() const 	{ return dir; }

	/*
	* sets ribi mask of the sign
	* Caution: it will modify way ribis directly!
	*/
	void set_dir(ribi_t::ribi dir);

	void set_zustand(signalzustand z) {zustand = z; calc_bild();}
	signalzustand get_zustand() { return (signalzustand)zustand; }

	typ get_typ() const { return roadsign; }
	const char* get_name() const { return "Roadsign"; }

	// assuming this is a private way sign
	uint16 get_player_mask() const { return (ticks_ow<<8)|ticks_ns; }

	/**
	 * waytype associated with this object
	 */
	waytype_t get_waytype() const { return besch ? besch->get_wtyp() : invalid_wt; }

	roadsign_t(loadsave_t *file);
	roadsign_t(spieler_t *sp, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t* besch);

	const roadsign_besch_t *get_besch() const {return besch;}

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	~roadsign_t();

	// since traffic lights need their own window
	void zeige_info();

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	virtual void info(cbuffer_t & buf) const;

	/**
	 * berechnet aktuelles bild
	 */
	virtual void calc_bild();

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

	inline void set_bild( image_id b ) { bild = b; }
	image_id get_bild() const { return bild; }

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id get_after_bild() const { return after_bild; }

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
	void entferne(spieler_t *sp);

	void laden_abschliessen();

	// static routines from here
private:
	static vector_tpl<const roadsign_besch_t *> liste;
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
	static void fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, sint16 sound_ok);

	static const roadsign_besch_t *roadsign_search(roadsign_besch_t::types flag, const waytype_t wt, const uint16 time);

	static const roadsign_besch_t *find_besch(const char *name) { return table.get(name); }
};

#endif
