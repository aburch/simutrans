/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <algorithm>
#include "factorylist_stats_t.h"

#include "../simgraph.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simfab.h"
#include "../simwin.h"
#include "../simworld.h"

#include "components/list_button.h"

#include "../bauer/warenbauer.h"
#include "../utils/cbuffer_t.h"


factorylist_stats_t::factorylist_stats_t(karte_t* w, factorylist::sort_mode_t sortby, bool sortreverse) :
    fab_list(0)
{
	welt = w;
	setze_groesse(koord(210, welt->gib_fab_list().count()*(LINESPACE+1)-10));
	sort(sortby,sortreverse);
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void factorylist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE+1);
	if (line >= fab_list.get_count()) {
		return;
	}

	fabrik_t* fab = fab_list[line];
	if (!fab) {
		return;
	}

	const koord3d pos = fab->gib_pos();
	if (IS_LEFTRELEASE(ev)) {
		grund_t *gr = welt->lookup(pos.gib_2d())->gib_kartenboden();
		if (gr) {
			gebaeude_t* gb = gr->find<gebaeude_t>();
			if (gb) {
				create_win(320, 0, -1, new fabrik_info_t(fab, gb), w_info, magic_none);
			}
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->setze_ij_off(pos);
	}
} // end of function factorylist_stats_t::infowin_event(const event_t * ev)



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void factorylist_stats_t::zeichnen(koord offset)
{
	//DBG_DEBUG("factorylist_stats_t()","zeichnen()");
	const struct clip_dimension cd = display_gib_clip_wh();
	const int start = cd.y-LINESPACE-1;
	const int end = cd.yy+LINESPACE+1;

	static cbuffer_t buf(256);
	int xoff = offset.x;
	int yoff = offset.y;

	for (uint32 i=0; i<fab_list.get_count()  &&  yoff<end; i++) {

		// skip invisible lines
		if(yoff<start) {
			yoff += LINESPACE+1;
			continue;
		}

		const fabrik_t* fab = fab_list[i];
		if(fab) {
			//DBG_DEBUG("factorylist_stats_t()","zeichnen() factory %i",i);
			unsigned indikatorfarbe = fabrik_t::status_to_color[fab->get_status()];

			buf.clear();
			//		buf.append(i+1);
			//		buf.append(".) ");
			buf.append(fab_list[i]->gib_name());
			buf.append(" (");

			if (!fab->gib_eingang().empty()) {
				buf.append(fab->get_total_in());
			}
			else {
				buf.append("-");
			}
			buf.append(", ");

			if (!fab->gib_ausgang().empty()) {
				buf.append(fab->get_total_out());
			}
			else {
				buf.append("-");
			}
			buf.append(", ");

			buf.append(fab->get_base_production());
			buf.append(") ");


			//display_ddd_box_clip(xoff+7, yoff+2, 8, 8, MN_GREY0, MN_GREY4);
			display_fillbox_wh_clip(xoff+2, yoff+2, INDICATOR_WIDTH, INDICATOR_HEIGHT, indikatorfarbe, true);

			if (fab->get_prodfaktor() > 16) {
				display_color_img(skinverwaltung_t::electricity->gib_bild_nr(0), xoff+4+INDICATOR_WIDTH, yoff, 0, false, false);
			}

			// show text
			display_proportional_clip(xoff+INDICATOR_WIDTH+6+10,yoff,buf,ALIGN_LEFT,COL_BLACK,true);
		}
	yoff += LINESPACE+1;
	}
}


class compare_factories
{
	public:
		compare_factories(factorylist::sort_mode_t sortby_, bool reverse_) :
			sortby(sortby_),
			reverse(reverse_)
		{}

		bool operator ()(const fabrik_t* a, const fabrik_t* b)
		{
			int cmp;
			switch (sortby) {
				default:
				case factorylist::by_name:
					cmp = 0;
					break;

				case factorylist::by_input:
				{
					int a_in = (a->gib_eingang().empty() ? -1 : a->get_total_in());
					int b_in = (b->gib_eingang().empty() ? -1 : b->get_total_in());
					cmp = a_in - b_in;
					break;
				}

				case factorylist::by_output:
				{
					int a_out = (a->gib_ausgang().empty() ? -1 : a->get_total_out());
					int b_out = (b->gib_ausgang().empty() ? -1 : b->get_total_out());
					cmp = a_out - b_out;
					break;
				}

				case factorylist::by_maxprod:
					cmp = a->get_base_production() - b->get_base_production();
					break;

				case factorylist::by_status:
					cmp = a->get_status() - b->get_status();
					break;

				case factorylist::by_power:
					cmp = a->get_prodfaktor() - b->get_prodfaktor();
					break;
			}
			if (cmp == 0) cmp = strcmp(a->gib_name(), b->gib_name());
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		const factorylist::sort_mode_t sortby;
		const bool reverse;
};


void factorylist_stats_t::sort(factorylist::sort_mode_t sortby, bool sortreverse)
{
	fab_list.clear();
	fab_list.resize(welt->gib_fab_list().count());
	for (slist_iterator_tpl<fabrik_t*> i(welt->gib_fab_list()); i.next();) {
		fab_list.push_back(i.get_current());
	}
	std::sort(fab_list.begin(), fab_list.end(), compare_factories(sortby, sortreverse));
}
