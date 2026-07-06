/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef OBJ_WAY_RUNWAY_H
#define OBJ_WAY_RUNWAY_H

#ifdef DEBUG
// prints the number of reservations an cleared runway path to takeoff
#define DEBUG_RUNWAYS
#endif

#include "../../tpl/vector_tpl.h"
#include "../../convoihandle.h"

#include "schiene.h"


/**
 * Class for aiport runaway in Simutrans.
 * system type==1 are for takeoff
 */
class runway_t : public schiene_t
{
private:
	vector_tpl<convoihandle_t> reservations;
	koord3d takeoff;

public:
	static const way_desc_t *default_runway;

	runway_t(loadsave_t *file);

	runway_t();

	inline waytype_t get_waytype() const OVERRIDE {return air_wt;}

	/**
	* @param[out] buf additional info is reservation!
	*/
	void info(cbuffer_t & buf) const OVERRIDE;

	void rdwr(loadsave_t *file) OVERRIDE;

	// the code below is only used for runways
	// the more reservations, the higher the cost for landing there.
	// Should quickly lead to equal usage of runways
	uint32 get_reservation_count() const { return reservations.get_count(); }

	bool can_reserve(koord3d ask_takeoff);

	using schiene_t::can_reserve;

	void add_convoi_reservation(convoihandle_t cnv) { reservations.append_unique(cnv); }

	void add_convoi_reservation(convoihandle_t cnv, koord3d targetpos) { reservations.append_unique(cnv); takeoff = targetpos; }

	void remove_convoi_reservation( convoihandle_t cnv ) { reservations.remove(cnv); }

	bool unreserve( convoihandle_t cnv ) OVERRIDE { reservations.remove(cnv); schiene_t::unreserve(cnv); return true; }

#ifdef DEBUG_RUNWAYS
#ifdef MULTI_THREAD
	virtual void display_after(int xpos, int ypos, const sint8 clip_num) const OVERRIDE;
#else
	virtual void display_after(int xpos, int ypos, bool is_global) const OVERRIDE;
#endif

	FLAGGED_PIXVAL get_outline_colour() const OVERRIDE;
#endif
};

#endif
