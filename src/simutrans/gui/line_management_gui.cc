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

#include "../vehicle/vehicle.h"

#include "../convoihandle.h"
#include "../simconvoi.h"
#include "../obj/depot.h"
#include "../simhalt.h"
#include "../simline.h"
#include "../tool/simmenu.h"
#include "../player/simplay.h"

#include "../utils/cbuffer.h"
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
	"Road toll",
	"Profit",
	"Convoys",
	"Distance",
	"Maxspeed"
};

static const uint8 cost_type_color[MAX_LINE_COST] =
{
	COL_FREE_CAPACITY,
	COL_TRANSPORTED,
	COL_REVENUE,
	COL_OPERATION,
	COL_TOLL,
	COL_PROFIT,
	COL_CONVOI_COUNT,
	COL_DISTANCE,
	COL_MAXSPEED
};

static uint8 idx2cost[MAX_LINE_COST] =
{
	LINE_CAPACITY,
	LINE_TRANSPORTED_GOODS,
	LINE_REVENUE,
	LINE_OPERATIONS,
	LINE_WAYTOLL,
	LINE_PROFIT,
	LINE_CONVOIS,
	LINE_DISTANCE,
	LINE_MAXSPEED,
};

static const bool cost_type_money[ MAX_LINE_COST ] =
{
	false,
	false,
	true,
	true,
	true,
	true,
	false,
	false,
	false
};

// which buttons as defualt in statistic
static bool cost_type_default[MAX_LINE_COST] = { 0,0,0,0,0,0,0,0,0 };

// statistics as first default to open
static int default_opening_tab = 1;

line_management_gui_t::line_management_gui_t( linehandle_t line_, player_t* player_, int active_tab) :
	gui_frame_t( translator::translate( "Fahrplan" ), player_ ),
	scrolly_convois( gui_scrolled_list_t::windowskin ),
	scrolly_halts( gui_scrolled_list_t::windowskin ),
	loading_info( &loading_text )
{
	set_table_layout( 3, 0 );
	set_alignment(ALIGN_TOP);
	line = line_;
	player = player_;

	// line name
	inp_name.add_listener( this );
	add_component( &inp_name, 3 );

	lb_convoi_count.set_text( "no convois" );
	add_component( &lb_convoi_count, 3 );

	capacity_bar.add_color_value( &load, color_idx_to_rgb( COL_GREEN ) );
	add_component( &capacity_bar, 2 );
	new_component<gui_fill_t>();

	add_component( &lb_profit_value );
	add_component( &loading_info, 2 );
	loading_text.printf( translator::translate("Capacity: %s\nLoad: %d (%d%%)"), 0, 0, 0);

	// tab panel: connections, chart panels, details
	add_component( &switch_mode, 3 );
	switch_mode.add_listener( this );

	switch_mode.add_tab( &container_schedule, translator::translate( "Fahrplan" ) );
	container_schedule.set_table_layout( 1, 0 );
	container_schedule.add_component( &scd );

	scd.add_listener( this );

	switch_mode.add_tab( &container_stats, translator::translate( "Chart" ) );

	container_stats.set_table_layout( 1, 0 );

	chart.set_dimension( 12, 10000 );
	chart.set_background( SYSCOL_CHART_BACKGROUND );
	chart.set_min_size( scr_size( 0, CHART_HEIGHT ) );
	container_stats.add_component( &chart );

	switch_mode.add_tab( &container_convois, translator::translate( "cl_title" ) );

	container_convois.set_table_layout( 1, 0 );
	container_convois.add_table( 4, 1 )->set_force_equal_columns( true );

	bt_delete_line.init( button_t::roundbox | button_t::flexible, "Delete Line" );
	bt_delete_line.set_tooltip( "Delete the selected line (if without associated convois)." );
	bt_delete_line.add_listener( this );
	bt_delete_line.disable();
	container_convois.add_component( &bt_delete_line );

	bt_withdraw_line.init( button_t::roundbox_state | button_t::flexible, "Withdraw All" );
	bt_withdraw_line.set_tooltip( "Convoi is sold when all wagons are empty." );
	bt_withdraw_line.add_listener( this );
	container_convois.add_component( &bt_withdraw_line );

	bt_find_convois.init( button_t::roundbox | button_t::flexible, "Find matching convois" );
	bt_find_convois.set_tooltip( "Add convois with similar schedule to this line." );
	bt_find_convois.add_listener( this );
	container_convois.add_component( &bt_find_convois );

	container_convois.end_table();

	container_convois.add_component(&scrolly_convois);

	switch_mode.add_tab(&container_halts, translator::translate("hl_title"));
	container_halts.set_table_layout(1,0);

	scrolly_halts.set_maximize( true );
	container_halts.add_component(&scrolly_halts);

	if (active_tab < 0) {
		active_tab = default_opening_tab;
	}
	switch_mode.set_active_tab_index(active_tab);

	if (line.is_bound() ) {
		init();
	}
	old_convoi_count = old_halt_count = 0;

	set_resizemode(diagonal_resize);
	reset_min_windowsize();
	set_windowsize(get_windowsize());
}


void line_management_gui_t::init()
{
	if( line.is_bound() ) {
		// title
		set_name(line->get_name() );
		// schedule
		scd.init( line->get_schedule(), player, convoihandle_t(), linehandle_t() );
		// we use local buffer to prevent sudden death on line deletion
		tstrncpy(old_line_name, line->get_name(), sizeof(old_line_name));
		tstrncpy(line_name, line->get_name(), sizeof(line_name));
		inp_name.set_text(line_name, sizeof(line_name));

		bt_delete_line.enable();
		// init_chart
		if( chart.get_curve_count() == 0 ) {
			container_stats.add_table( 4, 3 )->set_force_equal_columns( true );
			for( int cost = 0; cost < MAX_LINE_COST; cost++ ) {
				uint16 curve = chart.add_curve( color_idx_to_rgb( cost_type_color[ cost ] ), line->get_finance_history(), MAX_LINE_COST, idx2cost[cost], MAX_MONTHS, cost_type_money[ cost ], cost_type_default[cost], true, cost_type_money[ cost ] * 2 );

				button_t *b = container_stats.new_component<button_t>();
				b->init( button_t::box_state_automatic | button_t::flexible, cost_type[ cost ] );
				b->background_color = color_idx_to_rgb( cost_type_color[ cost ] );
				b->pressed = cost_type_default[cost];
				b->add_listener( this );

				button_to_chart.append( b, &chart, curve );
			}
			container_stats.end_table();
			old_convoi_count = -1; // recalc!
		}
		// start editing
		scd.highlight_schedule(switch_mode.get_aktives_tab() == &container_schedule);
		if (line->count_convoys() > 0) {
			minimap_t::get_instance()->set_selected_cnv(line->get_convoy(0));
		}
	}
}


void line_management_gui_t::draw(scr_coord pos, scr_size size)
{
	if(  line.is_bound()  ) {

		player_t *ap = welt->get_active_player();
		bool is_change_allowed = player == ap  &&  !welt->get_active_player()->is_locked();

		bool has_changed = false; // then we need to recalc sizes ...

		if (!is_change_allowed) {
			bt_delete_line.enable(false);
		}
		bt_withdraw_line.enable(is_change_allowed);
		bt_find_convois.enable(is_change_allowed);

		if(  line->count_convoys() != old_convoi_count  ) {

			old_convoi_count = line->count_convoys();

			// update convoi info
			switch( old_convoi_count ) {
				case 0: lb_convoi_count.set_text( "no convois" );
					break;
				case 1: lb_convoi_count.set_text( "1 convoi" );
					break;
				default:
					lb_convoi_count_text.clear();
					lb_convoi_count_text.printf( translator::translate( "%d convois" ), old_convoi_count );
					lb_convoi_count.set_text( lb_convoi_count_text );
					break;
			}

			bt_withdraw_line.enable(is_change_allowed  &&  old_convoi_count != 0 );

				// fill convoi container
			scrolly_convois.clear_elements();
			for( uint32 i = 0; i < line->count_convoys(); i++ ) {
				convoihandle_t cnv = line->get_convoy( i );
				scrolly_convois.new_component<gui_convoiinfo_t>( cnv );
			}
			has_changed = true;

			if( old_convoi_count>0 ) {
				scrolly_convois.set_maximize( true );
			}
		}

		capacity = 0;
		load = 0;
		for( uint32 i = 0; i < line->count_convoys(); i++ ) {
			convoihandle_t cnv = line->get_convoy( i );
			for (unsigned i = 0; i<cnv->get_vehicle_count(); i++) {
				capacity += cnv->get_vehicle(i)->get_cargo_max();
				load += cnv->get_vehicle( i )->get_total_cargo();
			}
		}
		capacity_bar.set_base( capacity );

		// we check if cap is zero, since theoretically a
		// conv can consist of only 1 vehicle, which has no cap (eg. locomotive)
		// and we do not like to divide by zero, do we?
		sint32 loadfactor = 0;
		if (capacity > 0) {
			loadfactor = (load * 100) / capacity;
		}

		char ctmp[ 20 ];
		number_to_string(ctmp, capacity, 2);
		loading_text.clear();
		loading_text.printf( translator::translate("Capacity: %s\nLoad: %d (%d%%)"), ctmp, load, loadfactor );

		char profit_str[64];
		money_to_string( profit_str, line->get_finance_history( 0, LINE_PROFIT )/100.0, true );
		lb_profit_value_text.clear();
		lb_profit_value_text.printf( "%s %s", translator::translate("Gewinn"), profit_str );
		lb_profit_value.set_text_pointer( lb_profit_value_text );
		lb_profit_value.set_color( line->get_finance_history( 0, LINE_PROFIT ) >= 0 ? MONEY_PLUS : MONEY_MINUS );

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
	else {
		destroy_win( this );
	}
	gui_frame_t::draw(pos,size);
}



void line_management_gui_t::rdwr(loadsave_t *file)
{
	sint32 cont_xoff, cont_yoff;
	sint32 halt_xoff, halt_yoff;
	uint8 player_nr;
	if( file->is_saving() ) {
		cont_xoff = scrolly_convois.get_scroll_x();
		cont_yoff = scrolly_convois.get_scroll_y();
		halt_xoff = scrolly_halts.get_scroll_x();
		halt_yoff = scrolly_halts.get_scroll_y();
		player_nr = player->get_player_nr();
	}

	scr_size size = get_windowsize();
	size.rdwr( file );
	file->rdwr_long( cont_xoff );
	file->rdwr_long( cont_yoff );
	file->rdwr_long( halt_xoff );
	file->rdwr_long( halt_yoff );
	file->rdwr_byte( player_nr );
	file->rdwr_str( old_line_name, lengthof( old_line_name ) );
	file->rdwr_str( line_name, lengthof( line_name ) );

	simline_t::rdwr_linehandle_t(file, line);
	scd.rdwr( file );

	if(  file->is_loading()  ) {
		player = welt->get_player( player_nr );
		gui_frame_t::set_owner(player);
		if(  line.is_bound()  ) {
			set_windowsize(size);
			set_windowsize( size );
			win_set_magic(this, (ptrdiff_t)line.get_rep());

			scrolly_convois.set_scroll_position( cont_xoff, cont_yoff );
			scrolly_halts.set_scroll_position( halt_xoff, halt_yoff );

			init();
		}
		else {
			line = linehandle_t();
			destroy_win( this );
			dbg->error( "line_management_gui_t::rdwr", "Could not restore schedule window for line id %i", line.get_id() );
		}
	}

	switch_mode.rdwr( file );
	button_to_chart.rdwr( file );
}


void line_management_gui_t::apply_schedule()
{
	if(  scd.has_pending_changes()  &&  line.is_bound()  &&  (player == welt->get_active_player()  ||  welt->get_active_player()->is_public_service()) ) {
		// update line schedule via tool!
		tool_t *tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
		cbuffer_t buf;
		buf.printf( "g,%i,", line.get_id() );
		scd.get_schedule()->sprintf_schedule( buf );
		tool->set_default_param( buf );
		world()->set_tool( tool, line->get_owner() );
		// since init always returns false, it is safe to delete immediately
		delete tool;
	}
}


bool line_management_gui_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	if(line->count_convoys()>0) {
		minimap_t::get_instance()->set_selected_cnv(line->get_convoy(0));
	}

	if( comp == &scd ) {
		if( !v.p ) {
			// revert
			scd.init( line->get_schedule(), player, convoihandle_t(), line );
			reset_min_windowsize();
		}
		return true;
	}
	else if( comp == &switch_mode ) {
		bool edit_schedule = switch_mode.get_aktives_tab() == &container_schedule;
		if( edit_schedule ) {
			scd.init( line->get_schedule(), player, convoihandle_t(), line );
			scd.highlight_schedule( true );
			reset_min_windowsize();
		}
		else {
			default_opening_tab = v.i;
			scd.highlight_schedule( false );
			apply_schedule();
		}
	}
	else if(  comp == &inp_name  ) {
		rename_line();
	}
	else if(  comp == &bt_delete_line  ) {
		if(  line.is_bound()  ) {
			tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "d,%i", line.get_id() );
			tmp_tool->set_default_param(buf);
			welt->set_tool( tmp_tool, player );
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
			depot_t::update_all_win();
		}
	}
	else if(  comp == &bt_withdraw_line  ) {
		bt_withdraw_line.pressed ^= 1;
		if (  line.is_bound()  ) {
			tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "w,%i,%i", line.get_id(), bt_withdraw_line.pressed );
			tmp_tool->set_default_param(buf);
			welt->set_tool( tmp_tool, player );
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
		}
	}
	else if(  comp == &bt_find_convois  ) {
		FOR(vector_tpl<convoihandle_t>, cnv, welt->convoys()) {
			if(  cnv->get_owner()==player  ) {
				if(  !cnv->get_line().is_bound()  ) {
					if(  line->get_schedule()->matches( welt, cnv->get_schedule() )  ) {
						// same schedule, and no line =< add to line
						char id[16];
						sprintf(id, "%i,%i", line.get_id(), cnv->get_schedule()->get_current_stop());
						cnv->call_convoi_tool('l', id);
					}
				}
			}
		}
	}
	else {
		const vector_tpl<gui_button_to_chart_t*> &l = button_to_chart.list();
		for( uint32 i = 0; i<l.get_count(); i++ ) {
			if( comp == l[i]->get_button() ) {
				cost_type_default[i] = l[i]->get_button()->pressed;
				break;
			}
		}
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


bool line_management_gui_t::infowin_event( const event_t *ev )
{
	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_CLOSE  ) {
		if(  switch_mode.get_aktives_tab() == &container_schedule  ) {
			apply_schedule();
		}
		scd.highlight_schedule( false );
		minimap_t::get_instance()->set_selected_cnv(convoihandle_t());
	}

	if(  ev->ev_class == INFOWIN  &&  ev->ev_code == WIN_TOP  ) {
		if(  switch_mode.get_aktives_tab() == &container_schedule  ) {
			scd.highlight_schedule( true );
		}
		if (line->count_convoys() > 0) {
			minimap_t::get_instance()->set_selected_cnv(line->get_convoy(0));
		}
	}

	return gui_frame_t::infowin_event( ev );
}
