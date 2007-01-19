/*
 * tunnel.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simplay.h"
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
	setze_besitzer( sp );
	bild = after_bild = IMG_LEER;
}



void
tunnel_t::calc_bild()
{
	const grund_t *gr = welt->lookup(gib_pos());
	if(gr->ist_karten_boden()) {
		hang_t::typ hang = gr->gib_grund_hang();
		setze_bild( besch->gib_hintergrund_nr(hang, gib_pos().z >= welt->get_snowline()));
		after_bild = besch->gib_vordergrund_nr(hang, gib_pos().z >= welt->get_snowline());
	}
	else {
		setze_bild( IMG_LEER );
		after_bild = IMG_LEER;
	}
}



void tunnel_t::rdwr(loadsave_t *file)
{
	ding_t::rdwr(file);
	if(file->get_version()>=99001) {
		char  buf[256];
		if(file->is_loading()) {
			file->rdwr_str(buf,255);
			besch = tunnelbauer_t::gib_besch(buf);
		}
		else {
			strcpy( buf, besch->gib_name() );
			file->rdwr_str(buf,0);
		}
	}
}



void tunnel_t::laden_abschliessen()
{
	const grund_t *gr = welt->lookup(gib_pos());
	spieler_t *sp=gib_besitzer();

	if(besch==NULL) {
		// find a matching besch
		besch = tunnelbauer_t::find_tunnel( (waytype_t)gr->gib_weg_nr(0)->gib_besch()->gib_wtyp(), 450, 0);
	}

	if(sp) {
		// inside tunnel => do nothing but change maitainance
		weg_t *weg = gr->gib_weg(besch->gib_waytype());
		weg->setze_max_speed(besch->gib_topspeed());
		sp->add_maintenance(-weg->gib_besch()->gib_wartung());
		sp->add_maintenance( besch->gib_wartung() );
		if(!gr->ist_karten_boden()){
			return;
		}
	}
}



// correct speed and maitainace
void tunnel_t::entferne( spieler_t *sp2 )
{
	if(sp2==NULL) {
		// only set during destroying of the map
		return;
	}
	spieler_t *sp = gib_besitzer();
	if(sp) {
		// inside tunnel => do nothing but change maitainance
		const grund_t *gr = welt->lookup(gib_pos());
		if(gr) {
			weg_t *weg = gr->gib_weg( besch->gib_waytype() );
			assert(weg);
			weg->setze_max_speed( weg->gib_besch()->gib_topspeed() );
			sp->add_maintenance( weg->gib_besch()->gib_wartung());
			sp->add_maintenance( -besch->gib_wartung() );
		}
	}
	if(sp2) {
		sp2->buche( -besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION );
	}
}
