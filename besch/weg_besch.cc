/*
 * weg_besch.cc
 *
 * Copyright (c) 1997 - 2012 Hj. Malthaner and
 *        the simutrans development team
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include "weg_besch.h"
#include "../network/checksum.h"


waytype_t weg_besch_t::get_finance_waytype() const
{
	return get_styp() == type_tram ? tram_wt : get_wtyp();
}

void weg_besch_t::calc_checksum(checksum_t *chk) const
{
	obj_besch_transport_infrastructure_t::calc_checksum(chk);
	chk->input(max_weight);
	chk->input(styp);
	chk->input(has_double_slopes());
}
