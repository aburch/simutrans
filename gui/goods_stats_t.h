/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Display information about each configured good
 * as a list like display
 * @author Hj. Malthaner
 */

#ifndef good_stats_t_h
#define good_stats_t_h

#include "../simtypes.h"
#include "components/gui_aligned_container.h"

template<class T> class vector_tpl;
class goods_desc_t;

class goods_stats_t : public gui_aligned_container_t
{
	static karte_ptr_t welt;

public:
	goods_stats_t() {}

	// update list and resize
	void update_goodslist(vector_tpl<const goods_desc_t*>, int bonus);
};

#endif
