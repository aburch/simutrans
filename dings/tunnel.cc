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
#include "../utils/cbuffer_t.h"
#include "../bauer/tunnelbauer.h"

#include "../dataobj/loadsave.h"

#include "../besch/tunnel_besch.h"

#include "tunnel.h"




tunnel_t::tunnel_t(karte_t *welt, loadsave_t *file) : ding_t(welt)
{
	besch = 0;
	rdwr(file);
	step_frequency = 0;
	after_bild = IMG_LEER;
}


tunnel_t::tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch) :
	ding_t(welt, pos)
{
	this->besch = besch;
	setze_besitzer( sp );
	step_frequency = 0;
	after_bild = IMG_LEER;
}



void
tunnel_t::calc_bild()
{
	const grund_t *gr = welt->lookup(gib_pos());
	if(gr->ist_karten_boden()) {
		setze_bild( 0, besch->gib_hintergrund_nr(gr->gib_grund_hang()) );
		after_bild = besch->gib_vordergrund_nr(gr->gib_grund_hang());
	}
	else {
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
		besch = tunnelbauer_t::find_tunnel( gr->gib_weg_nr(0)->gib_waytype(), 999, 0 );
	}

	if(sp) {
		// inside tunnel => do nothing but change maitainance
		weg_t *weg = gr->gib_weg(besch->gib_waytype());
		weg->setze_max_speed(besch->gib_topspeed());
		sp->add_maintenance(-weg->gib_besch()->gib_wartung());
		sp->add_maintenance( besch->gib_wartung() );
		if(gr->gib_grund_hang()==gr->gib_weg_hang()){
			return;
		}
	}

	// correct speed and maitenance for old tunnels

	// proceed until the other end
	koord3d pos = gib_pos();
	const koord zv = koord(welt->lookup(pos)->gib_grund_hang());

	// now look up everything
	// reset speed and maitenance
	while(1) {
		tunnelboden_t *gr = dynamic_cast<tunnelboden_t *>(welt->lookup(pos));
		if(gr==NULL) {
			// no tunnel any more, or already assigned a description
			break;
		}
		if(gr->suche_obj(ding_t::tunnel)==NULL) {
			gr->obj_add(new tunnel_t(welt, pos, sp, besch));
			weg_t *weg = gr->gib_weg(besch->gib_waytype());
			weg->setze_max_speed(besch->gib_topspeed());
			sp->add_maintenance(-weg->gib_besch()->gib_wartung());
			sp->add_maintenance( besch->gib_wartung() );
		}
		pos += zv;
	}
}
