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
#include "../boden/wege/strasse.h"
#include "../boden/wege/schiene.h"
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
	clean_up = false;
}


tunnel_t::tunnel_t(karte_t *welt, koord3d pos, spieler_t *sp, const tunnel_besch_t *besch) :
	ding_t(welt, pos)
{
	this->besch = besch;
	setze_besitzer( sp );
	step_frequency = 0;
	clean_up = false;
}



/**
 * @return Einen Beschreibungsstring für das Objekt, der z.B. in einem
 * Beobachtungsfenster angezeigt wird.
 * @author Hj. Malthaner
 */
void tunnel_t::info(cbuffer_t & buf) const
{
	ding_t::info(buf);
}



void
tunnel_t::calc_bild()
{
	if(besch) {
		const grund_t *gr = welt->lookup(gib_pos());
		setze_bild( 0, besch->gib_hintergrund_nr(gr->gib_grund_hang()) );
		after_bild = besch->gib_vordergrund_nr(gr->gib_grund_hang());
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
	if(gr->gib_grund_hang()==0) {
		// old, inside tunnel ...
		step_frequency = 1;	// remove
		dbg->error("tunnel_t::laden_abschliessen()","tunnel entry at flat position at %i,%i will be removed.",gib_pos().x,gib_pos().y);
		return;
	}

	if(besch==NULL) {
		besch = tunnelbauer_t::find_tunnel( gr->gib_weg_nr(0)->gib_typ(), 999, 0 );
	}

	// correct speed and maitenance
	if(besch  &&  !clean_up) {

		// find other end
		koord3d pos = gib_pos();
		grund_t *gr = welt->lookup(pos);
		koord zv = koord::invalid;
		slist_tpl<koord3d>tracks;
		tunnel_t *tunnel=NULL;

		// now look up everything
		zv = koord(gr->gib_grund_hang());
		tracks.insert(pos);
		do {
			pos += zv;
			tracks.append(pos);
			gr = welt->lookup(pos);
			tunnel = (tunnel_t *)gr->suche_obj(ding_t::tunnel);
		} while(tunnel==NULL);

		if(!tunnel->clean_up) {

			// reset speed and maitenance
			spieler_t *sp=gib_besitzer();
			while(!tracks.is_empty()) {
				pos = tracks.remove_first();
				grund_t *gr = welt->lookup(pos);
				weg_t *weg = gr->gib_weg(besch->gib_wegtyp());
				weg->setze_max_speed(besch->gib_topspeed());
				sp->add_maintenance(-weg->gib_besch()->gib_wartung());
				sp->add_maintenance( besch->gib_wartung() );
			}

			// and mark done ...
			tunnel->clean_up = true;
			clean_up = true;
		}
	}
	calc_bild();
}
