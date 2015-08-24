/*
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Signalboxes
 */

#include "simsignalbox.h"
#include "simworld.h"
#include "simcity.h"
#include "obj/signal.h"
#include "boden/wege/weg.h"
#include "besch/haus_besch.h"
#include "simdebug.h"

slist_tpl<signalbox_t *> signalbox_t::all_signalboxes;


#ifdef INLINE_OBJ_TYPE
signalbox_t::signalbox_t(loadsave_t *file) : gebaeude_t(obj_t::typ::signalbox)
#else
signalbox_t::signalbox_t(loadsave_t *file) : gebaeude_t(file)
#endif

{
	rdwr(file);
	all_signalboxes.append(this);
	add_to_world_list();
}

#ifdef INLINE_OBJ_TYPE
signalbox_t::signalbox_t(koord3d pos, player_t *player, const haus_tile_besch_t *t) :
    gebaeude_t(obj_t::typ::signalbox, pos, player, t)
#else
signalbox_t::signalbox_t(koord3d pos, player_t *player, const haus_tile_besch_t *t) :
    gebaeude_t(pos, player, t)
#endif
{
	all_signalboxes.append(this);
	add_to_world_list();
}

signalbox_t::~signalbox_t()
{
	// Delete all signals linked to this box
	FOR(slist_tpl<koord3d>, k, signals)
	{
		grund_t* gr = welt->lookup(k);
		if(!gr)
		{
			continue;
		}
		weg_t* way = gr->get_weg_nr(0);

		if(!way || !way->get_signal(ribi_t::alle))
		{
			way = gr->get_weg_nr(1);
		}

		if(!way|| !way->get_signal(ribi_t::alle))
		{
			continue;
		}
		signal_t* s = way->get_signal(ribi_t::alle);
		s->set_signalbox(koord3d::invalid);
		s->cleanup(get_owner());
		delete s;
		way->count_sign();
	}
	all_signalboxes.remove(this);
	const grund_t* gr = welt->lookup(get_pos());
	if(gr)
	{
		welt->remove_building_from_world_list(this);
		// No need to remove this from the city statistics here, as this will be done by the gebaeude_t parent object.
	}
}

signal_t* signalbox_t::get_signal_from_location(koord3d k)
{
	grund_t* gr = welt->lookup(k);
	if(!gr)
	{
		return NULL;
	}
	weg_t* way = gr->get_weg_nr(0);

	if(!way || !way->get_signal(ribi_t::alle))
	{
		way = gr->get_weg_nr(1);
	}

	if(!way|| !way->get_signal(ribi_t::alle))
	{
		return  NULL;
	}
	signal_t* s = way->get_signal(ribi_t::alle);
	return s;
}

void signalbox_t::add_to_world_list(bool lock)
{
	welt->add_building_to_world_list(this);
	stadt_t* city = welt->get_city(get_pos().get_2d());
	if(city)
	{		
		set_stadt(city);
		city->update_city_stats_with_building(this, false);
	}
}

void signalbox_t::rdwr(loadsave_t *file) 
{
	gebaeude_t::rdwr(file);
	
	uint32 signals_count = signals.get_count();
	file->rdwr_long(signals_count); 
	if(file->is_saving())
	{
		FOR(slist_tpl<koord3d>, k, signals)
		{
			k.rdwr(file); 
		}
	}
	else // Loading
	{
		koord3d k;
		for(sint32 i = 0; i < signals_count; i++)
		{
			k.rdwr(file);
			signals.append(k);
		}
	}
}

void signalbox_t::rotate90()
{
	FOR(slist_tpl<koord3d>, k, signals)
	{
		k.rotate90(welt->get_size().y-1);
	}
}

void signalbox_t::remove_signal(signal_t* s)
{
	koord3d k = s->get_pos();
	signals.remove(k); 
}

bool signalbox_t::add_signal(signal_t* s)
{
	if(can_add_signal(s))
	{
		signals.append(s->get_pos()); 
		s->set_signalbox(get_pos()); 
		return true;
	}

	return false;
}

bool signalbox_t::can_add_signal(signal_t* s)
{
	if(!s || (s->get_owner() != get_owner()))
	{
		return false;
	}
	uint32 group = s->get_besch()->get_signal_group();

	if(group) // A signal with a group of 0 needs no signalbox and does not work with signalboxes
	{
		uint32 my_groups = get_tile()->get_besch()->get_clusters();
		if(my_groups & group)
		{
			// The signals form part of a matching group: allow addition
			return true;
		}
	}
	return false;
}

bool signalbox_t::transfer_signal(signal_t* s, signalbox_t* sb)
{
	if(add_signal(s))
	{
		sb->remove_signal(s);  
	}
	else
	{
		return false;
	}
}

koord signalbox_t::transfer_all_signals(signalbox_t* sb)
{
	uint16 success = 0;
	uint16 failure = 0;
	FOR(slist_tpl<koord3d>, k, signals)
	{
		signal_t* s = get_signal_from_location(k); 
		if(!s)
		{
			dbg->error("signalbox_t::transfer_all_signals(signalbox_t* sb)", "Signal cannot be retrieved"); 
			continue;
		}
		if(sb->transfer_signal(s, this))
		{
			success++;
		}
		else
		{
			failure++;
		}
	}

	return koord(success, failure); 
}