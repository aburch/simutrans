/*
 * simware.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>
#include <string.h>

#include "simio.h"
#include "simmem.h"
#include "simdebug.h"
#include "simtypes.h"
#include "simware.h"

#include "dataobj/translator.h"
#include "dataobj/loadsave.h"
#include "dataobj/koord.h"

#include "besch/ware_besch.h"
#include "bauer/warenbauer.h"



const ware_besch_t *ware_t::index_to_besch[256];



ware_t::ware_t() : ziel(-1, -1), zwischenziel(-1, -1), zielpos(-1, -1)
{
	menge = 0;
	index = 0;
}


ware_t::ware_t(const ware_besch_t *wtyp) : ziel(-1, -1), zwischenziel(-1, -1), zielpos(-1, -1)
{
	menge = 0;
	index = warenbauer_t::gib_index(wtyp);
}

ware_t::ware_t(loadsave_t *file)
{
	rdwr(file);
}


void
ware_t::setze_typ(const ware_besch_t* type)
{
	index = type->gib_index();
}



void
ware_t::rdwr(loadsave_t *file)
{
	sint32 amount = menge;
	file->rdwr_long(amount, " ");
	menge = amount;
	if(file->get_version()<99008) {
		sint32 max;
		file->rdwr_long(max, " ");
	}

	const char *typ = NULL;
	uint8 catg=0;

	if(file->is_saving()) {
		typ = gib_typ()->gib_name();
	}
	if(file->get_version()>=88005) {
		file->rdwr_byte(catg,"c");
	}
	file->rdwr_str(typ, " ");
	if(file->is_loading()) {
		const ware_besch_t *type = warenbauer_t::gib_info(typ);
		guarded_free(const_cast<char *>(typ));
		if(type==NULL) {
			dbg->warning("ware_t::rdwr()","unknown ware of catg %d!",catg);
			index = warenbauer_t::gib_info_catg(catg)->gib_index();
			menge = 0;
		}
		else {
			index = type->gib_index();
		}
	}
	ziel.rdwr(file);
	zwischenziel.rdwr(file);
	zielpos.rdwr(file);
}
