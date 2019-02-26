/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef obj_signal_h
#define obj_signal_h

#include "roadsign.h"

#include "../simobj.h"


/**
 * Signals for rail tracks.
 *
 * @see blockstrecke_t
 * @see blockmanager
 * @author Hj. Malthaner
 */
class signal_t : public roadsign_t
{
public:
	signal_t(loadsave_t *file);
	signal_t(player_t *player, koord3d pos, ribi_t::ribi dir,const roadsign_desc_t *desc, bool preview = false) : roadsign_t(player,pos,dir,desc,preview) { state = rot;}

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	void info(cbuffer_t & buf) const OVERRIDE;

	typ get_typ() const OVERRIDE { return obj_t::signal; }
	const char *get_name() const OVERRIDE {return "Signal";}

	/**
	* Calculate the actual image
	*/
	void calc_image() OVERRIDE;
};

#endif
