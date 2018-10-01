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
#include "descriptor/building_desc.h"
#include "simdebug.h"
#include "simtool.h"

slist_tpl<signalbox_t *> signalbox_t::all_signalboxes;


#ifdef INLINE_OBJ_TYPE
signalbox_t::signalbox_t(loadsave_t *file) : gebaeude_t(signalbox)
#else
signalbox_t::signalbox_t(loadsave_t *file) : gebaeude_t(file)
#endif

{
	rdwr(file);
	all_signalboxes.append(this);
	add_to_world_list();
}

#ifdef INLINE_OBJ_TYPE
signalbox_t::signalbox_t(koord3d pos, player_t *player, const building_tile_desc_t *t) :
    gebaeude_t(signalbox, pos, player, t)
#else
signalbox_t::signalbox_t(koord3d pos, player_t *player, const building_tile_desc_t *t) :
    gebaeude_t(pos, player, t)
#endif
{
	all_signalboxes.append(this);
	add_to_world_list();
}

signalbox_t::~signalbox_t()
{
	player_t* player = welt->get_active_player();
	if(player->get_selected_signalbox() == this)
	{
		player->set_selected_signalbox(NULL);
	}
	
	// Delete all signals linked to this box
	FOR(slist_tpl<koord3d>, k, signals)
	{
		grund_t* gr = welt->lookup(k);
		if(!gr)
		{
			continue;
		}

		signal_t* s = gr->find<signal_t>();
		if(!s)
		{
			continue;
		}
		s->set_signalbox(koord3d::invalid);
		s->cleanup(get_owner());
		delete s;
		weg_t* way = gr->get_weg_nr(0);
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

void signalbox_t::add_to_world_list(bool lock)
{
	welt->add_building_to_world_list(access_first_tile());
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
	gebaeude_t::rotate90();
	slist_tpl<koord3d> temp_list;
	FOR(slist_tpl<koord3d>, k, signals)
	{
		k.rotate90(welt->get_size().y-1);
		temp_list.append(k); 
	}

	signals.clear();
	signals.append_list(temp_list); 
}

void signalbox_t::remove_signal(signal_t* s)
{
	koord3d k = s->get_pos();
	signals.remove(k); 
}

bool signalbox_t::add_signal(signal_t* s)
{
	if(can_add_signal(s) && can_add_more_signals())
	{
		signals.append(s->get_pos()); 
		s->set_signalbox(get_pos()); 
		return true;
	}

	return false;
}

bool signalbox_t::can_add_signal(const roadsign_desc_t* d) const
{
	uint32 group = d->get_signal_group();

	if(group) // A signal with a group of 0 needs no signalbox and does not work with signalboxes
	{
		uint32 my_groups = get_first_tile()->get_tile()->get_desc()->get_clusters();
		if(my_groups & group)
		{
			// The signals form part of a matching group: allow addition
			return true;
		}
	}
	return false;
}

bool signalbox_t::can_add_signal(const signal_t* s) const
{
	if(!s || (s->get_owner() != get_owner()))
	{
		return false;
	}
	
	return can_add_signal(s->get_desc());
}

bool signalbox_t::can_add_more_signals() const
{
	return signals.get_count() < get_first_tile()->get_tile()->get_desc()->get_capacity();
}

bool signalbox_t::transfer_signal(signal_t* s, signalbox_t* sb)
{
	if(sb == this)
	{
		return false;
	}

	if(!s->get_desc()->get_working_method() != moving_block)
	{
		if(s->get_desc()->get_max_distance_to_signalbox() != 0 && ((s->get_desc()->get_max_distance_to_signalbox() / welt->get_settings().get_meters_per_tile()) < shortest_distance(s->get_pos().get_2d(), sb->get_pos().get_2d())))
		{
			return false;
		}
	}

	if(add_signal(s))
	{
		sb->remove_signal(s);  
		return true;
	}
	else
	{
		return false;
	}
}

koord signalbox_t::transfer_all_signals(signalbox_t* sb)
{
	if(sb == this)
	{
		return(koord(0,0));
	}

	uint16 success = 0;
	uint16 failure = 0;
	slist_tpl<koord3d> duplicate_signals_list;
	FOR(slist_tpl<koord3d>, k1, signals)
	{
		duplicate_signals_list.append(k1);
	}
	FOR(slist_tpl<koord3d>, k, duplicate_signals_list)
	{
		grund_t* gr = welt->lookup(k);
		if(gr)
		{
			signal_t* s = gr->find<signal_t>();
			if(!s)
			{
				dbg->error("signalbox_t::transfer_all_signals(signalbox_t* sb)", "Signal cannot be retrieved"); 
				continue;
			}

			if(!s->get_desc()->get_working_method() != moving_block)
			{
				if (s->get_desc()->get_max_distance_to_signalbox() != 0 && sb && ((s->get_desc()->get_max_distance_to_signalbox() / welt->get_settings().get_meters_per_tile()) < shortest_distance(s->get_pos().get_2d(), sb->get_pos().get_2d())))
				{
					failure++;
					continue;
				}
			}

			if(sb && sb->transfer_signal(s, this))
			{
				success++;
			}
			else
			{
				failure++;
			}
		}
	}

	return koord(success, failure); 
}

