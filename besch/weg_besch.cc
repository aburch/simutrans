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
#include "../boden/wege/weg.h"


waytype_t weg_besch_t::get_finance_waytype() const
{
	return get_styp() == weg_t::type_tram ? tram_wt : get_wtyp();
}
