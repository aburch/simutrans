/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
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

#include "../besch/tunnel_besch.h"

#include "leitung2.h"
#include "../bauer/wegbauer.h"

#include "tunnel.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t tunnel_calc_bild_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
#endif


tunnel_t::tunnel_t(loadsave_t* const file) : obj_no_info_t()
{
	besch = 0;
	rdwr(file);
	bild = after_bild = IMG_LEER;
	broad_type = 0;
}


tunnel_t::tunnel_t(koord3d pos, spieler_t *sp, const tunnel_besch_t *besch) :
	obj_no_info_t(pos)
{
	assert(besch);
	this->besch = besch;
	set_besitzer( sp );
	bild = after_bild = IMG_LEER;
	broad_type = 0;
}


waytype_t tunnel_t::get_waytype() const
{
	return besch ? besch->get_waytype() : invalid_wt;
}


void tunnel_t::calc_bild()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &tunnel_calc_bild_mutex );
#endif
	const grund_t *gr = welt->lookup(get_pos());
	if(  gr->ist_karten_boden()  &&  besch  ) {
		hang_t::typ hang = gr->get_grund_hang();

		broad_type = 0;
		if(  besch->has_broad_portals()  ) {
			ribi_t::ribi dir = ribi_t::rotate90( ribi_typ( hang ) );
			if(  dir==0  ) {
				dbg->error( "tunnel_t::calc_bild()", "pos=%s, dir=%i, hang=%i", get_pos().get_str(), dir, hang );
			}
			else {
				const grund_t *gr_l = welt->lookup(get_pos() + dir);
				tunnel_t* tunnel_l = gr_l ? gr_l->find<tunnel_t>() : NULL;
				if(  tunnel_l  &&  tunnel_l->get_besch() == besch  &&  gr_l->get_grund_hang() == hang  ) {
					broad_type += 1;
					if(  !(tunnel_l->get_broad_type() & 2)  ) {
						tunnel_l->calc_bild();
					}
				}
				const grund_t *gr_r = welt->lookup(get_pos() - dir);
				tunnel_t* tunnel_r = gr_r ? gr_r->find<tunnel_t>() : NULL;
				if(  tunnel_r  &&  tunnel_r->get_besch() == besch  &&  gr_r->get_grund_hang() == hang  ) {
					broad_type += 2;
					if(  !(tunnel_r->get_broad_type() & 1)  ) {
						tunnel_r->calc_bild();
					}
				}
			}
		}

		set_bild( besch->get_hintergrund_nr( hang, get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate, broad_type ) );
		set_after_bild( besch->get_vordergrund_nr( hang, get_pos().z >= welt->get_snowline()  ||  welt->get_climate( get_pos().get_2d() ) == arctic_climate, broad_type ) );
	}
	else {
		set_bild( IMG_LEER );
		set_after_bild( IMG_LEER );
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &tunnel_calc_bild_mutex );
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
			besch = tunnelbauer_t::get_besch(buf);
			if(  besch==NULL  ) {
				besch = tunnelbauer_t::get_besch(translator::compatibility_name(buf));
			}
			if(  besch==NULL  ) {
				welt->add_missing_paks( buf, karte_t::MISSING_WAY );
			}
		}
		else {
			strcpy( buf, besch->get_name() );
			file->rdwr_str(buf,0);
		}
	}
}


void tunnel_t::laden_abschliessen()
{
	const grund_t *gr = welt->lookup(get_pos());
	spieler_t *sp=get_besitzer();

	if(besch==NULL) {
		// find a matching besch
		if (gr->get_weg_nr(0)==NULL) {
			// no way? underground powerline
			if (gr->get_leitung()) {
				besch = tunnelbauer_t::find_tunnel(powerline_wt, 1, 0);
			}
			// no tunnel -> use dummy road tunnel
			if (besch==NULL) {
				besch = tunnelbauer_t::find_tunnel(road_wt, 1, 0);
			}
		}
		else {
			besch = tunnelbauer_t::find_tunnel(gr->get_weg_nr(0)->get_besch()->get_wtyp(), 450, 0);
			if(  besch == NULL  ) {
				dbg->error( "tunnel_t::laden_abschliessen()", "Completely unknown tunnel for this waytype: Lets use a rail tunnel!" );
				besch = tunnelbauer_t::find_tunnel(track_wt, 1, 0);
			}
		}
	}

	if(sp) {
		// change maintenance
		weg_t *weg = gr->get_weg(besch->get_waytype());
		if(weg) {
			weg->set_max_speed(besch->get_topspeed());
			spieler_t::add_maintenance( sp, -weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
		}
		leitung_t *lt = gr->get_leitung();
		if(lt) {
			spieler_t::add_maintenance( sp, -lt->get_besch()->get_wartung(), powerline_wt );
		}
		spieler_t::add_maintenance( sp,  besch->get_wartung(), besch->get_finance_waytype() );
	}
}


// correct speed and maintenance
void tunnel_t::entferne( spieler_t *sp2 )
{
	spieler_t *sp = get_besitzer();
	if(sp) {
		// inside tunnel => do nothing but change maintenance
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t *weg = gr->get_weg( besch->get_waytype() );
			if(weg)	{
				weg->set_max_speed( weg->get_besch()->get_topspeed() );
				spieler_t::add_maintenance( sp,  weg->get_besch()->get_wartung(), weg->get_besch()->get_finance_waytype());
			}
			spieler_t::add_maintenance( sp,  -besch->get_wartung(), besch->get_finance_waytype() );
		}
	}
	spieler_t::book_construction_costs(sp2, -besch->get_preis(), get_pos().get_2d(), besch->get_finance_waytype() );
}


void tunnel_t::set_bild( image_id b )
{
	mark_image_dirty( bild, get_yoff() );
	mark_image_dirty( b, get_yoff() );
	bild = b;
}


void tunnel_t::set_after_bild( image_id b )
{
	mark_image_dirty( after_bild, get_yoff() );
	mark_image_dirty( b, get_yoff() );
	after_bild = b;
}


// returns NULL, if removal is allowed
// players can remove public owned ways
const char *tunnel_t::ist_entfernbar(const spieler_t *sp)
{
	if (get_player_nr()==1) {
		return NULL;
	}
	else {
		return obj_t::ist_entfernbar(sp);
	}
}
