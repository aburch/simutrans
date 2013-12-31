/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "labellist_stats_t.h"

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simtypes.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../player/simplay.h"

#include "../obj/gebaeude.h"
#include "../obj/label.h"

#include "../besch/haus_besch.h"
#include "../besch/skin_besch.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "gui_theme.h"



labellist_stats_t::labellist_stats_t(labellist::sort_mode_t sortby, bool sortreverse, bool filter)
{
	get_unique_labels(sortby,sortreverse,filter);
	recalc_size();
	line_selected = 0xFFFFFFFFu;
}

class compare_labels
{
	public:
		compare_labels(labellist::sort_mode_t sortby_, bool reverse_, bool filter_) :
			sortby(sortby_),
			reverse(reverse_),
			filter(filter_)
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
						label_t* a_l = welt->lookup_kartenboden(a)->find<label_t>();
						label_t* b_l = welt->lookup_kartenboden(b)->find<label_t>();
						if(a_l && b_l) {
							cmp = a_l->get_besitzer()->get_player_nr() - b_l->get_besitzer()->get_player_nr();
						}
					}
					break;
				}
			}
			if(cmp==0) {
				const char* a_name = welt->lookup_kartenboden(a)->get_text();
				const char* b_name = welt->lookup_kartenboden(b)->get_text();

				cmp = strcmp(a_name, b_name);
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		labellist::sort_mode_t sortby;
		bool reverse, filter;
		static karte_ptr_t welt;
};
karte_ptr_t compare_labels::welt;

void labellist_stats_t::get_unique_labels(labellist::sort_mode_t sb, bool sr, bool fi)
{
	sortby = sb;
	sortreverse = sr;
	filter = fi;

	labels.clear();
	last_world_labels = welt->get_label_list().get_count();
	labels.resize(last_world_labels);

	FOR(slist_tpl<koord>, const& pos, welt->get_label_list()) {
		label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		const char* name = welt->lookup_kartenboden(pos)->get_text();
		// some old version games don't have label nor name.
		// Check them to avoid crashes.
		if(label  &&  name  &&  (!filter  ||  (label  &&  (label->get_besitzer() == welt->get_active_player())))) {
			labels.insert_ordered( pos, compare_labels(sortby, sortreverse, filter) );
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
	if(  ev->button_state>0  &&  ev->cx>0  &&  ev->cx<15  ) {
		line_selected = line;
	}

	if (IS_LEFTRELEASE(ev)) {
		if(  ev->cx>0  &&  ev->cx<15  ) {
			welt->get_viewport()->change_world_position(pos);
		}
		else if(welt->lookup_kartenboden(pos)->find<label_t>()) {
			// avoid crash
				welt->lookup_kartenboden(pos)->find<label_t>()->zeige_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->get_viewport()->change_world_position(pos);
	}
	return false;
} // end of function labellist_stats_t::infowin_event(const event_t * ev)


void labellist_stats_t::recalc_size()
{
	sint16 x_size = 0;
	sint16 y_size = 0;

	// loop copied from ::draw(), trimmed to minimum for x_size calculation

	static cbuffer_t buf;

	FOR(vector_tpl<koord>, const& pos, labels) {
		buf.clear();

		// the other infos
		const label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		//PLAYER_COLOR_VAL col = COL_WHITE;
		buf.printf(" (%d,%d)", pos.x, pos.y);

		if(  label  ) {
			//col = (PLAYER_FLAG|label->get_besitzer()->get_player_color1());
			grund_t *gr = welt->lookup(label->get_pos());
			if(  gr  &&  gr->get_text()  ) {
				buf.append(gr->get_text());
			}
		}
		const int px_len = proportional_string_width(buf);
		if(  px_len>x_size  ) {
			x_size = px_len;
		}

		y_size +=LINESPACE+1;
	}

	set_size(scr_size(x_size+10+4,y_size));
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void labellist_stats_t::draw(scr_coord offset)
{
	if(  last_world_labels!=welt->get_label_list().get_count()  ) {
		// some deleted/ added => resort
		get_unique_labels(sortby,sortreverse,filter);
		recalc_size();
	}

	// keep previous maximum width
	int x_size = get_size().w-10-4;

	clip_dimension const cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE+1;
	const int end = cd.yy;

	static cbuffer_t buf;
	int yoff = offset.y;


	// changes to loop affecting x_size must be copied to ::recalc_size()
	uint32 sel = line_selected;
	FORX(vector_tpl<koord>, const& pos, labels, yoff += LINESPACE + 1) {
		if (yoff >= end) break;

		// skip invisible lines
		if (yoff < start) continue;

		// goto button
		display_img_aligned( gui_theme_t::pos_button_img[ sel == 0 ], scr_rect( offset.x, yoff, 14, LINESPACE ), ALIGN_CENTER_V | ALIGN_CENTER_H, true );
		sel --;

		buf.clear();

		// the other infos
		const label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		PLAYER_COLOR_VAL col = COL_WHITE;
		buf.printf(" (%d,%d)", pos.x, pos.y);

		if(label) {
			col = (PLAYER_FLAG|label->get_besitzer()->get_player_color1());
			grund_t *gr = welt->lookup(label->get_pos());
			if(gr && gr->get_text()) {
				buf.append(gr->get_text());
			}
		}
		const int px_len = display_proportional_clip(offset.x+10+4,yoff,buf,ALIGN_LEFT,col,true);
		if(  px_len>x_size  ) {
			x_size = px_len;
		}
	}

	const scr_size size(max(x_size+10+4,get_size().w),labels.get_count()*(LINESPACE+1));
	if(  size!=get_size()  ) {
		set_size(size);
	}
}
