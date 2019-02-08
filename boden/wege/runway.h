/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_runway_h
#define boden_wege_runway_h

#include "../../tpl/vector_tpl.h"
#include "../../convoihandle_t.h"

#include "schiene.h"


/**
 * Class for aiport runaway in Simutrans.
 * system type==1 are for takeoff
 */
class runway_t : public schiene_t
{
private:
	vector_tpl<convoihandle_t> reservations;

public:
	static const way_desc_t *default_runway;

	runway_t(loadsave_t *file);

	runway_t();

	inline waytype_t get_waytype() const OVERRIDE {return air_wt;}

	/**
	* @return additional info is reservation!
	* @author prissi
	*/
	void info(cbuffer_t & buf) const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	// the code below is only used for runways
	// the more reservations, the higher the cost for landing there.
	// Should quickly lead to equal usage of runways
	uint32 get_reservation_count() const { return reservations.get_count(); }

	void add_convoi_reservation( convoihandle_t cnv ) { reservations.append_unique(cnv); }

	void remove_convoi_reservation( convoihandle_t cnv ) { reservations.remove(cnv); }

	bool unreserve( convoihandle_t cnv ) OVERRIDE { reservations.remove(cnv); schiene_t::unreserve(cnv); return true; }
};

#endif
