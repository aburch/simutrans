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

#include "../simunits.h"
#include "../simdepot.h"
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
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
#include "messagebox.h"

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
	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max( D_BUTTON_WIDTH, view.get_size().w ) + D_MARGIN_RIGHT;

	input.set_pos( scr_coord(D_MARGIN_LEFT,D_MARGIN_TOP) );
	reset_cnv_name();
	input.add_listener(this);
	add_component(&input);

	add_component(&view);

	// this convoi doesn't belong to an AI
	button.init(button_t::roundbox, "Fahrplan", scr_coord(BUTTON1_X,offset_below_viewport));
	button.set_tooltip("Alters a schedule.");
	button.add_listener(this);
	add_component(&button);

	go_home_button.init(button_t::roundbox, "go home", scr_coord(BUTTON2_X,offset_below_viewport));
	go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
	go_home_button.add_listener(this);
	add_component(&go_home_button);

	no_load_button.init(button_t::roundbox, "no load", scr_coord(BUTTON3_X,offset_below_viewport));
	no_load_button.set_tooltip("No goods are loaded onto this convoi.");
	no_load_button.add_listener(this);
	add_component(&no_load_button);

	//Position is set in convoi_info_t::set_windowsize()
	follow_button.init(button_t::roundbox_state, "follow me", scr_coord(view.get_pos().x, offset_below_viewport), scr_size(view.get_size().w, D_BUTTON_HEIGHT));
	follow_button.set_tooltip("Follow the convoi on the map.");
	follow_button.add_listener(this);
	add_component(&follow_button);

	chart.set_pos(scr_coord(D_MARGIN_LEFT,offset_below_viewport+D_BUTTON_HEIGHT+11));
	chart.set_size(scr_size(total_width-D_MARGIN_LEFT-D_MARGIN_RIGHT, CHART_HEIGHT));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	const sint16 offset_below_chart = offset_below_viewport+D_BUTTON_HEIGHT+11 // chart position
	                                  +CHART_HEIGHT                            // chart size
	                                  +D_V_SPACE;                  // chart x-axis labels plus space

	for (int cost = 0; cost<convoi_t::MAX_CONVOI_COST; cost++) {
		chart.add_curve( color_idx_to_rgb(cost_type_color[cost]), cnv->get_finance_history(), convoi_t::MAX_CONVOI_COST, cost, MAX_MONTHS, cost_type_money[cost], false, true, cost_type_money[cost]*2 );
		filterButtons[cost].init(button_t::box_state, cost_type[cost],
						 scr_coord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(cost%4), offset_below_chart + (D_BUTTON_HEIGHT+D_V_SPACE)*(cost/4)),
			scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background_color = color_idx_to_rgb(cost_type_color[cost]);
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_component(filterButtons + cost);
	}
	add_component(&chart);

	chart_total_size = filterButtons[convoi_t::MAX_CONVOI_COST-1].get_pos().y + D_BUTTON_HEIGHT + D_V_SPACE - (chart.get_pos().y - 11);

	add_component(&sort_label);

	const sint16 yoff = offset_below_viewport + LINESPACE + 2*D_V_SPACE + D_BUTTON_HEIGHT;

	sort_button.init(button_t::roundbox, sort_text[env_t::default_sortmode], scr_coord(BUTTON1_X,yoff));
	sort_button.set_tooltip("Sort by");
	sort_button.add_listener(this);
	add_component(&sort_button);

	toggler.init(button_t::roundbox_state, "Chart", scr_coord(BUTTON3_X,yoff));
	toggler.set_tooltip("Show/hide statistics");
	toggler.add_listener(this);
	add_component(&toggler);

	details_button.init(button_t::roundbox, "Details", scr_coord(BUTTON4_X,yoff));
	details_button.set_tooltip("Vehicle details");
	details_button.add_listener(this);
	add_component(&details_button);

	text.set_pos( scr_coord(0,0) );
	scrolly.set_pos(scr_coord(D_MARGIN_LEFT, yoff + D_BUTTON_HEIGHT + D_V_SPACE));
	scrolly.set_show_scroll_x(true);
	add_component(&scrolly);

	filled_bar.add_color_value(&cnv->get_loading_limit(), color_idx_to_rgb(COL_YELLOW));
	filled_bar.add_color_value(&cnv->get_loading_level(), color_idx_to_rgb(COL_GREEN));
	add_component(&filled_bar);

	speed_bar.set_base(max_convoi_speed);
	speed_bar.set_vertical(false);
	speed_bar.add_color_value(&mean_convoi_speed, color_idx_to_rgb(COL_GREEN));
	add_component(&speed_bar);

	// we update this ourself!
	route_bar.add_color_value(&cnv_route_index, color_idx_to_rgb(COL_GREEN));
	add_component(&route_bar);

	// goto line button
	line_button.init( button_t::posbutton, NULL, scr_coord(D_MARGIN_LEFT, D_MARGIN_TOP + D_BUTTON_HEIGHT + D_V_SPACE + LINESPACE*4 ) );
	line_button.set_targetpos( koord(0,0) );
	line_button.add_listener( this );
	line_bound = false;

	cnv->set_sortby( env_t::default_sortmode );

	set_windowsize(scr_size(total_width, D_DEFAULT_HEIGHT));
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
	if(!cnv.is_bound()) {
		destroy_win(this);
	}
	else {
		// make titlebar dirty to display the correct coordinates
		if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {
			if(  line_bound  &&  !cnv->get_line().is_bound()  ) {
				remove_component( &line_button );
				line_bound = false;
			}
			else if(  !line_bound  &&  cnv->get_line().is_bound()  ) {
				add_component( &line_button );
				line_bound = true;
			}
			button.enable();

			if(  route_search_in_progress  ) {
				go_home_button.disable();
			}
			else {
				go_home_button.enable();
			}

			if(  grund_t* gr=welt->lookup(cnv->get_schedule()->get_current_entry().pos)  ) {
				go_home_button.pressed = gr->get_depot() != NULL;
			}
			details_button.pressed = win_get_magic( magic_convoi_detail+cnv.get_id() );

			no_load_button.pressed = cnv->get_no_load();
			no_load_button.enable();
		}
		else {
			if(  line_bound  ) {
				// do not jump to other player line window
				remove_component( &line_button );
				line_bound = false;
			}
			button.disable();
			go_home_button.disable();
			no_load_button.disable();
		}
		follow_button.pressed = (welt->get_viewport()->get_follow_convoi()==cnv);

		// buffer update now only when needed by convoi itself => dedicated buffer for this
		const int old_len=freight_info.len();
		cnv->get_freight_info(freight_info);
		if(  old_len!=freight_info.len()  ) {
			text.recalc_size();
		}

		route_bar.set_base(cnv->get_route()->get_count()-1);
		cnv_route_index = cnv->front()->get_route_index() - 1;

		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		set_dirty();

		PUSH_CLIP(pos.x+1,pos.y+D_TITLEBAR_HEIGHT,size.w-2,size.h-D_TITLEBAR_HEIGHT);

		// convoi information
		char tmp[256];
		cbuffer_t buf;
		static cbuffer_t info_buf;

		scr_coord_val xpos = pos.x+D_MARGIN_LEFT;
		scr_coord_val ypos = pos.y+D_TITLEBAR_HEIGHT+D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE;

		// use median speed to avoid flickering
		mean_convoi_speed += speed_to_kmh(cnv->get_akt_speed()*4);
		mean_convoi_speed /= 2;
		buf.printf( translator::translate("%i km/h (max. %ikm/h)"), (mean_convoi_speed+3)/4, speed_to_kmh(cnv->get_min_top_speed()) );
		display_proportional_rgb( xpos, ypos, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		ypos += LINESPACE;

		// next important: income stuff
		int len = display_proportional_rgb( xpos, ypos, translator::translate("Gewinn"), ALIGN_LEFT, SYSCOL_TEXT, true ) + 5;
		money_to_string( tmp, cnv->get_jahresgewinn()/100.0 );
		len += display_proportional_rgb( xpos+len, ypos, tmp, ALIGN_LEFT, cnv->get_jahresgewinn()>0?MONEY_PLUS:MONEY_MINUS, true )+5;
		money_to_string( tmp+1, cnv->get_running_cost()/100.0 );
		strcat( tmp, "/km)" );
		tmp[0] = '(';
		display_proportional_rgb( xpos+len, ypos, tmp, ALIGN_LEFT, SYSCOL_TEXT, true );
		ypos += LINESPACE;

		// the weight entry
		info_buf.clear();
		info_buf.append( translator::translate("Gewicht") );
		info_buf.append( ": " );
		info_buf.append( cnv->get_sum_gesamtweight()/1000.0, 1 );
		info_buf.append( "t (" );
		info_buf.append( (cnv->get_sum_gesamtweight()-cnv->get_sum_weight())/1000.0, 1 );
		info_buf.append( "t)" );
		display_proportional_rgb( xpos, ypos, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		ypos += LINESPACE;

		// next stop
		const schedule_t * schedule = cnv->get_schedule();
		info_buf.clear();
		info_buf.append(translator::translate("Fahrtziel"));
		schedule_t::gimme_stop_name(info_buf, welt, cnv->get_owner(), schedule->get_current_entry(), 34);
		len = display_proportional_clip_rgb( xpos, ypos, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true );

		// convoi load indicator
		const scr_coord_val offset = max( len+D_MARGIN_LEFT, 167)+3;
		route_bar.set_pos( scr_coord( offset,ypos-pos.y-D_TITLEBAR_HEIGHT+(LINESPACE-D_INDICATOR_HEIGHT)/2) );
		route_bar.set_size( scr_size(view.get_pos().x-offset-D_H_SPACE, D_INDICATOR_HEIGHT) );
		ypos += LINESPACE;

		/*
		 * only show assigned line, if there is one!
		 * @author hsiegeln
		 */
		if(  cnv->get_line().is_bound()  ) {
			len = display_proportional_rgb( xpos+D_BUTTON_HEIGHT, ypos, translator::translate("Serves Line:"), ALIGN_LEFT, SYSCOL_TEXT, true );
			display_proportional_clip_rgb( xpos+D_BUTTON_HEIGHT+D_H_SPACE+len, ypos, cnv->get_line()->get_name(), ALIGN_LEFT, cnv->get_line()->get_state_color(), true );
		}
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
	if(  comp == &details_button  ) {
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

	if (  comp == &toggler  ) {
		show_hide_statistics( toggler.pressed^1 );
		return true;
	}

	for(  int i = 0;  i < convoi_t::MAX_CONVOI_COST;  i++  ) {
		if(  comp == &filterButtons[i]  ) {
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
			tool->set_default_param(buf);
			welt->set_tool(tool, cnv->get_owner());
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

	input.set_size(scr_size(get_windowsize().w - D_MARGIN_LEFT-D_MARGIN_RIGHT, D_EDIT_HEIGHT));

	view.set_pos( scr_coord(get_windowsize().w - view.get_size().w - D_MARGIN_LEFT, D_MARGIN_TOP+D_BUTTON_HEIGHT+D_V_SPACE ));
	follow_button.set_pos( view.get_pos() + scr_coord( 0, view.get_size().h ) );

	scrolly.set_size( get_client_windowsize()-scrolly.get_pos() );

	const sint16 yoff = scrolly.get_pos().y - D_BUTTON_HEIGHT - D_V_SPACE;
	sort_button.set_pos(scr_coord(BUTTON1_X,yoff));
	toggler.set_pos(scr_coord(BUTTON3_X,yoff));
	details_button.set_pos(scr_coord(BUTTON4_X,yoff));
	sort_label.set_pos(scr_coord(D_MARGIN_LEFT, yoff - LINESPACE - D_V_SPACE));

	// convoi speed indicator
	speed_bar.set_pos( scr_coord(170,view.get_pos().y+0*LINESPACE+(LINESPACE-D_INDICATOR_HEIGHT)/2 ));
	speed_bar.set_size(scr_size(view.get_pos().x - 175, D_INDICATOR_HEIGHT));

	// convoi load indicator
	filled_bar.set_pos(scr_coord(170,view.get_pos().y+2*LINESPACE+(LINESPACE-D_INDICATOR_HEIGHT)/2 ));
	filled_bar.set_size(scr_size(view.get_pos().x - 175, D_INDICATOR_HEIGHT));
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
		for(  int i = 0;  i < convoi_t::MAX_CONVOI_COST;  i++  ) {
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
	file->rdwr_bool( stats );
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
		if(  file->get_version()<111001  ) {
			for(  int i = 0;  i < 6;  i++  ) {
				w->filterButtons[i].pressed = (flags>>i)&1;
				if(w->filterButtons[i].pressed) {
					w->chart.show_curve(i);
				}
			}
		}
		else if(  file->get_version()<112008  ) {
			for(  int i = 0;  i < 7;  i++  ) {
				w->filterButtons[i].pressed = (flags>>i)&1;
				if(w->filterButtons[i].pressed) {
					w->chart.show_curve(i);
				}
			}
		}
		else {
			for(  int i = 0;  i < convoi_t::MAX_CONVOI_COST;  i++  ) {
				w->filterButtons[i].pressed = (flags>>i)&1;
				if(w->filterButtons[i].pressed) {
					w->chart.show_curve(i);
				}
			}
		}
		if(  stats  ) {
			w->show_hide_statistics( true );
		}
		cnv->get_freight_info(w->freight_info);
		w->text.recalc_size();
		w->scrolly.set_scroll_position( xoff, yoff );
		// we must invalidate halthandle
		cnv = convoihandle_t();
		destroy_win( this );
	}
}
