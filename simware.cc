/*
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
#include "simhalt.h"
#include "simtypes.h"
#include "simworld.h"
#include "simware.h"
#include "dataobj/loadsave.h"
#include "dataobj/koord.h"

#include "besch/ware_besch.h"
#include "bauer/warenbauer.h"



const ware_besch_t *ware_t::index_to_besch[256];



ware_t::ware_t() : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = 0;
}


ware_t::ware_t(const ware_besch_t *wtyp) : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = wtyp->gib_index();
}

ware_t::ware_t(karte_t *welt,loadsave_t *file)
{
	rdwr(welt,file);
}


void
ware_t::setze_besch(const ware_besch_t* type)
{
	index = type->gib_index();
}



void
ware_t::rdwr(karte_t *welt,loadsave_t *file)
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
		typ = gib_besch()->gib_name();
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
	// convert coordinate to halt indices
	if(file->is_saving()) {
		koord ziel_koord = ziel.is_bound() ? ziel->gib_basis_pos() : koord::invalid;
		koord zwischenziel_koord = zwischenziel.is_bound() ? zwischenziel->gib_basis_pos() : koord::invalid;
		ziel_koord.rdwr(file);
		zwischenziel_koord.rdwr(file);
	}
	else {
		koord ziel_koord;
		ziel_koord.rdwr(file);
		ziel = welt->get_halt_koord_index(ziel_koord);
		ziel_koord.rdwr(file);
		zwischenziel = welt->get_halt_koord_index(ziel_koord);
	}
	zielpos.rdwr(file);
}



void
ware_t::laden_abschliessen(karte_t *welt)
{
	// since some halt was referred by with several koordinates
	// this routine will correct it
	if(ziel.is_bound()) {
		ziel = welt->lookup(ziel->gib_init_pos())->gib_halt();
	}
	if(zwischenziel.is_bound()) {
		zwischenziel = welt->lookup(zwischenziel->gib_init_pos())->gib_halt();
	}
}
