/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../player/simplay.h"
#include "../simwerkz.h"
#include "../boden/grund.h"
#include "../boden/tunnelboden.h"
#include "../simimg.h"
#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"

#include "../besch/tunnel_besch.h"

#include "tunnel.h"




tunnel_t::tunnel_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = 0;
	rdwr(file);
	bild = after_bild = IMG_LEER;
}


tunnel_t::tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch) :
	ding_t(welt, pos)
{
	assert(besch);
	this->besch = besch;
	set_besitzer( sp );
	bild = after_bild = IMG_LEER;
}



void
tunnel_t::calc_bild()
{
	const grund_t *gr = welt->lookup(get_pos());
	if(gr->ist_karten_boden()) {
		hang_t::typ hang = gr->get_grund_hang();
		set_bild( besch->get_hintergrund_nr(hang, get_pos().z >= welt->get_snowline()));
		after_bild = besch->get_vordergrund_nr(hang, get_pos().z >= welt->get_snowline());
	}
	else {
		set_bild( IMG_LEER );
		after_bild = IMG_LEER;
	}
}



void tunnel_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "tunnel_t" );
	ding_t::rdwr(file);
	if(file->get_version()>=99001) {
		char  buf[256];
		if(file->is_loading()) {
			file->rdwr_str(buf,255);
			besch = tunnelbauer_t::get_besch(buf);
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
		besch = tunnelbauer_t::find_tunnel( (waytype_t)gr->get_weg_nr(0)->get_besch()->get_wtyp(), 450, 0);
	}

	if(sp) {
		// change maintainance
		weg_t *weg = gr->get_weg(besch->get_waytype());
		if(weg) {
			weg->set_max_speed(besch->get_topspeed());
			weg->set_max_speed(besch->get_topspeed());
			sp->add_maintenance(-weg->get_besch()->get_wartung());
		}
		sp->add_maintenance(besch->get_wartung());
	}
}



// correct speed and maitainace
void tunnel_t::entferne( spieler_t *sp2 )
{
	if(sp2==NULL) {
		// only set during destroying of the map
		return;
	}
	spieler_t *sp = get_besitzer();
	if(sp) {
		// inside tunnel => do nothing but change maitainance
		const grund_t *gr = welt->lookup(get_pos());
		if(gr) {
			weg_t *weg = gr->get_weg( besch->get_waytype() );
			weg->set_max_speed( weg->get_besch()->get_topspeed() );
			weg->set_max_weight( weg->get_besch()->get_max_weight() );
			weg->add_way_constraints(besch->get_way_constraints_permissive(), besch->get_way_constraints_prohibitive());
			sp->add_maintenance(weg->get_besch()->get_wartung());
			sp->add_maintenance(-besch->get_wartung() );
		}
	}
	spieler_t::accounting(sp2, -besch->get_preis(), get_pos().get_2d(), COST_CONSTRUCTION );
}
