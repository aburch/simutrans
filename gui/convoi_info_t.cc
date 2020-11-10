/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "convoi_info_t.h"
#include "replace_frame.h"

#include "../simunits.h"
#include "../simdepot.h"
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "simwin.h"
#include "../convoy.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"
#include "schedule_gui.h"
#include "times_history.h"
// @author hsiegeln
#include "../simlinemgmt.h"
#include "../simline.h"
#include "messagebox.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"



#include "convoi_detail_t.h"

static const char cost_type[BUTTON_COUNT][64] =
{
	"Free Capacity",
	"Transported",
	"Distance",
	"Average speed",
	//"Maxspeed",
	"Comfort",
	"Revenue",
	"Operation",
	"Refunds",
	"Road toll",
	"Profit"
#ifdef ACCELERATION_BUTTON
	, "Acceleration"
#endif
};

static const int cost_type_color[BUTTON_COUNT] =
{
	COL_FREE_CAPACITY,
	COL_TRANSPORTED,
	COL_DISTANCE,
	COL_AVERAGE_SPEED,
//	COL_MAXSPEED,
	COL_COMFORT,
	COL_REVENUE,
	COL_OPERATION,
	COL_CAR_OWNERSHIP,
	COL_TOLL,
	COL_PROFIT
#ifdef ACCELERATION_BUTTON
	, COL_YELLOW
#endif
};

static const bool cost_type_money[BUTTON_COUNT] =
{
	false,
	false,
	false,
	false,
	//false,
	false,
	true,
	true,
	true,
	true,
	true
#ifdef ACCELERATION_BUTTON
	, false
#endif
};

static uint8 statistic[convoi_t::MAX_CONVOI_COST] = {
	convoi_t::CONVOI_CAPACITY, convoi_t::CONVOI_TRANSPORTED_GOODS, convoi_t::CONVOI_DISTANCE, convoi_t::CONVOI_AVERAGE_SPEED, convoi_t::CONVOI_COMFORT,
	convoi_t::CONVOI_REVENUE, convoi_t::CONVOI_OPERATIONS, convoi_t::CONVOI_REFUNDS, convoi_t::CONVOI_WAYTOLL, convoi_t::CONVOI_PROFIT
};

//bool convoi_info_t::route_search_in_progress=false;

/**
 * This variable defines by which column the table is sorted
 * Values:			0 = destination
 *                  1 = via
 *                  2 = via_amount
 *                  3 = amount
 *					4 = origin
 *					5 = origin_amount
 *					6 = destination (detail)
 * @author prissi - amended by jamespetts (origins)
 */
const char *convoi_info_t::sort_text[SORT_MODES] =
{
	"Zielort",
	"via",
	"via Menge",
	"Menge",
	"origin (detail)",
	"origin (amount)",
	"destination (detail)",
	"wealth (detail)",
	"wealth (via)",
	"accommodation (detail)",
	"accommodation (via)"
};


convoi_info_t::convoi_info_t(convoihandle_t cnv)
:	gui_frame_t( cnv->get_name(), cnv->get_owner() ),
	scrolly(&text),
	text(&freight_info),
	view(cnv->front(), scr_size(max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	sort_label(translator::translate("loaded passenger/freight"))
{
	this->cnv = cnv;
	this->mean_convoi_speed = speed_to_kmh(cnv->get_akt_speed()*4);
	this->max_convoi_speed = speed_to_kmh(cnv->get_min_top_speed()*4);

	const sint16 offset_below_viewport = D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE + view.get_size().h;

	scr_coord cursor(D_MARGIN_LEFT, D_MARGIN_TOP);
	input.set_pos( cursor );
	cursor.y += D_BUTTON_HEIGHT + D_V_SPACE;

	reset_cnv_name();
	input.add_listener(this);
	add_component(&input);

	add_component(&view);

	scr_coord dummy(D_MARGIN_LEFT,D_MARGIN_TOP);

	// this convoi doesn't belong to an AI
	button.init(button_t::roundbox, "Fahrplan", dummy, D_BUTTON_SIZE);
	button.set_tooltip("Alters a schedule.");
	button.add_listener(this);
	add_component(&button);

	go_home_button.init(button_t::roundbox, "go home", dummy, D_BUTTON_SIZE);
	go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
	go_home_button.add_listener(this);
	add_component(&go_home_button);

	no_load_button.init(button_t::roundbox, "no load", dummy, D_BUTTON_SIZE);
	no_load_button.set_tooltip("No goods are loaded onto this convoi.");
	no_load_button.add_listener(this);
	add_component(&no_load_button);

	replace_button.init(button_t::roundbox, "Replace", dummy, D_BUTTON_SIZE);
	replace_button.set_tooltip("Automatically replace this convoy.");
	add_component(&replace_button);
	replace_button.add_listener(this);

	times_history_button.init(button_t::roundbox, "times_history", dummy, D_BUTTON_SIZE);
	times_history_button.set_tooltip("view_journey_times_history_of_this_convoy");
	add_component(&times_history_button);
	times_history_button.add_listener(this);

	//Position is set in convoi_info_t::set_windowsize()
	follow_button.init(button_t::roundbox_state, "follow me", dummy, scr_size(view.get_size().w, D_BUTTON_HEIGHT));
	follow_button.set_tooltip("Follow the convoi on the map.");
	follow_button.add_listener(this);
	add_component(&follow_button);

	// chart
	//chart.set_pos(scr_coord(88,offset_below_viewport+D_BUTTON_HEIGHT+16));
	//chart.set_size(scr_size(total_width-88-4, 100));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_ltr(env_t::left_to_right_graphs);
	const sint16 offset_below_chart = offset_below_viewport+D_BUTTON_HEIGHT+11 // chart position
	                                  +88                                      // chart size
	                                  +6+LINESPACE+D_V_SPACE;                  // chart x-axis labels plus space

	int btn;
	for (btn = 0; btn < convoi_t::MAX_CONVOI_COST; btn++) {
		chart.add_curve( cost_type_color[btn], cnv->get_finance_history(), convoi_t::MAX_CONVOI_COST, statistic[btn], MAX_MONTHS, cost_type_money[btn], false, true, cost_type_money[btn]*2 );
		filterButtons[btn].init(button_t::box_state, cost_type[btn],
			scr_coord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(btn%4), offset_below_chart+(D_BUTTON_HEIGHT+D_V_SPACE)*(btn/4)),
			D_BUTTON_SIZE);
		filterButtons[btn].add_listener(this);
		filterButtons[btn].background_color = cost_type_color[btn];
		filterButtons[btn].set_visible(false);
		filterButtons[btn].pressed = false;
		if((statistic[btn] == convoi_t::CONVOI_REFUNDS) && cnv->get_line().is_bound())
		{
			continue;
		}
		else
		{
			add_component(filterButtons + btn);
		}
	}


#ifdef ACCELERATION_BUTTON
	//Bernd Gabriel, Sep, 24 2009: acceleration curve:

	for (int i = 0; i < MAX_MONTHS; i++)
	{
		physics_curves[i][0] = 0;
	}

	chart.add_curve(cost_type_color[btn], (sint64*)physics_curves, 1,0, MAX_MONTHS, cost_type_money[btn], false, true, cost_type_money[btn]*2);
	filterButtons[btn].init(button_t::box_state, cost_type[btn],
			scr_coord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(btn%4), view.get_size().h+174+(D_BUTTON_HEIGHT+D_H_SPACE)*(btn/4)),
			D_BUTTON_SIZE);
	filterButtons[btn].add_listener(this);
	filterButtons[btn].background_color = cost_type_color[btn];
	filterButtons[btn].set_visible(false);
	filterButtons[btn].pressed = false;
	add_component(filterButtons + btn);
#endif
	statistics_height = 16 + view.get_size().h+174+(D_BUTTON_HEIGHT+D_H_SPACE)*(btn/4 + 1) - chart.get_pos().y;

	add_component(&chart);

	chart_total_size = filterButtons[convoi_t::MAX_CONVOI_COST-1].get_pos().y + D_BUTTON_HEIGHT + D_V_SPACE - (chart.get_pos().y - 11);

	add_component(&sort_label);


	freight_sort_selector.clear_elements();
	for (int i = 0; i < SORT_MODES; i++)
	{
		freight_sort_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(sort_text[i]), SYSCOL_TEXT));
	}
	freight_sort_selector.set_focusable(true);
	freight_sort_selector.set_selection(env_t::default_sortmode);
	freight_sort_selector.set_highlight_color(1);
	freight_sort_selector.set_pos(dummy);
	freight_sort_selector.set_max_size(scr_size(D_BUTTON_WIDTH * 2, LINESPACE * 5 + 2 + 16));
	freight_sort_selector.add_listener(this);
	add_component(&freight_sort_selector);

	toggler.init(button_t::roundbox_state, "Chart", dummy, D_BUTTON_SIZE);
	toggler.set_tooltip("Show/hide statistics");
	toggler.add_listener(this);
	add_component(&toggler);

	details_button.init(button_t::roundbox, "Details", dummy, D_BUTTON_SIZE);
	details_button.set_tooltip("Vehicle details");
	details_button.add_listener(this);
	add_component(&details_button);

	reverse_button.init(button_t::square_state, "reverse route", dummy, scr_size(D_BUTTON_WIDTH*2, D_BUTTON_HEIGHT));
	reverse_button.add_listener(this);
	reverse_button.set_tooltip("When this is set, the vehicle will visit stops in reverse order.");
	reverse_button.pressed = cnv->get_reverse_schedule();
	add_component(&reverse_button);

	text.set_pos( scr_coord(D_H_SPACE,D_V_SPACE) );
	scrolly.set_pos(dummy);

	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	filled_bar.add_color_value(&cnv->get_loading_limit(), COL_YELLOW);
	filled_bar.add_color_value(&cnv->get_loading_level(), COL_GREEN);
	add_component(&filled_bar);

	speed_bar.set_base(max_convoi_speed);
	speed_bar.set_vertical(false);
	speed_bar.add_color_value(&mean_convoi_speed, COL_GREEN);
	add_component(&speed_bar);

	// we update this ourself!
	route_bar.add_color_value(&cnv_route_index, COL_GREEN);
	add_component(&route_bar);

	// goto line button
	line_button.init( button_t::posbutton, NULL, dummy);
	line_button.set_targetpos( koord(0,0) );
	line_button.add_listener( this );
	line_bound = false;

	cnv->set_sortby( env_t::default_sortmode );

	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max(D_BUTTON_WIDTH, view.get_size().w) + D_MARGIN_RIGHT;
	set_windowsize(scr_size(total_width, view.get_size().h+208+D_SCROLLBAR_HEIGHT));
	set_min_windowsize(scr_size(total_width, view.get_size().h+131+D_SCROLLBAR_HEIGHT));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


// only handle a pending renaming ...
convoi_info_t::~convoi_info_t()
{
	// rename if necessary
	rename_cnv();
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 * @author Hj. Malthaner
 */
void convoi_info_t::draw(scr_coord pos, scr_size size)
{
	if (!cnv.is_bound() || cnv->in_depot() || cnv->get_vehicle_count() == 0)
	{
		destroy_win(this);
	}
	else {
		//Bernd Gabriel, Dec, 02 2009: common existing_convoy_t for acceleration curve and weight/speed info.
		convoi_t &convoy = *cnv.get_rep();

#ifdef ACCELERATION_BUTTON
		//Bernd Gabriel, Sep, 24 2009: acceleration curve:
		if (filterButtons[ACCELERATION_BUTTON].is_visible() && filterButtons[ACCELERATION_BUTTON].pressed)
		{
			const int akt_speed_soll = kmh_to_speed(convoy.calc_max_speed(convoy.get_weight_summary()));
			float32e8_t akt_v = 0;
			sint32 akt_speed = 0;
			sint32 sp_soll = 0;
			int i = MAX_MONTHS;
			physics_curves[--i][0] = akt_speed;
			while (i > 0)
			{
				convoy.calc_move(welt->get_settings(), 15 * 64, akt_speed_soll, akt_speed_soll, SINT32_MAX_VALUE, SINT32_MAX_VALUE, akt_speed, sp_soll, akt_v);
				physics_curves[--i][0] = speed_to_kmh(akt_speed);
			}
		}
#endif

		// Bernd Gabriel, 01.07.2009: show some colored texts and indicator
		input.set_color(cnv->has_obsolete_vehicles() ? COL_OBSOLETE : SYSCOL_TEXT);

		// make titlebar dirty to display the correct coordinates
		if (cnv->get_owner() == welt->get_active_player() && !welt->get_active_player()->is_locked()) {
			if (line_bound && !cnv->get_line().is_bound()) {
				remove_component(&line_button);
				line_bound = false;
			}
			else if (!line_bound  &&  cnv->get_line().is_bound()) {
				add_component(&line_button);
				line_bound = true;
			}
			button.enable();
			details_button.pressed = win_get_magic(magic_convoi_detail + cnv.get_id());
			go_home_button.enable(); // Will be disabled, if convoy goes to a depot.
			if (!cnv->get_schedule()->empty()) {
				const grund_t* g = welt->lookup(cnv->get_schedule()->get_current_entry().pos);
				if (g != NULL && g->get_depot()) {
					go_home_button.disable();
				}
				else {
					goto enable_home;
				}
			}
			else
			{
			enable_home:
				go_home_button.enable();
			}

			if (grund_t* gr = welt->lookup(cnv->get_schedule()->get_current_entry().pos)) {
				go_home_button.pressed = gr->get_depot() != NULL;
			}
			details_button.pressed = win_get_magic(magic_convoi_detail + cnv.get_id());

			no_load_button.pressed = cnv->get_no_load();
			no_load_button.enable();
			replace_button.pressed = cnv->get_replace();
			replace_button.set_text(cnv->get_replace() ? "Replacing" : "Replace");
			replace_button.enable();
			reverse_button.pressed = cnv->get_reverse_schedule();
			reverse_button.enable();
			times_history_button.enable();
		}
		else {
			if (line_bound) {
				// do not jump to other player line window
				remove_component(&line_button);
				line_bound = false;
			}
			button.disable();
			go_home_button.disable();
			no_load_button.disable();
			replace_button.disable();
			reverse_button.disable();
			times_history_button.disable();
		}
		follow_button.pressed = (welt->get_viewport()->get_follow_convoi() == cnv);

		// buffer update now only when needed by convoi itself => dedicated buffer for this
		const int old_len = freight_info.len();
		cnv->get_freight_info(freight_info);
		if (old_len != freight_info.len()) {
			text.recalc_size();
		}

		route_bar.set_base(cnv->get_route()->get_count() - 1);
		cnv_route_index = cnv->front()->get_route_index() - 1;

		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		set_dirty();

		PUSH_CLIP(pos.x + 1, pos.y + D_TITLEBAR_HEIGHT, size.w - 2, size.h - D_TITLEBAR_HEIGHT);

		//indicator
		{
			scr_coord ipos = pos + view.get_pos();
			scr_size isize = view.get_size();
			ipos.y += isize.h + 16; //what is the magic number 16?

			COLOR_VAL color = COL_BLACK;
			switch (cnv->get_state())
			{
			case convoi_t::INITIAL:
				color = COL_WHITE;
				break;
			case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
			case convoi_t::CAN_START_ONE_MONTH:
				color = COL_ORANGE;
				break;

			default:
				if (cnv->get_state() == convoi_t::NO_ROUTE || cnv->get_state() == convoi_t::NO_ROUTE_TOO_COMPLEX)
					color = COL_RED;
			}
			display_ddd_box_clip(ipos.x, ipos.y, isize.w, 8, MN_GREY0, MN_GREY4);
			display_fillbox_wh_clip(ipos.x + 1, ipos.y + 1, isize.w - 2, 6, color, true);
		}


		// convoi information
		static cbuffer_t info_buf;
		const int pos_x = pos.x + D_MARGIN_LEFT;
		const int pos_y0 = pos.y + view.get_pos().y + LINESPACE + D_V_SPACE + 2;
		const char *caption = translator::translate("%s:");



		uint32 empty_weight = convoy.get_vehicle_summary().weight;
		uint32 gross_weight = convoy.get_weight_summary().weight;

		COLOR_VAL speed_color = COL_BLACK;
		const int pos_y = pos_y0; // line 1
		char speed_text[256];

		air_vehicle_t* air_vehicle = NULL;
		if (cnv->front()->get_waytype() == air_wt)
		{
			air_vehicle = (air_vehicle_t*)cnv->front();
		}

		speed_bar.set_visible(false);

		switch (cnv->get_state())
		{
		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::WAITING_FOR_CLEARANCE:

			if (air_vehicle && air_vehicle->is_runway_too_short() == true)
			{
				sprintf(speed_text, "%s (%s) %i%s", translator::translate("Runway too short"), translator::translate("requires"), cnv->front()->get_desc()->get_minimum_runway_length(), translator::translate("m"));
				speed_color = COL_RED;
			}
			else
			{
				sprintf(speed_text, "%s", translator::translate("Waiting for clearance!"));
				speed_color = COL_YELLOW;
			}
			break;

		case convoi_t::CAN_START:
		case convoi_t::CAN_START_ONE_MONTH:

			sprintf(speed_text, "%s", translator::translate("Waiting for clearance!"));
			speed_color = COL_BLACK;
			break;

		case convoi_t::EMERGENCY_STOP:

			char emergency_stop_time[64];
			cnv->snprintf_remaining_emergency_stop_time(emergency_stop_time, sizeof(emergency_stop_time));

			sprintf(speed_text, translator::translate("emergency_stop %s left"), emergency_stop_time);
			speed_color = COL_RED;
			break;

		case convoi_t::LOADING:

			char waiting_time[64];
			cnv->snprintf_remaining_loading_time(waiting_time, sizeof(waiting_time));
			if (cnv->get_schedule()->get_current_entry().wait_for_time)
			{
				sprintf(speed_text, translator::translate("Waiting for schedule. %s left"), waiting_time);
				speed_color = COL_YELLOW;
			}
			else if (cnv->get_loading_limit())
			{
				if (!cnv->is_wait_infinite() && strcmp(waiting_time, "0:00"))
				{
					sprintf(speed_text, translator::translate("Loading (%i->%i%%), %s left!"), cnv->get_loading_level(), cnv->get_loading_limit(), waiting_time);
					speed_color = COL_YELLOW;
				}
				else
				{
					sprintf(speed_text, translator::translate("Loading (%i->%i%%)!"), cnv->get_loading_level(), cnv->get_loading_limit());
					speed_color = COL_YELLOW;
				}
			}
			else
			{
				sprintf(speed_text, translator::translate("Loading. %s left!"), waiting_time);
				speed_color = COL_BLACK;
			}

			break;

		case convoi_t::WAITING_FOR_LOADING_THREE_MONTHS:
		case convoi_t::WAITING_FOR_LOADING_FOUR_MONTHS:
			sprintf(speed_text, translator::translate("Loading (%i->%i%%) Long Time"), cnv->get_loading_level(), cnv->get_loading_limit());
			speed_color = COL_ORANGE;
			break;

		case convoi_t::REVERSING:

			char reversing_time[64];
			cnv->snprintf_remaining_reversing_time(reversing_time, sizeof(reversing_time));
			switch (cnv->get_terminal_shunt_mode()) {
			case convoi_t::rearrange:
			case convoi_t::shunting_loco:
				sprintf(speed_text, translator::translate("Shunting. %s left"), reversing_time);
				break;
			case convoi_t::change_direction:
				sprintf(speed_text, translator::translate("Changing direction. %s left"), reversing_time);
				break;
			default:
				sprintf(speed_text, translator::translate("Reversing. %s left"), reversing_time);
				break;
			}
			speed_color = COL_BLACK;
			break;

		case convoi_t::CAN_START_TWO_MONTHS:
		case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:

			if (air_vehicle && air_vehicle->is_runway_too_short() == true)
			{
				sprintf(speed_text, "%s (%s %i%s)", translator::translate("Runway too short"), translator::translate("requires"), cnv->front()->get_desc()->get_minimum_runway_length(), translator::translate("m"));
				speed_color = COL_RED;
			}
			else
			{
				sprintf(speed_text, "%s", translator::translate("clf_chk_stucked"));
				speed_color = COL_ORANGE;
			}
			break;

		case convoi_t::NO_ROUTE:

			if (air_vehicle && air_vehicle->is_runway_too_short() == true)
			{
				sprintf(speed_text, "%s (%s %i%s)", translator::translate("Runway too short"), translator::translate("requires"), cnv->front()->get_desc()->get_minimum_runway_length(), translator::translate("m"));
			}
			else
			{
				sprintf(speed_text, "%s", translator::translate("clf_chk_noroute"));
			}
			speed_color = COL_RED;
			break;

		case convoi_t::NO_ROUTE_TOO_COMPLEX:
			//sprintf(speed_text, translator::translate("no_route_too_complex_message"));
			sprintf(speed_text, "%s", translator::translate("clf_chk_noroute"));
			speed_color = COL_RED;
			break;

		case convoi_t::OUT_OF_RANGE:

			//sprintf(speed_text, translator::translate("out_of_range (max %i km)"), cnv->front()->get_desc()->get_range());
			sprintf(speed_text, "%s (%s %i%s)", translator::translate("out of range"), translator::translate("max"), cnv->front()->get_desc()->get_range(), translator::translate("km"));
			speed_color = COL_RED;
			break;

		default:
			if (air_vehicle && air_vehicle->is_runway_too_short() == true)
			{
				sprintf(speed_text, "%s (%s %i%s)", translator::translate("Runway too short"), translator::translate("requires"), cnv->front()->get_desc()->get_minimum_runway_length(), translator::translate("m"));
				speed_color = COL_RED;
			}
			else
			{
				speed_bar.set_visible(true);
				//use median speed to avoid flickering
				mean_convoi_speed += speed_to_kmh(cnv->get_akt_speed() * 4);
				mean_convoi_speed /= 2;
				const sint32 min_speed = convoy.calc_max_speed(convoy.get_weight_summary());
				const sint32 max_speed = convoy.calc_max_speed(weight_summary_t(empty_weight, convoy.get_current_friction()));
				sprintf(speed_text, translator::translate(min_speed == max_speed ? "%i km/h (max. %ikm/h)" : "%i km/h (max. %i %s %ikm/h)"),
					(mean_convoi_speed + 3) / 4, min_speed, translator::translate("..."), max_speed);
				speed_color = COL_BLACK;
			}
		}

		display_proportional(pos_x, pos_y, speed_text, ALIGN_LEFT, speed_color, true);



		//next important: income stuff
		{
			const int pos_y = pos_y0 + LINESPACE; // line 2
			char tmp[256];
			// Bernd Gabriel, 01.07.2009: inconsistent adding of ':'. Sometimes in code, sometimes in translation. Consistently moved to code.
			sprintf(tmp, caption, translator::translate("Profit"));
			int len = display_proportional(pos_x, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true) + 5;
			money_to_string(tmp, cnv->get_jahresgewinn() / 100.0);
			len += display_proportional(pos_x + len, pos_y, tmp, ALIGN_LEFT, cnv->get_jahresgewinn() > 0 ? MONEY_PLUS : MONEY_MINUS, true) + 5;
			// Bernd Gabriel, 17.06.2009: add fixed maintenance info
			uint32 fixed_monthly = welt->calc_adjusted_monthly_figure(cnv->get_fixed_cost());
			if (fixed_monthly)
			{
				char tmp_2[64];
				tmp_2[0] = '(';
				money_to_string(tmp_2 + 1, cnv->get_per_kilometre_running_cost() / 100.0);
				strcat(tmp_2, translator::translate("/km)"));
				sprintf(tmp, tmp_2, translator::translate(" %1.2f$/mon)"), fixed_monthly / 100.0);
			}
			else
			{
				//sprintf(tmp, translator::translate("(%1.2f$/km)"), cnv->get_per_kilometre_running_cost()/100.0 );
				tmp[0] = '(';
				money_to_string(tmp + 1, cnv->get_per_kilometre_running_cost() / 100.0);
				strcat(tmp, "/km)");
			}
			display_proportional(pos_x + len, pos_y, tmp, ALIGN_LEFT, cnv->has_obsolete_vehicles() ? COL_OBSOLETE : SYSCOL_TEXT, true);
		}

		//Average round trip time
		{
			const int pos_y = pos_y0 + 2 * LINESPACE; // line 3
			sint64 average_round_trip_time = cnv->get_average_round_trip_time();
			info_buf.clear();
			info_buf.printf(caption, translator::translate("Avg trip time"));
			if (average_round_trip_time) {
				char as_clock[32];
				welt->sprintf_ticks(as_clock, sizeof(as_clock), average_round_trip_time);
				info_buf.printf(" %s", as_clock);
			}
			display_proportional(pos_x, pos_y, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
		}

		// the weight entry
		{
			const int pos_y = pos_y0 + 3 * LINESPACE; // line 4
			char tmp[256];
			// Bernd Gabriel, 01.07.2009: inconsistent adding of ':'. Sometimes in code, sometimes in translation. Consistently moved to code.
			sprintf(tmp, caption, translator::translate("Gewicht"));
			const int len = display_proportional(pos_x, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true) + 5;
			const int freight_weight = gross_weight - empty_weight; // cnv->get_sum_gesamtweight() - cnv->get_sum_weight();
			sprintf(tmp, translator::translate(freight_weight ? "%g (%g) t" : "%g t"), gross_weight * 0.001f, freight_weight * 0.001f);
			display_proportional(pos_x + len, pos_y, tmp, ALIGN_LEFT,
				cnv->get_overcrowded() > 0 ? COL_DARK_PURPLE : // overcrowded
				!cnv->get_finance_history(0, convoi_t::CONVOI_TRANSPORTED_GOODS) && !cnv->get_finance_history(1, convoi_t::CONVOI_TRANSPORTED_GOODS) ? COL_YELLOW : // nothing moved in this and past month
				SYSCOL_TEXT, true);
		}

		{
			int pos_y = pos_y0 + 4 * LINESPACE; // line 5 (and later 6)
			// next stop
			char tmp[256];
			// Bernd Gabriel, 01.07.2009: inconsistent adding of ':'. Sometimes in code, sometimes in translation. Consistently moved to code.
			sprintf(tmp, caption, translator::translate("Fahrtziel")); // "Destination"
			int len = display_proportional(pos_x, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true) + 5;
			info_buf.clear();
			int existing_caracters = len / 4;
			int extra_caracters = (get_windowsize().w - get_min_windowsize().w) / 6;
			const schedule_t *schedule = cnv->get_schedule();
			schedule_gui_t::gimme_short_stop_name(info_buf, cnv->get_owner(), schedule, schedule->get_current_stop(), 50 - existing_caracters + extra_caracters);
			len += display_proportional_clip(pos_x + len, pos_y, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true) + 5;

			pos_y = pos_y0 + 5 * LINESPACE; // line 6
			sint32 cnv_route_index_left = cnv->get_route()->get_count() - 1 - cnv_route_index;
			info_buf.clear();

			double distance;
			char distance_display[13];
			distance = (double)(cnv_route_index_left * welt->get_settings().get_meters_per_tile()) / 1000.0;

			if (distance <= 0)
			{
				sprintf(distance_display, "0km");
			}
			else if (distance < 1)
			{
				sprintf(distance_display, "%.0fm", distance * 1000);
			}
			else
			{
				uint n_actual = distance < 5 ? 1 : 0;
				char tmp[10];
				number_to_string(tmp, distance, n_actual);
				sprintf(distance_display, "%skm", tmp);
			}
			info_buf.printf("%s %s", distance_display, translator::translate("left"));
			int offset = 189;
			display_proportional_clip(pos_x + offset, pos_y, info_buf, ALIGN_RIGHT, SYSCOL_TEXT, true);
		}

		/*
		 * only show assigned line, if there is one!
		 * @author hsiegeln
		 */
		if (cnv->get_line().is_bound()) {
			const int pos_y = pos_y0 + 6 * LINESPACE; // line 7
			const int line_x = pos_x + line_bound * 12;
			char tmp[256];
			// Bernd Gabriel, 01.07.2009: inconsistent adding of ':'. Sometimes in code, sometimes in translation. Consistently moved to code.
			sprintf(tmp, caption, translator::translate("Serves Line"));
			int len = display_proportional(line_x + 1, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true) + 5;

			int existing_caracters = len / 4;
			int extra_caracters = (get_windowsize().w - get_min_windowsize().w) / 6;
			char convoy_line[256] = "\0";
			sprintf(convoy_line, "%s", cnv->get_line()->get_name());
			tmp[0] = '\0';
			if (convoy_line[48 - existing_caracters + extra_caracters] != '\0')
			{
				convoy_line[45 - existing_caracters + extra_caracters] = '\0';
				sprintf(tmp, "...");
			}
			info_buf.clear();
			info_buf.append(convoy_line);
			info_buf.append(tmp);
			display_proportional_clip(line_x + len, pos_y, info_buf, ALIGN_LEFT, cnv->get_line()->get_state_color(), true);
		}
		//{
		//	int pos_y = pos_y0 + 7 * LINESPACE; // line 8 and 9
		//										// Status text
		//	char tmp[256] = "\0";
		//	COLOR_VAL status_color = SYSCOL_TEXT;
		//	int message_lines = 0;
		//	/*if (message_lines < 2 && cnv->get_jahresgewinn() < 0)	// Not sure if we really need to know this...
		//	{
		//		sprintf(tmp, (translator::translate("convoy_made_a_loss_last_month")));
		//		status_color = MONEY_MINUS;
		//		display_proportional_clip(pos_x, pos_y, tmp, ALIGN_LEFT, status_color, true);
		//		pos_y += LINESPACE;
		//		message_lines++;
		//	}*/
		//	if (message_lines < 2 && cnv->get_overcrowded() > 0)
		//	{
		//		sprintf(tmp, (translator::translate("frequently_overcrowded")));
		//		status_color = COL_DARK_PURPLE;
		//		display_proportional_clip(pos_x, pos_y, tmp, ALIGN_LEFT, status_color, true);
		//		pos_y += LINESPACE;
		//		message_lines++;
		//	}

		//	// Can upgrade while obsolete?
		//	const uint16 month_now = welt->get_timeline_year_month();
		//	int amount_of_upgradeable_vehicles = 0;
		//	bool has_obsolete_that_can_upgrade = false;

		//	for (uint16 i = 0; i < cnv->get_vehicle_count(); i++)
		//	{
		//		vehicle_t *v = cnv->get_vehicle(i);
		//		if (v->get_desc()->get_upgrades_count() > 0)
		//		{
		//			for (int k = 0; k < v->get_desc()->get_upgrades_count(); k++)
		//			{
		//				if (v->get_desc()->get_upgrades(k) && !v->get_desc()->get_upgrades(k)->is_future(month_now) && (!v->get_desc()->get_upgrades(k)->is_retired(month_now)))
		//				{
		//					has_obsolete_that_can_upgrade = true;
		//				}
		//			}
		//		}
		//	}
		//	if (message_lines < 2 && has_obsolete_that_can_upgrade)
		//	{
		//		sprintf(tmp, (translator::translate("obsolete_vehicles_with_upgrades")));
		//		status_color = COL_UPGRADEABLE;
		//		display_proportional_clip(pos_x, pos_y, tmp, ALIGN_LEFT, status_color, true);
		//		pos_y += LINESPACE;
		//		message_lines++;
		//	}
		//	if (message_lines < 2 && cnv->has_obsolete_vehicles() && !has_obsolete_that_can_upgrade) {
		//		sprintf(tmp, (translator::translate("obsolete_vehicles")));
		//		status_color = COL_OBSOLETE;
		//		display_proportional_clip(pos_x, pos_y, tmp, ALIGN_LEFT, status_color, true);
		//		pos_y += LINESPACE;
		//		message_lines++;
		//	}
		//	if (message_lines < 2 && (!cnv->get_finance_history(0, convoi_t::CONVOI_TRANSPORTED_GOODS) && !cnv->get_finance_history(1, convoi_t::CONVOI_TRANSPORTED_GOODS)))
		//	{
		//		sprintf(tmp, (translator::translate("nothing_moved_in_this_and_past_month")));
		//		status_color = COL_YELLOW;
		//		display_proportional_clip(pos_x, pos_y, tmp, ALIGN_LEFT, status_color, true);
		//		pos_y += LINESPACE;
		//		message_lines++;
		//	}
		//}

#ifdef DEBUG_CONVOY_STATES
		{
			// Debug: show covnoy states
			int debug_row = 9;
			{
				const int pos_y = pos_y0 + debug_row * LINESPACE;

				char state_text[32];
				switch (cnv->get_state())
				{
				case convoi_t::INITIAL:			sprintf(state_text, "INITIAL");	break;
				case convoi_t::EDIT_SCHEDULE:	sprintf(state_text, "EDIT_SCHEDULE");	break;
				case convoi_t::ROUTING_1:		sprintf(state_text, "ROUTING_1");	break;
				case convoi_t::ROUTING_2:		sprintf(state_text, "ROUTING_2");	break;
				case convoi_t::DUMMY5:			sprintf(state_text, "DUMMY5");	break;
				case convoi_t::NO_ROUTE:		sprintf(state_text, "NO_ROUTE");	break;
				case convoi_t::NO_ROUTE_TOO_COMPLEX:		sprintf(state_text, "NO_ROUTE_TOO_COMPLEX");	break;
				case convoi_t::DRIVING:			sprintf(state_text, "DRIVING");	break;
				case convoi_t::LOADING:			sprintf(state_text, "LOADING");	break;
				case convoi_t::WAITING_FOR_CLEARANCE:			sprintf(state_text, "WAITING_FOR_CLEARANCE");	break;
				case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:	sprintf(state_text, "WAITING_FOR_CLEARANCE_ONE_MONTH");	break;
				case convoi_t::CAN_START:			sprintf(state_text, "CAN_START");	break;
				case convoi_t::CAN_START_ONE_MONTH:	sprintf(state_text, "CAN_START_ONE_MONTH");	break;
				case convoi_t::SELF_DESTRUCT:		sprintf(state_text, "SELF_DESTRUCT");	break;
				case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:	sprintf(state_text, "WAITING_FOR_CLEARANCE_TWO_MONTHS");	break;
				case convoi_t::CAN_START_TWO_MONTHS:				sprintf(state_text, "CAN_START_TWO_MONTHS");	break;
				case convoi_t::LEAVING_DEPOT:	sprintf(state_text, "LEAVING_DEPOT");	break;
				case convoi_t::ENTERING_DEPOT:	sprintf(state_text, "ENTERING_DEPOT");	break;
				case convoi_t::REVERSING:		sprintf(state_text, "REVERSING");	break;
				case convoi_t::OUT_OF_RANGE:	sprintf(state_text, "OUT_OF_RANGE");	break;
				case convoi_t::EMERGENCY_STOP:	sprintf(state_text, "EMERGENCY_STOP");	break;
				case convoi_t::ROUTE_JUST_FOUND:	sprintf(state_text, "ROUTE_JUST_FOUND");	break;
				case convoi_t::WAITING_FOR_LOADING_THREE_MONTHS:	sprintf(state_text, "WAITING_FOR_LOADING_THREE_MONTHS");	break;
				case convoi_t::WAITING_FOR_LOADING_FOUR_MONTHS:	sprintf(state_text, "WAITING_FOR_LOADING_FOUR_MONTHS");	break;
				default:	sprintf(state_text, "default");	break;

				}

				display_proportional(pos_x, pos_y, state_text, ALIGN_LEFT, SYSCOL_TEXT, true);
				debug_row++;
			}
			if (air_vehicle && air_vehicle->is_runway_too_short() == true)
			{
				const int pos_y = pos_y0 + debug_row * LINESPACE;
				char runway_too_short[32];
				sprintf(runway_too_short, "air->runway_too_short");
				display_proportional(pos_x, pos_y, runway_too_short, ALIGN_LEFT, SYSCOL_TEXT, true);
				debug_row++;
			}
			if (cnv->front()->get_is_overweight() == true) // This doesnt flag!
			{
				const int pos_y = pos_y0 + debug_row * LINESPACE;
				char too_heavy[32];
				sprintf(too_heavy, "is_overweight");
				display_proportional(pos_x, pos_y, too_heavy, ALIGN_LEFT, SYSCOL_TEXT, true);
				debug_row++;
			}
		}
#endif
#ifdef DEBUG_PHYSICS
		/*
		 * Show braking distance
		 */
		{
			const int pos_y = pos_y0 + 6 * LINESPACE; // line 7
			const sint32 brk_meters = convoy.calc_min_braking_distance(convoy.get_weight_summary(), speed_to_v(cnv->get_akt_speed()));
			char tmp[256];
			sprintf(tmp, translator::translate("minimum brake distance"));
			const int len = display_proportional(pos_x, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true);
			sprintf(tmp, translator::translate(": %im"), brk_meters);
			display_proportional(pos_x + len, pos_y, tmp, ALIGN_LEFT, cnv->get_akt_speed() <= cnv->get_akt_speed_soll() ? SYSCOL_TEXT : SYSCOL_TEXT_STRONG, true);
		}
		{
			const int pos_y = pos_y0 + 7 * LINESPACE; // line 8
			char tmp[256];
			const settings_t &settings = welt->get_settings();
			const sint32 kmh = speed_to_kmh(cnv->next_speed_limit);
			const sint32 m_til_limit = settings.steps_to_meters(cnv->steps_til_limit).to_sint32();
			const sint32 m_til_brake = settings.steps_to_meters(cnv->steps_til_brake).to_sint32();
			if (kmh)
				sprintf(tmp, translator::translate("max %ikm/h in %im, brake in %im "), kmh, m_til_limit, m_til_brake);
			else
				sprintf(tmp, translator::translate("stop in %im, brake in %im "), m_til_limit, m_til_brake);
			const int len = display_proportional(pos_x, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true);
		}
		{
			const int pos_y = pos_y0 + 8 * LINESPACE; // line 9
			const sint32 current_friction = cnv->front()->get_frictionfactor();
			char tmp[256];
			sprintf(tmp, translator::translate("current friction factor"));
			const int len = display_proportional(pos_x, pos_y, tmp, ALIGN_LEFT, SYSCOL_TEXT, true);
			sprintf(tmp, translator::translate(": %i"), current_friction);
			display_proportional(pos_x + len, pos_y, tmp, ALIGN_LEFT, current_friction <= 20 ? SYSCOL_TEXT : SYSCOL_TEXT_STRONG, true);
		}
#endif
		POP_CLIP();
	}
}


bool convoi_info_t::is_weltpos()
{
	return (welt->get_viewport()->get_follow_convoi()==cnv);
}


koord3d convoi_info_t::get_weltpos( bool set )
{
	if(  set  ) {
		if(  !is_weltpos()  )  {
			welt->get_viewport()->set_follow_convoi( cnv );
		}
		else {
			welt->get_viewport()->set_follow_convoi( convoihandle_t() );
		}
		return koord3d::invalid;
	}
	else {
		return cnv->get_pos();
	}
}


// activate the statistic
void convoi_info_t::show_hide_statistics( bool show )
{
	toggler.pressed = show;
	const scr_coord offset = show ? scr_coord(0, chart_total_size) : scr_coord(0, -chart_total_size);
	set_min_windowsize(get_min_windowsize() + offset);
	scrolly.set_pos(scrolly.get_pos() + offset);
	chart.set_visible(show);
	set_windowsize(get_windowsize() + offset + scr_size(0,show?LINESPACE:-LINESPACE));
	resize(scr_coord(0,0));
	for(  int i = 0;  i < convoi_t::MAX_CONVOI_COST;  i++  ) {
		filterButtons[i].set_visible(toggler.pressed);
	}
}

/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool convoi_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	// follow convoi on map?
	if(comp == &follow_button) {
		if(welt->get_viewport()->get_follow_convoi()==cnv) {
			// stop following
			welt->get_viewport()->set_follow_convoi( convoihandle_t() );
		}
		else {
			welt->get_viewport()->set_follow_convoi(cnv);
		}
		return true;
	}

	// details?
	if(comp == &details_button) {
		create_win(20, 20, new convoi_detail_t(cnv), w_info, magic_convoi_detail+cnv.get_id() );
		return true;
	}

	if(  comp == &line_button  ) {
		cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), cnv->get_line() );
		welt->set_dirty();
	}

	if(  comp == &input  ) {
		// rename if necessary
		rename_cnv();
	}

	// sort by what
	if(comp == &freight_sort_selector) {
		// sort by what
		sint32 sort_mode = freight_sort_selector.get_selection();
		if (sort_mode < 0)
		{
			freight_sort_selector.set_selection(0);
			sort_mode = 0;
		}
		env_t::default_sortmode = (sort_mode_t)((int)(sort_mode)%(int)SORT_MODES);
		cnv->set_sortby( env_t::default_sortmode );
	}

	// some actions only allowed, when I am the player
	if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {

		if(comp == &button) {
			cnv->call_convoi_tool( 'f', NULL );
			return true;
		}

		//if(comp == &no_load_button    &&    !route_search_in_progress) {
		if(comp == &no_load_button) {
			cnv->call_convoi_tool( 'n', NULL );
			return true;
		}

		if(comp == &replace_button)
		{
			create_win(20, 20, new replace_frame_t(cnv, get_name()), w_info, magic_replace + cnv.get_id() );
			return true;
		}

		if(comp == &times_history_button)
		{
			create_win(20, 20, new times_history_t(linehandle_t(), cnv), w_info, magic_convoi_time_history + cnv.get_id() );
			return true;
		}

		if(comp == &go_home_button) {
			// limit update to certain states that are considered to be safe for schedule updates
			if(cnv->is_locked())
			{
				DBG_MESSAGE("convoi_info_t::action_triggered()","convoi state %i => cannot change schedule ... ", cnv->get_state() );
				return false;
			}
			go_home_button.pressed = true;
			cnv->call_convoi_tool('P', NULL);
			go_home_button.pressed = false;
			return true;
		} // end go home button

		if(comp == &reverse_button)
		{
			cnv->call_convoi_tool('V', NULL);
			reverse_button.pressed = !reverse_button.pressed;
		}
	}

	if (comp == &toggler) {
		show_hide_statistics( toggler.pressed^1 );
		return true;
	}

	for ( int i = 0; i<BUTTON_COUNT; i++) {
		if (comp == &filterButtons[i]) {
			filterButtons[i].pressed = !filterButtons[i].pressed;
			if(filterButtons[i].pressed) {
				chart.show_curve(i);
			}
			else {
				chart.hide_curve(i);
			}

			return true;
		}
	}

	return false;
}


void convoi_info_t::reset_cnv_name()
{
	// change text input of selected line
	if (cnv.is_bound()) {
		tstrncpy(old_cnv_name, cnv->get_name(), sizeof(old_cnv_name));
		tstrncpy(cnv_name, cnv->get_name(), sizeof(cnv_name));
		input.set_text(cnv_name, sizeof(cnv_name));
	}
}


void convoi_info_t::rename_cnv()
{
	if (cnv.is_bound()) {
		const char *t = input.get_text();
		// only change if old name and current name are the same
		// otherwise some unintended undo if renaming would occur
		if(  t  &&  t[0]  &&  strcmp(t, cnv->get_name())  &&  strcmp(old_cnv_name, cnv->get_name())==0) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "c%u,%s", cnv.get_id(), t );
			tool_t *tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tool->set_default_param( buf );
			welt->set_tool( tool, cnv->get_owner());
			// since init always returns false, it is safe to delete immediately
			delete tool;
			// do not trigger this command again
			tstrncpy(old_cnv_name, t, sizeof(old_cnv_name));
		}
	}
}


/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void convoi_info_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	const scr_coord_val right = get_windowsize().w - D_MARGIN_RIGHT;
	const scr_coord_val width = right - D_MARGIN_LEFT;
	scr_coord_val y = D_MARGIN_TOP;

	input.set_size(scr_size(width, D_BUTTON_HEIGHT));
	y += D_BUTTON_HEIGHT + D_V_SPACE;

	const scr_coord_val y0 = y + LINESPACE * 7;
	line_button.set_pos(scr_coord(D_MARGIN_LEFT, y0 - LINESPACE));

	view.set_pos(scr_coord(right - view.get_size().w, y));
	follow_button.set_pos(scr_coord(view.get_pos().x, y + view.get_size().h + 8));
	y += max(view.get_size().h + 8, LINESPACE * 7 + D_V_SPACE);

	bool too_small_for_5_buttons = view.get_pos().x < 5*(D_BUTTON_WIDTH + D_H_SPACE) - min(view.get_size().w, D_BUTTON_WIDTH);
	if (y0 + D_V_SPACE + D_BUTTON_HEIGHT <= y)
	{
		if (too_small_for_5_buttons)
		{
			replace_button.set_pos(scr_coord(BUTTON3_X, y - D_V_SPACE - D_BUTTON_HEIGHT));
		}
		else {
			replace_button.set_pos(scr_coord(BUTTON4_X, y));
		}
		times_history_button.set_pos(scr_coord(BUTTON1_X, y - D_V_SPACE - D_BUTTON_HEIGHT));
	}
	else {
		if (too_small_for_5_buttons)
		{
			y += D_BUTTON_HEIGHT + D_V_SPACE;
			times_history_button.set_pos(scr_coord(BUTTON1_X, y - D_V_SPACE - D_BUTTON_HEIGHT));
		}
		else {
			times_history_button.set_pos(scr_coord(BUTTON4_X + D_BUTTON_WIDTH + D_H_SPACE, y));
		}
		replace_button.set_pos(scr_coord(BUTTON4_X, y));
	}

	button.set_pos(scr_coord(BUTTON1_X, y));
	go_home_button.set_pos(scr_coord(BUTTON2_X, y));
	no_load_button.set_pos(scr_coord(BUTTON3_X, y));
	y += D_BUTTON_HEIGHT + D_V_SPACE;

	if (chart.is_visible())
	{
		y += LINESPACE;
		chart.set_pos(scr_coord(88, y));
		chart.set_size(scr_size(width-176, 100));
		y += 100 + D_V_SPACE + LINESPACE + D_V_SPACE;
		int cnt = 0;
		const int cols = max(1, (width + D_H_SPACE) / (D_BUTTON_WIDTH + D_H_SPACE));
		for (int btn = 0; btn < BUTTON_COUNT; btn++)
		{
			if((statistic[btn] == convoi_t::CONVOI_REFUNDS) && cnv->get_line().is_bound())
			{
				continue;
			}
			filterButtons[btn].set_pos(scr_coord(BUTTON_X(cnt % cols) + D_MARGIN_LEFT, y + BUTTON_Y(cnt/cols)));
			++cnt;
		}
		const int rows = (cnt - 1) / cols + 1;
		y += BUTTON_Y(rows);
	}

	sort_label.set_pos(scr_coord(BUTTON1_X, y));
	reverse_button.set_pos(scr_coord(BUTTON3_X, y));
	y += LINESPACE + D_V_SPACE;

	// Freight sort combobox
	freight_sort_selector.set_pos(scr_coord(BUTTON1_X, y));
	freight_sort_selector.set_size(scr_size(D_BUTTON_WIDTH * 2, D_BUTTON_HEIGHT));
	toggler.set_pos(scr_coord(BUTTON3_X, y));
	details_button.set_pos(scr_coord(BUTTON4_X, y));
	y += D_BUTTON_HEIGHT + D_V_SPACE;

	scrolly.set_pos(scr_coord(0, y));
	scrolly.set_size(get_client_windowsize()-scrolly.get_pos());

	const int pos_y = view.get_pos().y + 4;
	// convoi speed indicator
	speed_bar.set_pos(scr_coord(BUTTON3_X,pos_y+0*LINESPACE));
	speed_bar.set_size(scr_size(view.get_pos().x - BUTTON3_X - D_INDICATOR_HEIGHT, 4));

	// convoi load indicator
	filled_bar.set_pos(scr_coord(BUTTON3_X,pos_y+3*LINESPACE));
	filled_bar.set_size(scr_size(view.get_pos().x - BUTTON3_X - D_INDICATOR_HEIGHT, 4));

	// convoi load indicator
	route_bar.set_pos(scr_coord(BUTTON3_X,pos_y+5*LINESPACE));
	route_bar.set_size(scr_size(view.get_pos().x - BUTTON3_X - D_INDICATOR_HEIGHT, 4));
}



convoi_info_t::convoi_info_t()
:	gui_frame_t("", NULL),
	scrolly(&text),
	text(&freight_info),
	view(scr_size(64,64)),
	sort_label(translator::translate("loaded passenger/freight"))
{
}



void convoi_info_t::rdwr(loadsave_t *file)
{
	// convoy data
	if (file->get_version() <=112002) {
		// dummy data
		koord3d cnv_pos( koord3d::invalid);
		char name[128];
		name[0] = 0;
		cnv_pos.rdwr( file );
		file->rdwr_str( name, lengthof(name) );
	}
	else {
		// handle
		convoi_t::rdwr_convoihandle_t(file, cnv);
	}

	// window data
	uint32 flags = 0;
	if (file->is_saving()) {
		for(  int i = 0;  i < BUTTON_COUNT;  i++  ) {
			if(  filterButtons[i].pressed  ) {
				flags |= (1<<i);
			}
		}
	}
	scr_size size = get_windowsize();
	bool stats = toggler.pressed;
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();

	size.rdwr( file );
	file->rdwr_long( flags );
	file->rdwr_byte( env_t::default_sortmode );
	file->rdwr_bool(stats);
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );

	if(  file->is_loading()  ) {
		// convoy vanished
		if(  !cnv.is_bound()  ) {
			dbg->error( "convoi_info_t::rdwr()", "Could not restore convoi info window of (%d)", cnv.get_id() );
			destroy_win( this );
			return;
		}

		// now we can open the window ...
		scr_coord const& pos = win_get_pos(this);
		convoi_info_t *w = new convoi_info_t(cnv);
		create_win(pos.x, pos.y, w, w_info, magic_convoi_info + cnv.get_id());
		if(  stats  ) {
			size.h -= 170;
		}
		w->set_windowsize( size );
		int chart_records = convoi_t::MAX_CONVOI_COST;
		if(  file->get_version()<111001  ) {
			chart_records = 6;
		}
		else if(  file->get_version()<112008  ) {
			chart_records = 7;
		}
		else if (file->get_extended_version() < 14 || (file->get_extended_version() == 14 && file->get_extended_revision() < 20)) {
			chart_records = 9;
		}
		for(  int i = 0;  i < chart_records;  i++  ) {
			w->filterButtons[i].pressed = (flags>>i)&1;
			if(w->filterButtons[i].pressed) {
				w->chart.show_curve(i);
			}
		}
		if(  stats  ) {
			w->show_hide_statistics( true );
		}
		else
		{
			cnv->get_freight_info(w->freight_info);
		}
		w->text.recalc_size();
		w->scrolly.set_scroll_position( xoff, yoff );
		// we must invalidate halthandle
		cnv = convoihandle_t();
		destroy_win( this );
	}
}
