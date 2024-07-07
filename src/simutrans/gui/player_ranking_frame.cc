/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../macros.h"
#include "../simcolor.h"
#include "../world/simworld.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../builder/wegbauer.h"

#include "simwin.h"
#include "money_frame.h" // for the finances

#include "player_ranking_frame.h"


uint8 player_ranking_frame_t::transport_type_option = TT_ALL;

// Text should match that of the finance dialog
static const char* cost_type_name[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS] =
{
	"Revenue",
	"Ops Profit",
	"Margin (%%)",
	"Passagiere",
	"Post",
	"Goods",
	"Cash",
	"Net Wealth",
	"Convoys",
	//	"Vehicles",
	//	"Stops"
	// "way_distances" // Way kilometrage
	// "travel_distance"
};

static const uint8 cost_type[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS] =
{
	gui_chart_t::MONEY,
	gui_chart_t::MONEY,
	gui_chart_t::PERCENT,
	gui_chart_t::STANDARD,
	gui_chart_t::STANDARD,
	gui_chart_t::STANDARD,
	gui_chart_t::MONEY,
	gui_chart_t::MONEY,
	gui_chart_t::STANDARD
};

static const uint8 cost_type_color[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS] =
{
	COL_REVENUE,
	COL_PROFIT,
	COL_MARGIN,
	COL_LIGHT_PURPLE,
	COL_TRANSPORTED,
	COL_BROWN,
	COL_CASH,
	COL_WEALTH,
	COL_NEW_VEHICLES
};

// is_atv=1, ATV:vehicle finance record, ATC:common finance record
static const uint8 history_type_idx[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS * 2] =
{
	1,ATV_REVENUE,
	1,ATV_OPERATING_PROFIT,
	1,ATV_PROFIT_MARGIN,
	1,ATV_TRANSPORTED_PASSENGER,
	1,ATV_TRANSPORTED_MAIL,
	1,ATV_TRANSPORTED_GOOD,
	0,ATC_CASH,
	0,ATC_NETWEALTH,
	0,ATC_ALL_CONVOIS,
	//	1,ATV_VEHICLES,
	//	0,ATC_HALTS
};

sint16 years_back = 1;

static int compare_atv(uint8 player_nr_a, uint8 player_nr_b, uint8 atv_index)
{
	sint64 comp = 0;	// otherwise values may overflow
	player_t* a_player = world()->get_player(player_nr_a);
	player_t* b_player = world()->get_player(player_nr_b);

	if (a_player && b_player) {
		comp = b_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, years_back, atv_index) - a_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, years_back, atv_index);
		if (comp == 0  &&  years_back > 0) {
			// tie breaker: next year
			comp = b_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, years_back - 1, atv_index) - a_player->get_finance()->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, years_back - 1, atv_index);
		}
	}
	// if at least one if defined, it must go the the head of the list
	else if(a_player) {
		return true;
	}
	else if (b_player) {
		return false;
	}
	if (comp == 0) {
		comp = player_nr_b - player_nr_a;
	}
	return comp>0; // sort just test for larger than 0
}

static int compare_atc(uint8 player_nr_a, uint8 player_nr_b, uint8 atc_index)
{
	sint64 comp = 0;
	player_t* a_player = world()->get_player(player_nr_a);
	player_t* b_player = world()->get_player(player_nr_b);

	if (a_player && b_player) {
		comp = b_player->get_finance()->get_history_com_year(years_back, atc_index) - a_player->get_finance()->get_history_com_year(years_back, atc_index);
		if (comp == 0  &&  years_back > 0) {
			comp = b_player->get_finance()->get_history_com_year(years_back - 1, atc_index) - a_player->get_finance()->get_history_com_year(years_back - 1, atc_index);
		}
	}
	// if at least one if defined, it must go the the head of the list
	else if (a_player) {
		return true;
	}
	else if (b_player) {
		return false;
	}
	if (comp == 0) {
		comp = player_nr_b - player_nr_a;
	}
	return comp>0;
}

static int compare_revenue(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_REVENUE);
}
static int compare_profit(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_OPERATING_PROFIT);
}
static int compare_transport_pax(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_TRANSPORTED_PASSENGER);
}
static int compare_transport_mail(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_TRANSPORTED_MAIL);
}
static int compare_transport_goods(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_TRANSPORTED_GOOD);
}
static int compare_margin(player_button_t* const& a, player_button_t* const& b) {
	return compare_atv(a->get_player_nr(), b->get_player_nr(), ATV_PROFIT_MARGIN);
}
static int compare_cash(player_button_t* const& a, player_button_t* const& b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_CASH);
}
static int compare_netwealth(player_button_t* const& a, player_button_t* const& b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_NETWEALTH);
}
static int compare_convois(player_button_t* const& a, player_button_t* const& b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_ALL_CONVOIS);
}
/*
static int compare_halts(player_button_t* const& a, player_button_t* const& b) {
	return compare_atc(a->get_player_nr(), b->get_player_nr(), ATC_HALTS);
}
*/

player_button_t::player_button_t(uint8 player_nr_)
{
	player_nr = player_nr_;
	init(button_t::box_state | button_t::flexible, NULL, scr_coord(0, 0), D_BUTTON_SIZE);
	set_tooltip("Click to open the finance dialog.");
	update();
}

void player_button_t::update()
{
	player_t* player = world()->get_player(player_nr);
	if (player) {
		set_text(player->get_name());
		background_color = color_idx_to_rgb(player->get_player_color1() + env_t::gui_player_color_bright);
		enable();
		set_visible(true);
	}
	else {
		set_visible(false);
		disable();
	}
}


player_ranking_frame_t::player_ranking_frame_t(uint8 selected_player_nr) :
	gui_frame_t(translator::translate("Player ranking")),
	scrolly(&cont_players, true, true)
{
	selected_player = selected_player_nr;
	last_year = 0; // update on first drawing
	set_table_layout(1, 0);

	add_table(2, 1)->set_alignment(ALIGN_TOP);
	{
		add_component(&chart);
		sint32 years_back = clamp(welt->get_last_year() - welt->get_settings().get_starting_year(), 2, MAX_PLAYER_HISTORY_YEARS);
		chart.set_dimension(years_back, 10000);
		chart.set_seed(welt->get_last_year());
		chart.set_background(SYSCOL_CHART_BACKGROUND);

		cont_players.set_table_layout(3, 0);
		cont_players.set_alignment(ALIGN_CENTER_H);
		cont_players.set_margin(scr_size(0, 0), scr_size(D_SCROLLBAR_WIDTH + D_H_SPACE, D_SCROLLBAR_HEIGHT));

		for (int np = 0; np < MAX_PLAYER_COUNT; np++) {
			if (np == PUBLIC_PLAYER_NR) continue;
			if (welt->get_player(np)) {
				player_button_t* b = new player_button_t(np);
				b->add_listener(this);
				if (np == selected_player) {
					b->pressed = true;
				}
				buttons.append(b);
			}
		}

		add_table(2, 2);
		{
			years_back_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("This Year"), SYSCOL_TEXT);
			years_back_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Last Year"), SYSCOL_TEXT);
			years_back_c.add_listener(this);
			years_back_c.set_selection(0);
			add_component(&years_back_c);

			for (int i = 0, count = 0; i < TT_OTHER; ++i) {
				transport_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(finance_t::transport_type_values[i]), SYSCOL_TEXT);
				transport_types[count++] = i;
			}
			transport_type_c.add_listener(this);
			transport_type_c.set_selection(0);
			add_component(&transport_type_c);

			scrolly.set_maximize(true);
			scrolly.set_size_corner(false);
			add_component(&scrolly, 2);
		}
		end_table();
	}
	end_table();

	add_table(4, 0)->set_force_equal_columns(true);
	{
		for (uint8 i = 0; i < MAX_PLAYER_RANKING_CHARTS; i++) {
			bt_charts[i].init(button_t::roundbox_state | button_t::flexible, cost_type_name[i]);
			bt_charts[i].background_color = color_idx_to_rgb(cost_type_color[i]);
			if (i == selected_item) bt_charts[i].pressed = true;
			bt_charts[i].add_listener(this);
			add_component(&bt_charts[i]);
		}
	}
	end_table();

	update_chart(true);

	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}


player_ranking_frame_t::~player_ranking_frame_t()
{
	while (!buttons.empty()) {
		player_button_t* b = buttons.remove_first();
		cont_players.remove_component(b);
		delete b;
	}
}


static int (*compare_function[player_ranking_frame_t::MAX_PLAYER_RANKING_CHARTS])(player_button_t* const& a, player_button_t* const& b) =
{
	compare_revenue,//ATV_REVENUE,
	compare_profit,//ATV_OPERATING_PROFIT,
	compare_margin,//ATV_PROFIT_MARGIN,
	compare_transport_pax,//ATV_TRANSPORTED_PASSENGER,
	compare_transport_mail,//ATV_TRANSPORTED_MAIL,
	compare_transport_goods,//ATV_TRANSPORTED_GOOD,
	compare_cash,//ATC_CASH,
	compare_netwealth,//ATC_NETWEALTH,
	compare_convois//ATC_ALL_CONVOIS,
	//	1,ATV_VEHICLES,
	//	0,ATC_HALTS
};


bool player_ranking_frame_t::is_chart_table_zero(uint8 player_nr) const
{
	// search for any non-zero values
	if (player_t* player = welt->get_player(player_nr)) {
		const finance_t* finance = player->get_finance();
		const bool is_atv = history_type_idx[selected_item * 2];
		for (int y = 0; y < MAX_PLAYER_HISTORY_MONTHS; y++) {
			sint64 val = is_atv ? finance->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, y, history_type_idx[selected_item * 2 + 1])
				: finance->get_history_com_year(y, history_type_idx[selected_item * 2 + 1]);
			if (val) return false;
		}
	}

	return true;
}

/**
 * This method is called if an action is triggered
 */
bool player_ranking_frame_t::action_triggered(gui_action_creator_t* comp, value_t v)
{
	// Check the GUI list of buttons
	// player filter
	for (auto bt : buttons) {
		if (comp == bt) {
			if (player_t* p = welt->get_player(bt->get_player_nr())) {
				create_win(new money_frame_t(p), w_info, magic_finances_t + p->get_player_nr());
			}
			return true;
		}
	}

	// chart selector
	for (uint8 i = 0; i < MAX_PLAYER_RANKING_CHARTS; i++) {
		if (comp == &bt_charts[i]) {
			if (bt_charts[i].pressed) {
				// nothing to do
				return true;
			}
			selected_item = i;
			update_chart(true);
			return true;
		}
	}

	if (comp == &years_back_c) {
		update_chart(true);
		return true;
	}

	if (comp == &transport_type_c) {
		if (v.i > 0) {
			transport_type_option = (uint8)v.i;
			transport_type_option = transport_types[v.i];
			update_chart(true);
		}
		else {
			transport_type_option = TT_ALL;
		}
		return true;
	}

	return true;
}


void player_ranking_frame_t::update_chart(bool full_update)
{
	if (years_back == 0) {
		// first update player finances if in the current year
		for (int np = 0; np < MAX_PLAYER_COUNT - 1; np++) {
			if (player_t* player = welt->get_player(np)) {
				player->get_finance()->calc_finance_history();
			}
		}
	}

	// check if we have to upgrade to full update
	if (!full_update) {
		// check order
		full_update |= buttons.sort(compare_function[selected_item]);
		// check if the number of entries changed
		uint8 old_count = buttons.get_count();
		uint8 count = 0;
		for (uint i = 0; i < buttons.get_count(); i++) {
			const uint8 player_nr = buttons.at(i)->get_player_nr();
			// Exclude players who are not in the competition
			if (player_nr == PUBLIC_PLAYER_NR || is_chart_table_zero(player_nr)) {
				continue;
			}
			count++;
		}
		full_update |= count != old_count;
		scrolly.set_visible(count);
	}

	// deselect chart buttons
	for (uint8 i = 0; i < MAX_PLAYER_RANKING_CHARTS; i++) {
		bt_charts[i].pressed = i == selected_item;
	}

	// rebuilt list
	if (full_update) {
		cont_players.remove_all();
		uint8 count = 0;
		for (uint i = 0; i < buttons.get_count(); i++) {
			const uint8 player_nr = buttons.at(i)->get_player_nr();
			// Exclude players who are not in the competition
			if (player_nr == PUBLIC_PLAYER_NR || is_chart_table_zero(player_nr)) {
				continue;
			}
			count++;
			switch (count)
			{
			case 1:
				cont_players.new_component<gui_label_t>("1", color_idx_to_rgb(COL_YELLOW), gui_label_t::centered);
				break;
			case 2:
				cont_players.new_component<gui_label_t>("2", 0, gui_label_t::centered);
				break;
			case 3:
				cont_players.new_component<gui_label_t>("3", 0, gui_label_t::centered);
				break;
			default:
				gui_label_buf_t* lb = cont_players.new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::centered);
				lb->buf().printf("%u", count);
				lb->update();
				lb->set_min_width(lb->get_min_size().w);
				break;
			}

			if (player_nr != selected_player) {
				buttons.at(i)->pressed = false;
			}
			cont_players.add_component(buttons.at(i));
			cont_players.add_component(&lb_player_val[player_nr]);
		}
		cont_players.new_component_span<gui_fill_t>(1,1,3);
	}

	const bool is_atv = history_type_idx[selected_item * 2];
	sint32 chart_years_back = clamp(welt->get_last_year() - welt->get_settings().get_starting_year(), 2, MAX_PLAYER_HISTORY_YEARS);
	if (full_update) {
		chart.set_dimension(chart_years_back, 10000);

		// need to clear the chart once to update the suffix and digit
		chart.remove_curves();
		for (int np = 0; np < MAX_PLAYER_COUNT - 1; np++) {
			if (np == PUBLIC_PLAYER_NR) continue;
			if (player_t* player = welt->get_player(np)) {
				if (is_chart_table_zero(np)) {
					continue;
				}
				// create chart
				const int curve_type = (int)cost_type[selected_item];
				const int curve_precision = (curve_type == gui_chart_t::STANDARD) ? 0 : (curve_type == gui_chart_t::MONEY || curve_type == gui_chart_t::PERCENT) ? 2 : 1;
				//			gui_chart_t::chart_marker_t marker = (np==selected_player) ? gui_chart_t::square : gui_chart_t::none;
				chart.add_curve(color_idx_to_rgb(player->get_player_color1() + 3), *p_chart_table, MAX_PLAYER_COUNT - 1, np, chart_years_back, curve_type, true, false, curve_precision, NULL);
			}
		}

		for (int np = 0; np < MAX_PLAYER_COUNT - 1; np++) {
			if (player_t* player = welt->get_player(np)) {
				// update chart records
				for (int y = 0; y < chart_years_back; y++) {
					const finance_t* finance = player->get_finance();
					p_chart_table[y][np] = is_atv ? finance->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, y, history_type_idx[selected_item * 2 + 1])
						: finance->get_history_com_year(y, history_type_idx[selected_item * 2 + 1]);
				}
				chart.show_curve(np);
			}
		}
	}

	// update labels
	years_back = clamp(years_back_c.get_selection(), 0, chart_years_back);
	scr_size cont_min_sz = cont_players.get_min_size();
	for (uint i = 0; i < buttons.get_count(); i++) {
		const uint8 player_nr = buttons.at(i)->get_player_nr();
		// Exclude players who are not in the competition
		if (welt->get_player(i)==0  ||  player_nr == PUBLIC_PLAYER_NR  ||  is_chart_table_zero(player_nr)) {
			continue;
		}
		if (player_nr != selected_player) {
			buttons.at(i)->pressed = false;
		}
		const finance_t* finance = welt->get_player(i)->get_finance();
		PIXVAL color = SYSCOL_TEXT;
		sint64 value = is_atv ? finance->get_history_veh_year((transport_type)player_ranking_frame_t::transport_type_option, years_back, history_type_idx[selected_item * 2 + 1])
			: finance->get_history_com_year(years_back, history_type_idx[selected_item * 2 + 1]);
		switch (cost_type[selected_item]) {
		case gui_chart_t::MONEY:
			lb_player_val[i].buf().append_money(value / 100.0);
			color = value >= 0 ? (value > 0 ? MONEY_PLUS : SYSCOL_TEXT_UNUSED) : MONEY_MINUS;
			break;
		case gui_chart_t::PERCENT:
			lb_player_val[i].buf().append(value / 100.0, 2);
			lb_player_val[i].buf().append("%");
			color = value >= 0 ? (value > 0 ? MONEY_PLUS : SYSCOL_TEXT_UNUSED) : MONEY_MINUS;
			break;
		case gui_chart_t::STANDARD:
		default:
			lb_player_val[i].buf().append(value, 0);
			break;
		}
		lb_player_val[i].set_color(color);
		lb_player_val[i].set_align(gui_label_t::right);
		lb_player_val[i].update();
	}

	transport_type_c.set_visible(is_atv);
	if (full_update  ||  cont_min_sz!=cont_players.get_min_size()) {
		resize(scr_size(0, 0));
		reset_min_windowsize();
	}
}


void player_ranking_frame_t::draw(scr_coord pos, scr_size size)
{
	static sint8 updatetime = 10; // update all 10 frames
	if (last_year != world()->get_last_year()) {
		// new year has started:
		last_year = world()->get_last_year();
		sint32 years_back = clamp(last_year - welt->get_settings().get_starting_year(), 2, MAX_PLAYER_HISTORY_YEARS);
		// rebuilt year list
		sint32 sel = years_back_c.get_selection();
		years_back_c.clear_elements();
		years_back_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("This Year"), SYSCOL_TEXT);
		years_back_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Last Year"), SYSCOL_TEXT);
		for (int i = 1; i < years_back; i++) {
			sprintf(years_back_s[i - 1], "%i", last_year - i - 1);
			years_back_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(years_back_s[i - 1], SYSCOL_TEXT);
		}
		chart.set_seed(last_year);
		years_back_c.set_selection(sel <= 0 ? 0 : (sel == 1 ? 1 : sel + 1));
		updatetime = 0;
		update_chart(true);
	}
	else {
		update_chart(false);
	}

	gui_frame_t::draw(pos, size);
}


void player_ranking_frame_t::rdwr(loadsave_t* file)
{
	transport_type_c.rdwr(file);
	years_back_c.rdwr(file);
	file->rdwr_byte(selected_item);

	if (file->is_loading()) {
		last_year = world()->get_last_year();
		chart.set_seed(last_year);
		update_chart(true);
	}
	scrolly.rdwr(file);
}
