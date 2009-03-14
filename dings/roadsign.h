/*
 * Copyright (c) 2005 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dings_roadsign_h
#define dings_roadsign_h

#include "../simdings.h"
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
class roadsign_t : public ding_t, public sync_steppable
{
protected:
	image_id bild;
	image_id after_bild;

	enum { SHOW_FONT=1, SHOW_BACK=2, SWITCH_AUTOMATIC=16 };

	uint8 zustand;	// counter for steps ...

	uint8 automatic:1;

	uint8 dir;

	sint8 after_offset;

	const roadsign_besch_t *besch;

public:
	enum signalzustand {rot=0, gruen=1, naechste_rot=2 };

	/*
	 * return direction or the state of the traffic light
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi get_dir() const 	{ return dir; }
	void set_dir(ribi_t::ribi dir);

	void set_zustand(enum signalzustand z) {zustand = z; calc_bild();}
	enum signalzustand get_zustand() {return (enum signalzustand)zustand;}

	virtual enum ding_t::typ get_typ() const { return roadsign; }
	const char* get_name() const { return "Roadsign"; }

	roadsign_t(karte_t *welt, loadsave_t *file);
	roadsign_t(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t* besch);

	const roadsign_besch_t *get_besch() const {return besch;}

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	~roadsign_t();

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
	bool is_free_route(uint8 check_dir) const { return besch->is_free_route() &&  check_dir == dir; }

	// changes the state of a traffic light
	bool sync_step(long);

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
	void display_after(int xpos, int ypos, bool dirty) const;

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
	static void fill_menu(werkzeug_waehler_t *wzw, waytype_t wtyp, const karte_t *welt);

	static const roadsign_besch_t *roadsign_search(uint8 flag,const waytype_t wt,const uint16 time);

	static const roadsign_besch_t *find_besch(const char *name) { return table.get(name); }
};

#endif
