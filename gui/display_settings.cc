/*
* Copyright (c) 1997 - 2001 Hansjörg Malthaner
*
* This file is part of the Simutrans project under the artistic licence.
* (see licence.txt)
*/

/*
* Menu with display settings
* @author Hj. Malthaner
*/

#include "display_settings.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../display/simimg.h"
#include "../simintr.h"
#include "../simcolor.h"
#include "../boden/wege/strasse.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../obj/baum.h"
#include "../obj/zeiger.h"
#include "../display/simgraph.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"

#include "gui_theme.h"
#include "themeselector.h"
#include "loadfont_frame.h"
#include "simwin.h"

enum {
	IDBTN_SCROLL_INVERSE,
	IDBTN_PEDESTRIANS_AT_STOPS,
	IDBTN_PEDESTRIANS_IN_TOWNS,
	IDBTN_DAY_NIGHT_CHANGE,
	IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN,
	IDBTN_HIDE_TREES,
	IDBTN_TRANSPARENT_STATION_COVERAGE,
	IDBTN_SHOW_STATION_COVERAGE,
	IDBTN_UNDERGROUND_VIEW,
	IDBTN_SHOW_GRID,
	IDBTN_SHOW_STATION_NAMES_ARROW,
	IDBTN_SHOW_WAITING_BARS,
	IDBTN_SHOW_SLICE_MAP_VIEW,
	IDBTN_HIDE_BUILDINGS,
	IDBTN_SHOW_SCHEDULES_STOP,
	IDBTN_SHOW_THEMEMANAGER,
	IDBTN_SIMPLE_DRAWING,
	IDBTN_CHANGE_FONT,
	IDBTN_RIBI_ARROW,
	COLORS_MAX_BUTTONS, 
};

static button_t buttons[COLORS_MAX_BUTTONS];

/**
* class to visualize station names
*/
class gui_label_stationname_t : public gui_label_t
{
	karte_ptr_t welt;
public:
	gui_label_stationname_t(const char* text) : gui_label_t(text) {}

	void draw(scr_coord offset) OVERRIDE
	{
		scr_coord p = pos + offset;

		FLAGGED_PIXVAL pc = welt->get_active_player() ? color_idx_to_rgb(welt->get_active_player()->get_player_color1()+4) : color_idx_to_rgb(COL_ORANGE);
		const char *text = get_text_pointer();

		switch( env_t::show_names >> 2 ) {
		case 0:
			display_ddd_proportional_clip( p.x, p.y + get_size().h/2, proportional_string_width(text)+7, 0, pc, color_idx_to_rgb(COL_BLACK), text, 1 );
			break;
		case 1:
			display_outline_proportional_rgb( p.x, p.y, pc+1, color_idx_to_rgb(COL_BLACK), text, 1 );
			break;
		case 2:
			display_outline_proportional_rgb( p.x + LINESPACE + D_H_SPACE, p.y, color_idx_to_rgb(COL_YELLOW), color_idx_to_rgb(COL_BLACK), text, 1 );
			display_ddd_box_clip_rgb(         p.x,                     p.y,   LINESPACE,   LINESPACE,   pc-2, pc+2 );
			display_fillbox_wh_clip_rgb(      p.x+1,                   p.y+1, LINESPACE-2, LINESPACE-2, pc,   true);
			break;
		case 3: // show nothing
			break;
		}
	}

	scr_size get_min_size() const OVERRIDE
	{
		return gui_label_t::get_min_size() + scr_size(LINESPACE + D_H_SPACE, 4);
	}
};



gui_settings_t::gui_settings_t()
{
	set_table_layout( 1, 0 );
	// Show thememanager
	buttons[ IDBTN_SHOW_THEMEMANAGER ].init( button_t::roundbox_state | button_t::flexible, "Select a theme for display" );
	add_component( buttons + IDBTN_SHOW_THEMEMANAGER );

	// Change font
	buttons[ IDBTN_CHANGE_FONT ].init( button_t::roundbox_state | button_t::flexible, "Select display font" );
	add_component( buttons + IDBTN_CHANGE_FONT );

	// add controls to info container
	add_table(2,3);
	set_alignment(ALIGN_LEFT);
	// Frame time label
	new_component<gui_label_t>("Frame time:");
	frame_time_value_label.buf().printf(" 9999 ms");
	frame_time_value_label.update();
	add_component( &frame_time_value_label );
	// Idle time label
	new_component<gui_label_t>("Idle:");
	idle_time_value_label.buf().printf(" 9999 ms");
	idle_time_value_label.update();
	add_component( &idle_time_value_label );
	// FPS label
	new_component<gui_label_t>("FPS:");
	fps_value_label.buf().printf(" 99.9 fps");
	fps_value_label.update();
	add_component( &fps_value_label );
	// Simloops label
	new_component<gui_label_t>("Sim:");
	simloops_value_label.buf().printf(" 999.9");
	simloops_value_label.update();
	add_component( &simloops_value_label );
	end_table();
}

void gui_settings_t::draw(scr_coord offset)
{
	// Update label buffers
	frame_time_value_label.buf().printf(" %d ms", get_frame_time() );
	frame_time_value_label.update();
	idle_time_value_label.buf().printf(" %d ms", world()->get_idle_time() );
	idle_time_value_label.update();

	// fps_label
	uint32 target_fps = world()->is_fast_forward() ? 10 : env_t::fps;
	uint32 loops = world()->get_realFPS();
	PIXVAL color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops < (target_fps*16*3)/4  ) {
		color = color_idx_to_rgb(( loops <= target_fps*16/2 ) ? COL_RED : COL_YELLOW);
	}
	fps_value_label.set_color(color);
	fps_value_label.buf().printf(" %d fps", loops/16 );
#if MSG_LEVEL >= 3
	if(  env_t::simple_drawing  ) {
		fps_value_label.buf().append( "*" );
	}
#endif
	fps_value_label.update();

	//simloops_label
	loops = world()->get_simloops();
	color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops <= 30  ) {
		color = color_idx_to_rgb((loops<=20) ? COL_RED : COL_YELLOW);
	}
	simloops_value_label.set_color(color);
	simloops_value_label.buf().printf(" %d%c%d", loops/10, get_fraction_sep(), loops%10 );
	simloops_value_label.update();

	// All components are updated, now draw them...
	gui_aligned_container_t::draw(offset);
}


map_settings_t::map_settings_t()
{
	set_table_layout( 1, 0 );
	add_table( 2, 0 );
	// Show grid checkbox
	buttons[ IDBTN_SHOW_GRID ].init( button_t::square_state, "show grid" );
	add_component( buttons + IDBTN_SHOW_GRID, 2 );

	// Underground view checkbox
	buttons[ IDBTN_UNDERGROUND_VIEW ].init( button_t::square_state, "underground mode" );
	add_component( buttons + IDBTN_UNDERGROUND_VIEW, 2 );

	// Show slice map view checkbox
	buttons[ IDBTN_SHOW_SLICE_MAP_VIEW ].init( button_t::square_state, "sliced underground mode" );
	add_component( buttons + IDBTN_SHOW_SLICE_MAP_VIEW );

	// underground slice edit
	inp_underground_level.set_value( grund_t::underground_mode == grund_t::ugm_level ? grund_t::underground_level : world()->get_zeiger()->get_pos().z );
	inp_underground_level.set_limits( world()->get_groundwater() - 10, 32 );
	inp_underground_level.add_listener( this );
	add_component( &inp_underground_level );

	// Day/night change checkbox
	buttons[ IDBTN_DAY_NIGHT_CHANGE ].init( button_t::square_state, "8WORLD_CHOOSE" );
	add_component( buttons + IDBTN_DAY_NIGHT_CHANGE, 2 );

	// Brightness label
	new_component<gui_label_t>( "1LIGHT_CHOOSE" );

	// brightness edit
	brightness.set_value( env_t::daynight_level );
	brightness.set_limits( 0, 9 );
	brightness.add_listener( this );
	add_component( &brightness );

	// Scroll inverse checkbox
	buttons[ IDBTN_SCROLL_INVERSE ].init( button_t::square_state, "4LIGHT_CHOOSE" );
	add_component( buttons + IDBTN_SCROLL_INVERSE, 2 );

	// Scroll speed label
	new_component<gui_label_t>( "3LIGHT_CHOOSE" );

	// Scroll speed edit
	scrollspeed.set_value( abs( env_t::scroll_multi ) );
	scrollspeed.set_limits( 1, 9 );
	scrollspeed.add_listener( this );
	add_component( &scrollspeed );

	// Toggle simple drawing for debugging
#ifdef DEBUG
	buttons[IDBTN_SIMPLE_DRAWING].init(button_t::square_state, "Simple drawing");
	add_component(buttons+IDBTN_SIMPLE_DRAWING, 2);
#endif

	end_table();
}

bool map_settings_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	// Brightness edit
	if( &brightness == comp ) {
		env_t::daynight_level = (sint8)v.i;
	}
	// Scroll speed edit
	if( &scrollspeed == comp ) {
		env_t::scroll_multi = (sint16)(buttons[ IDBTN_SCROLL_INVERSE ].pressed ? -v.i : v.i);
	}
	// underground slice edit
	if( comp == &inp_underground_level ) {
		if( grund_t::underground_mode == grund_t::ugm_level ) {
			grund_t::underground_level = inp_underground_level.get_value();

			// calc new images
			world()->update_underground();
		}
	}
	return true;
}

transparency_settings_t::transparency_settings_t()
{
	set_table_layout( 1, 0 );
	add_table( 2, 0 );

	// Transparent instead of hidden checkbox
	buttons[ IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN ].init( button_t::square_state, "hide transparent" );
	add_component( buttons + IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN, 2 );

	// Hide trees checkbox
	buttons[ IDBTN_HIDE_TREES ].init( button_t::square_state, "hide trees" );
	add_component( buttons + IDBTN_HIDE_TREES, 2 );

	// Hide buildings
	hide_buildings.set_focusable( false );
	hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "no buildings hidden" ), SYSCOL_TEXT );
	hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "hide city building" ), SYSCOL_TEXT );
	hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "hide all building" ), SYSCOL_TEXT );
	hide_buildings.set_selection( env_t::hide_buildings );
	add_component( &hide_buildings, 2 );
	hide_buildings.add_listener( this );

	buttons[ IDBTN_HIDE_BUILDINGS ].set_tooltip( "hide objects under cursor" );
	buttons[ IDBTN_HIDE_BUILDINGS ].init( button_t::square_state, "Smart hide objects" );
	add_component( buttons + IDBTN_HIDE_BUILDINGS );

	// Smart hide objects edit
	cursor_hide_range.set_value( env_t::cursor_hide_range );
	cursor_hide_range.set_limits( 0, 10 );
	cursor_hide_range.add_listener( this );
	add_component( &cursor_hide_range );

	end_table();
}

bool transparency_settings_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	// Smart hide objects edit
	if( &cursor_hide_range == comp ) {
		env_t::cursor_hide_range = cursor_hide_range.get_value();
	}
	// Hide building
	if( &hide_buildings == comp ) {
		env_t::hide_buildings = v.i;
		world()->set_dirty();
	}
	return true;
}


station_settings_t::station_settings_t()
{
	set_table_layout( 1, 0 );
	add_table( 2, 0 );

	// Transparent station coverage
	buttons[ IDBTN_TRANSPARENT_STATION_COVERAGE ].init( button_t::square_state, "transparent station coverage" );
	buttons[ IDBTN_TRANSPARENT_STATION_COVERAGE ].pressed = env_t::use_transparency_station_coverage;
	add_component( buttons + IDBTN_TRANSPARENT_STATION_COVERAGE, 2 );

	// Show station coverage
	buttons[ IDBTN_SHOW_STATION_COVERAGE ].init( button_t::square_state, "show station coverage" );
	add_component( buttons + IDBTN_SHOW_STATION_COVERAGE, 2 );

	// Show station names arrow
	add_table( 2, 1 );
	{
		buttons[ IDBTN_SHOW_STATION_NAMES_ARROW ].set_typ( button_t::arrowright );
		buttons[ IDBTN_SHOW_STATION_NAMES_ARROW ].set_tooltip( "show station names" );
		add_component( buttons + IDBTN_SHOW_STATION_NAMES_ARROW );
		new_component<gui_label_stationname_t>( "show station names" );
	}
	end_table();
	new_component<gui_empty_t>();

	// Show waiting bars checkbox
	buttons[ IDBTN_SHOW_WAITING_BARS ].init( button_t::square_state, "show waiting bars" );
	buttons[ IDBTN_SHOW_WAITING_BARS ].pressed = env_t::show_names & 2;
	add_component( buttons + IDBTN_SHOW_WAITING_BARS, 2 );

	end_table();
}

traffic_settings_t::traffic_settings_t()
{
	set_table_layout( 1, 0 );
	add_table( 2, 0 );

	// Pedestrians in towns checkbox
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].init(button_t::square_state, "6LIGHT_CHOOSE");
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].pressed = world()->get_settings().get_random_pedestrians();
	add_component(buttons+IDBTN_PEDESTRIANS_IN_TOWNS, 2);

	// Pedestrians at stops checkbox
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].init(button_t::square_state, "5LIGHT_CHOOSE");
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].pressed = world()->get_settings().get_show_pax();
	add_component(buttons+IDBTN_PEDESTRIANS_AT_STOPS, 2);

	// Traffic density label
	new_component<gui_label_t>("6WORLD_CHOOSE");

	// Traffic density edit
	traffic_density.set_value(world()->get_settings().get_traffic_level());
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);
	add_component(&traffic_density);

	// Convoy tooltip
	convoy_tooltip.set_focusable(false);
	convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("convoi error tooltips"), SYSCOL_TEXT);
	convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("convoi mouseover tooltips"), SYSCOL_TEXT);
	convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("all convoi tooltips"), SYSCOL_TEXT);
	convoy_tooltip.set_selection(env_t::show_vehicle_states);
	add_component(&convoy_tooltip, 2);
	convoy_tooltip.add_listener(this);

	// Convoy follow mode
	new_component<gui_label_t>("Convoi following mode");

	follow_mode.set_focusable(false);
	follow_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("None"), SYSCOL_TEXT);
	follow_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("underground mode"), SYSCOL_TEXT);
	follow_mode.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("sliced underground mode"), SYSCOL_TEXT);
	follow_mode.set_selection(env_t::follow_convoi_underground);
	add_component(&follow_mode);
	follow_mode.add_listener(this);

	// Show schedule's stop checkbox
	buttons[IDBTN_SHOW_SCHEDULES_STOP].init( button_t::square_state ,  "Highlite schedule" );
	add_component(buttons+IDBTN_SHOW_SCHEDULES_STOP, 2);

	// convoi booking message options
	money_booking.set_focusable( false );
	money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Show all revenue messages"), SYSCOL_TEXT );
	money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Show only player's revenue"), SYSCOL_TEXT );
	money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Show no revenue messages"), SYSCOL_TEXT );
	money_booking.set_selection( env_t::show_money_message );
	add_component(&money_booking, 2);
	money_booking.add_listener(this);
	
	// ribi arrow
	buttons[IDBTN_RIBI_ARROW].init(button_t::square_state, "show connected directions");
	buttons[IDBTN_RIBI_ARROW].pressed = strasse_t::show_masked_ribi;
	add_component(buttons+IDBTN_RIBI_ARROW, 2);

	end_table();
}

bool traffic_settings_t::action_triggered( gui_action_creator_t *comp, value_t v )
{
	// Traffic density edit
	if( &traffic_density == comp ) {
		if( !env_t::networkmode || world()->get_active_player_nr() == 1 ) {
			static char level[ 16 ];
			sprintf( level, "%li", v.i );
			tool_t::simple_tool[ TOOL_TRAFFIC_LEVEL & 0xFFF ]->set_default_param( level );
			world()->set_tool( tool_t::simple_tool[ TOOL_TRAFFIC_LEVEL & 0xFFF ], world()->get_active_player() );
		}
		else {
			traffic_density.set_value( world()->get_settings().get_traffic_level() );
		}
	}
	// Convoy tooltip
	if( &convoy_tooltip == comp ) {
		env_t::show_vehicle_states = v.i;
	}

	if( &follow_mode == comp ) {
		env_t::follow_convoi_underground = v.i;
	}
		
	if( &money_booking == comp ) {
		env_t::show_money_message = v.i;
	}
	return true;
}



color_gui_t::color_gui_t() :
	gui_frame_t( translator::translate( "Helligk. u. Farben" ) ),
	scrolly_gui(&gui_settings),
	scrolly_map(&map_settings),
	scrolly_transparency(&transparency_settings),
	scrolly_station(&station_settings),
	scrolly_traffic(&traffic_settings)
{
	set_table_layout( 1, 0 );

	scrolly_gui.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_map.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_transparency.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_station.set_scroll_amount_y(D_BUTTON_HEIGHT/2);
	scrolly_traffic.set_scroll_amount_y(D_BUTTON_HEIGHT/2);

	tabs.add_tab(&scrolly_gui, translator::translate("GUI settings"));
	tabs.add_tab(&scrolly_map, translator::translate("map view"));
	tabs.add_tab(&scrolly_transparency, translator::translate("transparencies"));
	tabs.add_tab(&scrolly_station, translator::translate("station labels"));
	tabs.add_tab(&scrolly_traffic, translator::translate("traffic settings"));
	add_component(&tabs);

	for( int i = 0; i < COLORS_MAX_BUTTONS; i++ ) {
		buttons[ i ].add_listener( this );
	}

	set_resizemode(diagonal_resize);
	set_min_windowsize( scr_size(D_DEFAULT_WIDTH,get_min_windowsize().h+map_settings.get_size().h) );
	set_windowsize( scr_size(D_DEFAULT_WIDTH,get_min_windowsize().h+map_settings.get_size().h) );
	resize( scr_coord( 0, 0 ) );
}

bool color_gui_t::action_triggered( gui_action_creator_t *comp, value_t)
{
	int i;
	for(  i=0;  i<COLORS_MAX_BUTTONS  &&  comp!=buttons+i;  i++  ) { }

	switch( i )
	{
	case IDBTN_SCROLL_INVERSE:
		env_t::scroll_multi = -env_t::scroll_multi;
		break;
	case IDBTN_PEDESTRIANS_AT_STOPS:
		if( !env_t::networkmode || welt->get_active_player_nr() == 1 ) {
			welt->set_tool( tool_t::simple_tool[ TOOL_TOOGLE_PAX & 0xFFF ], welt->get_active_player() );
		}
		break;
	case IDBTN_PEDESTRIANS_IN_TOWNS:
		if( !env_t::networkmode || welt->get_active_player_nr() == 1 ) {
			welt->set_tool( tool_t::simple_tool[ TOOL_TOOGLE_PEDESTRIANS & 0xFFF ], welt->get_active_player() );
		}
		break;
	case IDBTN_HIDE_TREES:
		env_t::hide_trees = !env_t::hide_trees;
		baum_t::recalc_outline_color();
		break;
	case IDBTN_DAY_NIGHT_CHANGE:
		env_t::night_shift = !env_t::night_shift;
		break;
	case IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN:
		env_t::hide_with_transparency = !env_t::hide_with_transparency;
		baum_t::recalc_outline_color();
		break;
	case IDBTN_TRANSPARENT_STATION_COVERAGE:
		env_t::use_transparency_station_coverage = !env_t::use_transparency_station_coverage;
		break;
	case IDBTN_SHOW_STATION_COVERAGE:
		env_t::station_coverage_show = env_t::station_coverage_show == 0 ? 0xFF : 0;
		break;
	case IDBTN_UNDERGROUND_VIEW:
		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode( buttons[ IDBTN_UNDERGROUND_VIEW ].pressed ? grund_t::ugm_none : grund_t::ugm_all, map_settings.inp_underground_level.get_value() );

		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
		break;
	case IDBTN_SHOW_GRID:
		grund_t::toggle_grid();
		break;
	case IDBTN_SHOW_STATION_NAMES_ARROW:
		if( env_t::show_names & 1 ) {
			if( (env_t::show_names >> 2) == 2 ) {
				env_t::show_names &= 2;
			}
			else {
				env_t::show_names += 4;
			}
		}
		else {
			env_t::show_names &= 2;
			env_t::show_names |= 1;
		}
		break;
	case IDBTN_SHOW_WAITING_BARS:
		env_t::show_names ^= 2;
		break;
	case IDBTN_SHOW_SLICE_MAP_VIEW:
		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode( buttons[ IDBTN_SHOW_SLICE_MAP_VIEW ].pressed ? grund_t::ugm_none : grund_t::ugm_level, map_settings.inp_underground_level.get_value() );

		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
		break;
	case IDBTN_HIDE_BUILDINGS:
		// see simtool.cc::tool_hide_under_cursor_t::init
		env_t::hide_under_cursor = !env_t::hide_under_cursor  &&  env_t::cursor_hide_range > 0;

		// renew toolbar
		tool_t::update_toolbars();
		break;
	case IDBTN_SHOW_SCHEDULES_STOP:
		env_t::visualize_schedule = !env_t::visualize_schedule;
		break;
	case IDBTN_SHOW_THEMEMANAGER:
		create_win( new themeselector_t(), w_info, magic_themes );
		break;
	case IDBTN_SIMPLE_DRAWING:
		env_t::simple_drawing = !env_t::simple_drawing;
		break;
	case IDBTN_CHANGE_FONT:
		create_win( new loadfont_frame_t(), w_info, magic_font );
		break;
	case IDBTN_RIBI_ARROW:
		strasse_t::show_masked_ribi ^= 1;
		break;
	default:
		assert( 0 );
	}

	welt->set_dirty();

	return true;
}


void color_gui_t::draw(scr_coord pos, scr_size size)
{
	// Update button states that was changed with keyboard ...
	buttons[IDBTN_PEDESTRIANS_AT_STOPS].pressed = welt->get_settings().get_show_pax();
	buttons[IDBTN_PEDESTRIANS_IN_TOWNS].pressed = welt->get_settings().get_random_pedestrians();
	buttons[IDBTN_HIDE_TREES].pressed = env_t::hide_trees;
	buttons[IDBTN_HIDE_BUILDINGS].pressed = env_t::hide_under_cursor;
	buttons[IDBTN_SHOW_STATION_COVERAGE].pressed = env_t::station_coverage_show;
	buttons[IDBTN_UNDERGROUND_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[IDBTN_SHOW_GRID].pressed = grund_t::show_grid;
	buttons[IDBTN_SHOW_WAITING_BARS].pressed = (env_t::show_names&2)!=0;
	buttons[IDBTN_SHOW_SLICE_MAP_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[IDBTN_SHOW_SCHEDULES_STOP].pressed = env_t::visualize_schedule;
	buttons[IDBTN_SIMPLE_DRAWING].pressed = env_t::simple_drawing;
	buttons[IDBTN_SIMPLE_DRAWING].enable(welt->is_paused());
	buttons[IDBTN_SCROLL_INVERSE].pressed = env_t::scroll_multi < 0;
	buttons[IDBTN_DAY_NIGHT_CHANGE].pressed = env_t::night_shift;
	buttons[IDBTN_SHOW_SLICE_MAP_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[IDBTN_UNDERGROUND_VIEW].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[IDBTN_TRANSPARENT_STATION_COVERAGE].pressed = env_t::use_transparency_station_coverage;
	buttons[IDBTN_TRANSPARENT_INSTEAD_OF_HIDDEN].pressed = env_t::hide_with_transparency;
	buttons[IDBTN_RIBI_ARROW].pressed = strasse_t::show_masked_ribi;
	buttons[IDBTN_RIBI_ARROW].enable(skinverwaltung_t::ribi_arrow!=NULL);

	// All components are updated, now draw them...
	gui_frame_t::draw(pos, size);
}
