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
#include "../utils/cbuffer_t.h"


/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool citylist_frame_t::sortreverse = false;

const char *citylist_frame_t::sort_text[citylist_stats_t::SORT_MODES] = {
	"Name",
	"citicens",
	"Growth"
};

const char citylist_frame_t::hist_type[karte_t::MAX_WORLD_COST][20] =
{
	"citicens",
	"Growth",
	"Towns",
	"Factories",
	"Convoys",
	"Verkehrsteilnehmer",
	"ratio_pax",
	"Passagiere",
	"sended",
	"Post",
	"Arrived",
	"Goods"
};

const uint8 citylist_frame_t::hist_type_color[karte_t::MAX_WORLD_COST] =
{
	COL_WHITE,
	COL_DARK_GREEN,
	COL_LIGHT_PURPLE,
	71,
	COL_TURQUOISE,
	COL_OPS_PROFIT,
	COL_LIGHT_BLUE,
	COL_SOFT_BLUE,
	COL_LIGHT_YELLOW,
	COL_YELLOW,
	COL_LIGHT_BROWN,
	COL_BROWN
};

const uint8 citylist_frame_t::hist_type_type[karte_t::MAX_WORLD_COST] =
{
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	STANDARD,
	PERCENT,
	STANDARD,
	PERCENT,
	STANDARD,
	PERCENT,
	STANDARD
};

#define CHART_HEIGHT (168)
#define SCROLL_START (D_MARGIN_TOP+D_V_SPACE*2+LINESPACE*2+D_BUTTON_HEIGHT)

static unsigned old_cities = 0;


citylist_frame_t::citylist_frame_t() :
	gui_frame_t(translator::translate("City list")),
	sort_label(translator::translate("hl_txt_sort")),
	scrolly(gui_scrolled_list_t::windowskin)
{
	old_cities = 0;

	sort_label.set_pos(scr_coord(BUTTON1_X, D_MARGIN_TOP+D_V_SPACE+LINESPACE ) );
	add_component(&sort_label);

	sortedby.init(button_t::roundbox, sort_text[citylist_stats_t::sort_mode & 0x1F], scr_coord(BUTTON1_X, D_MARGIN_TOP+D_V_SPACE+LINESPACE*2) );
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, citylist_stats_t::sort_mode > citylist_stats_t::SORT_MODES ? "hl_btn_sort_desc" : "hl_btn_sort_asc", scr_coord(BUTTON2_X, D_MARGIN_TOP+D_V_SPACE+LINESPACE*2));
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	show_stats.init(button_t::roundbox_state, "Chart", scr_coord(BUTTON4_X, D_MARGIN_TOP+D_V_SPACE+LINESPACE*2));
	show_stats.set_tooltip("Show/hide statistics");
	show_stats.add_listener(this);
	add_component(&show_stats);

	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	year_month_tabs.set_pos(scr_coord(0,SCROLL_START));
	year_month_tabs.set_size(scr_size(D_DEFAULT_WIDTH, CHART_HEIGHT-D_BUTTON_HEIGHT*3-D_TITLEBAR_HEIGHT));
	year_month_tabs.set_visible(false);
	add_component(&year_month_tabs);

	const sint16 yb = CHART_HEIGHT-D_V_SPACE-D_TAB_HEADER_HEIGHT-(karte_t::MAX_WORLD_COST/4)*(D_BUTTON_HEIGHT+D_V_SPACE);
	chart.set_pos(scr_coord(D_MARGIN_LEFT,D_TAB_HEADER_HEIGHT));
	chart.set_size(scr_size(D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT,yb));
	chart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_YEARS);
	chart.set_visible(false);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		chart.add_curve(color_idx_to_rgb(hist_type_color[cost]), welt->get_finance_history_year(), karte_t::MAX_WORLD_COST, cost, MAX_WORLD_HISTORY_YEARS, hist_type_type[cost], false, true, (cost==1) ? 1 : 0 );
	}

	mchart.set_pos(scr_coord(D_MARGIN_LEFT,D_TAB_HEADER_HEIGHT));
	mchart.set_size(scr_size(D_DEFAULT_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT,yb));
	mchart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_MONTHS);
	mchart.set_visible(false);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);
	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		mchart.add_curve(color_idx_to_rgb(hist_type_color[cost]), welt->get_finance_history_month(), karte_t::MAX_WORLD_COST, cost, MAX_WORLD_HISTORY_MONTHS, hist_type_type[cost], false, true, (cost==1) ? 1 : 0 );
	}

	for (int cost = 0; cost<karte_t::MAX_WORLD_COST; cost++) {
		filterButtons[cost].init(button_t::box_state, hist_type[cost], scr_coord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(cost%4), SCROLL_START+D_TAB_HEADER_HEIGHT+yb+(D_BUTTON_HEIGHT+D_V_SPACE)*(cost/4)), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background_color = color_idx_to_rgb(hist_type_color[cost]);
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_component(filterButtons + cost);
	}

	scrolly.set_pos(scr_coord(1,SCROLL_START));
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_DEFAULT_HEIGHT));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+(show_stats.pressed ? SCROLL_START+CHART_HEIGHT : SCROLL_START)+3*max( gui_theme_t::gui_pos_button_size.h, LINESPACE ) ) );

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


bool citylist_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		int i = citylist_stats_t::sort_mode & ~citylist_stats_t::SORT_REVERSE;
		i = (i + 1) % citylist_stats_t::SORT_MODES;
		sortedby.set_text(sort_text[i]);
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)(i | (citylist_stats_t::sort_mode & citylist_stats_t::SORT_REVERSE));
		scrolly.sort(0,NULL);
	}
	else if(komp == &sorteddir) {
		bool reverse = citylist_stats_t::sort_mode <= citylist_stats_t::SORT_MODES;
		sorteddir.set_text(reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)((citylist_stats_t::sort_mode & ~citylist_stats_t::SORT_REVERSE) + (reverse*citylist_stats_t::SORT_REVERSE) );
		scrolly.sort(0,NULL);
	}
	else if(komp == &show_stats) {
		show_stats.pressed = !show_stats.pressed;
		chart.set_visible( show_stats.pressed );
		year_month_tabs.set_visible( show_stats.pressed );
		set_min_windowsize( scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+(show_stats.pressed ? SCROLL_START+CHART_HEIGHT : SCROLL_START)+3*max( gui_theme_t::gui_pos_button_size.h, LINESPACE ) ) );
		for(  int i=0;  i<karte_t::MAX_WORLD_COST;  i++ ) {
			filterButtons[i].set_visible(show_stats.pressed);
		}
		resize( scr_coord(0,0) );
	}
	else {
		for(  int i=0;  i<karte_t::MAX_WORLD_COST;  i++ ) {
			if(  komp == filterButtons+i  ) {
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

	scr_size size = get_windowsize()-scr_size(0,SCROLL_START+D_TITLEBAR_HEIGHT); // window size - scroll start
	if(show_stats.pressed) {
		// additional space for statistics
		size.h -= CHART_HEIGHT;
	}
	scrolly.set_pos( scr_coord(0, SCROLL_START+(show_stats.pressed*CHART_HEIGHT) ) );
	scrolly.set_size(size);
	set_dirty();
}


void citylist_frame_t::draw(scr_coord pos, scr_size size)
{
	welt->update_history();

	if(  world()->get_cities().get_count() != old_cities  ) {
		scrolly.clear_elements();
		FOR(const weighted_vector_tpl<stadt_t *>,city,world()->get_cities()) {
			scrolly.append_element( new citylist_stats_t(city) );
		}
		scrolly.sort(0,NULL);
		old_cities = world()->get_cities().get_count();
	}

	gui_frame_t::draw(pos,size);

	cbuffer_t buf;
	buf.append( translator::translate("Total inhabitants:") );
	buf.append( welt->get_finance_history_month()[0], 0 );
	display_proportional_rgb( pos.x+D_MARGIN_LEFT, pos.y+D_MARGIN_TOP+D_TITLEBAR_HEIGHT, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
}
