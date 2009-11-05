/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
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
	index = wtyp->get_index();
}

ware_t::ware_t(karte_t *welt,loadsave_t *file)
{
	rdwr(welt,file);
}


void ware_t::set_besch(const ware_besch_t* type)
{
	index = type->get_index();
}



void ware_t::rdwr(karte_t *welt,loadsave_t *file)
{
	sint32 amount = menge;
	file->rdwr_long(amount, " ");
	menge = amount;
	if(file->get_version()<99008) {
		sint32 max;
		file->rdwr_long(max, " ");
	}

	uint8 catg=0;
	if(file->get_version()>=88005) {
		file->rdwr_byte(catg,"c");
	}

	if(file->is_saving()) {
		const char *typ = NULL;
		typ = get_besch()->get_name();
		file->rdwr_str(typ);
	}
	else {
		char typ[256];
		file->rdwr_str(typ,256);
		const ware_besch_t *type = warenbauer_t::get_info(typ);
		if(type==NULL) {
			dbg->warning("ware_t::rdwr()","unknown ware of catg %d!",catg);
			index = warenbauer_t::get_info_catg(catg)->get_index();
			menge = 0;
		}
		else {
			index = type->get_index();
		}
	}
	// convert coordinate to halt indices
	if(file->is_saving()) {
		koord ziel_koord = ziel.is_bound() ? ziel->get_basis_pos() : koord::invalid;
		ziel_koord.rdwr(file);
		koord zwischenziel_koord = zwischenziel.is_bound() ? zwischenziel->get_basis_pos() : koord::invalid;
		zwischenziel_koord.rdwr(file);
	}
	else {
		koord ziel_koord;
		ziel_koord.rdwr(file);
		ziel = welt->get_halt_koord_index(ziel_koord);
		koord zwischen_ziel_koord;
		zwischen_ziel_koord.rdwr(file);
		zwischenziel = welt->get_halt_koord_index(zwischen_ziel_koord);
	}
	zielpos.rdwr(file);
}



void ware_t::laden_abschliessen(karte_t *welt,spieler_t *sp)
{
	// since some halt was referred by with several koordinates
	// this routine will correct it
	if(ziel.is_bound()) {
		ziel = welt->lookup(ziel->get_init_pos())->get_halt();
	}
	if(zwischenziel.is_bound()) {
		zwischenziel = welt->lookup(zwischenziel->get_init_pos())->get_halt();
	}
}
