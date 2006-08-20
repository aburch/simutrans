/*
 * roadsign.h
 *
 * Copyright (c) 2005 prissi
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_roadsign_h
#define dings_roadsign_h

#include "../simdings.h"
#include "../besch/roadsign_besch.h"

class roadsign_besch_t;
class werkzeug_parameter_waehler_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class roadsign_t : public ding_t
{
private:
     // foreground
    uint16 after_bild;

protected:
	unsigned char zustand;	// counter for steps ...

	unsigned char blockend;

	unsigned char dir;

	const roadsign_besch_t *besch;

	uint32 last_switch;	// change state here ...

public:
	/*
	 * return direction or the state of the traffic light
	 * @author Hj. Malthaner
	 */
	ribi_t::ribi get_dir() const 	{ return dir; };
	void set_dir(ribi_t::ribi dir);

	enum ding_t::typ gib_typ() const {return roadsign;};
	const char *gib_name() const {return "Roadsign";};

	roadsign_t(karte_t *welt, loadsave_t *file);
	roadsign_t(karte_t *welt, koord3d pos, ribi_t::ribi dir, const roadsign_besch_t* besch);

	const roadsign_besch_t *gib_besch() const {return besch;};

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
	void calc_bild();


	// true, if a free route choose point (these are always single way the avoid recalculation of long return routes)
	bool roadsign_t::is_free_route(uint8 check_dir) const { return besch->is_free_route() &&  check_dir==dir; };



	// changes the state of a traffic light
	virtual bool step(long);



    /**
     * Falls etwas nach den Vehikeln gemalt werden muß.
     * @author V. Meyer
     */
    virtual int gib_after_bild() const {return after_bild;};

    	void rdwr(loadsave_t *file);

	/**
	 * Wird nach dem Laden der Welt aufgerufen - üblicherweise benutzt
	 * um das Aussehen des Dings an Boden und Umgebung anzupassen
	 *
	 * @author Hj. Malthaner
	 */
	virtual void laden_abschliessen();

	// substracts cost
	void entferne(spieler_t *sp);

    static bool register_besch(roadsign_besch_t *besch);
    static bool alles_geladen();

	/**
	 * Fill menu with icons of given stops from the list
	 * @author Hj. Malthaner
	 */
	static void roadsign_t::fill_menu(werkzeug_parameter_waehler_t *wzw,
		int (* werkzeug)(spieler_t *, karte_t *, koord, value_t),
		int sound_ok,
		int sound_ko,
		int cost);

};

#endif
