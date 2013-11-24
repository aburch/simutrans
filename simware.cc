/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>
#include <string.h>

#include "simmem.h"
#include "simdebug.h"
#include "simfab.h"
#include "simhalt.h"
#include "simtypes.h"
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
	to_factory = 0;
}


ware_t::ware_t(const ware_besch_t *wtyp) : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = wtyp->get_index();
	to_factory = 0;
}

ware_t::ware_t(loadsave_t *file)
{
	rdwr(file);
}


void ware_t::set_besch(const ware_besch_t* type)
{
	index = type->get_index();
}



void ware_t::rdwr(loadsave_t *file)
{
	sint32 amount = menge;
	file->rdwr_long(amount);
	menge = amount;
	if(file->get_version()<99008) {
		sint32 max;
		file->rdwr_long(max);
	}

	if(  file->get_version()>=110005  ) {
		uint8 factory_going = to_factory;
		file->rdwr_byte(factory_going);
		to_factory = factory_going;
	}
	else if(  file->is_loading()  ) {
		to_factory = 0;
	}

	uint8 catg=0;
	if(file->get_version()>=88005) {
		file->rdwr_byte(catg);
	}

	if(file->is_saving()) {
		const char *typ = NULL;
		typ = get_besch()->get_name();
		file->rdwr_str(typ);
	}
	else {
		char typ[256];
		file->rdwr_str(typ, lengthof(typ));
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
	if(file->get_version() > 110005) {
		// save halt id directly
		if(file->is_saving()) {
			uint16 halt_id = ziel.is_bound() ? ziel.get_id() : 0;
			file->rdwr_short(halt_id);
			halt_id = zwischenziel.is_bound() ? zwischenziel.get_id() : 0;
			file->rdwr_short(halt_id);
		}
		else {
			uint16 halt_id;
			file->rdwr_short(halt_id);
			ziel.set_id(halt_id);
			file->rdwr_short(halt_id);
			zwischenziel.set_id(halt_id);
		}

	}
	else {
		// save halthandles via coordinates
		if(file->is_saving()) {
			koord ziel_koord = ziel.is_bound() ? ziel->get_basis_pos() : koord::invalid;
			ziel_koord.rdwr(file);
			koord zwischenziel_koord = zwischenziel.is_bound() ? zwischenziel->get_basis_pos() : koord::invalid;
			zwischenziel_koord.rdwr(file);
		}
		else {
			koord ziel_koord;
			ziel_koord.rdwr(file);
			ziel = haltestelle_t::get_halt_koord_index(ziel_koord);
			koord zwischen_ziel_koord;
			zwischen_ziel_koord.rdwr(file);
			zwischenziel = haltestelle_t::get_halt_koord_index(zwischen_ziel_koord);
		}
	}
	zielpos.rdwr(file);
	// restore factory-flag
	if(  file->get_version()<110005  &&  file->is_loading()  ) {
		if (fabrik_t::get_fab(zielpos)) {
			to_factory = 1;
		}
	}
}



void ware_t::laden_abschliessen(karte_t *welt)
{
	if(  welt->load_version<=111005  ) {
		// since some halt was referred by with several koordinates
		// this routine will correct it
		if(ziel.is_bound()) {
			ziel = haltestelle_t::get_halt_koord_index(ziel->get_init_pos());
		}
		if(zwischenziel.is_bound()) {
			zwischenziel = haltestelle_t::get_halt_koord_index(zwischenziel->get_init_pos());
		}
	}
	update_factory_target();
}


void ware_t::rotate90(sint16 y_size )
{
	zielpos.rotate90( y_size );
	update_factory_target();
}


void ware_t::update_factory_target()
{
	if (to_factory) {
		// assert that target coordinates are unique for cargo going to the same factory
		// as new cargo will be generated with possibly new factory coordinates
		fabrik_t *fab = fabrik_t::get_fab(zielpos );
		if (fab) {
			zielpos = fab->get_pos().get_2d();
		}
	}
}
