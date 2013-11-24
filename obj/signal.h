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
 * Signale für die Bahnlinien.
 *
 * @see blockstrecke_t
 * @see blockmanager
 * @author Hj. Malthaner
 */
class signal_t : public roadsign_t
{
public:
	signal_t(loadsave_t *file);
	signal_t(spieler_t *sp, koord3d pos, ribi_t::ribi dir,const roadsign_besch_t *besch) : roadsign_t(sp,pos,dir,besch) { zustand = rot;}

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	virtual void info(cbuffer_t & buf) const;

	typ get_typ() const { return obj_t::signal; }
	const char *get_name() const {return "Signal";}

	/**
	* berechnet aktuelles bild
	*/
	void calc_bild();
};

#endif
