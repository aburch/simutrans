/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "convoi_info_t.h"
#include "minimap.h"

#include "../vehicle/rail_vehicle.h"
#include "../simcolor.h"
#include "../display/viewport.h"
#include "../simworld.h"
#include "../simmenu.h"
#include "simwin.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/loadsave.h"
#include "../simconvoi.h"
#include "../simline.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "depot_frame.h"
#include "line_item.h"
#include "convoi_detail_t.h"
#include "line_management_gui.h"

#define CHART_HEIGHT (100)

static const char cost_type[convoi_t::MAX_CONVOI_COST][64] =
{
	"Free Capacity", "Transported", "Revenue", "Operation", "Profit", "Distance", "Maxspeed", "Way toll"
};

static const uint8 cost_type_color[convoi_t::MAX_CONVOI_COST] =
{
	COL_FREE_CAPACITY,
	COL_TRANSPORTED,
	COL_REVENUE,
	COL_OPERATION,
	COL_PROFIT,
	COL_DISTANCE,
	COL_MAXSPEED,
	COL_TOLL
};

static const bool cost_type_money[convoi_t::MAX_CONVOI_COST] =
{
	false, false, true, true, true, false, false, true
};



bool convoi_info_t::route_search_in_progress=false;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = destination
 *                 1 = via
 *                 2 = via_amount
 *                 3 = amount
 */
const char *convoi_info_t::sort_text[SORT_MODES] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge"
};



convoi_info_t::convoi_info_t(convoihandle_t cnv, bool edit_schedule) :
	gui_frame_t(""),
	text(&freight_info),
	view(scr_size(max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	scroll_freight(&container_freight, true, true)
{
	if( cnv.is_bound() ) {
		init( cnv );
		if( edit_schedule ) {
			change_schedule();
		}
	}
}

void convoi_info_t::init(convoihandle_t cnv)
{
	this->cnv = cnv;
	this->mean_convoi_speed = speed_to_kmh(cnv->get_akt_speed()*4);
	this->max_convoi_speed = speed_to_kmh(cnv->get_min_top_speed()*4);
	gui_frame_t::set_name(cnv->get_name());
	gui_frame_t::set_owner(cnv->get_owner());

	minimap_t::get_instance()->set_selected_cnv(cnv);
	set_table_layout(1,0);

	input.add_listener(this);
	reset_cnv_name();
	add_component(&input);

	// top part: speedbars, view, buttons
	add_table(2,0)->set_alignment(ALIGN_TOP);
	{
		container_top = add_table(1,0);
		{
			add_table(2,1);
			add_component(&speed_label);
			add_component(&speed_bar);
			end_table();
			add_table(3,1);
			new_component<gui_label_t>("Gewinn");
			profit_label.set_align(gui_label_t::left);
			add_component(&profit_label);
			add_component(&running_cost_label);
			end_table();
			add_table(2,1);
			add_component(&weight_label);
			add_component(&filled_bar);
			end_table();
			add_table(2,1);
			add_component(&target_label);
			add_component(&route_bar);
			end_table();
			add_table(2,1);
			line_button2.init( button_t::arrowright, NULL );
			line_button2.add_listener( this );
			add_component( &line_button2 );
			add_component( &line_label );
			end_table();
		}
		end_table();

		add_component(&view);
		view.set_obj(cnv->front());
	}
	end_table();

	// tab panel: connections, chart panels, details
	add_component(&switch_mode);
	switch_mode.add_listener( this );
	switch_mode.add_tab(&scroll_freight, translator::translate("Freight"));

	container_freight.set_table_layout(1,0);
	container_freight.add_table(2,1);
	{
		container_freight.new_component<gui_label_t>("loaded passenger/freight");

		sort_button.init(button_t::roundbox, sort_text[env_t::default_sortmode]);
		sort_button.set_tooltip("Sort by");
		sort_button.add_listener(this);
		container_freight.add_component(&sort_button);
	}
	container_freight.end_table();
	container_freight.add_component(&text);

	switch_mode.add_tab(&container_schedule, translator::translate("Fahrplan"));
	// tooltip would be set_tooltip("Alters a schedule.");

	container_schedule.set_table_layout(1, 0);

	container_schedule.add_table( 3, 1 );
	{
		container_schedule.new_component<gui_label_t>("Serves Line:");
		line_selector.clear_elements();
		container_schedule.add_component(&line_selector);
		line_selector.add_listener(this);
		line_button.init( button_t::roundbox, "Update Line" );
		line_button.set_tooltip("Modify the selected line");
		line_button.add_listener(this);
		container_schedule.add_component(&line_button);
	}
	container_schedule.end_table();

	scd.init(cnv->get_schedule(), cnv->get_owner(), cnv, cnv->get_line() );
	init_line_selector();
	container_schedule.add_component(&scd);
	scd.add_listener(this);

	switch_mode.add_tab(&container_stats, translator::translate("Chart"));

	container_stats.set_table_layout(1,0);

	chart.set_dimension(12, 10000);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_min_size(scr_size(0, CHART_HEIGHT));
	container_stats.add_component(&chart);

	container_stats.add_table(4,2)->set_force_equal_columns(true);

	for (int cost = 0; cost<convoi_t::MAX_CONVOI_COST; cost++) {
		uint16 curve = chart.add_curve( color_idx_to_rgb(cost_type_color[cost]), cnv->get_finance_history(), convoi_t::MAX_CONVOI_COST, cost, MAX_MONTHS, cost_type_money[cost], false, true, cost_type_money[cost]*2 );

		button_t *b = container_stats.new_component<button_t>();
		b->init(button_t::box_state_automatic  | button_t::flexible, cost_type[cost]);
		b->background_color = color_idx_to_rgb(cost_type_color[cost]);
		b->pressed = false;

		button_to_chart.append(b, &chart, curve);
	}
	container_stats.end_table();

	cnv->set_sortby( env_t::default_sortmode );

	// convoy details in tab
	switch_mode.add_tab(&container_details, translator::translate("Vehicle details"));

	container_details.set_table_layout(1,0);

	container_details.add_table(4, 1)->set_force_equal_columns(true);
	{
		no_load_button.init(button_t::roundbox | button_t::flexible, "no load");
		no_load_button.set_tooltip("No goods are loaded onto this convoi.");
		no_load_button.add_listener(this);
		container_details.add_component(&no_load_button);

		withdraw_button.init(button_t::roundbox | button_t::flexible, "withdraw");
		withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
		withdraw_button.add_listener(this);
		container_details.add_component(&withdraw_button);

		go_home_button.init(button_t::roundbox | button_t::flexible, "go home");
		go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
		go_home_button.add_listener(this);
		container_details.add_component(&go_home_button);

		sale_button.init(button_t::roundbox | button_t::flexible, "Verkauf");
		sale_button.set_tooltip("Remove vehicle from map. Use with care!");
		sale_button.add_listener(this);
		container_details.add_component(&sale_button);
	}
	container_details.end_table();

	details = container_details.new_component<convoi_detail_t>(cnv);

	// indicator bars
	filled_bar.add_color_value(&cnv->get_loading_limit(), color_idx_to_rgb(COL_YELLOW));
	filled_bar.add_color_value(&cnv->get_loading_level(), color_idx_to_rgb(COL_GREEN));

	speed_bar.set_base(max_convoi_speed);
	speed_bar.set_vertical(false);
	speed_bar.add_color_value(&mean_convoi_speed, color_idx_to_rgb(COL_GREEN));

	// we update this ourself!
	route_bar.init(&cnv_route_index, 0);
	if( cnv->get_vehicle_count()>0  &&  dynamic_cast<rail_vehicle_t *>(cnv->front()) ) {
		// only for trains etc.
		route_bar.set_reservation( &next_reservation_index );
	}
	route_bar.set_height(9);

	update_labels();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(diagonal_resize);
}


// only handle a pending renaming ...
convoi_info_t::~convoi_info_t()
{
	button_to_chart.clear();
	// rename if necessary
	rename_cnv();
}


// apply new schedule
void convoi_info_t::apply_schedule()
{
	if(  (!cnv.is_bound())  ||  (cnv->get_state()!=convoi_t::EDIT_SCHEDULE  &&  cnv->get_state()!=convoi_t::INITIAL)  ) {
		// no change allowed (one can only enter this state when editing was allowed)
		return;
	}

	// do not send changes if the convoi is about to be deleted
	if (cnv->get_state() != convoi_t::SELF_DESTRUCT) {
		if (cnv->in_depot()) {
			const grund_t* const ground = welt->lookup(cnv->get_home_depot());
			if (ground) {
				const depot_t* const depot = ground->get_depot();
				if (depot) {
					depot_frame_t* const frame = dynamic_cast<depot_frame_t*>(win_get_magic((ptrdiff_t)depot));
					if (frame) {
						frame->update_data();
					}
				}
			}
		}
		// update new line instead
		if (line != cnv->get_line()) {
			char id[16];
			sprintf(id, "%i,%i", line.get_id(), cnv->get_schedule()->get_current_stop());
			cnv->call_convoi_tool('l', id);
		}
		// since waiting times might be different from line
		cbuffer_t buf;
		scd.get_schedule()->sprintf_schedule(buf);
		cnv->call_convoi_tool('g', buf);
	}
}


void convoi_info_t::init_line_selector()
{
	if (cnv.is_bound()) {
		line_selector.clear_elements();
		int selection = 0;
		vector_tpl<linehandle_t> lines;

		cnv->get_owner()->simlinemgmt.get_lines(cnv->get_schedule()->get_type(), &lines);

		bool new_bound = false;
		FOR(vector_tpl<linehandle_t>, other_line, lines) {
			if( scd.get_schedule()->matches( world(), other_line->get_schedule() ) ) {
				if(  line != other_line  ) {
					line = other_line;
					reset_min_windowsize();
					scd.init( scd.get_schedule(), cnv->get_owner(), cnv, line );
				}
				new_bound = true;
				break;
			}
		}
		if(  !new_bound  &&  line.is_bound()  ) {
			// remove linehandle from schedule
			line = linehandle_t();
			scd.init( scd.get_schedule(), cnv->get_owner(), cnv, line );
		}

		int offset = 2;
		if (!line.is_bound()) {
			selection = 0;
			offset = 3;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("<individual schedule>"), SYSCOL_TEXT);
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("<promote to line>"), SYSCOL_TEXT);
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>("--------------------------------", SYSCOL_TEXT);
		}

		FOR(vector_tpl<linehandle_t>, other_line, lines) {
			line_selector.new_component<line_scrollitem_t>(other_line);
			if (line == other_line) {
				selection = line_selector.count_elements() - 1;
			}
		}

		line_selector.set_selection(selection);
		line_scrollitem_t::sort_mode = line_scrollitem_t::SORT_BY_NAME;
		line_selector.sort(offset);
		old_line_count = cnv->get_owner()->simlinemgmt.get_line_count();
	}
}


void convoi_info_t::update_labels()
{
	switch (cnv->get_state())
	{
		case convoi_t::DRIVING:
			route_bar.set_state(0);
			break;
		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
		case convoi_t::CAN_START_ONE_MONTH:
		case convoi_t::CAN_START_TWO_MONTHS:
			route_bar.set_state(2);
			break;
		case convoi_t::NO_ROUTE:
			route_bar.set_state(3);
			break;
		default:
			route_bar.set_state(1);
			break;
	}
	// use median speed to avoid flickering
	mean_convoi_speed += speed_to_kmh(cnv->get_akt_speed()*4);
	mean_convoi_speed /= 2;
	speed_label.buf().printf( translator::translate("%i km/h (max. %ikm/h)"), (mean_convoi_speed+3)/4, speed_to_kmh(cnv->get_min_top_speed()) );
	speed_label.update();

	profit_label.append_money(cnv->get_jahresgewinn()/100.0);
	profit_label.update();

	running_cost_label.buf().append("(");
	running_cost_label.append_money(cnv->get_running_cost()/100.0);
	running_cost_label.buf().append("/km)");
	running_cost_label.update();

	weight_label.buf().append( translator::translate("Gewicht") );
	weight_label.buf().append( ": " );
	weight_label.buf().append( cnv->get_sum_gesamtweight()/1000.0, 1 );
	weight_label.buf().append( "t (" );
	weight_label.buf().append( (cnv->get_sum_gesamtweight()-cnv->get_sum_weight())/1000.0, 1 );
	weight_label.buf().append( "t)" );
	weight_label.update();

	// next stop
	target_label.buf().append(translator::translate("Fahrtziel"));
	schedule_t::gimme_stop_name(target_label.buf(), welt, cnv->get_owner(), cnv->get_schedule()->get_current_entry(), 34);
	target_label.update();

	// only show assigned line, if there is one!
	if(  cnv->get_line().is_bound()  ) {
		line_label.buf().append(cnv->get_line()->get_name());
		line_label.set_color(cnv->get_line()->get_state_color());
	}
	else {
		line_label.buf().clear();
	}
	line_button2.set_visible( cnv->get_line().is_bound() );
	line_label.update();

	// buffer update now only when needed by convoi itself => dedicated buffer for this
	const int old_len=freight_info.len();
	cnv->get_freight_info(freight_info);
	if(  old_len!=freight_info.len()  ) {
		text.recalc_size();
		scroll_freight.set_size( scroll_freight.get_size() );
	}

	// realign container - necessary if strings changed length
	container_top->set_size( container_top->get_size() );
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 */
void convoi_info_t::draw(scr_coord pos, scr_size size)
{
	if(!cnv.is_bound()) {
		destroy_win(this);
		return;
	}
	next_reservation_index = cnv->get_next_reservation_index();

	bool is_change_allowed = cnv->get_owner() == welt->get_active_player()  &&  !welt->get_active_player()->is_locked();

	if(  line_selector.get_selection()>1  &&  !line.is_bound()  ) {
		init_line_selector();
	}
	if(  !scd.get_schedule()->empty()  &&  line_selector.get_selection()==1  &&  !line.is_bound()  ) {
		// catch new schedule if promoting to line
		init_line_selector();
		if( !line.is_bound() ) {
			line_selector.set_selection(1);
		}
		reset_min_windowsize();
	}
	else if(  old_schedule_count != scd.get_schedule()->get_count()  ) {
		// entry added or removed
		init_line_selector();
		reset_min_windowsize();
	}
	else if(  old_line_count != cnv->get_owner()->simlinemgmt.get_line_count()  ) {
		// line added or removed
		init_line_selector();
		reset_min_windowsize();
	}
	old_schedule_count = scd.get_schedule()->get_count();

	line_button.enable( dynamic_cast<line_scrollitem_t*>(line_selector.get_selected_item()) );
	line_button2.enable( line.is_bound() );

	go_home_button.enable(!route_search_in_progress && is_change_allowed);
	if(  grund_t* gr=welt->lookup(cnv->get_schedule()->get_current_entry().pos)  ) {
		go_home_button.pressed = gr->get_depot() != NULL;
	}

	no_load_button.pressed = cnv->get_no_load();
	no_load_button.enable(is_change_allowed);
	withdraw_button.enable(is_change_allowed);
	sale_button.enable(is_change_allowed);
	line_selector.enable( is_change_allowed );

	withdraw_button.pressed = cnv->get_withdraw();

	update_labels();

	route_bar.set_base(cnv->get_route()->get_count()-1);
	cnv_route_index = cnv->front()->get_route_index() - 1;

	// all gui stuff set => display it
	gui_frame_t::draw(pos, size);
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
		return cnv.is_bound() ?  cnv->get_pos() : koord3d::invalid;
	}
}


/**
 * This method is called if an action is triggered
 */
bool convoi_info_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	minimap_t::get_instance()->set_selected_cnv(cnv);
	if(  comp == &line_button  ) {
		// open selected line as schedule
		if( line_scrollitem_t* li = dynamic_cast<line_scrollitem_t*>(line_selector.get_selected_item()) ) {
			if(  li->get_line().is_bound()  ) {
				cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), li->get_line(), 0 );
			}
		}
	}
	else if(  comp == &line_button2  ) {
		// open selected line as schedule
		if( cnv->get_line().is_bound() ) {
			cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), cnv->get_line(), -1 );
		}
	}
	else if(  comp == &input  ) {
		// rename if necessary
		rename_cnv();
	}
	// sort by what
	else if(  comp == &sort_button  ) {
		// sort by what
		env_t::default_sortmode = (sort_mode_t)((int)(cnv->get_sortby()+1)%(int)SORT_MODES);
		sort_button.set_text(sort_text[env_t::default_sortmode]);
		cnv->set_sortby( env_t::default_sortmode );
	}

	bool edit_allowed = (cnv.is_bound() && (cnv->get_owner() == welt->get_active_player() || welt->get_active_player()->is_public_service()));

	if (comp == &switch_mode) {
		if (v.i == 1) {
			if(edit_allowed  &&  !cnv->in_depot()) {
				// if not in depot:
				// set state to EDIT_SCHEDULE, calls cnv->schedule->start_editing(), reset in gui_schedule_t::~gui_schedule_t
				cnv->call_convoi_tool('s', "1");
				scd.init(cnv->get_schedule(), cnv->get_owner(), cnv, cnv->get_line());
				reset_min_windowsize();
			}
		}
		else if(cnv->get_state()==convoi_t::EDIT_SCHEDULE  ||  cnv->get_state()==convoi_t::INITIAL) {
			apply_schedule();
		}
		scd.highlight_schedule(v.i == 1);
	}

	// some actions only allowed, when I am the player
	if(edit_allowed) {

		if(  comp == &no_load_button    &&    !route_search_in_progress  ) {
			cnv->call_convoi_tool( 'n', NULL );
			return true;
		}

		if(  comp == &go_home_button  &&  !route_search_in_progress  ) {
			// limit update to certain states that are considered to be safe for schedule updates
			int state = cnv->get_state();
			if(state==convoi_t::EDIT_SCHEDULE  ||  cnv->get_state()==convoi_t::INITIAL) {
				return true;
			}

			grund_t* gr = welt->lookup(cnv->get_schedule()->get_current_entry().pos);
			const bool enable_gohome = gr && gr->get_depot() == NULL;

			if(  enable_gohome  ) {
				// go to depot
				route_search_in_progress = true;
				cnv->call_convoi_tool( 'd', NULL );
			}
			else {
				// back to normal schedule
				schedule_t* schedule = cnv->get_schedule()->copy();
				schedule->remove(); // remove depot entry

				cbuffer_t buf;
				schedule->sprintf_schedule( buf );
				cnv->call_convoi_tool( 'g', buf );
				delete schedule;
			}
		} // end go home button

		if (comp == &sale_button) {
			cnv->call_convoi_tool('x', NULL);
			return true;
		}
		else if (comp == &withdraw_button) {
			cnv->call_convoi_tool('w', NULL);
			return true;
		}
		else if (comp == &scd) {
			if( v.p == NULL ) {
				scd.init( cnv->get_schedule(), cnv->get_owner(), cnv, cnv->get_line() );
				// revert schedule
				init_line_selector();
				reset_min_windowsize();
			}
		}
		else if (comp == &line_selector) {
			uint32 selection = v.i;
			if(  line_scrollitem_t* li = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection))  ) {
				line = li->get_line();
				scd.init(line->get_schedule(), cnv->get_owner(), cnv, line);
				reset_min_windowsize();
			}
			else if(  v.i==1  ) {
				// create line schedule via tool!
				tool_t* tool = create_tool(TOOL_CHANGE_LINE | SIMPLE_TOOL);
				cbuffer_t buf;
				buf.printf("c,0,%i,%ld,", (int)scd.get_schedule()->get_type(), (long)(intptr_t)cnv->get_schedule());
				scd.get_schedule()->sprintf_schedule(buf);
				tool->set_default_param(buf);
				welt->set_tool(tool, cnv->get_owner());
				// since init always returns false, it is safe to delete immediately
				delete tool;
			}
			else {
				// remove line
				line = linehandle_t();
				line_selector.set_selection(0);
				schedule_t *temp = scd.get_schedule()->copy();
				scd.init(temp, cnv->get_owner(), cnv, line);
				reset_min_windowsize();
				delete temp;
			}
			return true;
		}
	}

	return false;
}


void convoi_info_t::change_schedule()
{
	if( switch_mode.get_aktives_tab() != &container_schedule ) {
		switch_mode.set_active_tab_index(1);
		scd.highlight_schedule( true );
		value_t v( 1 );
		action_triggered( &switch_mode, 1 );
	}
}


bool convoi_info_t::infowin_event(const event_t *ev)
{
	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
		if(  switch_mode.get_aktives_tab() == &container_schedule  ) {
			// apply schedule here, to reset wait_lock for convoys in convoi_t::set_schedule
			apply_schedule();
		}
		scd.highlight_schedule(false);
		minimap_t::get_instance()->set_selected_cnv(convoihandle_t());
	}

	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_TOP  ) {
		if(  switch_mode.get_aktives_tab() == &container_schedule  &&  !cnv->in_depot()  ) {
			cnv->call_convoi_tool( 's', "1" );
			scd.highlight_schedule( true );
			minimap_t::get_instance()->set_selected_cnv(cnv);
		}
	}

	return gui_frame_t::infowin_event(ev);
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
			tool->set_default_param(buf);
			welt->set_tool(tool, cnv->get_owner());
			// since init always returns false, it is safe to delete immediately
			delete tool;
			// do not trigger this command again
			tstrncpy(old_cnv_name, t, sizeof(old_cnv_name));
		}
	}
}


void convoi_info_t::rdwr(loadsave_t *file)
{
	// handle
	convoi_t::rdwr_convoihandle_t(file, cnv);

	file->rdwr_byte( env_t::default_sortmode );

	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	// init window
	if(  file->is_loading()  &&  cnv.is_bound()) {
		init(cnv);
		win_set_magic(this, magic_convoi_info + cnv.get_id());
	}

	// after initialization
	// components
	scroll_freight.rdwr(file);
	switch_mode.rdwr(file);

	// schedule stuff
	simline_t::rdwr_linehandle_t(file, line);
	scd.rdwr(file);

	// button-to-chart array
	button_to_chart.rdwr(file);

	// convoi details
	details->rdwr(file);

	// convoy vanished
	if(  !cnv.is_bound()  ) {
		dbg->error( "convoi_info_t::rdwr()", "Could not restore convoi info window of (%d)", cnv.get_id() );
		destroy_win( this );
		return;
	}

	if(  file->is_loading()  ) {
		reset_min_windowsize();
		set_windowsize(size);
	}
}

