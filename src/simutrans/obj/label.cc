/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */


#include <stdio.h>

#include "../world/simworld.h"
#include "simobj.h"
#include "../display/simimg.h"
#include "../simskin.h"
#include "../gui/simwin.h"
#include "../simhalt.h"
#include "../player/simplay.h"
#include "../gui/label_info.h"

#include "../descriptor/ground_desc.h"
#include "../descriptor/skin_desc.h"

#include "../dataobj/environment.h"

#include "label.h"

#ifdef MULTI_THREAD
#include "../utils/simthread.h"
static pthread_mutex_t add_label_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif


label_t::label_t(loadsave_t *file) :
	obj_t()
{
	rdwr(file);
}


label_t::label_t(koord3d pos, player_t *player, const char *text) :
	obj_t(pos)
{
	set_owner( player );
	welt->add_label(pos.get_2d());
	grund_t *gr=welt->lookup_kartenboden(pos.get_2d());
	if(gr) {
		if (text) {
			gr->set_text(text);
		}
		player_t::book_construction_costs(player, welt->get_settings().cst_buy_land, pos.get_2d(), ignore_wt);
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


image_id label_t::get_image() const
{
	grund_t *gr=welt->lookup(get_pos());
	return gr && gr->obj_bei(0) == sim::up_cast<obj_t const*>(this) ? skinverwaltung_t::belegtzeiger->get_image_id(0) : IMG_EMPTY;
}


void label_t::show_info()
{
	label_t* l = this;
	create_win(new label_info_t(l), w_info, (ptrdiff_t)this );
}
