/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Displays an information window for a convoi
 */

#include <stdio.h>

#include "convoi_info_t.h"

#include "../vehicle/simvehicle.h"
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
#include "convoi_detail_t.h"

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
 * @author prissi
 */
const char *convoi_info_t::sort_text[SORT_MODES] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge"
};



convoi_info_t::convoi_info_t(convoihandle_t cnv)
:	gui_frame_t(""),
	text(&freight_info),
	view(scr_size(max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
	scroll_freight(&container_freight, true, true)
{
	if (cnv.is_bound()) {
		init(cnv);
	}
}

void convoi_info_t::init(convoihandle_t cnv)
{
	this->cnv = cnv;
	this->mean_convoi_speed = speed_to_kmh(cnv->get_akt_speed()*4);
	this->max_convoi_speed = speed_to_kmh(cnv->get_min_top_speed()*4);
	gui_frame_t::set_name(cnv->get_name());
	gui_frame_t::set_owner(cnv->get_owner());

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

			add_component(&container_line);
			container_line.set_table_layout(3,1);
			container_line.add_component(&line_button);
			container_line.new_component<gui_label_t>("Serves Line:");
			container_line.add_component(&line_label);
			// goto line button
			line_button.init( button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE + LINESPACE*4 ) );
			line_button.set_targetpos( koord(0,0) );
			line_button.add_listener( this );
			line_bound = false;
		}
		end_table();

		add_table(1,0)->set_alignment(ALIGN_TOP);//  |  ALIGN_CENTER_H);
		{
			add_component(&view);
			view.set_obj(cnv->front());
		}
		end_table();
	}
	end_table();

	add_table(4,1)->set_force_equal_columns(true);
	{
		// this convoi doesn't belong to an AI
		button.init(button_t::roundbox | button_t::flexible, "Fahrplan");
		button.set_tooltip("Alters a schedule.");
		button.add_listener(this);
		add_component(&button);

		go_home_button.init(button_t::roundbox | button_t::flexible, "go home");
		go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
		go_home_button.add_listener(this);
		add_component(&go_home_button);

		no_load_button.init(button_t::roundbox | button_t::flexible, "no load");
		no_load_button.set_tooltip("No goods are loaded onto this convoi.");
		no_load_button.add_listener(this);
		add_component(&no_load_button);

		follow_button.init(button_t::roundbox_state | button_t::flexible, "follow me");
		follow_button.set_tooltip("Follow the convoi on the map.");
		follow_button.add_listener(this);
		add_component(&follow_button);
	}
	end_table();

	// tab panel: connections, chart panels, details
	add_component(&switch_mode);
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
	details = container_details.new_component<convoi_detail_t>(cnv);


	// indicator bars
	filled_bar.add_color_value(&cnv->get_loading_limit(), color_idx_to_rgb(COL_YELLOW));
	filled_bar.add_color_value(&cnv->get_loading_level(), color_idx_to_rgb(COL_GREEN));

	speed_bar.set_base(max_convoi_speed);
	speed_bar.set_vertical(false);
	speed_bar.add_color_value(&mean_convoi_speed, color_idx_to_rgb(COL_GREEN));
	// we update this ourself!
	route_bar.add_color_value(&cnv_route_index, color_idx_to_rgb(COL_GREEN));

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


void convoi_info_t::update_labels()
{
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
 * @author Hj. Malthaner
 */
void convoi_info_t::draw(scr_coord pos, scr_size size)
{
	if(!cnv.is_bound()) {
		destroy_win(this);
	}

	// make titlebar dirty to display the correct coordinates
	if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {

		if (line_bound != cnv->get_line().is_bound()  ) {
			line_bound = cnv->get_line().is_bound();
			container_line.set_visible(line_bound);
		}
		line_button.enable();
		
		if(  cnv->is_coupled()  ||  cnv->get_coupling_convoi().is_bound()  ) {
			button.set_tooltip("Please decouple the other convoy to edit the schedule.");
			button.disable();
		} else {
			button.set_tooltip("Alters a schedule.");
			button.enable();
		}

		if(  cnv->get_coupling_convoi().is_bound()  ) {
			go_home_button.set_tooltip("Uncouple the back convoy now.");
			go_home_button.set_text("release back");
			go_home_button.enable();
		}
		else if(  cnv->is_coupled()  ||  route_search_in_progress  ) {
			go_home_button.disable();
		}
		else {
			go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
			go_home_button.set_text("go home");
			go_home_button.enable();
		}

		if(  grund_t* gr=welt->lookup(cnv->get_schedule()->get_current_entry().pos)  ) {
			go_home_button.pressed = gr->get_depot() != NULL;
		}
		no_load_button.pressed = cnv->get_no_load();
		no_load_button.enable();
	}
	else {
		if(  line_bound  ) {
			// do not jump to other player line window
			line_button.disable();
			remove_component( &line_button );
			line_bound = false;
		}
		button.disable();
		go_home_button.disable();
		no_load_button.disable();
	}

	// update button & labels
	follow_button.pressed = (welt->get_viewport()->get_follow_convoi()==cnv);
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
		return cnv->get_pos();
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

	if(  comp == &line_button  ) {
		cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), cnv->get_line() );
		welt->set_dirty();
	}

	if(  comp == &input  ) {
		// rename if necessary
		rename_cnv();
	}

	// sort by what
	if(  comp == &sort_button  ) {
		// sort by what
		env_t::default_sortmode = (sort_mode_t)((int)(cnv->get_sortby()+1)%(int)SORT_MODES);
		sort_button.set_text(sort_text[env_t::default_sortmode]);
		cnv->set_sortby( env_t::default_sortmode );
	}

	// some actions only allowed, when I am the player
	if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {

		if(  comp == &button  ) {
			cnv->call_convoi_tool( 'f', NULL );
			return true;
		}

		if(  comp == &no_load_button    &&    !route_search_in_progress  ) {
			cnv->call_convoi_tool( 'n', NULL );
			return true;
		}

		if(  comp == &go_home_button  &&  !route_search_in_progress  ) {
			// limit update to certain states that are considered to be safe for schedule updates
			int state = cnv->get_state();
			if(state==convoi_t::EDIT_SCHEDULE) {
				return true;
			}
			
			if(  cnv->get_coupling_convoi().is_bound()  ) {
				cnv->call_convoi_tool('r', NULL);
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
	if(  file->is_loading()  &&  cnv.is_bound())
	{
		init(cnv);

		reset_min_windowsize();
		set_windowsize(size);
	}

	// after initialization
	// components
	scroll_freight.rdwr(file);
	switch_mode.rdwr(file);
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
}
