/*
 * way_desc.cc
 *
 * Copyright (c) 1997 - 2012 Hj. Malthaner and
 *        the simutrans development team
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include "way_desc.h"
#include "../boden/wege/weg.h"
#include "../network/checksum.h"

waytype_t way_desc_t::get_finance_waytype() const
{
	return get_styp() == type_tram ? tram_wt : get_wtyp();
}

void way_desc_t::calc_checksum(checksum_t *chk) const
{
	obj_desc_transport_infrastructure_t::calc_checksum(chk);
	chk->input(axle_load);
	chk->input(styp);
	chk->input(has_double_slopes());

	//Extended values
	chk->input(way_constraints.get_permissive());
	chk->input(way_constraints.get_prohibitive());
}