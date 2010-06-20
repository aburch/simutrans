/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "labellist_stats_t.h"

#include "../simgraph.h"
#include "../simtypes.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../player/simplay.h"

#include "../dings/gebaeude.h"
#include "../dings/label.h"

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/list_button.h"


labellist_stats_t::labellist_stats_t(karte_t* w, labellist::sort_mode_t sortby, bool sortreverse, bool filter) :
    labels(10)
{
	welt = w;
	get_unique_labels(sortby,sortreverse,filter);
	set_groesse(koord(210, labels.get_count()*(LINESPACE+1)-10));
	line_selected = 0xFFFFFFFFu;
}

class compare_labels
{
	public:
		compare_labels(labellist::sort_mode_t sortby_, bool reverse_, bool filter_, karte_t * welt_) :
			sortby(sortby_),
			reverse(reverse_),
			filter(filter_),
			welt(welt_)
		{}

		bool operator ()(const koord a, const koord b)
		{
			int cmp = 0;
			switch (sortby) {
				default: NOT_REACHED
				case labellist::by_name:
				{
					cmp = 0;
					break;
				}
				case labellist::by_koord:
					cmp = a.x - b.x;
					if(cmp==0) {
						cmp = a.y - b.y;
					}
					break;
				case labellist::by_player:
				{
					if(!filter) {
						label_t* a_l = welt->lookup(a)->get_kartenboden()->find<label_t>();
						label_t* b_l = welt->lookup(b)->get_kartenboden()->find<label_t>();
						if(a_l && b_l) {
							cmp = a_l->get_besitzer()->get_player_nr() - b_l->get_besitzer()->get_player_nr();
						}
					}
					break;
				}
			}
			if(cmp==0) {
				const char* a_name = welt->lookup(a)->get_kartenboden()->get_text();
				const char* b_name = welt->lookup(b)->get_kartenboden()->get_text();

				cmp = strcmp(a_name, b_name);
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		labellist::sort_mode_t sortby;
		bool reverse;
		bool filter;
		karte_t * welt;
};


void labellist_stats_t::get_unique_labels(labellist::sort_mode_t sortby, bool sortreverse, bool filter)
{
	labels.clear();

	slist_iterator_tpl <koord> iter (welt->get_label_list());
	while(iter.next()) {
		koord pos = iter.get_current();
		label_t* label = welt->lookup(pos)->get_kartenboden()->find<label_t>();
		const char* name = welt->lookup(pos)->get_kartenboden()->get_text();
		// some old version games don't have label nor name.
		// Check them to avoid crashes.
		if(label  &&  name  &&  (!filter  ||  (label  &&  (label->get_besitzer() == welt->get_active_player())))) {
			labels.insert_ordered( pos, compare_labels(sortby, sortreverse, filter, welt) );
		}
	}
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool labellist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE+1);

	line_selected = 0xFFFFFFFFu;
	if (line>=labels.get_count()) {
		return false;
	}

	koord pos = labels[line];
	if (pos==koord::invalid) {
		return false;
	}

	// deperess goto button
	if(  ev->button_state>0  &&  ev->cx>0  &&  ev->cx<15  ) {
		line_selected = line;
	}

	if (IS_LEFTRELEASE(ev)) {
		if(  ev->cx>0  &&  ev->cx<15  ) {
			welt->change_world_position(pos);
		}
		else if(welt->lookup(pos)->get_kartenboden()->find<label_t>()) {
			// avoid crash
				welt->lookup(pos)->get_kartenboden()->find<label_t>()->zeige_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->change_world_position(pos);
	}
	return false;
} // end of function labellist_stats_t::infowin_event(const event_t * ev)


/**
 * Zeichnet die Komponente
 * @author Hj. Malthaner
 */
void labellist_stats_t::zeichnen(koord offset)
{
	const struct clip_dimension cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE+1;
	const int end = cd.yy;

	static cbuffer_t buf(128);
	int yoff = offset.y;

	for (uint32 i=0; i<labels.get_count()  &&  yoff<end; i++) {
		const koord pos = labels[i];

		// skip invisible lines
		if(yoff<start) {
			yoff += LINESPACE+1;
			continue;
		}

		// goto button
		display_color_img( i!=line_selected ? button_t::arrow_right_normal : button_t::arrow_right_pushed,
				offset.x+2, yoff, 0, false, true);

		buf.clear();

		// the other infos
		const label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		PLAYER_COLOR_VAL col = COL_WHITE;
		buf.append(" (");
		buf.append(pos.x);
		buf.append(",");
		buf.append(pos.y);
		buf.append(") ");

		if(label) {
			col = (PLAYER_FLAG|label->get_besitzer()->get_player_color1());
			grund_t *gr = welt->lookup(label->get_pos());
			if(gr && gr->get_text()) {
				buf.append(gr->get_text());
			}
		}
		display_proportional_clip(offset.x+10+4,yoff,buf,ALIGN_LEFT,col,true);

		yoff +=LINESPACE+1;
	}
}
