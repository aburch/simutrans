/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 *
 * Object which shows the label that indicates that the ground is owned by somebody
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simmem.h"
#include "../simskin.h"
#include "../simplay.h"

#include "../besch/grund_besch.h"

#include "../dataobj/umgebung.h"

#include "label.h"

label_t::label_t(karte_t *welt, loadsave_t *file) :
	ding_t(welt)
{
	rdwr(file);
	welt->add_label(gib_pos().gib_2d());
}


label_t::label_t(karte_t *welt, koord3d pos, spieler_t *sp, const char *text) :
	ding_t(welt, pos)
{
	setze_besitzer( sp );
	welt->add_label(pos.gib_2d());
	grund_t *gr=welt->lookup_kartenboden(pos.gib_2d());
	if(gr) {
		ding_t *d=gr->obj_bei(0);
		if(d==NULL  ||  d->gib_besitzer()==NULL) {
			sp->buche( umgebung_t::cst_buy_land, pos.gib_2d(), COST_CONSTRUCTION);
			if(d) {
				d->setze_besitzer(sp);
			}
		}
		if (text) {
			gr->setze_text(text);
		}
	}
}



label_t::~label_t()
{
	welt->remove_label( gib_pos().gib_2d() );
	grund_t *gr=welt->lookup_kartenboden(gib_pos().gib_2d());
	const char *txt = gr->gib_text();
	if(gr  &&  txt) {
		gr->setze_text(NULL);
		guarded_free( (void *)txt );
	}
}



image_id label_t::gib_bild() const
{
	grund_t *gr=welt->lookup(gib_pos());
	return (gr  &&  gr->obj_bei(0)==this) ? skinverwaltung_t::belegtzeiger->gib_bild_nr(0) : IMG_LEER;
}

