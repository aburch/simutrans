/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simwin.h"
#include "minimap.h"

#include "components/gui_convoiinfo.h"

#include "../dataobj/schedule.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/translator.h"

#include "../simhalt.h"
#include "../simline.h"
#include "../simtool.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "halt_list_stats.h"
#include "line_management_gui.h"

#define CHART_HEIGHT (100)

static const char *cost_type[MAX_LINE_COST] =
{
	"Free Capacity",
	"Transported",
	"Revenue",
	"Operation",
	"Profit",
	"Convoys",
	"Distance",
	"Maxspeed",
	"Road toll"
};

const uint8 cost_type_color[MAX_LINE_COST] =
{
	COL_FREE_CAPACITY,
	COL_TRANSPORTED,
	COL_REVENUE,
	COL_OPERATION,
	COL_PROFIT,
	COL_CONVOI_COUNT,
	COL_DISTANCE,
	COL_MAXSPEED,
	COL_TOLL
};

static const bool cost_type_money[MAX_LINE_COST] =
{
	false, false, true, true, true, false, false, false, true
};



line_management_gui_t::line_management_gui_t( linehandle_t line_, player_t* player_ ) :
	gui_frame_t( translator::translate( "Fahrplan" ), player_ ),
	scrolly_convois(gui_scrolled_list_t::windowskin),
	scrolly_halts(gui_scrolled_list_t::windowskin)
{
	set_table_layout(1, 0);
	line = line_;
	player = player_;

	// line name
	inp_name.add_listener(this);
	add_component(&inp_name);

	lb_convoi_count.set_text( "no convois" );
	add_component(&lb_convoi_count);

	// tab panel: connections, chart panels, details
	add_component(&switch_mode);
	switch_mode.add_listener( this );

	switch_mode.add_tab(&container_schedule, translator::translate("Fahrplan"));
	container_schedule.set_table_layout(1, 0);
	container_schedule.add_component(&scd);
#if 0
	container_schedule.add_table(4, 1)->set_force_equal_columns(true);
	{
		bt_delete_line.init( button_t::roundbox, "Delete Line" );
		bt_delete_line.set_tooltip("Delete the selected line (if without associated convois).");
		bt_delete_line.add_listener(this);
		bt_delete_line.disable();
		add_component(&bt_delete_line);

		withdraw_button.init(button_t::roundbox | button_t::flexible, "withdraw");
		withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
		withdraw_button.add_listener(this);
		container_schedule.add_component(&withdraw_button);

		go_home_button.init(button_t::roundbox | button_t::flexible, "go home");
		go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
		go_home_button.add_listener(this);
		container_schedule.add_component(&go_home_button);

		sale_button.init(button_t::roundbox | button_t::flexible, "Verkauf");
		sale_button.set_tooltip("Remove vehicle from map. Use with care!");
		sale_button.add_listener(this);
		container_schedule.add_component(&sale_button);
	}
	container_schedule.end_table();
#endif
	scd.add_listener(this);

	switch_mode.add_tab(&container_stats, translator::translate("Chart"));

	container_stats.set_table_layout(1,0);

	chart.set_dimension(12, 10000);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_min_size(scr_size(0, CHART_HEIGHT));
	container_stats.add_component(&chart);

	container_stats.add_table(4,3)->set_force_equal_columns(true);

	for (int cost = 0; cost<MAX_LINE_COST; cost++) {
		uint16 curve = chart.add_curve( color_idx_to_rgb(cost_type_color[cost]), line->get_finance_history(), MAX_LINE_COST, cost, MAX_MONTHS, cost_type_money[cost], false, true, cost_type_money[cost]*2 );

		button_t *b = container_stats.new_component<button_t>();
		b->init(button_t::box_state_automatic  | button_t::flexible, cost_type[cost]);
		b->background_color = color_idx_to_rgb(cost_type_color[cost]);
		b->pressed = false;

		button_to_chart.append(b, &chart, curve);
	}
	container_stats.end_table();

	switch_mode.add_tab(&container_convois, translator::translate("cl_title"));

	container_convois.set_table_layout(1,0);

	bt_withdraw_line.init( button_t::roundbox_state, "Withdraw All" );
	bt_withdraw_line.set_tooltip("Convoi is sold when all wagons are empty.");
	bt_withdraw_line.add_listener(this);
	container_convois.add_component(&bt_withdraw_line);

	scrolly_convois.set_maximize( true );
	container_convois.add_component(&scrolly_convois);
	
	switch_mode.add_tab(&container_halts, translator::translate("hl_title"));
	container_halts.set_table_layout(1,0);

	scrolly_halts.set_maximize( true );
	container_halts.add_component(&scrolly_halts);
	
	if (line.is_bound() ) {
		scd.init( line->get_schedule(), player, convoihandle_t(), linehandle_t() );
		// we use local buffer to prevent sudden death on line deletion
		tstrncpy(old_line_name, line->get_name(), sizeof(old_line_name));
		tstrncpy(line_name, line->get_name(), sizeof(line_name));
		inp_name.set_text(line_name, sizeof(line_name));

		set_name(line->get_name() );
	}
	old_convoi_count = old_halt_count = 0;

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(get_windowsize());
}


void line_management_gui_t::draw(scr_coord pos, scr_size size)
{
	if(  line.is_bound()  ) {
		bool is_change_allowed = player == welt->get_active_player()  &&  !welt->get_active_player()->is_locked();

		bool has_changed = false; // then we need to recalc sizes ...

		if(  line->count_convoys() != old_convoi_count  ) {

			// update convoi info
			old_convoi_count = line->count_convoys();
			switch( old_convoi_count ) {
				case 0: lb_convoi_count.set_text( "no convois" );
					break;
				case 1: lb_convoi_count.set_text( "1 convoi" );
					break;
				default:
					lb_convoi_count_text.printf( translator::translate( "%d convois" ), old_convoi_count );
					lb_convoi_count.set_text( lb_convoi_count_text );
					break;
			}
			bt_withdraw_line.enable( old_convoi_count != 0 );
			// fill convoi container
			scrolly_convois.clear_elements();
			for( uint32 i = 0; i < line->count_convoys(); i++ ) {
				if( i == 0 ) {
					// just to mark the schedule on the minimap
					minimap_t::get_instance()->set_selected_cnv( line->get_convoy( 0 ) );
				}
				scrolly_convois.new_component<gui_convoiinfo_t>( line->get_convoy( i ) );
			}
			has_changed = true;
		}
		if(  line->get_schedule()->get_count() != old_halt_count  ) {
			// update halt info
			old_halt_count = line->get_schedule()->get_count();
			// fill haltestellen container with info of stops of the line
			scrolly_halts.clear_elements();
			FOR(minivec_tpl<schedule_entry_t>, const& i, line->get_schedule()->entries) {
				halthandle_t const halt = haltestelle_t::get_halt(i.pos, player);
				if(  halt.is_bound()  ) {
					scrolly_halts.new_component<halt_list_stats_t>(halt);
				}
			}
			has_changed = true;
		}

		if( has_changed ) {
			reset_min_windowsize();
		}

		bt_withdraw_line.enable( is_change_allowed );

		if( strcmp( get_name(), line->get_name() ) ) {
			set_name( line->get_name() );
			welt->set_dirty();
		}
	}
	gui_frame_t::draw(pos,size);
}



void line_management_gui_t::rdwr(loadsave_t *file)
{
	sint32 cont_xoff = scrolly_convois.get_scroll_x();
	sint32 cont_yoff = scrolly_convois.get_scroll_y();
	sint32 halt_xoff = scrolly_halts.get_scroll_x();
	sint32 halt_yoff = scrolly_halts.get_scroll_y();

	scr_size size = get_windowsize();
	size.rdwr( file );
	file->rdwr_long( cont_xoff );
	file->rdwr_long( cont_yoff );
	file->rdwr_long( halt_xoff );
	file->rdwr_long( halt_yoff );

	simline_t::rdwr_linehandle_t(file, line);
	scd.rdwr( file );

	if(  file->is_loading()  ) {
		if(  line.is_bound()  ) {
			set_windowsize(size);
			win_set_magic(this, (ptrdiff_t)line.get_rep());
			set_windowsize( size );
			scrolly_convois.set_scroll_position( cont_xoff, cont_yoff );
			scrolly_halts.set_scroll_position( halt_xoff, halt_yoff );

			tstrncpy(old_line_name, line->get_name(), sizeof(old_line_name));
			tstrncpy(line_name, line->get_name(), sizeof(line_name));
			inp_name.set_text(line_name, sizeof(line_name));

			set_name(line->get_name() );
		}
		else {
			line = linehandle_t();
			destroy_win( this );
			dbg->error( "line_management_gui_t::rdwr", "Could not restore schedule window for line id %i", line.get_id() );
		}
	}
}


bool line_management_gui_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if( comp == &scd ) {
		if( v.p ) {
			// update line schedule via tool!
			tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "g,%i,", line.get_id() );
			scd.get_schedule()->sprintf_schedule( buf );
			tool->set_default_param(buf);
			world()->set_tool( tool, line->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tool;
			return true;
		}
		else {
			// revert
			scd.init( line->get_schedule(), player, convoihandle_t(), line );
		}
		return true;
	}
	else if(  comp == &inp_name  ) {
		rename_line();
	}
	return false;
}


void line_management_gui_t::rename_line()
{
	if(  line.is_bound()  ) {
		const char *t = inp_name.get_text();
		// only change if old name and current name are the same
		// otherwise some unintended undo if renaming would occur
		if(  t  &&  t[0]  &&  strcmp(t, line->get_name())  &&  strcmp(old_line_name, line->get_name())==0  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "l%u,%s", line.get_id(), t );
			tool_t *tmp_tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tmp_tool->set_default_param( buf );
			welt->set_tool( tmp_tool, line->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
			// do not trigger this command again
			tstrncpy(old_line_name, t, sizeof(old_line_name));
		}
	}
}


