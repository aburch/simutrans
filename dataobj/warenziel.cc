/*
 * CLASS DEPRACATED IN EXPERIMENTAL
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 

#include <stdio.h>

#include "warenziel.h"
#include "koord.h"

#include "../simmem.h"
#include "../besch/ware_besch.h"
#include "../bauer/warenbauer.h"

#include "../simtypes.h"
#include "loadsave.h"

warenziel_t::warenziel_t(loadsave_t *file)
{
	rdwr(file);
}


void
warenziel_t::rdwr(loadsave_t *file)
{
	// dummy ...
	koord ziel;
	ziel.rdwr(file);

	if(file->is_saving()) {
		const char *tn = warenbauer_t::get_info_catg_index(catg_index)->get_name();
		file->rdwr_str(tn);
	}
	else {
		char tn[256];
		file->rdwr_str(tn, lengthof(tn));
		halt = halthandle_t();
		catg_index = warenbauer_t::get_info(tn)->get_catg_index();
	}
}
*/
