/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "citylist_frame.h"
#include "citylist_stats.h"


#include "../player/simplay.h"
#include "../dataobj/translator.h"
#include "../simcolor.h"
#include "../simhalt.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer.h"
#include "components/gui_button_to_chart.h"
#include "components/gui_scrolled_list.h"


/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
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

char citylist_frame_t::name_filter[256] = "";


class playername_const_scroll_item_t : public gui_scrolled_list_t::const_text_scrollitem_t {
public:
	const uint8 player_nr;

	playername_const_scroll_item_t( player_t *pl ) : gui_scrolled_list_t::const_text_scrollitem_t( pl->get_name(), color_idx_to_rgb(pl->get_player_color1()+env_t::gui_player_color_dark) ), player_nr(pl->get_player_nr()) { }
};


citylist_frame_t::citylist_frame_t() :
	gui_frame_t(translator::translate("City list")),
	scrolly(gui_scrolled_list_t::windowskin, citylist_stats_t::compare)
{
	old_city_count = 0;
	old_halt_count = 0;

	set_table_layout(1,0);

	add_component(&citizens);

	add_component(&main);
	main.add_tab( &list, translator::translate("City list") );

	list.set_table_layout(1,0);

	list.add_table(3, 3);
	list.new_component<gui_label_t>("Filter:");
	name_filter_input.set_text(name_filter, lengthof(name_filter));
	list.add_component(&name_filter_input);
	list.new_component<gui_fill_t>();

	filter_by_owner.init( button_t::square_automatic, "Served by" );
	filter_by_owner.add_listener(this);
	filter_by_owner.set_tooltip( "At least one stop is connected to the town." );
	list.add_component(&filter_by_owner);

	filterowner.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("No player"), SYSCOL_TEXT);
	for( int i = 0; i < MAX_PLAYER_COUNT; i++ ) {
		if( player_t *pl=welt->get_player(i) ) {
			filterowner.new_component<playername_const_scroll_item_t>(pl);
			if( pl == welt->get_active_player() ) {
				filterowner.set_selection( filterowner.count_elements()-1 );
			}
		}
	}
	filterowner.add_listener(this);
	list.add_component(&filterowner);
	list.new_component<gui_fill_t>();


	list.new_component<gui_label_t>("hl_txt_sort");
	sortedby.set_unsorted(); // do not sort
	for (size_t i = 0; i < lengthof(sort_text); i++) {
		sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
	}
	sortedby.set_selection(citylist_stats_t::sort_mode & 0x1F);
	sortedby.add_listener(this);
	list.add_component(&sortedby);

	sorteddir.init(button_t::sortarrow_state, NULL);
	sorteddir.pressed = citylist_stats_t::sort_mode > citylist_stats_t::SORT_MODES;
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
	scrolly.set_maximize(true);
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
	old_city_count = world()->get_cities().get_count();
	scrolly.clear_elements();
	strcpy(last_name_filter, name_filter);
	if (filter_by_owner.pressed && filterowner.get_selection() == 0) {
		FOR(const weighted_vector_tpl<stadt_t*>, city, world()->get_cities()) {
			bool add = (name_filter[0] == 0 || utf8caseutf8(city->get_name(), name_filter));
			for (int i = 0; add && i < MAX_PLAYER_COUNT; i++) {
				if (player_t* pl = welt->get_player(i)) {
					if (city->is_within_players_network(pl)) {
						// already connected
						add = false;
					}
				}
			}
			if (add) {
				scrolly.new_component<citylist_stats_t>(city);
			}
		}
	}
	else {
		player_t* pl = (filter_by_owner.pressed && filterowner.get_selection() >= 0) ? welt->get_player(((const playername_const_scroll_item_t* )(filterowner.get_selected_item()))->player_nr) : NULL;
		FOR( const weighted_vector_tpl<stadt_t *>, city, world()->get_cities() ) {
			if(  pl == NULL  ||  city->is_within_players_network( pl ) ) {
				if(  last_name_filter[0] == 0  ||  utf8caseutf8(city->get_name(), last_name_filter)  ) {
					scrolly.new_component<citylist_stats_t>(city);
				}
			}
		}
	}
	old_halt_count = haltestelle_t::get_alle_haltestellen().get_count();
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


bool citylist_frame_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	if(comp == &sortedby) {
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)(v.i | (citylist_stats_t::sort_mode & citylist_stats_t::SORT_REVERSE));
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		bool reverse = citylist_stats_t::sort_mode <= citylist_stats_t::SORT_MODES;
		sorteddir.pressed = reverse;
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)((citylist_stats_t::sort_mode & ~citylist_stats_t::SORT_REVERSE) + (reverse * citylist_stats_t::SORT_REVERSE));
		scrolly.sort(0);
	}
	else if(comp == &filterowner) {
		if(  filter_by_owner.pressed ) {
			fill_list();
		}
	}
	else if( comp == &filter_by_owner ) {
		fill_list();
	}
	return true;
}


void citylist_frame_t::draw(scr_coord pos, scr_size size)
{
	welt->update_history();

	if(  world()->get_cities().get_count() != old_city_count  ||  strcmp(last_name_filter, name_filter)  ) {
		fill_list();
	}
	else if(  filter_by_owner.pressed  &&  old_halt_count != haltestelle_t::get_alle_haltestellen().get_count()  ) {
		fill_list();
	}
	update_label();

	gui_frame_t::draw(pos, size);
}


void citylist_frame_t::rdwr(loadsave_t* file)
{
	scr_size size = get_windowsize();
	sint16 sort_mode = citylist_stats_t::sort_mode;

	size.rdwr(file);
	scrolly.rdwr(file);
	year_month_tabs.rdwr(file);
	main.rdwr(file);
	filterowner.rdwr(file);
	sortedby.rdwr(file);
	file->rdwr_str(name_filter, lengthof(name_filter));
	file->rdwr_short(sort_mode);
	if (file->is_loading()) {
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)sort_mode;
		bool reverse = citylist_stats_t::sort_mode <= citylist_stats_t::SORT_MODES;
		sorteddir.pressed = reverse;
		fill_list();
		set_windowsize(size);
	}
}
