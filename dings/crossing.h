/*
 * Copyright (c) 2007 prissi
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_crossing_h
#define dings_crossing_h

#include "../simdings.h"
#include "../simtypes.h"
#include "../simimg.h"
#include "../besch/kreuzung_besch.h"
#include "../ifc/sync_steppable.h"
#include "../tpl/minivec_tpl.h"
#include "../tpl/slist_tpl.h"

class vehikel_basis_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class crossing_t : public ding_t, public sync_steppable
{
protected:
	image_id after_bild, bild;
	uint8 zustand:4;	// counter for steps ...
	uint8 ns:1;				// direction
	uint8 phase;

	minivec_tpl<const vehikel_basis_t *>on_way1;
	minivec_tpl<const vehikel_basis_t *>on_way2;

	const kreuzung_besch_t *besch;

	uint32 timer;	// change animation state here ...

public:
	enum ding_t::typ gib_typ() const { return crossing; }
	const char* gib_name() const { return "Kreuzung"; }

	crossing_t(karte_t *welt, loadsave_t *file);
	crossing_t(karte_t *welt, spieler_t *sp, koord3d pos, waytype_t wt1, waytype_t wt2);

	/**
	 * signale muessen bei der destruktion von der
	 * Blockstrecke abgemeldet werden
	 */
	virtual ~crossing_t();

	const kreuzung_besch_t *gib_besch() const { return besch; }

	/**
	 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	 * Beobachtungsfenster angezeigt wird.
	 * @author Hj. Malthaner
	 */
	void info(cbuffer_t & buf) const;

	// returns true, if the crossing can be passed by this vehicle
	bool request_crossing( const vehikel_basis_t * );

	// adds to crossing
	void add_to_crossing( const vehikel_basis_t *v );

	// removes the vehicle from the crossing
	void release_crossing( const vehikel_basis_t * );

	/* states of the crossing;
	 * since way2 has priority over way1 there is a third state, during a closing request
	 */
	enum { CROSSING_INVALID=0, CROSSING_OPEN, CROSSING_REQUEST_CLOSE, CROSSING_CLOSED };
	void set_state( uint8 new_state );
	uint8 get_state() { return zustand; }

	/**
	 * Dient zur Neuberechnung des Bildes
	 * @author Hj. Malthaner
	 */
	void calc_bild();

	// changes the state of a traffic light
	image_id gib_bild() const { return bild; }

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id gib_after_bild() const { return after_bild; }

	void rdwr(loadsave_t *file);

	// substracts cost
	void entferne(spieler_t *sp);

	void laden_abschliessen();

	// to be used for animations
	bool sync_step(long delta_t) {return false;}


	// static routines from here
private:
	static slist_tpl<const kreuzung_besch_t *> liste;
	static kreuzung_besch_t *can_cross_array[8][8];

public:
	static bool register_besch(kreuzung_besch_t *besch);
	static bool alles_geladen() {return true; }

	static const kreuzung_besch_t *get_crossing(const waytype_t ns, const waytype_t ow) {
		if(ns>7  ||  ow>7) return NULL;
		return can_cross_array[(int)ns][(int)ow];
	}
};

#endif
