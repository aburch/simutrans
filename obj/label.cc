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
#include "../simobj.h"
#include "../display/simimg.h"
#include "../simskin.h"
#include "../gui/simwin.h"
#include "../simhalt.h"
#include "../player/simplay.h"
#include "../gui/label_info.h"

#include "../besch/grund_besch.h"
#include "../besch/skin_besch.h"

#include "../dataobj/environment.h"

#include "label.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t add_label_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


label_t::label_t(loadsave_t *file) :
#ifdef INLINE_OBJ_TYPE
    obj_t(obj_t::label)
#else
	obj_t()
#endif
{
	rdwr(file);
}


label_t::label_t(koord3d pos, player_t *player, const char *text) :
#ifdef INLINE_OBJ_TYPE
	obj_t(obj_t::label, pos)
#else
	obj_t(pos)
#endif
{
	set_owner( player );
	welt->add_label(pos.get_2d());
	grund_t *gr=welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		if (text) {
			gr->set_text(text);
		}
		player_t::book_construction_costs(player, welt->get_land_value(pos), pos.get_2d(), ignore_wt);
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


void label_t::finish_rd()
{
#ifdef MULTI_THREAD
	pthread_mutex_lock( &add_label_mutex );
#endif
	// only now coordinates are known
	welt->add_label(get_pos().get_2d());

	// broken label? set text to ""
	grund_t *gr = welt->lookup_kartenboden(get_pos().get_2d());
	if (!gr->get_flag(grund_t::has_text)) {
		gr->set_text("");
	}
#ifdef MULTI_THREAD
	pthread_mutex_unlock( &add_label_mutex );
#endif
}


image_id label_t::get_bild() const
{
	grund_t *gr=welt->lookup(get_pos());
	return gr && gr->obj_bei(0) == sim::up_cast<obj_t const*>(this) ? skinverwaltung_t::belegtzeiger->get_bild_nr(0) : IMG_LEER;
}


void label_t::show_info()
{
	label_t* l = this;
	create_win(new label_info_t(l), w_info, (ptrdiff_t)this );
}
