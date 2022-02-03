/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#ifndef BODEN_WEGE_SCHIENE_H
#define BODEN_WEGE_SCHIENE_H


#include "weg.h"
#include "../../convoihandle.h"

class vehicle_t;

/**
 * Class for Rails in Simutrans.
 * Trains can run over rails.
 * Every rail belongs to a section block
 */
class schiene_t : public weg_t
{
protected:
	/**
	* Bound when this block was successfully reserved by the convoi
	*/
	convoihandle_t reserved;

public:
	static const way_desc_t *default_schiene;

	static bool show_reservations;

	/**
	* File loading constructor.
	*/
	schiene_t(loadsave_t *file);

	schiene_t();

	waytype_t get_waytype() const OVERRIDE {return track_wt;}

	/**
	* @param[out] buf additional info is reservation!
	*/
	void info(cbuffer_t & buf) const OVERRIDE;

	/**
	* true, if this rail can be reserved
	*/
	bool can_reserve(convoihandle_t c) const { return !reserved.is_bound()  ||  c==reserved; }

	/**
	* true, if this rail can be reserved
	*/
	bool is_reserved() const { return reserved.is_bound(); }

	/**
	* true, then this rail was reserved
	*/
	bool reserve(convoihandle_t c, ribi_t::ribi dir);

	/**
	* releases previous reservation
	*/
	virtual bool unreserve( convoihandle_t c);

	/**
	* releases previous reservation
	*/
	bool unreserve( vehicle_t *) { return unreserve(reserved); }

	/* called before deletion;
	 * last chance to unreserve tiles ...
	 */
	void cleanup(player_t *player) OVERRIDE;

	/**
	* gets the related convoi
	*/
	convoihandle_t get_reserved_convoi() const {return reserved;}

	void rdwr(loadsave_t *file) OVERRIDE;

	/**
	 * if a function return here a value with TRANSPARENT_FLAGS set
	 * then a transparent outline with the color form the lower 8 Bit is drawn
	 */
	FLAGGED_PIXVAL get_outline_colour() const OVERRIDE;

	/*
	 * to show reservations if needed
	 */
	image_id get_outline_image() const OVERRIDE { return weg_t::get_image(); }
};


template<> inline schiene_t* obj_cast<schiene_t>(obj_t* const d)
{
	return dynamic_cast<schiene_t*>(d);
}

#endif
