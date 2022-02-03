/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <string.h>

#include "simmem.h"
#include "simdebug.h"
#include "simfab.h"
#include "simhalt.h"
#include "simtypes.h"
#include "simware.h"
#include "world/simworld.h"
#include "dataobj/loadsave.h"
#include "dataobj/koord.h"

#include "descriptor/goods_desc.h"
#include "builder/goods_manager.h"



const goods_desc_t *ware_t::index_to_desc[256];

ware_t::ware_t() : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = 0;
	to_factory = 0;
}


ware_t::ware_t(const goods_desc_t *wtyp) : ziel(), zwischenziel(), zielpos(-1, -1)
{
	menge = 0;
	index = wtyp->get_index();
	to_factory = 0;
}


ware_t::ware_t(loadsave_t *file)
{
	rdwr(file);
}


void ware_t::rdwr(loadsave_t *file)
{
	sint32 amount = menge;
	file->rdwr_long(amount);
	menge = amount;
	if(file->is_version_less(99, 8)) {
		sint32 max;
		file->rdwr_long(max);
	}

	if(  file->is_version_atleast(110, 5)  ) {
		uint8 factory_going = to_factory;
		file->rdwr_byte(factory_going);
		to_factory = factory_going;
	}
	else if(  file->is_loading()  ) {
		to_factory = 0;
	}

	uint8 catg=0;
	if(file->is_version_atleast(88, 5)) {
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
	if(file->is_version_atleast(110, 6)) {
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
	if(  file->is_version_less(110, 5)  &&  file->is_loading()  ) {
		if (fabrik_t::get_fab(zielpos)) {
			to_factory = 1;
		}
	}
}


void ware_t::finish_rd(karte_t *welt)
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


sint64 ware_t::calc_revenue(const goods_desc_t* desc, waytype_t wt, sint32 speedkmh)
{
	static karte_ptr_t welt;

	const sint32 ref_kmh = welt->get_average_speed( wt );
	const sint32 kmh_base = (100 * speedkmh) / ref_kmh - 100;

	const sint32 grundwert128    = welt->get_settings().get_bonus_basefactor(); // minimal bonus factor
	const sint32 grundwert_bonus = 1000+kmh_base*desc->get_speed_bonus();      // speed bonus factor
	// take the larger of both
	return desc->get_value() * (grundwert128 > grundwert_bonus ? grundwert128 : grundwert_bonus);
}

ware_t::goods_amount_t ware_t::add_goods(goods_amount_t const number) {
	goods_amount_t const limit = GOODS_AMOUNT_LIMIT - menge;
	if (limit < number) {
		menge = GOODS_AMOUNT_LIMIT;
		return number - limit;
	}

	menge+= number;
	return 0;
}

ware_t::goods_amount_t ware_t::remove_goods(goods_amount_t const number) {
	if (menge < number) {
		goods_amount_t const remainder = number - menge;
		menge = 0;
		return remainder;
	}

	menge-= number;
	return 0;
}

bool ware_t::is_goods_amount_maxed() const {
	return menge == GOODS_AMOUNT_LIMIT;
}
