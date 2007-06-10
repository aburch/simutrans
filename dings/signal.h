/*
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef dings_signal_h
#define dings_signal_h

#include "roadsign.h"

#include "../simdings.h"


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
	signal_t(karte_t *welt, loadsave_t *file) : roadsign_t(welt,file) { zustand = rot;}
	signal_t(karte_t *welt, spieler_t *sp, koord3d pos, ribi_t::ribi dir,const roadsign_besch_t *besch) : roadsign_t(welt,sp,pos,dir,besch) { zustand = rot;}

	/**
	* @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
	* Beobachtungsfenster angezeigt wird.
	* @author Hj. Malthaner
	*/
	virtual void info(cbuffer_t & buf) const;

	enum ding_t::typ gib_typ() const {return ding_t::signal;}
	const char *gib_name() const {return "Signal";}

	/**
	* berechnet aktuelles bild
	*/
	void calc_bild();
};

#endif
