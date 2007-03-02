/*
 * warenbauer.cc
 *
 * Copyright (c) 1997 - 2002 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "../simdebug.h"
#include "../besch/ware_besch.h"
#include "../besch/spezial_obj_tpl.h"
#include "../simware.h"
#include "../simcolor.h"
#include "warenbauer.h"


stringhashtable_tpl<const ware_besch_t *> warenbauer_t::besch_names;

vector_tpl<ware_besch_t *> warenbauer_t::waren;

uint8 warenbauer_t::max_catg_index = 0;
uint8 warenbauer_t::max_special_catg_index = 0;

const ware_besch_t *warenbauer_t::passagiere = NULL;
const ware_besch_t *warenbauer_t::post = NULL;
const ware_besch_t *warenbauer_t::nichts = NULL;

ware_besch_t *warenbauer_t::load_passagiere = NULL;
ware_besch_t *warenbauer_t::load_post = NULL;
ware_besch_t *warenbauer_t::load_nichts = NULL;

static spezial_obj_tpl<ware_besch_t> spezial_objekte[] = {
    { &warenbauer_t::passagiere,    "Passagiere" },
    { &warenbauer_t::post,	    "Post" },
    { &warenbauer_t::nichts,	    "None" },
    { NULL, NULL }
};



bool
warenbauer_t::alles_geladen()
{
	if(!::alles_geladen(spezial_objekte)) {
		return false;
	}

	/**
	* Put special items in front:
	* Volker Meyer
	*/
	waren.insert_at(0,load_nichts);
	waren.insert_at(0,load_post);
	waren.insert_at(0,load_passagiere);

	if(waren.get_count()>255) {
		dbg->fatal("warenbauer_t::alles_geladen()","Too many different goods %i>255",waren.get_count()-1 );
	}

	// now assign unique category indexes for unique categories
	max_catg_index = 0;
	// first assign special freight (which always needs an own category)
	for( unsigned i=0;  i<waren.get_count();  i++  ) {
		if(waren[i]->gib_catg()==0) {
			waren[i]->catg_index = max_catg_index++;
		}
	}
	max_special_catg_index = max_catg_index-1;
	// now assign all unused waren.
	for( unsigned i=0;  i<waren.get_count();  i++  ) {
		// now search, if we already have a matching category
		if(waren[i]->gib_catg()>0) {
			waren[i]->catg_index = waren[i]->catg+max_special_catg_index;
			if(waren[i]->gib_catg_index()>=max_catg_index) {
				// find the higest catg used ...
				max_catg_index = waren[i]->gib_catg_index()+1;
			}
		}
	}

	// init the lookup table in ware_t
	for( unsigned i=0;  i<256;  i++  ) {
		if(i>=waren.get_count()) {
			// these entries will be never looked at;
			// however, if then this will generate an error
			ware_t::index_to_besch[i] = NULL;
		}
		else {
			assert(waren[i]->gib_index()==i);
			ware_t::index_to_besch[i] = waren[i];
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
	ware_t::index_to_besch[2] = NULL;

	DBG_MESSAGE("warenbauer_t::alles_geladen()","total goods %i, different kind of categories %i", waren.get_count(), max_catg_index );

	return true;
}


bool warenbauer_t::register_besch(ware_besch_t *besch)
{
	besch->value = besch->base_value;
	::register_besch(spezial_objekte, besch);
	besch_names.put(besch->gib_name(), const_cast<ware_besch_t *>(besch));

	if(besch==passagiere) {
		besch->ware_index = 0;
		load_passagiere = besch;
	} else if(besch==post) {
		besch->ware_index = 1;
		load_post = besch;
	} else if(besch != nichts) {
		besch->ware_index = waren.get_count()+3;
		waren.append(besch,1);
	}
	else {
		load_nichts = besch;
		besch->ware_index = 2;
	}
	return true;
}




const ware_besch_t *
warenbauer_t::gib_info(const char* name)
{
	const ware_besch_t* t = besch_names.get(name);
	if(t == NULL) {
		dbg->error("warenbauer_t::gib_info()", "No info for good '%s' available", name);
	}
	return t;
}



const ware_besch_t *
warenbauer_t::gib_info_catg(const uint8 catg)
{
	if(catg>0) {
		for(unsigned i=0;  i<gib_waren_anzahl();  i++  ) {
			if(waren[i]->catg==catg) {
				return waren[i];
			}
		}
	}
	dbg->warning("warenbauer_t::gib_info()", "No info for good catg %d available, set to passengers", catg);
	return waren[0];
}



const ware_besch_t *
warenbauer_t::gib_info_catg_index(const uint8 catg_index)
{
	for(unsigned i=0;  i<gib_waren_anzahl();  i++  ) {
		if(waren[i]->gib_catg_index()==catg_index) {
			return waren[i];
		}
	}
	// return none as default
	return waren[2];
}



// adjuster for dummies ...
void
warenbauer_t::set_multiplier(sint32 multiplier)
{
//DBG_MESSAGE("warenbauer_t::set_multiplier()","new factor %i",multiplier);
	for(unsigned i=0;  i<gib_waren_anzahl();  i++  ) {
		sint32 long_base_value = waren[i]->base_value;
		waren[i]->value = (uint16)((long_base_value*multiplier)/1000l);
	}
}
