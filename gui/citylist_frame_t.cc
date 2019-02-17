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
#include "components/gui_button_to_chart.h"


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


citylist_frame_t::citylist_frame_t() :
	gui_frame_t(translator::translate("City list")),
	scrolly(gui_scrolled_list_t::windowskin, citylist_stats_t::compare)
{
	set_table_layout(1,0);

	add_component(&citizens);

	add_component(&main);
	main.add_tab( &list, translator::translate("City list") );

	list.set_table_layout(1,0);
	list.new_component<gui_label_t>("hl_txt_sort");

	list.add_table(2,0);
	sortedby.init(button_t::roundbox, sort_text[citylist_stats_t::sort_mode & 0x1F]);
	sortedby.add_listener(this);
	list.add_component(&sortedby);

	sorteddir.init(button_t::roundbox, citylist_stats_t::sort_mode > citylist_stats_t::SORT_MODES ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	sorteddir.add_listener(this);
	list.add_component(&sorteddir);
	list.end_table();

	list.add_component(&scrolly);
	fill_list();

	main.add_tab( &statistics, translator::translate("Chart") );

	statistics.set_table_layout(1,0);
	statistics.add_component(&year_month_tabs);

	year_month_tabs.add_tab(&container_year, translator::translate("Years"));
	year_month_tabs.add_tab(&container_month, translator::translate("Months"));
	// .. put the same buttons in both containers
	button_t* buttons[karte_t::MAX_WORLD_COST];

	container_year.set_table_layout(1,0);
	container_year.add_component(&chart);
	chart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_YEARS);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_min_size(scr_size(0, 8*LINESPACE));

	container_year.add_table(4,3);
	for (int i = 0; i<karte_t::MAX_WORLD_COST; i++) {
		sint16 curve = chart.add_curve(color_idx_to_rgb(hist_type_color[i]), welt->get_finance_history_year(), karte_t::MAX_WORLD_COST, i, MAX_WORLD_HISTORY_YEARS, hist_type_type[i], false, true, (i==1) ? 1 : 0 );
		// add button
		buttons[i] = container_year.new_component<button_t>();
		buttons[i]->init(button_t::box_state_automatic | button_t::flexible, hist_type[i]);
		buttons[i]->background_color = color_idx_to_rgb(hist_type_color[i]);
		buttons[i]->pressed = false;

		button_to_chart.append(buttons[i], &chart, curve);
	}
	container_year.end_table();

	container_month.set_table_layout(1,0);
	container_month.add_component(&mchart);
	mchart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_MONTHS);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);
	mchart.set_min_size(scr_size(0, 8*LINESPACE));

	container_month.add_table(4,3);
	for (int i = 0; i<karte_t::MAX_WORLD_COST; i++) {
		sint16 curve = mchart.add_curve(color_idx_to_rgb(hist_type_color[i]), welt->get_finance_history_month(), karte_t::MAX_WORLD_COST, i, MAX_WORLD_HISTORY_MONTHS, hist_type_type[i], false, true, (i==1) ? 1 : 0 );

		// add button
		container_month.add_component(buttons[i]);
		button_to_chart.append(buttons[i], &mchart, curve);
	}
	container_month.end_table();

	update_label();

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
}


void citylist_frame_t::update_label()
{
	citizens.buf().append( translator::translate("Total inhabitants:") );
	citizens.buf().append( welt->get_finance_history_month()[0], 0 );
	citizens.update();
}


void citylist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(const weighted_vector_tpl<stadt_t *>,city,world()->get_cities()) {
		scrolly.new_component<citylist_stats_t>(city) ;
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


bool citylist_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		int i = citylist_stats_t::sort_mode & ~citylist_stats_t::SORT_REVERSE;
		i = (i + 1) % citylist_stats_t::SORT_MODES;
		sortedby.set_text(sort_text[i]);
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)(i | (citylist_stats_t::sort_mode & citylist_stats_t::SORT_REVERSE));
		scrolly.sort(0);
	}
	else if(komp == &sorteddir) {
		bool reverse = citylist_stats_t::sort_mode <= citylist_stats_t::SORT_MODES;
		sorteddir.set_text(reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)((citylist_stats_t::sort_mode & ~citylist_stats_t::SORT_REVERSE) + (reverse*citylist_stats_t::SORT_REVERSE) );
		scrolly.sort(0);
	}
	return true;
}


void citylist_frame_t::draw(scr_coord pos, scr_size size)
{
	welt->update_history();

	if(  (sint32)world()->get_cities().get_count() != scrolly.get_count()  ) {
		fill_list();
	}
	update_label();

	gui_frame_t::draw(pos, size);
}
