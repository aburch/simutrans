/*
 * goods_stats_t.h
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#ifndef good_stats_t_h
#define good_stats_t_h

#include "../simtypes.h"
#include "../ifc/gui_komponente.h"
class karte_t;

/**
 * Display information about each configured good
 * as a list like display
 * @author Hj. Malthaner
 */
class goods_stats_t : public gui_komponente_t
{
private:
	karte_t *welt;
	uint16 *goodslist;
	int bonus;

public:

	goods_stats_t(karte_t *welt);

	void update_goodslist(unsigned short *g, int b) {goodslist = g; bonus = b; }

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
