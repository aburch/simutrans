/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simdebug.h"
#include "../simworld.h"
#include "../simobj.h"
#include "../display/simimg.h"
#include "../player/simplay.h"
#include "../simtypes.h"

#include "../boden/grund.h"
#include "../descriptor/groundobj_desc.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include "../utils/simrandom.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/freelist.h"


#include "groundobj.h"

/******************************** static routines for desc management ****************************************************************/

vector_tpl<const groundobj_desc_t *> groundobj_t::groundobj_typen(0);

stringhashtable_tpl<groundobj_desc_t *> groundobj_t::desc_table;


bool compare_groundobj_desc(const groundobj_desc_t* a, const groundobj_desc_t* b)
{
	return strcmp(a->get_name(), b->get_name())<0;
}


bool groundobj_t::successfully_loaded()
{
	groundobj_typen.resize(desc_table.get_count());
	FOR(stringhashtable_tpl<groundobj_desc_t*>, const& i, desc_table) {
		groundobj_typen.insert_ordered(i.value, compare_groundobj_desc);
	}
	// iterate again to assign the index
	FOR(stringhashtable_tpl<groundobj_desc_t*>, const& i, desc_table) {
		i.value->index = groundobj_typen.index_of(i.value);
	}

	if(desc_table.empty()) {
		groundobj_typen.append( NULL );
		DBG_MESSAGE("groundobj_t", "No groundobj found - feature disabled");
	}
	return true;
}


bool groundobj_t::register_desc(groundobj_desc_t *desc)
{
	assert(desc->get_speed()==0);
	// remove duplicates
	if(  desc_table.remove( desc->get_name() )  ) {
		dbg->doubled( "groundobj", desc->get_name() );
	}
	desc_table.put(desc->get_name(), desc );
	return true;
}


/* also checks for distribution values
 * @author prissi
 */
const groundobj_desc_t *groundobj_t::random_groundobj_for_climate(climate_bits cl, slope_t::type slope  )
{
	// none there
	if(  desc_table.empty()  ) {
		return NULL;
	}

	int weight = 0;
	FOR(  vector_tpl<groundobj_desc_t const*>,  const i,  groundobj_typen  ) {
		if(  i->is_allowed_climate_bits(cl)  &&  (slope == slope_t::flat  ||  (i->get_phases() >= slope  &&  i->get_image_id(0,slope)!=IMG_EMPTY  )  )  ) {
			weight += i->get_distribution_weight();
		}
	}

	// now weight their distribution
	if(  weight > 0  ) {
		const int w=simrand(weight);
		weight = 0;
		FOR(vector_tpl<groundobj_desc_t const*>, const i, groundobj_typen) {
			if(  i->is_allowed_climate_bits(cl)  &&  (slope == slope_t::flat  ||  (i->get_phases() >= slope  &&  i->get_image_id(0,slope)!=IMG_EMPTY  )  )  ) {
				weight += i->get_distribution_weight();
				if(weight>=w) {
					return i;
				}
			}
		}
	}
	return NULL;
}



/******************************* end of static ******************************************/



// recalculates only the seasonal image
void groundobj_t::calc_image()
{
	const groundobj_desc_t *desc=get_desc();
	const sint16 seasons = desc->get_seasons()-1;
	uint8 season=0;

	// two possibilities
	switch(seasons) {
				// summer only
		case 0: season = 0;
				break;
				// summer, snow
		case 1: season = welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate;
				break;
				// summer, winter, snow
		case 2: season = (welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate) ? 2 : welt->get_season() == 1;
				break;
		default: if(  welt->get_snowline() <= get_pos().z  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate  ) {
					season = seasons;
				}
				else {
					// resolution 1/8th month (0..95)
					const uint32 yearsteps = (welt->get_current_month()%12)*8 + ((welt->get_ticks()>>(welt->ticks_per_world_month_shift-3))&7) + 1;
					season = (seasons*yearsteps-1)/96;
				}
				break;
	}
	// check for slopes?
	uint16 phase = 0;
	if(desc->get_phases()>1) {
		phase = welt->lookup(get_pos())->get_grund_hang();
	}
	image = get_desc()->get_image_id( season, phase );
}


groundobj_t::groundobj_t(loadsave_t *file) : obj_t()
{
	rdwr(file);
}


groundobj_t::groundobj_t(koord3d pos, const groundobj_desc_t *b ) : obj_t(pos)
{
	groundobjtype = groundobj_typen.index_of(b);
	calc_image();
}


bool groundobj_t::check_season(const bool)
{
	const image_id old_image = get_image();
	calc_image();

	if(  get_image() != old_image  ) {
		mark_image_dirty( get_image(), 0 );
	}
	return true;
}


void groundobj_t::rdwr(loadsave_t *file)
{
	xml_tag_t d( file, "groundobj_t" );

	obj_t::rdwr(file);

	if(file->is_saving()) {
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
	}
	else {
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));
		groundobj_desc_t *desc = desc_table.get( bname );
		if(  desc==NULL  ) {
			desc =  desc_table.get( translator::compatibility_name( bname ) );
		}
		if(  desc==NULL  ) {
			groundobjtype = simrand(groundobj_typen.get_count());
		}
		else {
			groundobjtype = desc->get_index();
		}
		// if not there, desc will be zero
	}
}


/**
 * Öffnet ein neues Beobachtungsfenster für das Objekt.
 * @author Hj. Malthaner
 */
void groundobj_t::show_info()
{
	if(env_t::tree_info) {
		obj_t::show_info();
	}
}


/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void groundobj_t::info(cbuffer_t & buf) const
{
	obj_t::info(buf);

	buf.append(translator::translate(get_desc()->get_name()));
	if (char const* const maker = get_desc()->get_copyright()) {
		buf.append("\n");
		buf.printf(translator::translate("Constructed by %s"), maker);
	}
	buf.append("\n");
	buf.append(translator::translate("cost for removal"));
	char buffer[128];
	money_to_string( buffer, get_desc()->get_price()/100.0 );
	buf.append( buffer );
}


void groundobj_t::cleanup(player_t *player)
{
	player_t::book_construction_costs(player, -get_desc()->get_price(), get_pos().get_2d(), ignore_wt);
	mark_image_dirty( get_image(), 0 );
}


void *groundobj_t::operator new(size_t /*s*/)
{
	return freelist_t::gimme_node(sizeof(groundobj_t));
}


void groundobj_t::operator delete(void *p)
{
	freelist_t::putback_node(sizeof(groundobj_t),p);
}
