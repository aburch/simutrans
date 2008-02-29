/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#ifndef good_stats_t_h
#define good_stats_t_h

#include "../simtypes.h"
#include "../ifc/gui_komponente.h"


/**
 * Display information about each configured good
 * as a list like display
 * @author Hj. Malthaner
 */
class goods_stats_t : public gui_komponente_t
{
private:
	uint16 *goodslist;
	int bonus;

public:
	goods_stats_t();

	void update_goodslist(unsigned short *g, int b) {goodslist = g; bonus = b; }

	/**
	* Zeichnet die Komponente
	* @author Hj. Malthaner
	*/
	void zeichnen(koord offset);
};

#endif
