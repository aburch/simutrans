/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "citylist_stats_t.h"
#include "city_info.h"

#include "../simcity.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../gui/simwin.h"
#include "../simworld.h"

#include "../descriptor/skin_desc.h"

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#define L_DIALOG_WIDTH (210)

static const char* total_bev_translation = NULL;
char citylist_stats_t::total_bev_string[128];


citylist_stats_t::citylist_stats_t(citylist::sort_mode_t sortby, bool sortreverse, bool own_network)
{
	total_bev_translation = translator::translate("Total inhabitants:");
	sort(sortby, sortreverse, own_network);
	recalc_size();
	line_selected = 0xFFFFFFFFu;
}


class compare_cities
{
	public:
		compare_cities(citylist::sort_mode_t sortby_, bool reverse_) :
			sortby(sortby_),
			reverse(reverse_)
		{}

		bool operator ()(const stadt_t* a, const stadt_t* b)
		{
			int cmp;
			switch (sortby) {
				default: NOT_REACHED
				case citylist::by_name:   cmp = strcmp(a->get_name(), b->get_name());    break;
				case citylist::by_size:   cmp = a->get_city_population() - b->get_city_population(); break;
				case citylist::by_growth: cmp = a->get_wachstum()  - b->get_wachstum();  break;
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		citylist::sort_mode_t sortby;
		bool reverse;
};


void citylist_stats_t::sort(citylist::sort_mode_t sb, bool sr, bool own_network)
{
	const weighted_vector_tpl<stadt_t*>& cities = welt->get_cities();

	sortby = sb;
	sortreverse = sr;
	filter_own_network = own_network;

	city_list.clear();

	FOR(weighted_vector_tpl<stadt_t*>, const i, cities) {
		// own network filter
		if (filter_own_network && !i->is_within_players_network(welt->get_active_player())) {
			continue;
		}
		city_list.insert_ordered(i, compare_cities(sortby, sortreverse));
	}
	city_list.resize(cities.get_count());
	set_size(scr_size(L_DIALOG_WIDTH, city_list.get_count()*LINESPACE + D_V_SPACE));
}


bool citylist_stats_t::infowin_event(const event_t * ev)
{
	const uint line = ev->cy / (LINESPACE + 1);

	line_selected = 0xFFFFFFFFu;
	if(  line >= city_list.get_count()  ) {
		return false;
	}

	stadt_t* stadt = city_list[line];
	if(  ev->button_state > 0  &&  ev->cx  >0  &&  ev->cx < 15  ) {
		line_selected = line;
	}

	if(  IS_LEFTRELEASE(ev)  &&  ev->cy > 0  ) {
		if(  ev->cx > 0  &&  ev->cx < 15  ) {
			if(  grund_t *gr = welt->lookup_kartenboden( stadt->get_center() )  ) {
				welt->get_viewport()->change_world_position( gr->get_pos() );
			}
		}
		else {
			stadt->show_info();
		}
	}
	else if(  IS_RIGHTRELEASE(ev)  &&  ev->cy > 0  ) {
		if(  grund_t *gr = welt->lookup_kartenboden( stadt->get_center() )  ) {
			welt->get_viewport()->change_world_position( gr->get_pos() );
		}
	}
	return false;
}


void citylist_stats_t::recalc_size()
{
	// show_scroll_x==false ->> size.w not important ->> no need to calc text pixel length
	set_size( scr_size(L_DIALOG_WIDTH, city_list.get_count() * (LINESPACE+1) ) );
}


void citylist_stats_t::draw(scr_coord offset)
{
	cbuffer_t buf;

	if(  welt->get_cities().get_count()!=city_list.get_count()  ) {
		// some deleted/ added => resort
		sort( sortby, sortreverse, filter_own_network );
		recalc_size();
	}

	sint32 sel = line_selected;
	clip_dimension cl = display_get_clip_wh();

	FORX(vector_tpl<stadt_t*>, const stadt, city_list, offset.y += LINESPACE + 1) {

		sint32 population = stadt->get_finance_history_month(0, HIST_CITICENS);
		sint32 growth = stadt->get_finance_history_month(0, HIST_GROWTH);
		uint16 left = D_POS_BUTTON_WIDTH+2;
		if(  offset.y + LINESPACE > cl.y  &&  offset.y <= cl.yy  ) {
			if( (stadt->get_finance_history_month(0, HIST_POWER_RECIEVED) * 9) > (welt->get_finance_history_month(0, HIST_POWER_NEEDED) / 10) )
			{
				display_color_img(skinverwaltung_t::electricity->get_image_id(0), offset.x + left, offset.y, 0, false, false);
			}
			else if (stadt->get_finance_history_month(0, HIST_POWER_RECIEVED) > 0) {
				display_img_blend(skinverwaltung_t::electricity->get_image_id(0), offset.x + left, offset.y, TRANSPARENT50_FLAG, false, false);
			}
			left += 9; // symbol width
			buf.clear();
			buf.append(stadt->get_name());
			display_text_proportional_len_clip(offset.x + left, offset.y, buf, ALIGN_LEFT, SYSCOL_TEXT, true, 25);

			left += (int)(D_BUTTON_WIDTH*1.4) + D_H_SPACE;
			left += proportional_string_width(" 00,000,000");
			buf.clear();
			buf.append( population, 0 );
			display_proportional_clip(offset.x + left, offset.y, buf, ALIGN_RIGHT, SYSCOL_TEXT, true);

			left += D_H_SPACE;
			display_fluctuation_triangle(offset.x + left, offset.y, LINESPACE-4, true, growth);
			left += 9;
			buf.clear();
			if (growth == 0) {
				buf.append("-");
			}
			else {
				buf.append(abs(growth), 0);
			}
			const bool allow_citygrowth = stadt->get_citygrowth();
			left += display_proportional_clip(offset.x + left, offset.y, buf, ALIGN_LEFT, allow_citygrowth ? SYSCOL_TEXT : COL_BLUE, true) + D_H_SPACE;

			if (!allow_citygrowth && skinverwaltung_t::alerts) {
				display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), offset.x + left, offset.y + 1, 0, false, true, translator::translate("City growth is restrained"));
			}

			// goto button
			bool selected = sel==0;
			if(  !selected  ) {
				// still on center?
				if(  grund_t *gr = welt->lookup_kartenboden( stadt->get_center() )  ) {
					selected = welt->get_viewport()->is_on_center( gr->get_pos() );
				}
			}
			display_img_aligned( gui_theme_t::pos_button_img[ selected ], scr_rect( offset.x, offset.y, 14, LINESPACE ), ALIGN_CENTER_V | ALIGN_CENTER_H, true );
			sel --;

			if(  win_get_magic( (ptrdiff_t)stadt )  ) {
				display_blend_wh( offset.x, offset.y, D_DEFAULT_WIDTH, LINESPACE, SYSCOL_TEXT, 25 );
			}
		}
	}
	// some cities there?
	const sint32 total_bev = welt->get_finance_history_month(0, HIST_CITICENS);
	if(  total_bev > 0  ) {
		buf.clear();
		buf.printf( "%s ", total_bev_translation);
		buf.append(total_bev, 0);
		tstrncpy(total_bev_string, buf, lengthof(total_bev_string));
	}
	else {
		total_bev_string[0] = 0;
	}
}
