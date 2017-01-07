/*
 * Copyright (c) 1997 - 2002 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../besch/ware_besch.h"
#include "../besch/spezial_obj_tpl.h"
#include "../simware.h"
#include "../simcolor.h"
#include "warenbauer.h"
#include "../dataobj/translator.h"


stringhashtable_tpl<const ware_besch_t *> warenbauer_t::desc_table;

vector_tpl<ware_besch_t *> warenbauer_t::waren;

uint8 warenbauer_t::max_catg_index = 0;

const ware_besch_t *warenbauer_t::passagiere = NULL;
const ware_besch_t *warenbauer_t::post = NULL;
const ware_besch_t *warenbauer_t::nichts = NULL;

ware_besch_t *warenbauer_t::load_passagiere = NULL;
ware_besch_t *warenbauer_t::load_post = NULL;
ware_besch_t *warenbauer_t::load_nichts = NULL;

static spezial_obj_tpl<ware_besch_t> const special_objects[] = {
	{ &warenbauer_t::passagiere,    "Passagiere" },
	{ &warenbauer_t::post,	    "Post" },
	{ &warenbauer_t::nichts,	    "None" },
	{ NULL, NULL }
};


bool warenbauer_t::successfully_loaded()
{
	if(!::successfully_loaded(special_objects)) {
		return false;
	}

	/**
	* Put special items in front:
	* Volker Meyer
	*/
	waren.insert_at(0,load_nichts);
	waren.insert_at(0,load_post);
	waren.insert_at(0,load_passagiere);

	if(waren.get_count()>=255) {
		dbg->fatal("warenbauer_t::successfully_loaded()","Too many different goods %i>255",waren.get_count()-1 );
	}

	// assign indexes
	for(  uint8 i=3;  i<waren.get_count();  i++  ) {
		waren[i]->ware_index = i;
	}

	// now assign unique category indexes for unique categories
	max_catg_index = 0;
	// first assign special freight (which always needs an own category)
	FOR(vector_tpl<ware_besch_t*>, const i, waren) {
		if (i->get_catg() == 0) {
			i->catg_index = max_catg_index++;
		}
	}
	// mapping of waren_t::catg to catg_index, map[catg] = catg_index
	uint8 map[255] = {0};

	FOR(vector_tpl<ware_besch_t*>, const i, waren) {
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
		if(i>=waren.get_count()) {
			// these entries will be never looked at;
			// however, if then this will generate an error
			ware_t::index_to_desc[i] = NULL;
		}
		else {
			assert(waren[i]->get_index()==i);
			ware_t::index_to_desc[i] = waren[i];
			if(waren[i]->color==255) {
				waren[i]->color = 16+4+((i-2)*8)%207;
			}
		}
	}
	// passenger and good colors
	if(waren[0]->color==255) {
		waren[0]->color = COL_GREY3;
	}
	if(waren[1]->color==255) {
		waren[1]->color = COL_YELLOW;
	}
	// none should never be loaded to something ...
	// however, some place do need the dummy ...
	ware_t::index_to_desc[2] = NULL;

	DBG_MESSAGE("warenbauer_t::successfully_loaded()","total goods %i, different kind of categories %i", waren.get_count(), max_catg_index );

	return true;
}


static bool compare_ware_desc(const ware_besch_t* a, const ware_besch_t* b)
{
	int diff = strcmp(a->get_name(), b->get_name());
	return diff < 0;
}

bool warenbauer_t::register_desc(ware_besch_t *desc)
{
	desc->value = desc->base_value;
	::register_desc(special_objects, desc);
	// avoid duplicates with same name
	ware_besch_t *old_desc = const_cast<ware_besch_t *>(desc_table.get(desc->get_name()));
	if(  old_desc  ) {
		dbg->warning( "warenbauer_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
		desc_table.remove(desc->get_name());
		waren.remove( old_desc );
	}
	desc_table.put(desc->get_name(), desc);

	if(desc==passagiere) {
		desc->ware_index = INDEX_PAS;
		load_passagiere = desc;
	} else if(desc==post) {
		desc->ware_index = INDEX_MAIL;
		load_post = desc;
	} else if(desc != nichts) {
		waren.insert_ordered(desc,compare_ware_desc);
	}
	else {
		load_nichts = desc;
		desc->ware_index = INDEX_NONE;
	}
	return true;
}


const ware_besch_t *warenbauer_t::get_info(const char* name)
{
	const ware_besch_t *ware = desc_table.get(name);
	if(  ware==NULL  ) {
		ware = desc_table.get(translator::compatibility_name(name));
	}
	if(  ware == NULL  ) {
		// to avoid crashed with NULL pointer in skripts return good NONE
		dbg->error( "warenbauer_t::get_info()", "No desc for %s", name );
		ware = warenbauer_t::nichts;
	}
	return ware;
}


const ware_besch_t *warenbauer_t::get_info_catg(const uint8 catg)
{
	if(catg>0) {
		for(unsigned i=0;  i<get_count();  i++  ) {
			if(waren[i]->catg==catg) {
				return waren[i];
			}
		}
	}
	dbg->warning("warenbauer_t::get_info()", "No info for good catg %d available, set to passengers", catg);
	return waren[0];
}


const ware_besch_t *warenbauer_t::get_info_catg_index(const uint8 catg_index)
{
	for(unsigned i=0;  i<get_count();  i++  ) {
		if(waren[i]->get_catg_index()==catg_index) {
			return waren[i];
		}
	}
	// return none as default
	return waren[2];
}


// adjuster for dummies ...
void warenbauer_t::set_multiplier(sint32 multiplier)
{
//DBG_MESSAGE("warenbauer_t::set_multiplier()","new factor %i",multiplier);
	for(unsigned i=0;  i<get_count();  i++  ) {
		sint32 long_base_value = waren[i]->base_value;
		waren[i]->value = (uint16)((long_base_value*multiplier)/1000l);
	}
}
