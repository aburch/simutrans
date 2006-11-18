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
		hang_t::typ hang = gr->gib_grund_hang();
		setze_bild( 0, besch->gib_hintergrund_nr(hang) );
		after_bild = besch->gib_vordergrund_nr(hang);
	}
	else {
		setze_bild( 0, IMG_LEER );
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

	// correct speed and maitenance for old tunnels

	// proceed until the other end
	koord3d pos = gib_pos();
	const koord zv = koord(welt->lookup(pos)->gib_grund_hang());
	pos += zv;

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
			// calc calculation will be completed after loading!
		}
		pos += zv;
	}
}



// correct speed and maitainace
void tunnel_t::entferne( spieler_t *sp )
{
	sp=gib_besitzer();
	if(sp) {
		// inside tunnel => do nothing but change maitainance
		const grund_t *gr = welt->lookup(gib_pos());
		weg_t *weg = gr->gib_weg( besch->gib_waytype() );
		assert(weg);
		weg->setze_max_speed( weg->gib_besch()->gib_topspeed() );
		sp->add_maintenance( weg->gib_besch()->gib_wartung());
		sp->add_maintenance( -besch->gib_wartung() );
		sp->buche( -besch->gib_preis(), gib_pos().gib_2d(), COST_CONSTRUCTION );
	}
}
