/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include <stdio.h>

#include "../simdebug.h"
#include "../simworld.h"
#include "../display/simimg.h"

#include "../utils/simrandom.h"
#include "../boden/grund.h"
#include "../dataobj/loadsave.h"

#include "simpeople.h"
#include "../descriptor/pedestrian_desc.h"

static uint32 const strecke[] = { 6000, 11000, 15000, 20000, 25000, 30000, 35000, 40000 };

static weighted_vector_tpl<const pedestrian_desc_t*> list;
stringhashtable_tpl<const pedestrian_desc_t *> pedestrian_t::table;


static bool compare_fussgaenger_desc(const pedestrian_desc_t* a, const pedestrian_desc_t* b)
{
	// sort pedestrian objects descriptors by their name
	return strcmp(a->get_name(), b->get_name())<0;
}


bool pedestrian_t::register_desc(const pedestrian_desc_t *desc)
{
	if(  table.remove(desc->get_name())  ) {
		dbg->warning( "pedestrian_t::register_desc()", "Object %s was overlaid by addon!", desc->get_name() );
	}
	table.put(desc->get_name(), desc);
	return true;
}


bool pedestrian_t::successfully_loaded()
{
	list.resize(table.get_count());
	if (table.empty()) {
		DBG_MESSAGE("pedestrian_t", "No pedestrians found - feature disabled");
	}
	else {
		vector_tpl<const pedestrian_desc_t*> temp_liste(0);
		FOR(stringhashtable_tpl<pedestrian_desc_t const*>, const& i, table) {
			// just entered them sorted
			temp_liste.insert_ordered(i.value, compare_fussgaenger_desc);
		}
		FOR(vector_tpl<pedestrian_desc_t const*>, const i, temp_liste) {
			list.append(i, i->get_distribution_weight());
		}
	}
	return true;
}


pedestrian_t::pedestrian_t(loadsave_t *file)
 : road_user_t()
{
	rdwr(file);
	if(desc) {
		welt->sync.add(this);
	}
}


pedestrian_t::pedestrian_t(grund_t *gr) :
	road_user_t(gr, simrand(65535)),
	desc(pick_any_weighted(list))
{
	time_to_life = pick_any(strecke);
	calc_image();
}


pedestrian_t::~pedestrian_t()
{
	if(  time_to_life>0  ) {
		welt->sync.remove( this );
	}
}


void pedestrian_t::calc_image()
{
	set_image(desc->get_image_id(ribi_t::get_dir(get_direction())));
}



void pedestrian_t::rdwr(loadsave_t *file)
{
	xml_tag_t f( file, "fussgaenger_t" );

	road_user_t::rdwr(file);

	if(!file->is_loading()) {
		const char *s = desc->get_name();
		file->rdwr_str(s);
	}
	else {
		char s[256];
		file->rdwr_str(s, lengthof(s));
		desc = table.get(s);
		// unknown pedestrian => create random new one
		if(desc == NULL  &&  !list.empty()  ) {
			desc = pick_any_weighted(list);
		}
	}

	if(file->get_version()<89004) {
		time_to_life = pick_any(strecke);
	}
}



// create a number (count) of pedestrians (if possible)
void pedestrian_t::generate_pedestrians_at(const koord3d k, int &count)
{
	if (list.empty()) {
		return;
	}

	grund_t* bd = welt->lookup(k);
	if (bd) {
		const weg_t* weg = bd->get_weg(road_wt);

		// we do not start on crossings (not overrunning pedestrians please
		if (weg && ribi_t::is_twoway(weg->get_ribi_unmasked())) {
			// we create maximal 4 pedestrians here for performance reasons
			for (int i = 0; i < 4 && count > 0; i++) {
				pedestrian_t* fg = new pedestrian_t(bd);
				bool ok = bd->obj_add(fg) != 0;	// 256 limit reached
				if (ok) {
					fg->calc_height(bd);
					if (i > 0) {
						// walk a little
						fg->sync_step( (i & 3) * 64 * 24);
					}
					welt->sync.add(fg);
					count--;
				}
				else {
					// delete it, if we could not put it on the map
					fg->set_flag(obj_t::not_on_map);
					// do not try to delete it from sync-list
					fg->time_to_life = 0;
					delete fg;
					return; // it is pointless to try again
				}
			}
		}
	}
}


sync_result pedestrian_t::sync_step(uint32 delta_t)
{
	time_to_life -= delta_t;

	if (time_to_life>0) {
		weg_next += 128*delta_t;
		weg_next -= do_drive( weg_next );
		return time_to_life>0 ? SYNC_OK : SYNC_DELETE;
	}
	return SYNC_DELETE;
}


grund_t* pedestrian_t::hop_check()
{
	grund_t *from = welt->lookup(pos_next);
	if(!from) {
		time_to_life = 0;
		return NULL;
	}

	// find the allowed directions
	const weg_t *weg = from->get_weg(road_wt);
	if(weg==NULL) {
		// no road anymore: destroy it
		time_to_life = 0;
		return NULL;
	}
	return from;
}


void pedestrian_t::hop(grund_t *gr)
{
	leave_tile();
	set_pos(gr->get_pos());
	calc_image();
	// no need to call enter_tile();
	gr->obj_add(this);

	const weg_t *weg = gr->get_weg(road_wt);
	// new target
	grund_t *to = NULL;
	// ribi opposite to current direction
	ribi_t::ribi reverse_direction = ribi_t::backward( get_direction() );
	// all possible directions
	ribi_t::ribi ribi = weg->get_ribi_unmasked() & (~reverse_direction);
	// randomized offset
	const uint8 offset = (ribi > 0  &&  ribi_t::is_single(ribi)) ? 0 : simrand(4);

	for(uint r = 0; r < 4; r++) {
		ribi_t::ribi const test_ribi = ribi_t::nsew[ (r+offset) & 3];

		if(  (ribi & test_ribi)!=0  &&  gr->get_neighbour(to, road_wt, test_ribi) )	{
			// this is our next target
			break;
		}
	}

	if (to) {
		pos_next = to->get_pos();
		direction = calc_set_direction(get_pos(), pos_next);
	}
	else {
		// turn around
		direction = reverse_direction;
		dx = -dx;
		dy = -dy;
		pos_next = get_pos();
		// .. but this looks ugly, so disappear
		time_to_life = 0;
	}
}
