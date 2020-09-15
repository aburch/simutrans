/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "citylist_frame_t.h"
#include "citylist_stats_t.h"


#include "../dataobj/translator.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"
#include "components/gui_button_to_chart.h"


/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
citylist_stats_t::sort_mode_t citylist_frame_t::sortby = citylist_stats_t::SORT_BY_NAME;
static uint8 default_sortmode = 0;


const char *citylist_frame_t::sort_text[citylist_stats_t::SORT_MODES] = {
	"Name",
	"citicens",
	"Growth",
	"by_region"
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
	PERCENT,
	STANDARD,
	PERCENT,
	STANDARD,
	PERCENT,
	STANDARD,
	PERCENT
};


citylist_frame_t::citylist_frame_t() :
	gui_frame_t(translator::translate("City list")),
	scrolly(gui_scrolled_list_t::windowskin, citylist_stats_t::compare)
{
	set_table_layout(1, 0);

	add_table(3, 0);
	{
		add_component(&citizens);

		fluctuation_world.set_show_border_value(false);
		add_component(&fluctuation_world);

		new_component<gui_fill_t>();
	}
	end_table();

	add_component(&main);
	main.add_tab(&list, translator::translate("City list"));

	list.set_table_layout(1, 0);
	list.new_component<gui_label_t>("hl_txt_sort");

	list.add_table(5, 0);
	for (int i = 0; i < citylist_stats_t::SORT_MODES; i++) {
		sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
	}
	sortedby.set_selection(default_sortmode);
	sortedby.add_listener(this);
	list.add_component(&sortedby);

	// sort ascend/descend button
	sort_asc.init(button_t::arrowup_state, "");
	sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
	sort_asc.add_listener(this);
	sort_asc.pressed = citylist_stats_t::sortreverse;
	list.add_component(&sort_asc);

	sort_desc.init(button_t::arrowdown_state, "");
	sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
	sort_desc.add_listener(this);
	sort_desc.pressed = !(citylist_stats_t::sortreverse);
	list.add_component(&sort_desc);
	list.new_component<gui_margin_t>(LINESPACE);

	filter_within_network.init(button_t::square_state, "Within own network");
	filter_within_network.set_tooltip("Show only cities within the active player's transportation network");
	filter_within_network.add_listener(this);
	filter_within_network.pressed = citylist_stats_t::filter_own_network;
	list.add_component(&filter_within_network);

	list.end_table();

	list.add_component(&scrolly);
	fill_list();

	main.add_tab(&statistics, translator::translate("Chart"));

	statistics.set_table_layout(1, 0);
	statistics.add_component(&year_month_tabs);

	year_month_tabs.add_tab(&container_year, translator::translate("Years"));
	year_month_tabs.add_tab(&container_month, translator::translate("Months"));
	// .. put the same buttons in both containers
	button_t* buttons[karte_t::MAX_WORLD_COST];

	container_year.set_table_layout(1, 0);
	container_year.add_component(&chart);
	chart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_YEARS);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_ltr(env_t::left_to_right_graphs);
	chart.set_min_size(scr_size(0, 7 * LINESPACE));

	container_year.add_table(4, 0);
	for (int i = 0; i < karte_t::MAX_WORLD_COST; i++) {
		sint16 curve = chart.add_curve(color_idx_to_rgb(hist_type_color[i]), welt->get_finance_history_year(), karte_t::MAX_WORLD_COST, i, MAX_WORLD_HISTORY_YEARS, hist_type_type[i], false, true, (i == 1) ? 1 : 0);
		// add button
		buttons[i] = container_year.new_component<button_t>();
		buttons[i]->init(button_t::box_state_automatic | button_t::flexible, hist_type[i]);
		buttons[i]->background_color = color_idx_to_rgb(hist_type_color[i]);
		buttons[i]->pressed = false;

		button_to_chart.append(buttons[i], &chart, curve);
	}
	container_year.end_table();

	container_month.set_table_layout(1, 0);
	container_month.add_component(&mchart);
	mchart.set_dimension(12, karte_t::MAX_WORLD_COST*MAX_WORLD_HISTORY_MONTHS);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);
	mchart.set_ltr(env_t::left_to_right_graphs);
	mchart.set_min_size(scr_size(0, 7 * LINESPACE));

	container_month.add_table(4,0);
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
	citizens.buf().append(translator::translate("Total inhabitants:"));
	citizens.buf().append(welt->get_finance_history_month()[0], 0);
	citizens.update();

	fluctuation_world.set_value(welt->get_finance_history_month(1, karte_t::WORLD_GROWTH));
}


void citylist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(const weighted_vector_tpl<stadt_t *>, city, world()->get_cities()) {
		if (!citylist_stats_t::filter_own_network ||
			(citylist_stats_t::filter_own_network && city->is_within_players_network(welt->get_active_player()))) {
			scrolly.new_component<citylist_stats_t>(city);
		}
	}
	scrolly.sort(0);
	scrolly.set_size(scrolly.get_size());
}


bool citylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			set_sortierung((citylist_stats_t::sort_mode_t)tmp);
		}
		else {
			sortedby.set_selection(0);
			set_sortierung(citylist_stats_t::SORT_BY_NAME);
		}
		default_sortmode = (uint8)tmp;
		citylist_stats_t::sort_mode = (citylist_stats_t::sort_mode_t)(default_sortmode | (citylist_stats_t::sort_mode & citylist_stats_t::SORT_MODES));
		scrolly.sort(0);
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		citylist_stats_t::sortreverse = !citylist_stats_t::sortreverse;
		scrolly.sort(0);
		sort_asc.pressed = citylist_stats_t::sortreverse;
		sort_desc.pressed = !(citylist_stats_t::sortreverse);
	}
	else if (comp == &filter_within_network) {
		citylist_stats_t::filter_own_network = !citylist_stats_t::filter_own_network;
		filter_within_network.pressed = citylist_stats_t::filter_own_network;
		fill_list();
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
