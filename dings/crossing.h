/*
 * Copyright (c) 2007 prissi
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef dings_crossing_h
#define dings_crossing_h

#include "../simtypes.h"
#include "../simimg.h"
#include "../besch/kreuzung_besch.h"
#include "../dataobj/crossing_logic.h"

class vehikel_basis_t;

/**
 * road sign for traffic (one way minimum speed, traffic lights)
 * @author Hj. Malthaner
 */
class crossing_t : public ding_t
{
protected:
	image_id after_bild, bild;
	uint8 ns;				// direction
	uint8 zustand;	// only needed for loading ...
	crossing_logic_t *logic;
	const kreuzung_besch_t *besch;

public:
	enum ding_t::typ get_typ() const { return crossing; }
	const char* get_name() const { return "Kreuzung"; }

	crossing_t(karte_t *welt, loadsave_t *file);
	crossing_t(karte_t *welt, spieler_t *sp, koord3d pos, const kreuzung_besch_t *besch, uint8 ns = 0);

	virtual ~crossing_t();

	void rotate90();

	const kreuzung_besch_t *get_besch() const { return besch; }

	/**
	 * @return string (only used for debugg at the moment)
	 * @author prissi
	 */
	void info(cbuffer_t & buf) const { logic->info(buf); }

	// no info
	void zeige_info() {}

	/**
	 * @returns NULL wenn OK, ansonsten eine Fehlermeldung
	 * @author Hj. Malthaner
	 */
	virtual const char *ist_entfernbar(const spieler_t *sp);

	// returns true, if the crossing can be passed by this vehicle
	bool request_crossing( const vehikel_basis_t *v ) { return logic->request_crossing( v ); }

	// adds to crossing
	void add_to_crossing( const vehikel_basis_t *v ) { return logic->add_to_crossing( v ); }

	// removes the vehicle from the crossing
	void release_crossing( const vehikel_basis_t *v ) { return logic->release_crossing( v ); }

	crossing_logic_t::crossing_state_t get_state() { return logic->get_state(); }

	// called from the logic directly
	void state_changed();

	uint8 get_dir() { return ns; }

	void set_logic( crossing_logic_t *l ) { logic = l; }
	crossing_logic_t *get_logic() { return logic; }

	/**
	 * Dient zur Neuberechnung des Bildes
	 * @author Hj. Malthaner
	 */
	void calc_bild();

	// changes the state of a traffic light
	image_id get_bild() const { return bild; }

	/**
	* For the front image hiding vehicles
	* @author prissi
	*/
	image_id get_after_bild() const { return after_bild; }

	void rdwr(loadsave_t *file);

	void laden_abschliessen();
};

#endif
