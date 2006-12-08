/*
 * schiene.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_schiene_h
#define boden_wege_schiene_h


#include "weg.h"
#include "../../convoihandle_t.h"

class vehikel_t;

/**
 * Klasse für Schienen in Simutrans.
 * Auf den Schienen koennen Züge fahren.
 * Jede Schiene gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class schiene_t : public weg_t
{
protected:
	/**
	* Bound when this block was sucessfully reserved by the convoi
	* @author prissi
	*/
	convoihandle_t reserved;

public:
	static const weg_besch_t *default_schiene;

	/**
	* File loading constructor.
	* @author Hj. Malthaner
	*/
	schiene_t(karte_t *welt, loadsave_t *file);

	schiene_t(karte_t *welt);

	virtual waytype_t gib_waytype() const {return track_wt;}

	/**
	* Calculates the image of this pice of railroad track.
	*
	* @author Hj. Malthaner
	*/
	void calc_bild(koord3d) { weg_t::calc_bild(); }

	/**
	* @return Infotext zur Schiene
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const;

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool can_reserve( convoihandle_t c) const { return !reserved.is_bound()  ||  c==reserved; }

	/**
	* true, if this rail can be reserved
	* @author prissi
	*/
	bool is_reserved() const { return reserved.is_bound(); }

	/**
	* true, then this rail was reserved
	* @author prissi
	*/
	bool reserve( convoihandle_t c);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve( convoihandle_t c);

	/**
	* releases previous reservation
	* @author prissi
	*/
	bool unreserve( vehikel_t *);

	/**
	* gets the related convoi
	* @author prissi
	*/
	convoihandle_t get_reserved_convoi() const {return reserved;}

	void rdwr(loadsave_t *file);
};

#endif
