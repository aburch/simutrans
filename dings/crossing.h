/*
 * roadsign.h
 *
 * Copyright (c) 2005 prissi
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_crossing_h
#define dings_crossing_h

#include "../simdings.h"
#include "../simtypes.h"
#include "../besch/kreuzung_besch.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/inthashtable_tpl.h"

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class crossing_t : public ding_t, public sync_steppable
{
protected:
	image_id after_bild;
	uint8 zustand;	// counter for steps ...
	uint8 phase;

	const kreuzung_besch_t *besch;

	uint32 timer;	// change state here ...

public:
	enum ding_t::typ gib_typ() const { return crossing; }
	const char* gib_name() const { return "Kreuzung"; }

	crossing_t(karte_t *welt, loadsave_t *file);
	crossing_t(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t wt1, waytype_t wt2);

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	~crossing_t();

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

	// changes the state of a traffic light
	virtual bool sync_step(long);

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	virtual image_id gib_after_bild() const {return after_bild;}

	void rdwr(loadsave_t *file);

	// substracts cost
	void entferne(spieler_t *sp);

	void laden_abschliessen();

	// static routines from here
private:
	static slist_tpl<const kreuzung_besch_t *> liste;
	static kreuzung_besch_t *can_cross_array[16][16];

public:
	static bool register_besch(kreuzung_besch_t *besch);
	static bool alles_geladen() {return true; }

	static const kreuzung_besch_t *get_crossing(const waytype_t ns, const waytype_t ow) {
		if(ns>15  ||  ow>15) return NULL;
		return can_cross_array[(int)ns][(int)ow];
	}
};

#endif
