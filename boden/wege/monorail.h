/*
 * monorail.h
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef boden_wege_monorail_h
#define boden_wege_monorail_h


#include "schiene.h"


/**
 * Klasse für Schienen in Simutrans.
 * Auf den Schienen koennen Züge fahren.
 * Jede Schiene gehört zu einer Blockstrecke
 *
 * @author Hj. Malthaner
 * @version 1.0
 */
class monorail_t : public schiene_t
{
public:
	static const weg_besch_t *default_monorail;

	/**
	 * Basic constructor.
	 * @author prissi
	 */
	monorail_t(karte_t *welt) : schiene_t(welt) { setze_besch(default_monorail); }

	/**
	 * File loading constructor.
	 * @author prissi
	 */
	monorail_t(karte_t *welt, loadsave_t *file);

	/**
	 * Destruktor. Entfernt etwaige Debug-Meldungen vom Feld
	 * @author prissi
	 */
	virtual ~monorail_t() {}

	virtual const char *gib_typ_name() const {return "Monorail";}
	virtual waytype_t gib_typ() const {return monorail_wt;}

	/**
	 * @return Infotext zur Schiene
	 * @author Hj. Malthaner
	 */
	void info(cbuffer_t & buf) const;

	void rdwr(loadsave_t *file);
};

#endif
