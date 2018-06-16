/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef boden_wege_schiene_h
#define boden_wege_schiene_h


#include "weg.h"
#include "../../convoihandle_t.h"

class vehicle_t;

/**
 * Class for Rails in Simutrans.
 * Trains can run over rails.
 * Every rail belongs to a section block
 *
 * @author Hj. Malthaner
 */
class schiene_t : public weg_t
{
protected:
	/**
	* Bound when this block was successfully reserved by the convoi
	* @author prissi
	*/
	convoihandle_t reserved;

public:
	static const way_desc_t *default_schiene;

	static bool show_reservations;

	/**
	* File loading constructor.
	* @author Hj. Malthaner
	*/
	schiene_t(loadsave_t *file);

	schiene_t();

	virtual waytype_t get_waytype() const {return track_wt;}

	/**
	* @return additional info is reservation!
	* @author prissi
	*/
	void info(cbuffer_t & buf) const;

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool can_reserve(convoihandle_t c) const { return !reserved.is_bound()  ||  c==reserved; }

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool is_reserved() const { return reserved.is_bound(); }

	/**
	* true, then this rail was reserved
	* @author prissi
	*/
	bool reserve(convoihandle_t c, ribi_t::ribi dir);

	/**
	* releases previous reservation
	* @author prissi
	*/
	virtual bool unreserve( convoihandle_t c);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve( vehicle_t *) { return unreserve(reserved); }

	/* called before deletion;
	 * last chance to unreserve tiles ...
	 */
	virtual void cleanup(player_t *player);

	/**
	* gets the related convoi
	* @author prissi
	*/
	convoihandle_t get_reserved_convoi() const {return reserved;}

	void rdwr(loadsave_t *file);

	/**
	 * if a function return here a value with TRANSPARENT_FLAGS set
	 * then a transparent outline with the color form the lower 8 Bit is drawn
	 * @author kierongreen
	 */
	virtual FLAGGED_PIXVAL get_outline_colour() const { return (show_reservations  &&  reserved.is_bound()) ? TRANSPARENT75_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_RED) : 0;}

	/*
	 * to show reservations if needed
	 */
	virtual image_id get_outline_image() const { return weg_t::get_image(); }
};


template<> inline schiene_t* obj_cast<schiene_t>(obj_t* const d)
{
	return dynamic_cast<schiene_t*>(d);
}

#endif
