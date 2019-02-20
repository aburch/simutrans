/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

 /*
 * The citylist dialog
 */

#include "citylist_frame_t.h"
#include "citylist_stats_t.h"


#include "../dataobj/translator.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"


/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool citylist_frame_t::sortreverse = false;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
citylist::sort_mode_t citylist_frame_t::sortby = citylist::by_name;

const char *citylist_frame_t::sort_text[citylist::SORT_MODES] = {
	"Name",
	"citicens",
	"Growth"
};

const char citylist_frame_t::hist_type[karte_t::MAX_WORLD_COST][21] =
{
	"citicens",
	"Jobs",
	"Visitor demand",
	"Growth",
	"Towns",
	"Factories",
	"Convoys",
	"Verkehrsteilnehmer",
	"ratio_pax",
	"Passagiere",
	"ratio_mail",
	"Post",
	"ratio_goods",
	"Goods",
	"Car ownership"
};

const char citylist_frame_t::hist_type_tooltip[karte_t::MAX_WORLD_COST][256] =
{
	"The number of inhabitants of the region",
	"The number of jobs in the region",
	"The number of visitors demanded in the region",
	"The number of inhabitants by which the region has increased",
	"The number of urban areas in the region",
	"The number of industries in the region",
	"The total number of convoys in the region",
	"The number of private car trips in the region overall",
	"The percentage of passengers transported in the region overall",
	"The total number of passengers wanting to be transported in the region",
	"The amount of mail transported in the region overall",
	"The total amount of mail generated in the region",
	"The percentage of available goods that have been transported in the region",
	"The total number of mail/passengers/goods transported in the region",
	"The percentage of people who have access to a private car"
};

const uint8 citylist_frame_t::hist_type_color[karte_t::MAX_WORLD_COST] =
{
	COL_WHITE,
	COL_GREY6,
	COL_GREY3,
	COL_DARK_GREEN,
	COL_LIGHT_PURPLE,
	71 /*COL_GREEN*/,
	COL_TURQUOISE,
	COL_TRAFFIC,
	COL_LIGHT_BLUE,
	COL_PASSENGERS,
	COL_LIGHT_YELLOW,
	COL_YELLOW,
	COL_LIGHT_BROWN,
	COL_BROWN,
	COL_CAR_OWNERSHIP
};

const uint8 citylist_frame_t::hist_type_type[karte_t::MAX_WORLD_COST] =
{
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	MONEY,
	STANDARD,
	MONEY,
	STANDARD,
	MONEY,
	STANDARD,
	STANDARD
};

#define CHART_HEIGHT (168)
#define TOTAL_HEIGHT (D_TITLEBAR_HEIGHT+3*(LINESPACE+1)+42+1)

citylist_frame_t::citylist_frame_t() :
	gui_frame_t(translator::translate("City list")),
	sort_label(translator::translate("hl_txt_sort")),
	stats(sortby,sortreverse),
	scrolly(&stats)
{
	sort_label.set_pos(scr_coord(BUTTON1_X, 40-D_BUTTON_HEIGHT-(LINESPACE+1)));
	add_component(&sort_label);

	sortedby.init(button_t::roundbox, "", scr_coord(BUTTON1_X, 40-D_BUTTON_HEIGHT), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, "", scr_coord(BUTTON2_X, 40-D_BUTTON_HEIGHT), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	show_stats.init(button_t::roundbox_state, "Chart", scr_coord(BUTTON4_X, 40-D_BUTTON_HEIGHT), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	show_stats.set_tooltip("Show/hide statistics");
	show_stats.add_listener(this);
	add_component(&show_stats);

	// name buttons
	sortedby.set_text(sort_text[get_sortierung()]);
	sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	year_month_tabs.set_pos(scr_coord(0,42));
	year_month_tabs.set_size(scr_size(D_DEFAULT_WIDTH, CHART_HEIGHT-D_BUTTON_HEIGHT*3-D_TITLEBAR_HEIGHT));
//	year_month_tabs.add_listener(this);
	year_month_tabs.set_visible(false);
	add_component(&year_month_tabs);

	const sint16 yb = 42+CHART_HEIGHT-D_BUTTON_HEIGHT*4-8;
	chart.set_pos(scr_coord(60,8+D_TAB_HEADER_HEIGHT));
	chart.set_size(scr_size(D_DEFAULT_WIDTH-60-8,yb-16-42-10-D_TAB_HEADER_HEIGHT));
	chart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_YEARS);
	chart.set_visible(false);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_ltr(env_t::left_to_right_graphs);
	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		chart.add_curve(hist_type_color[cost], welt->get_finance_history_year(), karte_t::MAX_WORLD_COST, cost, MAX_WORLD_HISTORY_YEARS, hist_type_type[cost], false, true, (cost==1) ? 1 : 0 );
	}

	mchart.set_pos(scr_coord(60,8+D_TAB_HEADER_HEIGHT));
	mchart.set_size(scr_size(D_DEFAULT_WIDTH-60-8,yb-16-42-10-D_TAB_HEADER_HEIGHT));
	mchart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_MONTHS);
	mchart.set_visible(false);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);
	mchart.set_ltr(env_t::left_to_right_graphs);
	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		mchart.add_curve(hist_type_color[cost], welt->get_finance_history_month(), karte_t::MAX_WORLD_COST, cost, MAX_WORLD_HISTORY_MONTHS, hist_type_type[cost], false, true, (cost==1) ? 1 : 0 );
	}

	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		filterButtons[cost].init(button_t::box_state, hist_type[cost], scr_coord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(cost%4), yb+(D_BUTTON_HEIGHT+2)*(cost/4)), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[cost].set_tooltip(hist_type_tooltip[cost]);
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background_color = hist_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_component(filterButtons + cost);
	}

	scrolly.set_pos(scr_coord(1,42));
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, TOTAL_HEIGHT+CHART_HEIGHT));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, TOTAL_HEIGHT));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


bool citylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		set_sortierung((citylist::sort_mode_t)((get_sortierung() + 1) % citylist::SORT_MODES));
		sortedby.set_text(sort_text[get_sortierung()]);
		stats.sort(get_sortierung(),get_reverse());
		stats.recalc_size();
	}
	else if(comp == &sorteddir) {
		set_reverse(!get_reverse());
		sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		stats.sort(get_sortierung(),get_reverse());
		stats.recalc_size();
	}
	else if(comp == &show_stats) {
		show_stats.pressed = !show_stats.pressed;
		chart.set_visible( show_stats.pressed );
		year_month_tabs.set_visible( show_stats.pressed );
		set_min_windowsize( scr_size(D_DEFAULT_WIDTH, show_stats.pressed ? TOTAL_HEIGHT+CHART_HEIGHT : TOTAL_HEIGHT));
		for(  int i=0;  i<karte_t::MAX_WORLD_COST;  i++ ) {
			filterButtons[i].set_visible(show_stats.pressed);
		}
		resize( scr_coord(0,0) );
	}
	else {
		for(  int i=0;  i<karte_t::MAX_WORLD_COST;  i++ ) {
			if(  comp == filterButtons+i  ) {
				filterButtons[i].pressed = !filterButtons[i].pressed;
				if (filterButtons[i].pressed) {
					chart.show_curve(i);
					mchart.show_curve(i);
				}
				else {
					chart.hide_curve(i);
					mchart.hide_curve(i);
				}
			}
		}
	}
	return true;
}


void citylist_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);

	scr_size size = get_windowsize()-scr_size(0,D_TITLEBAR_HEIGHT+42+1);	// window size - title - 42(header)
	if(show_stats.pressed) {
		// additional space for statistics
		size += scr_size(0,-CHART_HEIGHT);
	}
	scrolly.set_pos( scr_coord(0, 42+(show_stats.pressed*CHART_HEIGHT) ) );
	scrolly.set_size(size);
	set_dirty();
}


void citylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(show_stats.pressed) {
		welt->update_history();
	}
	gui_frame_t::draw(pos,size);

	display_proportional( pos.x+2, pos.y+18, citylist_stats_t::total_bev_string, ALIGN_LEFT, SYSCOL_TEXT, true );
}
