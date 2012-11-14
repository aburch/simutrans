/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 *
 * Object which shows the label that indicates that the ground is owned by somebody
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simdings.h"
#include "../simimg.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simhalt.h"
#include "../player/simplay.h"
#include "../gui/label_info.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../dataobj/umgebung.h"

#include "label.h"

#if MULTI_THREAD>1
#include <pthread.h>
static pthread_mutex_t add_label_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


label_t::label_t(karte_t *welt, loadsave_t *file) :
	ding_t(welt)
{
	rdwr(file);
}


label_t::label_t(karte_t *welt, koord3d pos, spieler_t *sp, const char *text) :
	ding_t(welt, pos)
{
	set_besitzer( sp );
	welt->add_label(pos.get_2d());
	grund_t *gr=welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		if (text) {
			gr->set_text(text);
		}
		spieler_t::accounting(sp, welt->get_settings().cst_buy_land, pos.get_2d(), COST_CONSTRUCTION);
	}
}


label_t::~label_t()
{
	koord k = get_pos().get_2d();
	welt->remove_label(k);
	grund_t *gr = welt->lookup_kartenboden(k);
	if(gr) {
		// do not remove name from halts
		if (!gr->is_halt()  ||  gr->get_halt()->get_basis_pos3d()!=gr->get_pos()) {
			gr->set_text(NULL);
		}
	}
}


void label_t::laden_abschliessen()
{
#if MULTI_THREAD>1
	pthread_mutex_lock( &add_label_mutex );
#endif
	// only now coordinates are known
	welt->add_label(get_pos().get_2d());
#if MULTI_THREAD>1
	pthread_mutex_unlock( &add_label_mutex );
#endif
}


image_id label_t::get_bild() const
{
	grund_t *gr=welt->lookup(get_pos());
	return gr && gr->obj_bei(0) == sim::up_cast<ding_t const*>(this) ? skinverwaltung_t::belegtzeiger->get_bild_nr(0) : IMG_LEER;
}


void label_t::zeige_info()
{
	label_t* l = this;
	create_win(new label_info_t(welt, l), w_info, (ptrdiff_t)this );
}
