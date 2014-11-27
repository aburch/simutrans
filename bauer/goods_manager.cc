/*
 * Copyright (c) 1997 - 2002 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../descriptor/goods_desc.h"
#include "../descriptor/spezial_obj_tpl.h"
#include "../simware.h"
#include "../simcolor.h"
#include "goods_manager.h"
#include "../dataobj/translator.h"


stringhashtable_tpl<const goods_desc_t *> goods_manager_t::desc_names;

vector_tpl<goods_desc_t *> goods_manager_t::goods;

uint8 goods_manager_t::max_catg_index = 0;

const goods_desc_t *goods_manager_t::passengers = NULL;
const goods_desc_t *goods_manager_t::mail = NULL;
const goods_desc_t *goods_manager_t::none = NULL;

goods_desc_t *goods_manager_t::load_passengers = NULL;
goods_desc_t *goods_manager_t::load_mail = NULL;
goods_desc_t *goods_manager_t::load_none = NULL;

static spezial_obj_tpl<goods_desc_t> const special_objects[] = {
	{ &goods_manager_t::passengers, "Passagiere" },
	{ &goods_manager_t::mail,	    "Post" },
	{ &goods_manager_t::none,	    "None" },
	{ NULL, NULL }
};


bool goods_manager_t::successfully_loaded()
{
	if(!::successfully_loaded(special_objects)) {
		return false;
	}

	/**
	* Put special items in front:
	* Volker Meyer
	*/
	goods.insert_at(0,load_none);
	goods.insert_at(0,load_mail);
	goods.insert_at(0,load_passengers);

	if(goods.get_count()>=255) {
		dbg->fatal("goods_manager_t::successfully_loaded()","Too many different goods %i>255",goods.get_count()-1 );
	}

	// assign indexes, and fix number_of_classes
	for(  uint8 i=3;  i<goods.get_count();  i++  ) {
		goods[i]->goods_index = i;
		goods[i]->fix_number_of_classes();
	}

	// now assign unique category indexes for unique categories
	max_catg_index = 0;
	// first assign special freight (which always needs an own category)
	FOR(vector_tpl<goods_desc_t*>, const i, goods) {
		if (i->get_catg() == 0) {
			i->catg_index = max_catg_index++;
		}
	}
	// mapping of waren_t::catg to catg_index, map[catg] = catg_index
	uint8 map[255] = {0};

	FOR(vector_tpl<goods_desc_t*>, const i, goods) {
		uint8 const catg = i->get_catg();
		if(  catg > 0  ) {
			if(  map[catg] == 0  ) { // We didn't found this category yet -> just create new index.
				map[catg] = max_catg_index++;
			}
			i->catg_index = map[catg];
		}
	}

	// init the lookup table in ware_t
	for( unsigned i=0;  i<256;  i++  ) {
		if(i>=goods.get_count()) {
			// these entries will be never looked at;
			// however, if then this will generate an error
			ware_t::index_to_desc[i] = NULL;
		}
		else {
			assert(goods[i]->get_index()==i);
			ware_t::index_to_desc[i] = goods[i];
			if(goods[i]->color==255) {
				goods[i]->color = 16+4+((i-2)*8)%207;
			}
		}
	}
	// passenger and good colors
	if(goods[0]->color==255) {
		goods[0]->color = COL_GREY3;
	}
	if(goods[1]->color==255) {
		goods[1]->color = COL_YELLOW;
	}
	// none should never be loaded to something ...
	// however, some place do need the dummy ...
	ware_t::index_to_desc[2] = NULL;

	DBG_MESSAGE("goods_manager_t::successfully_loaded()","total goods %i, different kind of categories %i", goods.get_count(), max_catg_index );

	return true;
}


static bool compare_ware_desc(const goods_desc_t* a, const goods_desc_t* b)
{
	int diff = strcmp(a->get_name(), b->get_name());
	return diff < 0;
}

bool goods_manager_t::register_desc(goods_desc_t *desc)
{
	desc->values.clear();
	ITERATE(desc->base_values, i)
	{
		desc->values.append(desc->base_values[i]);
	}
	::register_desc(special_objects, desc);
	// avoid duplicates with same name
	goods_desc_t *old_desc = const_cast<goods_desc_t *>(desc_names.get(desc->get_name()));
	if(  old_desc  ) {
		dbg->warning( "goods_manager_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
		desc_names.remove(desc->get_name());
		goods.remove( old_desc );
	}
	desc_names.put(desc->get_name(), desc);

	if(desc==passengers) {
		desc->goods_index = INDEX_PAS;
		load_passengers = desc;
	} else if(desc==mail) {
		desc->goods_index = INDEX_MAIL;
		load_mail = desc;
	} else if(desc != none) {
		goods.insert_ordered(desc,compare_ware_desc);
	}
	else {
		load_none = desc;
		desc->goods_index = INDEX_NONE;
	}
	return true;
}


const goods_desc_t *goods_manager_t::get_info(const char* name)
{
	const goods_desc_t *ware = desc_names.get(name);
	if(  ware==NULL  ) {
		ware = desc_names.get(translator::compatibility_name(name));
	}
	if(  ware == NULL  ) {
		// to avoid crashed with NULL pointer in skripts return good NONE
		dbg->warning( "goods_manager_t::get_info()", "No desc for %s", name );
		ware = goods_manager_t::none;
	}
	return ware;
}


const goods_desc_t *goods_manager_t::get_info_catg(const uint8 catg)
{
	if(catg>0) {
		for(unsigned i=0;  i<get_count();  i++  ) {
			if(goods[i]->catg==catg) {
				return goods[i];
			}
		}
	}
	dbg->warning("goods_manager_t::get_info()", "No info for good catg %d available, set to passengers", catg);
	return goods[0];
}


const goods_desc_t *goods_manager_t::get_info_catg_index(const uint8 catg_index)
{
	for(unsigned i=0;  i<get_count();  i++  ) {
		if(goods[i]->get_catg_index()==catg_index) {
			return goods[i];
		}
	}
	// return none as default
	return goods[2];
}


// adjuster for dummies ...
void goods_manager_t::set_multiplier(sint32 multiplier, uint16 scale_factor)
{
//DBG_MESSAGE("goods_manager_t::set_multiplier()","new factor %i",multiplier);
	for(unsigned i=0;  i<get_count();  i++  ) 
	{
		goods[i]->values.clear();
		ITERATE(goods[i]->base_values, n)
		{
			sint32 long_base_value = goods[i]->base_values[n].price;
			uint16 new_value = (uint16)((long_base_value * multiplier) / 1000l);
			goods[i]->values.append(fare_stage_t(goods[i]->base_values[n].to_distance, new_value));
		}
		goods[i]->set_scale(scale_factor);
	}
}

