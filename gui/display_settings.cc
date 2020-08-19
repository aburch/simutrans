/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Menu with display settings
 * @author Hj. Malthaner
 */

#include "display_settings.h"

#include "../path_explorer.h"
#include "../simhalt.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../display/simimg.h"
#include "../simintr.h"
#include "../simcolor.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../obj/baum.h"
#include "../obj/zeiger.h"
#include "../display/simgraph.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"
#include "themeselector.h"
#include "simwin.h"
#include "../obj/gebaeude.h"

// y coordinates
#define GRID_MODE						(6)
#define UNDERGROUND						(GRID_MODE+13)
#define SLICE							(UNDERGROUND+13)
#define SLICE_LABEL						(SLICE+13)
#define DAY_NIGHT						(SLICE_LABEL+13)
#define BRIGHTNESS						(DAY_NIGHT+13)
#define SCROLL_INVERS					(BRIGHTNESS+13)
#define SCROLL_SPEED					(SCROLL_INVERS+13)
#define LEFT_TO_RIGHT_GRAPHS			(SCROLL_SPEED+13)

#define SEPERATE1						(LEFT_TO_RIGHT_GRAPHS+13)

#define USE_TRANSPARENCY	(SEPERATE1+4)
#define HIDE_TREES			(USE_TRANSPARENCY+13)
#define HIDE_CITY_HOUSES	(HIDE_TREES+13)
#define HIDE_UNDER_CURSOR	(HIDE_CITY_HOUSES+13)
#define CURSOR_HIDE_RANGE	(HIDE_UNDER_CURSOR)

#define SEPERATE2 (CURSOR_HIDE_RANGE+13)

#define USE_TRANSPARENCY_STATIONS		(SEPERATE2+4)
#define SHOW_STATION_COVERAGE			(USE_TRANSPARENCY_STATIONS+13)
#define SHOW_STATION_SIGNS				(SHOW_STATION_COVERAGE+13)
#define SHOW_STATION_GOODS				(SHOW_STATION_SIGNS+13)
#define SHOW_SIGNALBOX_COVERAGE				(SHOW_STATION_GOODS+13)

#define SEPERATE3						(SHOW_SIGNALBOX_COVERAGE+13)

#define CITY_WALKER								(SEPERATE3+4)
#define STOP_WALKER								(CITY_WALKER+13)
#define DENS_TRAFFIC							(STOP_WALKER+13)
#define CONVOI_TOOLTIPS							(DENS_TRAFFIC+13)
#define CONVOI_NAMEPLATES						(CONVOI_TOOLTIPS+13)
#define CONVOI_LOADINGBAR						(CONVOI_NAMEPLATES+13)
#define HIGHLITE_SCHEDULE						(CONVOI_LOADINGBAR+13)

#define SEPERATE4	(HIGHLITE_SCHEDULE+13)

#define FPS_DATA						(SEPERATE4+4)
#define IDLE_DATA						(FPS_DATA+13)
#define FRAME_DATA						(IDLE_DATA+13)
#define LOOP_DATA						(FRAME_DATA+13)

#define SEPERATE5						(LOOP_DATA+13)

#define PHASE_REBUILD_CONNEXIONS		(SEPERATE5+7)
#define PHASE_FILTER_ELIGIBLE			(PHASE_REBUILD_CONNEXIONS+13)
#define PHASE_FILL_MATRIX				(PHASE_FILTER_ELIGIBLE+13)
#define PHASE_EXPLORE_PATHS				(PHASE_FILL_MATRIX+13)
#define PHASE_REROUTE_GOODS				(PHASE_EXPLORE_PATHS+13)
#define PATH_EXPLORE_STATUS				(PHASE_REROUTE_GOODS+13)
#define PATH_EXPLORE_STATUS_TEXT		(PATH_EXPLORE_STATUS+13)


#define L_DIALOG_HEIGHT					(PATH_EXPLORE_STATUS_TEXT+30)

// Local params
#define L_DIALOG_WIDTH (220)

const char *color_gui_t::cnv_tooltip_setting_texts[MAX_CONVOY_TOOLTIP_OPTIONS] = {
	"convoi mouseover tooltips",
	"convoi error tooltips",
	"own convoi tooltips",
	"all convoi tooltips"
};

const char *color_gui_t::nameplate_setting_texts[MAX_NAMEPLATE_OPTIONS] = {
	"no convoy nameplate",
	"mouseover convoy name",
	"active player's line name",
	"always show convoy name"
};

const char *color_gui_t::loadingbar_setting_texts[MAX_LOADING_BAR_OPTIONS] = {
	"no convoy loading bar",
	"mouseover loading bar",
	"active player's loading bar",
	"always show loading bar"
};


color_gui_t::color_gui_t() :
gui_frame_t( translator::translate("Helligk. u. Farben") )
{
	// brightness
	brightness.set_pos( scr_coord(L_DIALOG_WIDTH-10-40,BRIGHTNESS-1) );
	brightness.set_size( scr_size( 40, D_BUTTON_HEIGHT-1 ) );
	brightness.set_value( env_t::daynight_level );
	brightness.set_limits( 0, 9 );
	brightness.add_listener(this);

	// scrollspeed
	scrollspeed.set_pos( scr_coord(L_DIALOG_WIDTH-10-40,SCROLL_SPEED-1) );
	scrollspeed.set_size( scr_size( 40, D_BUTTON_HEIGHT-1 ) );
	scrollspeed.set_value( abs(env_t::scroll_multi) );
	scrollspeed.set_limits( 1, 9 );
	scrollspeed.add_listener(this);

	// range to hide under mouse cursor
	cursor_hide_range.set_pos( scr_coord(L_DIALOG_WIDTH-10-45,CURSOR_HIDE_RANGE-1) );
	cursor_hide_range.set_size( scr_size( 45, D_BUTTON_HEIGHT-1 ) );
	cursor_hide_range.set_value(env_t::cursor_hide_range);
	cursor_hide_range.set_limits( 0, 10 );
	cursor_hide_range.add_listener(this);

	// traffic density
	traffic_density.set_pos( scr_coord(L_DIALOG_WIDTH-10-45,DENS_TRAFFIC-1) );
	traffic_density.set_size( scr_size( 45, D_BUTTON_HEIGHT-1 ) );
	traffic_density.set_value(welt->get_settings().get_traffic_level());
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);

	uint8 b = 5;

	// other settings
	//6
	buttons[++b].set_pos( scr_coord(10,SCROLL_INVERS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("4LIGHT_CHOOSE");
	buttons[b].pressed = env_t::scroll_multi < 0;
	buttons[b].set_tooltip("The main game window can be scrolled by right-clicking and dragging the ground.");

	//7
	buttons[++b].set_pos( scr_coord(10,STOP_WALKER) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("5LIGHT_CHOOSE");
	buttons[b].pressed =welt->get_settings().get_show_pax();
	buttons[b].set_tooltip("Pedestrians will appear near stops whenver a passenger vehicle unloads there.");

	//8
	buttons[++b].set_pos( scr_coord(10,CITY_WALKER) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("6LIGHT_CHOOSE");
	buttons[b].pressed =welt->get_settings().get_random_pedestrians();
	buttons[b].set_tooltip("Pedestrians will appear randomly in towns.");

	//9
	buttons[++b].set_pos( scr_coord(10,DAY_NIGHT) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("8WORLD_CHOOSE");
	buttons[b].pressed = env_t::night_shift;
	buttons[b].set_tooltip("Whether the lighting in the main game window simulates a periodic transition between day and night.");

	//10
	buttons[++b].set_pos( scr_coord(10,USE_TRANSPARENCY) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("hide transparent");
	buttons[b].pressed = env_t::hide_with_transparency;
	buttons[b].set_tooltip("All hidden items (such as trees and buildings) will appear as transparent.");

	//11
	buttons[++b].set_pos( scr_coord(10,HIDE_TREES) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("hide trees");
	buttons[b].set_tooltip("Trees will be miniaturised or made transparent in the main game window.");

	// left right for hide messages

	//12
	buttons[++b].set_pos( scr_coord(10,HIDE_CITY_HOUSES) );
	buttons[b].set_typ(button_t::arrowleft);
	//13
	buttons[++b].set_pos( scr_coord(L_DIALOG_WIDTH-10-10-2,HIDE_CITY_HOUSES) );
	buttons[b].set_typ(button_t::arrowright);

	//14
	buttons[++b].set_pos( scr_coord(10,USE_TRANSPARENCY_STATIONS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("transparent station coverage");
	buttons[b].pressed = env_t::use_transparency_station_coverage;
	buttons[b].set_tooltip("The display of the station coverage can either be a transparent rectangle or a series of boxes.");

	//15
	buttons[++b].set_pos( scr_coord(10,SHOW_STATION_COVERAGE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show station coverage");
	buttons[b].set_tooltip("Show from how far that passengers or goods will come to use your stops. Toggle with the v key.");

	//16
	buttons[++b].set_pos(scr_coord(10, SHOW_SIGNALBOX_COVERAGE));
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show signalbox coverage");
	buttons[b].set_tooltip("Show coverage radius of the signalbox.");


	//17
	buttons[++b].set_pos( scr_coord(10,UNDERGROUND) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("underground mode");
	buttons[b].set_tooltip("See under the ground, to build tunnels and underground railways/metros. Toggle with SHIFT + U");

	//18
	buttons[++b].set_pos( scr_coord(10,GRID_MODE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show grid");
	buttons[b].set_tooltip("Shows the borderlines of each tile in the main game window. Can be useful for construction. Toggle with the # key.");

	//19
	buttons[++b].set_pos( scr_coord(10,SHOW_STATION_SIGNS) );
	buttons[b].set_typ(button_t::arrowright);
	//buttons[b].set_tooltip("show station names");
/*	buttons[b].set_text("show station names");
	buttons[b].pressed = env_t::show_names&1;*/
	buttons[b].set_tooltip("Shows the names of the individual stations in the main game window.");

	//20
	buttons[++b].set_pos( scr_coord(10,SHOW_STATION_GOODS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show waiting bars");
	buttons[b].pressed = env_t::show_names&2;
	buttons[b].set_tooltip("Shows a bar graph representing the number of passengers/mail/goods waiting at stops.");

	//21
	buttons[++b].set_pos( scr_coord(10,SLICE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("sliced underground mode");
	buttons[b].set_tooltip("See under the ground, one layer at a time. Toggle with CTRL + U. Move up/down in layers with HOME and END.");

	// underground slice edit
	inp_underground_level.set_pos( scr_coord(L_DIALOG_WIDTH-10-50, SLICE_LABEL-1) );
	inp_underground_level.set_size( scr_size( 50, D_BUTTON_HEIGHT-1 ) );
	//inp_underground_level.align_to(&buttons[20], ALIGN_CENTER_V);
	inp_underground_level.set_value( grund_t::underground_mode==grund_t::ugm_level ? grund_t::underground_level : welt->get_zeiger()->get_pos().z);
	inp_underground_level.set_limits(welt->get_groundwater()-10, 32);
	inp_underground_level.add_listener(this);

	//22
	buttons[++b].set_pos( scr_coord(10, LEFT_TO_RIGHT_GRAPHS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("Inverse graphs");
	buttons[b].pressed = !env_t::left_to_right_graphs;
	buttons[b].set_tooltip("Graphs right to left instead of left to right");


	// left/right for convoi tooltips
	buttons[0].set_pos( scr_coord(10,CONVOI_TOOLTIPS) );
	buttons[0].set_typ(button_t::arrowleft);
	buttons[1].set_pos( scr_coord(L_DIALOG_WIDTH-10-10-2,CONVOI_TOOLTIPS) );
	buttons[1].set_typ(button_t::arrowright);
	// left/right for convoi nameplates
	buttons[25].set_pos(scr_coord(10, CONVOI_NAMEPLATES));
	buttons[25].set_typ(button_t::arrowleft);
	buttons[26].set_pos(scr_coord(L_DIALOG_WIDTH - 10 - 10 - 2, CONVOI_NAMEPLATES));
	buttons[26].set_typ(button_t::arrowright);
	// left/right for convoi loadingbar
	buttons[27].set_pos(scr_coord(10, CONVOI_LOADINGBAR));
	buttons[27].set_typ(button_t::arrowleft);
	buttons[28].set_pos(scr_coord(L_DIALOG_WIDTH - 10 - 10 - 2, CONVOI_LOADINGBAR));
	buttons[28].set_typ(button_t::arrowright);

	//23, Hide buildings and trees under mouse cursor
	buttons[++b].set_pos( scr_coord(10,HIDE_UNDER_CURSOR) );
	buttons[b].set_typ( button_t::square_state );
	buttons[b].set_text( "Smart hide objects" );
	buttons[b].set_tooltip( "hide objects under cursor" );

	//24
	buttons[++b].set_pos( scr_coord(10,HIGHLITE_SCHEDULE) );
	buttons[b].set_typ( button_t::square_state );
	buttons[b].set_text( "Highlite schedule" );
	buttons[b].set_tooltip( "Highlight the locations of stops on the current schedule" );

	for(int i=0;  i<COLORS_MAX_BUTTONS;  i++ ) {
		buttons[i].add_listener(this);
	}

	// add buttons for sensible keyboard tab order
	add_component( buttons+17 );
	add_component( buttons+16 );
	add_component( buttons+20 );
	add_component( &inp_underground_level );
	add_component( buttons+9 );
	add_component( &brightness );
	add_component( buttons+6 );
	add_component( &scrollspeed );
	add_component( buttons+10 );
	add_component( buttons+11 );
	add_component( buttons+21);
	add_component( &cursor_hide_range );
	add_component( buttons+12 );
	add_component( buttons+13 );
	add_component( buttons+14 );
	add_component( buttons+15 );
	add_component( buttons+18 );
	add_component( buttons+19 );
	add_component( buttons+8 );
	add_component( buttons+7 );
	add_component( &traffic_density );
	add_component( buttons+0 );
	add_component( buttons+1 );
	add_component( buttons+22);
	add_component(buttons + 23);
	add_component(buttons + 24);
	add_component(buttons + 25);
	add_component(buttons + 26);
	add_component(buttons + 27);
	add_component(buttons + 28);

	// unused buttons
	// add_component( buttons+2 );
	// add_component( buttons+3 );
	// add_component( buttons+4 );
	// add_component( buttons+5 );

	set_resizemode(gui_frame_t::horizonal_resize);
	set_min_windowsize( scr_size(L_DIALOG_WIDTH, L_DIALOG_HEIGHT) );
	set_windowsize( scr_size(L_DIALOG_WIDTH, L_DIALOG_HEIGHT) );
}


void color_gui_t::set_windowsize(scr_size size)
{
	scr_coord_val column;
	scr_coord_val delta_w = size.w - get_windowsize().w;

	for(  int i=0;  i<COLORS_MAX_BUTTONS;  i++  ) {
		if(  buttons[i].get_type() == button_t::square_state  ) {
			// resize buttons too to fix text
			buttons[i].set_width(buttons[i].get_size().w + delta_w);
		}
	}

	gui_frame_t::set_windowsize(size);

	column = size.w - D_MARGIN_RIGHT - inp_underground_level.get_size().w;
	inp_underground_level.set_pos ( scr_coord( column, inp_underground_level.get_pos().y ) );
	brightness.set_pos            ( scr_coord( column, brightness.get_pos().y            ) );
	scrollspeed.set_pos           ( scr_coord( column, scrollspeed.get_pos().y           ) );
	traffic_density.set_pos       ( scr_coord( column, traffic_density.get_pos().y       ) );
	cursor_hide_range.set_pos     ( scr_coord( column, cursor_hide_range.get_pos().y     ) );

	column = size.w - D_MARGIN_RIGHT - D_ARROW_RIGHT_WIDTH;
	buttons[1].set_pos            ( scr_coord( column, buttons[1].get_pos().y            ) );
	buttons[13].set_pos           ( scr_coord( column, buttons[13].get_pos().y           ) );
	buttons[26].set_pos           ( scr_coord( column, buttons[26].get_pos().y           ) );

	column = size.w - D_MARGINS_X;
	divider1.set_width            ( column );
	divider2.set_width            ( column );
	divider3.set_width            ( column );
	divider4.set_width            ( column );

}


bool color_gui_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	if(&brightness==comp) {
		env_t::daynight_level = (sint8)v.i;
	} else if(&traffic_density==comp) {
		if(  !env_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			static char level[16];
			sprintf(level, "%li", v.i);
			tool_t::simple_tool[TOOL_TRAFFIC_LEVEL&0xFFF]->set_default_param( level );
			welt->set_tool( tool_t::simple_tool[TOOL_TRAFFIC_LEVEL&0xFFF], welt->get_active_player() );
		}
		else {
			traffic_density.set_value(welt->get_settings().get_traffic_level());
		}
	} else if(&scrollspeed==comp) {
		env_t::scroll_multi = (sint16)( buttons[6].pressed ? -v.i : v.i );
	} else if (&cursor_hide_range==comp) {
		env_t::cursor_hide_range = cursor_hide_range.get_value();
	} else if((buttons+0)==comp) {
		env_t::show_vehicle_states = (env_t::show_vehicle_states+(MAX_CONVOY_TOOLTIP_OPTIONS-1))% MAX_CONVOY_TOOLTIP_OPTIONS;
	} else if((buttons+1)==comp) {
		env_t::show_vehicle_states = (env_t::show_vehicle_states+1)% MAX_CONVOY_TOOLTIP_OPTIONS;
	} else if((buttons+6)==comp) {
		buttons[6].pressed ^= 1;
		env_t::scroll_multi = -env_t::scroll_multi;
	} else if((buttons+7)==comp) {
		if(  !env_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_tool( tool_t::simple_tool[TOOL_TOOGLE_PAX&0xFFF], welt->get_active_player() );
		}
	} else if((buttons+8)==comp) {
		if(  !env_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_tool( tool_t::simple_tool[TOOL_TOOGLE_PEDESTRIANS&0xFFF], welt->get_active_player() );
		}
	} else if((buttons+9)==comp) {
		env_t::night_shift = !env_t::night_shift;
		buttons[9].pressed ^= 1;
	} else if((buttons+10)==comp) {
		env_t::hide_with_transparency = !env_t::hide_with_transparency;
		buttons[10].pressed ^= 1;
		baum_t::recalc_outline_color();
	} else if((buttons+11)==comp) {
		env_t::hide_trees = !env_t::hide_trees;
		baum_t::recalc_outline_color();
	} else if((buttons+12)==comp) {
		env_t::hide_buildings = (env_t::hide_buildings+2)%3;
	} else if((buttons+13)==comp) {
		env_t::hide_buildings = (env_t::hide_buildings+1)%3;
	} else if((buttons+14)==comp) {
		env_t::use_transparency_station_coverage = !env_t::use_transparency_station_coverage;
		buttons[14].pressed ^= 1;
	}
	else if ((buttons + 15) == comp) {
		env_t::station_coverage_show = env_t::station_coverage_show == 0 ? 0xFF : 0;
	}
	else if ((buttons + 16) == comp) {
		env_t::signalbox_coverage_show = env_t::signalbox_coverage_show == 0 ? 0xFF : 0;
	}
	else if ((buttons + 17) == comp) {
		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode(buttons[17].pressed ? grund_t::ugm_none : grund_t::ugm_all, inp_underground_level.get_value());
		buttons[17].pressed = grund_t::underground_mode == grund_t::ugm_all;
		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
	}
	else if ((buttons + 18) == comp) {
		grund_t::toggle_grid();
	}
	else if ((buttons + 19) == comp) {
		if (env_t::show_names & 1) {
			if ((env_t::show_names >> 2) == 2) {
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
	}
	else if ((buttons + 20) == comp) {
		env_t::show_names ^= 2;
	}
	else if ((buttons + 21) == comp) {
		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode(buttons[21].pressed ? grund_t::ugm_none : grund_t::ugm_level, inp_underground_level.get_value());
		buttons[21].pressed = grund_t::underground_mode == grund_t::ugm_level;
		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
	}
	else if ((buttons + 23) == comp) {
		// see simtool.cc::tool_hide_under_cursor_t::init
		env_t::hide_under_cursor = !env_t::hide_under_cursor  &&  env_t::cursor_hide_range > 0;
		buttons[23].pressed = env_t::hide_under_cursor;
		// renew toolbar
		tool_t::update_toolbars();
	}
	else if ((buttons + 24) == comp) {
		env_t::visualize_schedule = !env_t::visualize_schedule;
		buttons[24].pressed = env_t::visualize_schedule;
	}
	else if (comp == &inp_underground_level) {
		if (grund_t::underground_mode == grund_t::ugm_level) {
			grund_t::underground_level = inp_underground_level.get_value();
			// calc new images
			welt->update_underground();
		}
	}
	else if ((buttons + 22) == comp) {
		env_t::left_to_right_graphs = !env_t::left_to_right_graphs;
		buttons[22].pressed ^= 1;
	}
	else if ((buttons + 25) == comp) {
		env_t::show_cnv_nameplates = (env_t::show_cnv_nameplates + (MAX_NAMEPLATE_OPTIONS-1)) % MAX_NAMEPLATE_OPTIONS;
	}
	else if ((buttons + 26) == comp) {
		env_t::show_cnv_nameplates = (env_t::show_cnv_nameplates + 1) % MAX_NAMEPLATE_OPTIONS;
	}
	else if ((buttons + 27) == comp) {
		env_t::show_cnv_loadingbar = (env_t::show_cnv_loadingbar + (MAX_LOADING_BAR_OPTIONS-1)) % MAX_LOADING_BAR_OPTIONS;
	}
	else if ((buttons + 28) == comp) {
		env_t::show_cnv_loadingbar = (env_t::show_cnv_loadingbar + 1) % MAX_LOADING_BAR_OPTIONS;
	}

	welt->set_dirty();
	return true;
}


void color_gui_t::draw(scr_coord pos, scr_size size)
{
	const int x = pos.x;
	const int y = pos.y + D_TITLEBAR_HEIGHT; // compensate for title bar
	char buf[128];

	// can be changed also with keys ...
	buttons[ 7].pressed = welt->get_settings().get_show_pax();
	buttons[ 8].pressed = welt->get_settings().get_random_pedestrians();
	buttons[11].pressed = env_t::hide_trees;
	buttons[15].pressed = env_t::station_coverage_show;
	buttons[16].pressed = env_t::signalbox_coverage_show;
	buttons[17].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[18].pressed = grund_t::show_grid;
	//buttons[19].pressed = env_t::show_names&1;
	buttons[20].pressed = (env_t::show_names&2)!=0;
	buttons[21].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[23].pressed = env_t::hide_under_cursor;
	buttons[24].pressed = env_t::visualize_schedule;

	gui_frame_t::draw(pos, size);

	// draw the label type
	if(  env_t::show_names&1  ) {
		PLAYER_COLOR_VAL pc = welt->get_active_player() ? welt->get_active_player()->get_player_color1()+4 : COL_ORANGE;
		const char *text = translator::translate("show station names");
		switch( env_t::show_names >> 2 ) {
			case 0:
				display_ddd_proportional_clip( 16+x+buttons[19].get_pos().x, y+buttons[19].get_pos().y+(LINESPACE/2), proportional_string_width(text)+7, 0, pc, SYSCOL_TEXT, text, 1 );
				break;
			case 1:
				display_outline_proportional( 16+x+buttons[19].get_pos().x, y+buttons[19].get_pos().y, pc+1, SYSCOL_TEXT, text, 1 );
				break;
			case 2:
				display_outline_proportional( 16+x+buttons[19].get_pos().x+16, y+buttons[19].get_pos().y, COL_YELLOW, SYSCOL_TEXT, text, 1 );
				display_ddd_box_clip( 16+x+buttons[19].get_pos().x, y+buttons[19].get_pos().y, LINESPACE, LINESPACE, pc-2, pc+2 );
				display_fillbox_wh(16 + x + buttons[19].get_pos().x + 1, y + buttons[19].get_pos().y + 1, LINESPACE - 2, LINESPACE - 2, pc, true);
				break;
		}
	}

	// separator
	const sint16 w = this->get_windowsize().w;
	display_ddd_box_clip(x+10, y+SEPERATE1+1, w-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE2+1, w-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE3+1, w-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE4+1, w-20, 0, MN_GREY0, MN_GREY4);

	display_proportional_clip(x+10, y+BRIGHTNESS+1, translator::translate("1LIGHT_CHOOSE"), ALIGN_LEFT, SYSCOL_TEXT, true);

	display_proportional_clip(x+10, y+SCROLL_SPEED+1, translator::translate("3LIGHT_CHOOSE"), ALIGN_LEFT, SYSCOL_TEXT, true);

	display_proportional_clip(x+10, y+DENS_TRAFFIC+1, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, SYSCOL_TEXT, true);

	const char *hhc = translator::translate( env_t::hide_buildings==0 ? "no buildings hidden" : (env_t::hide_buildings==1 ? "hide city building" : "hide all building") );
	display_proportional_clip(x+10+16, y+HIDE_CITY_HOUSES+1, hhc, ALIGN_LEFT, SYSCOL_TEXT, true);

	display_proportional_clip(x+10+16, y+CONVOI_TOOLTIPS+1, translator::translate(cnv_tooltip_setting_texts[env_t::show_vehicle_states]), ALIGN_LEFT, SYSCOL_TEXT, true);

	display_proportional_clip(x+10+16, y+CONVOI_NAMEPLATES+1, translator::translate(nameplate_setting_texts[env_t::show_cnv_nameplates]), ALIGN_LEFT, SYSCOL_TEXT, true);

	display_proportional_clip(x + 10 + 16, y + CONVOI_LOADINGBAR + 1, translator::translate(loadingbar_setting_texts[env_t::show_cnv_loadingbar]), ALIGN_LEFT, SYSCOL_TEXT, true);

	int len=15+display_proportional_clip(x+10, y+FPS_DATA, translator::translate("Frame time:"), ALIGN_LEFT, SYSCOL_TEXT, true);
	sprintf(buf,"%ld ms", get_frame_time() );
	display_proportional_clip(x+len, y+FPS_DATA, buf, ALIGN_LEFT, SYSCOL_TEXT_HIGHLIGHT, true);

	len = 15+display_proportional_clip(x+10, y+IDLE_DATA, translator::translate("Idle:"), ALIGN_LEFT, SYSCOL_TEXT, true);
	display_proportional_clip(x+len, y+IDLE_DATA, ntos(welt->get_idle_time(), "%d ms"), ALIGN_LEFT, SYSCOL_TEXT_HIGHLIGHT, true);

	uint8 farbe;
	uint32 loops;
	loops=welt->get_realFPS();
	farbe = COL_WHITE;
	uint32 target_fps = welt->is_fast_forward() ? 10 : env_t::fps;
	if(loops<(target_fps*3)/4) {
		farbe = (loops<=target_fps/2) ? COL_RED : COL_YELLOW;
	}
	len = 15+display_proportional_clip(x+10, y+FRAME_DATA, translator::translate("FPS:"), ALIGN_LEFT, SYSCOL_TEXT, true);
	sprintf(buf,"%d fps", loops );
#if MSG_LEVEL >= 3
	if(  env_t::simple_drawing  ) {
		strcat( buf, "*" );
	}
#endif
	display_proportional_clip(x+len, y+FRAME_DATA, buf, ALIGN_LEFT, farbe, true);

	loops=welt->get_simloops();
	farbe = COL_WHITE;
	if(loops<=30) {
		farbe = (loops<=20) ? COL_RED : COL_YELLOW;
	}
	len = 15+display_proportional_clip(x+10, y+LOOP_DATA, translator::translate("Sim:"), ALIGN_LEFT, SYSCOL_TEXT, true);
	sprintf( buf, "%d%c%d", loops/10, get_fraction_sep(), loops%10 );
	display_proportional_clip(x+len, y+LOOP_DATA, buf, ALIGN_LEFT, farbe, true);

	// Added by : Knightly
	PLAYER_COLOR_VAL text_colour, figure_colour;

	text_colour = SYSCOL_TEXT;
	figure_colour = SYSCOL_TEXT_TITLE;

	char status_string[128];
	if ( path_explorer_t::is_processing() )
	{
		sprintf(status_string, "%s (%s) / %s", translator::translate(path_explorer_t::get_current_category_name()), path_explorer_t::get_current_class_name(), path_explorer_t::get_current_phase_name());
		/*
		tstrncpy(status_string, translator::translate( path_explorer_t::get_current_category_name() ), 15);
		strcat(status_string, " (");
		tstrncpy(status_string, (path_explorer_t::get_current_class_name()), 32);
		strcat(status_string, ")");
		strcat(status_string, " / ");
		strncat(status_string, translator::translate( path_explorer_t::get_current_phase_name() ), 8);
		*/
	}
	else
	{
		sprintf(status_string, "stand-by");
	}

	len = 15+display_proportional_clip(x+10, y+PHASE_REBUILD_CONNEXIONS, translator::translate("Rebuild connexions:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_REBUILD_CONNEXIONS, ntos(path_explorer_t::get_limit_rebuild_connexions(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_FILTER_ELIGIBLE, translator::translate("Filter eligible halts:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_FILTER_ELIGIBLE, ntos(path_explorer_t::get_limit_filter_eligible(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_FILL_MATRIX, translator::translate("Fill path matrix:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_FILL_MATRIX, ntos(path_explorer_t::get_limit_fill_matrix(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_EXPLORE_PATHS, translator::translate("Explore paths:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_EXPLORE_PATHS, ntos((long)path_explorer_t::get_limit_explore_paths(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_REROUTE_GOODS, translator::translate("Re-route goods:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_REROUTE_GOODS, ntos(path_explorer_t::get_limit_reroute_goods(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PATH_EXPLORE_STATUS, translator::translate("Status:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+10, y+PATH_EXPLORE_STATUS_TEXT, status_string, ALIGN_LEFT, figure_colour, true);
}
