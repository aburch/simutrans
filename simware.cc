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

#include "descriptor/goods_desc.h"
#include "bauer/goods_manager.h"



const goods_desc_t *ware_t::index_to_desc[256];



ware_t::ware_t() : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = 0;
	arrival_time = 0;
}


ware_t::ware_t(const goods_desc_t *wtyp) : ziel(), zwischenziel(), zielpos(-1, -1)
{
	//This constructor is called from simcity.cc
	menge = 0;
	index = wtyp->get_index();
	arrival_time = 0;
}

// Constructor for new revenue system: packet of cargo keeps track of its origin.
//@author: jamespetts
ware_t::ware_t(const goods_desc_t *wtyp, halthandle_t o) : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = wtyp->get_index();
	origin = o;
	arrival_time = 0;
}


ware_t::ware_t(loadsave_t *file)
{
	rdwr(file);
}


void ware_t::set_desc(const goods_desc_t* type)
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

	if(file->get_version()>=110005 && file->get_extended_version() < 12) 
	{
		// Was "to_factory" / "factory_going".
		uint8 dummy;
		file->rdwr_byte(dummy);
	}

	uint8 catg=0;
	if(file->get_version()>=88005) {
		file->rdwr_byte(catg);
	}

	if(file->is_saving()) {
		const char *typ = NULL;
		typ = get_desc()->get_name();
		file->rdwr_str(typ);
	}
	else {
		char typ[256];
		file->rdwr_str(typ, lengthof(typ));
		const goods_desc_t *type = goods_manager_t::get_info(typ);
		if(type==NULL) {
			dbg->warning("ware_t::rdwr()","unknown ware of catg %d!",catg);
			index = goods_manager_t::get_info_catg(catg)->get_index();
			menge = 0;
		}
		else {
			index = type->get_index();
		}
	}
	// convert coordinate to halt indices
	if(file->get_version() > 110005 && (file->get_extended_version() >= 10 || file->get_extended_version() == 0))
	{
		// save halt id directly
		if(file->is_saving()) 
		{
			uint16 halt_id = ziel.is_bound() ? ziel.get_id() : 0;
			file->rdwr_short(halt_id);
			halt_id = zwischenziel.is_bound() ? zwischenziel.get_id() : 0;
			file->rdwr_short(halt_id);
			if(file->get_extended_version() >= 1)
			{
				halt_id = origin.is_bound() ? origin.get_id() : 0;	
				file->rdwr_short(halt_id);
			}	
		}

		else
		{
			uint16 halt_id;
			file->rdwr_short(halt_id);
			ziel.set_id(halt_id);
			file->rdwr_short(halt_id);
			zwischenziel.set_id(halt_id);
			if(file->get_extended_version() >= 1)
			{
				file->rdwr_short(halt_id);			
				origin.set_id(halt_id);
			}
			else
			{
				origin = zwischenziel;
			}
		}
	}
	else 
	{
		if(file->is_saving()) 
		{
			koord ziel_koord = ziel.is_bound() ? ziel->get_basis_pos() : koord::invalid;
			ziel_koord.rdwr(file);
			koord zwischenziel_koord = zwischenziel.is_bound() ? zwischenziel->get_basis_pos() : koord::invalid;
			zwischenziel_koord.rdwr(file);
			if(file->get_extended_version() >= 1)
			{
				koord origin_koord = origin.is_bound() ? origin->get_basis_pos() : koord::invalid;	
				origin_koord.rdwr(file);
			}
		}
		else 
		{
			koord ziel_koord(file);
			ziel = haltestelle_t::get_halt_koord_index(ziel_koord);
			koord zwischen_ziel_koord(file);
			zwischenziel = haltestelle_t::get_halt_koord_index(zwischen_ziel_koord);
		
			if(file->get_extended_version() >= 1)
			{
				koord origin_koord;	

				origin_koord.rdwr(file);
				if(file->get_extended_version() == 1)
				{				
					// Simutrans-Extended save version 1 had extra parameters
					// such as "previous transfer" intended for use in the new revenue
					// system. In the end, the system was designed differently, and
					// these values are not present in versions 2 and above.
					koord dummy;
					dummy.rdwr(file);
				}
			
				origin = haltestelle_t::get_halt_koord_index(origin_koord);
			
			}
			else
			{
				origin = zwischenziel;
			}
		}
	}
	zielpos.rdwr(file);

	if(file->get_extended_version() == 1)
	{
		uint32 dummy_2;
		file->rdwr_long(dummy_2);
		file->rdwr_long(dummy_2);
	}

	if(file->get_extended_version() >= 2)
	{
		if(file->get_version() < 110007)
		{
			// Was accumulated distance
			// (now handled in convoys)
			uint32 dummy = 0;
			file->rdwr_long(dummy);
		}
		if(file->get_extended_version() < 4)
		{
			// Was journey steps
			uint8 dummy;
			file->rdwr_byte(dummy);
		}
		file->rdwr_longlong(arrival_time);
	}
	else
	{
		arrival_time = 0;
	}

	if(file->get_extended_version() >= 10 && file->get_version() >= 111000)
	{
		if(file->is_saving()) 
		{
			uint16 halt_id = last_transfer.is_bound() ? last_transfer.get_id() : 0;
			file->rdwr_short(halt_id);
		}
		else
		{
			uint16 halt_id;
			file->rdwr_short(halt_id);
			last_transfer.set_id(halt_id);
		}
	}
	else
	{
		last_transfer.set_id(origin.get_id());
	}

	if(file->get_extended_version() >= 12)
	{
		bool commuting = is_commuting_trip;
		file->rdwr_bool(commuting);
		is_commuting_trip = commuting;
	}
}

//"finish loading" (BG); "Invite finish" (Google); "load lock" (Babelfish).
void ware_t::finish_rd(karte_t *welt)
{
	if(  welt->load_version.version <= 111005  ) {
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
	// assert that target coordinates are unique for cargo going to the same factory
	// as new cargo will be generated with possibly new factory coordinates
	fabrik_t *fab = fabrik_t::get_fab( zielpos );
	if (fab) {
		zielpos = fab->get_pos().get_2d();
	}
}
