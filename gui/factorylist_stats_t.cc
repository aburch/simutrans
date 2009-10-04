/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include "factorylist_stats_t.h"

#include "../simgraph.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simfab.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../simskin.h"

#include "components/list_button.h"

#include "../bauer/warenbauer.h"
#include "../besch/skin_besch.h"
#include "../utils/cbuffer_t.h"


factorylist_stats_t::factorylist_stats_t(karte_t* w, factorylist::sort_mode_t sortby, bool sortreverse)
{
	welt = w;
	sort(sortby,sortreverse);
	line_selected = 0xFFFFFFFFu;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void factorylist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE+1);
	line_selected = 0xFFFFFFFFu;
	if (line >= fab_list.get_count()) {
		return;
	}

	fabrik_t* fab = fab_list[line];
	if (!fab) {
		return;
	}

	// deperess goto button
	if(  ev->button_state>0  &&  ev->cx>0  &&  ev->cx<15  ) {
		line_selected = line;
	}

	if (IS_LEFTRELEASE(ev)) {
		if(ev->cx>0  &&  ev->cx<15) {
			const koord3d pos = fab->get_pos();
			welt->change_world_position(pos);
		}
		else {
			fab->zeige_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		const koord3d pos = fab->get_pos();
		welt->change_world_position(pos);
	}
} // end of function factorylist_stats_t::infowin_event(const event_t * ev)



/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void factorylist_stats_t::zeichnen(koord offset)
{
	image_id const arrow_right_normal = skinverwaltung_t::window_skin->get_bild(10)->get_nummer();
	const struct clip_dimension cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE-1;
	const int end = cd.yy+LINESPACE+1;

	static cbuffer_t buf(256);
	int xoff = offset.x+16;
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
			buf.append(fab_list[i]->get_name());
			buf.append(" (");

			if (!fab->get_eingang().empty()) {
				buf.append(fab->get_total_in(),0);
			}
			else {
				buf.append("-");
			}
			buf.append(", ");

			if (!fab->get_ausgang().empty()) {
				buf.append(fab->get_total_out(),0);
			}
			else {
				buf.append("-");
			}
			buf.append(", ");

			buf.append(fab->get_current_production(),0);
			buf.append(") ");


			//display_ddd_box_clip(xoff+7, yoff+2, 8, 8, MN_GREY0, MN_GREY4);
			display_fillbox_wh_clip(xoff+2, yoff+2, INDICATOR_WIDTH, INDICATOR_HEIGHT, indikatorfarbe, true);

			if (fab->get_prodfaktor() > 16) {
				display_color_img(skinverwaltung_t::electricity->get_bild_nr(0), xoff+4+INDICATOR_WIDTH, yoff, 0, false, false);
			}

			// show text
			display_proportional_clip(xoff+INDICATOR_WIDTH+6+10,yoff,buf,ALIGN_LEFT,COL_BLACK,true);

			if(i!=line_selected) {
				// goto information
				display_color_img(arrow_right_normal, xoff-14, yoff, 0, false, true);
			}
			else {
				// select goto button
				display_color_img(skinverwaltung_t::window_skin->get_bild(11)->get_nummer(),
					xoff-14, yoff, 0, false, true);
			}

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
					int a_in = (a->get_eingang().empty() ? -1 : (int)a->get_total_in());
					int b_in = (b->get_eingang().empty() ? -1 : (int)b->get_total_in());
					cmp = a_in - b_in;
					break;
				}

				case factorylist::by_output:
				{
					int a_out = (a->get_ausgang().empty() ? -1 : (int)a->get_total_out());
					int b_out = (b->get_ausgang().empty() ? -1 : (int)b->get_total_out());
					cmp = a_out - b_out;
					break;
				}

				case factorylist::by_maxprod:
					cmp = a->get_base_production()*a->get_prodfaktor() - b->get_base_production()*b->get_prodfaktor();
					break;

				case factorylist::by_status:
					cmp = a->get_status() - b->get_status();
					break;

				case factorylist::by_power:
					cmp = a->get_prodfaktor() - b->get_prodfaktor();
					break;
			}
			if (cmp == 0) cmp = strcmp(a->get_name(), b->get_name());
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		const factorylist::sort_mode_t sortby;
		const bool reverse;
};


void factorylist_stats_t::sort(factorylist::sort_mode_t sortby, bool sortreverse)
{
	fab_list.clear();
	fab_list.resize(welt->get_fab_list().get_count());
	for (slist_iterator_tpl<fabrik_t*> i(welt->get_fab_list()); i.next();) {
		fab_list.append(i.get_current());
	}
	std::sort(fab_list.begin(), fab_list.end(), compare_factories(sortby, sortreverse));
	set_groesse(koord(210, welt->get_fab_list().get_count()*(LINESPACE+1)-10));
}
