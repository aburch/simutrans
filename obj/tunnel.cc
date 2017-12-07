/*
 * Copyright (c) 1997 - 2001 Hansj�rg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simobj.h"
#include "../player/simplay.h"
#include "../boden/grund.h"
#include "../display/simimg.h"
#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "../descriptor/tunnel_desc.h"

#include "leitung2.h"
#include "../bauer/wegbauer.h"

#include "tunnel.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t tunnel_calc_image_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif


tunnel_t::tunnel_t(loadsave_t* const file) : obj_no_info_t()
{
	desc = 0;
	rdwr(file);
	image = foreground_image = IMG_EMPTY;
	broad_type = 0;
}


tunnel_t::tunnel_t(koord3d pos, player_t *player, const tunnel_desc_t *desc) :
	obj_no_info_t(pos)
{
	assert(desc);
	this->desc = desc;
	set_owner( player );
	image = foreground_image = IMG_EMPTY;
	broad_type = 0;
}


waytype_t tunnel_t::get_waytype() const
{
	return desc ? desc->get_waytype() : invalid_wt;
}


void tunnel_t::calc_image()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &tunnel_calc_image_mutex );
#endif
	const grund_t *gr = welt->lookup(get_pos());
	if(  gr->ist_karten_boden()  &&  desc  ) {
		slope_t::type hang = gr->get_grund_hang();

		broad_type = 0;
		if(  desc->has_broad_portals()  ) {
			ribi_t::ribi dir = ribi_t::rotate90( ribi_type( hang ) );
			if(  dir==0  ) {
				dbg->error( "tunnel_t::calc_image()", "pos=%s, dir=%i, hang=%i", get_pos().get_str(), dir, hang );
			}
			else {
				const grund_t *gr_l = welt->lookup(get_pos() + dir);
				tunnel_t* tunnel_l = gr_l ? gr_l->find<tunnel_t>() : NULL;
				if(  tunnel_l  &&  tunnel_l->get_desc() == desc  &&  gr_l->get_grund_hang() == hang  ) {
					broad_type += 1;
					if(  !(tunnel_l->get_broad_type() & 2)  ) {
						tunnel_l->calc_image();
					}
				}
				const grund_t *gr_r = welt->lookup(get_pos() - dir);
				tunnel_t* tunnel_r = gr_r ? gr_r->find<tunnel_t>() : NULL;
				if(  tunnel_r  &&  tunnel_r->get_desc() == desc  &&  gr_r->get_grund_hang() == hang  ) {
					broad_type += 2;
					if(  !(tunnel_r->get_broad_type() & 1)  ) {
						tunnel_r->calc_image();
					}
				}
			}
		}

		set_image( desc->get_background_id( hang, get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate, broad_type ) );
		set_foreground_image( desc->get_foreground_id( hang, get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate, broad_type ) );
	}
	else {
		set_image( IMG_EMPTY );
		set_foreground_image( IMG_EMPTY );
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &tunnel_calc_image_mutex );
#endif
}



void tunnel_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "tunnel_t" );
	obj_t::rdwr(file);
	if(  file->get_version() >= 99001 ) {
		char  buf[256];
		if(  file->is_loading()  ) {
			file->rdwr_str(buf, lengthof(buf));
			desc = tunnel_builder_t::get_desc(buf);
			if(  desc==NULL  ) {
				desc = tunnel_builder_t::get_desc(translator::compatibility_name(buf));
			}
			if(  desc==NULL  ) {
				welt->add_missing_paks( buf, world_t::MISSING_WAY );
			}
		}
		else {
			strcpy( buf, desc->get_name() );
			file->rdwr_str(buf,0);
		}
	}
}


void tunnel_t::finish_rd()
{
	const grund_t *gr = welt->lookup(get_pos());
	player_t *player=get_owner();

	if(desc==NULL) {
		// find a matching desc
		if (gr->get_weg_nr(0)==NULL) {
			// no way? underground powerline
			if (gr->get_leitung()) {
				desc = tunnel_builder_t::get_tunnel_desc(powerline_wt, 1, 0);
			}
			// no tunnel -> use dummy road tunnel
			if (desc==NULL) {
				desc = tunnel_builder_t::get_tunnel_desc(road_wt, 1, 0);
			}
		}
		else {
			desc = tunnel_builder_t::get_tunnel_desc(gr->get_weg_nr(0)->get_desc()->get_wtyp(), 450, 0);
			if(  desc == NULL  ) {
				dbg->error( "tunnel_t::finish_rd()", "Completely unknown tunnel for this waytype: Lets use a rail tunnel!" );
				desc = tunnel_builder_t::get_tunnel_desc(track_wt, 1, 0);
			}
		}
	}

	if(player) {
		// change maintenance
		weg_t *weg = gr->get_weg(desc->get_waytype());
		if(weg) {
			weg->set_max_speed(desc->get_topspeed());
			player_t::add_maintenance( player, -weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
		}
		leitung_t *lt = gr->get_leitung();
		if(lt) {
			player_t::add_maintenance( player, -lt->get_desc()->get_maintenance(), powerline_wt );
		}
		player_t::add_maintenance( player,  desc->get_maintenance(), desc->get_finance_waytype() );
	}
}


// correct speed and maintenance
void tunnel_t::cleanup( player_t *player2 )
{
	player_t *player = get_owner();
	if(player) {
		// inside tunnel => do nothing but change maintenance
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t *weg = gr->get_weg( desc->get_waytype() );
			if(weg)	{
				weg->set_max_speed( weg->get_desc()->get_topspeed() );
				player_t::add_maintenance( player,  weg->get_desc()->get_maintenance(), weg->get_desc()->get_finance_waytype());
			}
			player_t::add_maintenance( player,  -desc->get_maintenance(), desc->get_finance_waytype() );
		}
	}
	player_t::book_construction_costs(player2, -desc->get_price(), get_pos().get_2d(), desc->get_finance_waytype() );
}


void tunnel_t::set_image( image_id b )
{
	mark_image_dirty( image, 0 );
	mark_image_dirty( b, 0 );
	image = b;
}


void tunnel_t::set_foreground_image( image_id b )
{
	mark_image_dirty( foreground_image, 0 );
	mark_image_dirty( b, 0 );
	foreground_image = b;
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *tunnel_t::is_deletable(const player_t *player)
{
	if (get_player_nr()==welt->get_public_player()->get_player_nr()) {
		return NULL;
	}
	else {
		return obj_t::is_deletable(player);
	}
}
