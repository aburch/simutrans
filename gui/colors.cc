/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "components/list_button.h"

#include "colors.h"

#include "../path_explorer.h"
#include "../simhalt.h"
#include "../simdebug.h"
#include "../simworld.h"
#include "../simimg.h"
#include "../simintr.h"
#include "../simcolor.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dings/zeiger.h"
#include "../simgraph.h"
#include "../simmenu.h"
#include "../player/simplay.h"

#include "../utils/simstring.h"

// y coordinates
#define GRID_MODE						(6)
#define UNDERGROUND						(GRID_MODE+13)
#define SLICE							(UNDERGROUND+13)
#define DAY_NIGHT						(SLICE+13)
#define BRIGHTNESS						(DAY_NIGHT+13)
#define SCROLL_INVERS					(BRIGHTNESS+13)
#define SCROLL_SPEED					(SCROLL_INVERS+13)
#define LEFT_TO_RIGHT_GRAPHS			(SCROLL_SPEED+13)

#define SEPERATE1						(LEFT_TO_RIGHT_GRAPHS+13)

#define USE_TRANSPARENCY				(SEPERATE1+4)
#define HIDE_TREES						(USE_TRANSPARENCY+13)
#define HIDE_CITY_HOUSES				(HIDE_TREES+13)

#define SEPERATE2						(HIDE_CITY_HOUSES+13)

#define USE_TRANSPARENCY_STATIONS		(SEPERATE2+4)
#define SHOW_STATION_COVERAGE			(USE_TRANSPARENCY_STATIONS+13)
#define SHOW_STATION_SIGNS				(SHOW_STATION_COVERAGE+13)
#define SHOW_STATION_GOODS				(SHOW_STATION_SIGNS+13)

#define SEPERATE3						(SHOW_STATION_GOODS+13)

#define CITY_WALKER						(SEPERATE3+4)
#define STOP_WALKER						(CITY_WALKER+13)
#define DENS_TRAFFIC					(STOP_WALKER+13)
#define CONVOI_TOOLTIPS					(DENS_TRAFFIC+13)

#define SEPERATE4						(CONVOI_TOOLTIPS+13)

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


#define BOTTOM							(PATH_EXPLORE_STATUS+30)

// x coordinates
#define RIGHT_WIDTH (220)


color_gui_t::color_gui_t(karte_t *welt) :
	gui_frame_t("Helligk. u. Farben")
{
	this->welt = welt;

	// underground slice
	inp_underground_level.set_pos( koord(RIGHT_WIDTH-10-50, SLICE) );
	inp_underground_level.set_groesse( koord( 50, BUTTON_HEIGHT-1 ) );
	inp_underground_level.set_value( grund_t::underground_mode==grund_t::ugm_level ? grund_t::underground_level : welt->get_zeiger()->get_pos().z);
	inp_underground_level.set_limits(welt->get_grundwasser()-10, 32);
	inp_underground_level.add_listener(this);

	// brightness
	brightness.set_pos( koord(RIGHT_WIDTH-10-40,BRIGHTNESS) );
	brightness.set_groesse( koord( 40, BUTTON_HEIGHT-1 ) );
	brightness.set_value( umgebung_t::daynight_level );
	brightness.set_limits( 0, 9 );
	brightness.add_listener(this);

	// scrollspeed
	scrollspeed.set_pos( koord(RIGHT_WIDTH-10-40,SCROLL_SPEED) );
	scrollspeed.set_groesse( koord( 40, BUTTON_HEIGHT-1 ) );
	scrollspeed.set_value( abs(umgebung_t::scroll_multi) );
	scrollspeed.set_limits( 1, 9 );
	scrollspeed.add_listener(this);

	// traffic density
	traffic_density.set_pos( koord(RIGHT_WIDTH-10-45,DENS_TRAFFIC) );
	traffic_density.set_groesse( koord( 45, BUTTON_HEIGHT-1 ) );
	traffic_density.set_value(welt->get_settings().get_verkehr_level());
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);

	uint8 b = 5;

	// other settings
	//6
	buttons[++b].set_pos( koord(10,SCROLL_INVERS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("4LIGHT_CHOOSE");
	buttons[b].pressed = umgebung_t::scroll_multi < 0;
	buttons[b].set_tooltip("The main game window can be scrolled by right-clicking and dragging the ground.");

	//7
	buttons[++b].set_pos( koord(10,STOP_WALKER) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("5LIGHT_CHOOSE");
	buttons[b].pressed =welt->get_settings().get_show_pax();
	buttons[b].set_tooltip("Pedestrians will appear near stops whenver a passenger vehicle unloads there.");

	//8
	buttons[++b].set_pos( koord(10,CITY_WALKER) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("6LIGHT_CHOOSE");
	buttons[b].pressed =welt->get_settings().get_random_pedestrians();
	buttons[b].set_tooltip("Pedestrians will appear randomly in towns.");

	//9
	buttons[++b].set_pos( koord(10,DAY_NIGHT) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("8WORLD_CHOOSE");
	buttons[b].pressed = umgebung_t::night_shift;
	buttons[b].set_tooltip("Whether the lighting in the main game window simulates a periodic transition between day and night.");

	//10
	buttons[++b].set_pos( koord(10,USE_TRANSPARENCY) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("hide transparent");
	buttons[b].pressed = umgebung_t::hide_with_transparency;
	buttons[b].set_tooltip("All hidden items (such as trees and buildings) will appear as transparent.");

	//11
	buttons[++b].set_pos( koord(10,HIDE_TREES) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("hide trees");
	buttons[b].set_tooltip("Trees will be miniaturised or made transparent in the main game window.");

	// left right for hide messages

	//12
	buttons[++b].set_pos( koord(10,HIDE_CITY_HOUSES) );
	buttons[b].set_typ(button_t::arrowleft);
	//13
	buttons[++b].set_pos( koord(RIGHT_WIDTH-10-10,HIDE_CITY_HOUSES) );
	buttons[b].set_typ(button_t::arrowright);

	//14
	buttons[++b].set_pos( koord(10,USE_TRANSPARENCY_STATIONS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("transparent station coverage");
	buttons[b].pressed = umgebung_t::use_transparency_station_coverage;
	buttons[b].set_tooltip("The display of the station coverage can either be a transparent rectangle or a series of boxes.");

	//15
	buttons[++b].set_pos( koord(10,SHOW_STATION_COVERAGE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show station coverage");
	buttons[b].set_tooltip("Show from how far that passengers or goods will come to use your stops. Toggle with the v key.");

	//16
	buttons[++b].set_pos( koord(10,UNDERGROUND) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("underground mode");
	buttons[b].set_tooltip("See under the ground, to build tunnels and underground railways/metros. Toggle with SHIFT + U");

	//17
	buttons[++b].set_pos( koord(10,GRID_MODE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show grid");
	buttons[b].set_tooltip("Shows the borderlines of each tile in the main game window. Can be useful for construction. Toggle with the # key.");

	//18
	buttons[++b].set_pos( koord(10,SHOW_STATION_SIGNS) );
	buttons[b].set_typ(button_t::arrowright);
	//buttons[b].set_tooltip("show station names");
/*	buttons[b].set_text("show station names");
	buttons[b].pressed = umgebung_t::show_names&1;*/
	buttons[b].set_tooltip("Shows the names of the individual stations in the main game window.");

	//19	
	buttons[++b].set_pos( koord(10,SHOW_STATION_GOODS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show waiting bars");
	buttons[b].pressed = umgebung_t::show_names&2;
	buttons[b].set_tooltip("Shows a bar graph representing the number of passengers/mail/goods waiting at stops.");

	//20
	buttons[++b].set_pos( koord(10,SLICE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("sliced underground mode");
	buttons[b].set_tooltip("See under the ground, one layer at a time. Toggle with CTRL + U. Move up/down in layers with HOME and END.");

	//21
	buttons[++b].set_pos( koord(10, LEFT_TO_RIGHT_GRAPHS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("Inverse graphs");
	buttons[b].pressed = !umgebung_t::left_to_right_graphs;
	buttons[b].set_tooltip("Graphs right to left instead of left to right");

	// left/right for convoi tooltips
	buttons[0].set_pos( koord(10,CONVOI_TOOLTIPS) );
	buttons[0].set_typ(button_t::arrowleft);
	buttons[1].set_pos( koord(RIGHT_WIDTH-10-10,CONVOI_TOOLTIPS) );
	buttons[1].set_typ(button_t::arrowright);

	for(int i=0;  i<COLORS_MAX_BUTTONS;  i++ ) {
		buttons[i].add_listener(this);
	//		add_komponente( buttons+i );
	}

	// add buttons for sensible keyboard tab order
	add_komponente( buttons+17 );
	add_komponente( buttons+16 );
	add_komponente( buttons+20 );
	add_komponente( &inp_underground_level );
	add_komponente( buttons+9 );
	add_komponente( &brightness );
	add_komponente( buttons+6 );
	add_komponente( &scrollspeed );
	add_komponente( buttons+10 );
	add_komponente( buttons+11 );
	add_komponente( buttons+12 );
	add_komponente( buttons+13 );
	add_komponente( buttons+14 );
	add_komponente( buttons+15 );
	add_komponente( buttons+18 );
	add_komponente( buttons+19 );
	add_komponente( buttons+8 );
	add_komponente( buttons+7 );
	add_komponente( &traffic_density );
	add_komponente( buttons+0 );
	add_komponente( buttons+1 );

	// unused buttons
	// add_komponente( buttons+2 );
	// add_komponente( buttons+3 );
	// add_komponente( buttons+4 );
	// add_komponente( buttons+5 );


	set_resizemode(gui_frame_t::horizonal_resize);
	set_min_windowsize( koord(RIGHT_WIDTH, BOTTOM) );
	set_fenstergroesse( koord(RIGHT_WIDTH, BOTTOM) );
}


void color_gui_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse(groesse);
	const sint16 w = groesse.x;
	inp_underground_level.set_pos( koord(w-10-50, SLICE) );
	brightness.set_pos( koord(w-10-40,BRIGHTNESS) );
	scrollspeed.set_pos( koord(w-10-40,SCROLL_SPEED) );
	traffic_density.set_pos( koord(w-10-45,DENS_TRAFFIC) );
	buttons[1].set_pos( koord(w-10-10,CONVOI_TOOLTIPS) );
	buttons[13].set_pos( koord(w-10-10,HIDE_CITY_HOUSES) );
}


bool color_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	if(&brightness==komp) {
		umgebung_t::daynight_level = v.i;
	} else if(&traffic_density==komp) {
		if(  !umgebung_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			static char level[16];
			sprintf(level, "%li", v.i);
			werkzeug_t::simple_tool[WKZ_TRAFFIC_LEVEL&0xFFF]->set_default_param( level );
			welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TRAFFIC_LEVEL&0xFFF], welt->get_active_player() );
		}
		else {
			traffic_density.set_value(welt->get_settings().get_verkehr_level());
		}
	} else if(&scrollspeed==komp) {
		umgebung_t::scroll_multi = buttons[6].pressed ? -v.i : v.i;
	} else if((buttons+0)==komp) {
		umgebung_t::show_vehicle_states = (umgebung_t::show_vehicle_states+2)%3;
	} else if((buttons+1)==komp) {
		umgebung_t::show_vehicle_states = (umgebung_t::show_vehicle_states+1)%3;
	} else if((buttons+6)==komp) {
		buttons[6].pressed ^= 1;
		umgebung_t::scroll_multi = -umgebung_t::scroll_multi;
	} else if((buttons+7)==komp) {
		if(  !umgebung_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TOOGLE_PAX&0xFFF], welt->get_active_player() );
		}
	} else if((buttons+8)==komp) {
		if(  !umgebung_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TOOGLE_PEDESTRIANS&0xFFF], welt->get_active_player() );
		}
	} else if((buttons+9)==komp) {
		umgebung_t::night_shift = !umgebung_t::night_shift;
		buttons[9].pressed ^= 1;
	} else if((buttons+10)==komp) {
		umgebung_t::hide_with_transparency = !umgebung_t::hide_with_transparency;
		buttons[10].pressed ^= 1;
	} else if((buttons+11)==komp) {
		umgebung_t::hide_trees = !umgebung_t::hide_trees;
	} else if((buttons+12)==komp) {
		umgebung_t::hide_buildings = (umgebung_t::hide_buildings+2)%3;
	} else if((buttons+13)==komp) {
		umgebung_t::hide_buildings = (umgebung_t::hide_buildings+1)%3;
	} else if((buttons+14)==komp) {
		umgebung_t::use_transparency_station_coverage = !umgebung_t::use_transparency_station_coverage;
		buttons[14].pressed ^= 1;
	} else if((buttons+15)==komp) {
		umgebung_t::station_coverage_show = umgebung_t::station_coverage_show==0 ? 0xFF : 0;
	} else if((buttons+16)==komp) {
		// see simwerkz.cc::wkz_show_underground_t::init
		grund_t::set_underground_mode(buttons[16].pressed ? grund_t::ugm_none : grund_t::ugm_all, inp_underground_level.get_value());
		buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;
		// calc new images
		welt->update_map();
		// renew toolbar
		werkzeug_t::update_toolbars(welt);
	} else if((buttons+17)==komp) {
		grund_t::toggle_grid();
	} else if((buttons+18)==komp) {
		if(  umgebung_t::show_names&1  ) {
			if(  (umgebung_t::show_names>>2) == 2  ) {
				umgebung_t::show_names &= 2;
			}
			else {
				umgebung_t::show_names += 4;
			}
		}
		else {
			umgebung_t::show_names &= 2;
			umgebung_t::show_names |= 1;
		}
	} else if((buttons+19)==komp) {
		umgebung_t::show_names ^= 2;
	} else if((buttons+20)==komp) {
		// see simwerkz.cc::wkz_show_underground_t::init
		grund_t::set_underground_mode(buttons[20].pressed ? grund_t::ugm_none : grund_t::ugm_level, inp_underground_level.get_value());
		buttons[20].pressed = grund_t::underground_mode == grund_t::ugm_level;
		// calc new images
		welt->update_map();
		// renew toolbar
		werkzeug_t::update_toolbars(welt);
	} else if (komp == &inp_underground_level) {
		if(grund_t::underground_mode==grund_t::ugm_level) {
			grund_t::underground_level = inp_underground_level.get_value();
			// calc new images
			welt->update_map();
		}
	} else if ((buttons+21)==komp) {
		umgebung_t::left_to_right_graphs ^= 1;
		buttons[21].pressed = !umgebung_t::left_to_right_graphs;
	}

	welt->set_dirty();
	return true;
}


void color_gui_t::zeichnen(koord pos, koord gr)
{
	const int x = pos.x;
	const int y = pos.y+16;	// compensate for title bar
	char buf[128];

	// can be changed also with keys ...
	buttons[ 7].pressed = welt->get_settings().get_show_pax();
	buttons[ 8].pressed = welt->get_settings().get_random_pedestrians();
	buttons[11].pressed = umgebung_t::hide_trees;
	buttons[15].pressed = umgebung_t::station_coverage_show;
	buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[17].pressed = grund_t::show_grid;
	//buttons[18].pressed = umgebung_t::show_names&1;
	buttons[19].pressed = (umgebung_t::show_names&2)!=0;
	buttons[21].pressed = grund_t::underground_mode == grund_t::ugm_level;

	gui_frame_t::zeichnen(pos, gr);

	// draw the lable stype
	if(  umgebung_t::show_names&1  ) {
		PLAYER_COLOR_VAL pc = welt->get_active_player() ? welt->get_active_player()->get_player_color1()+4 : COL_ORANGE;
		const char *text = translator::translate("show station names");
		switch( umgebung_t::show_names >> 2 ) {
			case 0:
				display_ddd_proportional_clip( 16+x+buttons[18].get_pos().x, y+buttons[18].get_pos().y+(LINESPACE/2), proportional_string_width(text)+7, 0, pc, COL_BLACK, text, 1 );
				break;
			case 1:
				display_outline_proportional( 16+x+buttons[18].get_pos().x, y+buttons[18].get_pos().y, pc+1, COL_BLACK, text, 1 );
				break;
			case 2:
				display_outline_proportional( 16+x+buttons[18].get_pos().x+16, y+buttons[18].get_pos().y, COL_YELLOW, COL_BLACK, text, 1 );
				display_ddd_box_clip( 16+x+buttons[18].get_pos().x, y+buttons[18].get_pos().y, LINESPACE, LINESPACE, pc-2, pc+2 );
				display_fillbox_wh( 16+x+buttons[18].get_pos().x+1, y+buttons[18].get_pos().y+1, LINESPACE-2, LINESPACE-2, pc, 1 );
				break;
		}
	}

	// seperator
	const sint16 w = this->get_fenstergroesse().x;
	display_ddd_box_clip(x+10, y+SEPERATE1, w-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE2, w-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE3, w-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE4, w-20, 0, MN_GREY0, MN_GREY4);

	display_proportional_clip(x+10, y+BRIGHTNESS+1, translator::translate("1LIGHT_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);

	display_proportional_clip(x+10, y+SCROLL_SPEED+1, translator::translate("3LIGHT_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);

	display_proportional_clip(x+10, y+DENS_TRAFFIC+1, translator::translate("6WORLD_CHOOSE"), ALIGN_LEFT, COL_BLACK, true);

	const char *hhc = translator::translate( umgebung_t::hide_buildings==0 ? "no buildings hidden" : (umgebung_t::hide_buildings==1 ? "hide city building" : "hide all building") );
	display_proportional_clip(x+10+16, y+HIDE_CITY_HOUSES+1, hhc, ALIGN_LEFT, COL_BLACK, true);

	const char *ctc = translator::translate( umgebung_t::show_vehicle_states==0 ? "convoi error tooltips" : (umgebung_t::show_vehicle_states==1 ? "convoi mouseover tooltips" : "all convoi tooltips") );
	display_proportional_clip(x+10+16, y+CONVOI_TOOLTIPS+1, ctc, ALIGN_LEFT, COL_BLACK, true);

	int len=15+display_proportional_clip(x+10, y+FPS_DATA, translator::translate("Frame time:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf(buf,"%ld ms", get_frame_time() );
	display_proportional_clip(x+len, y+FPS_DATA, buf, ALIGN_LEFT, COL_WHITE, true);

	len = 15+display_proportional_clip(x+10, y+IDLE_DATA, translator::translate("Idle:"), ALIGN_LEFT, COL_BLACK, true);
	display_proportional_clip(x+len, y+IDLE_DATA, ntos(welt->get_schlaf_zeit(), "%d ms"), ALIGN_LEFT, COL_WHITE, true);

	uint8 farbe;
	uint32 loops;
	loops=welt->get_realFPS();
	farbe = COL_WHITE;
	uint32 target_fps = welt->is_fast_forward() ? 10 : umgebung_t::fps;
	if(loops<(target_fps*3)/4) {
		farbe = (loops<=target_fps/2) ? COL_RED : COL_YELLOW;
	}
	len = 15+display_proportional_clip(x+10, y+FRAME_DATA, translator::translate("FPS:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf(buf,"%d fps", loops );
	display_proportional_clip(x+len, y+FRAME_DATA, buf, ALIGN_LEFT, farbe, true);

	loops=welt->get_simloops();
	farbe = COL_WHITE;
	if(loops<=30) {
		farbe = (loops<=20) ? COL_RED : COL_YELLOW;
	}
	len = 15+display_proportional_clip(x+10, y+LOOP_DATA, translator::translate("Sim:"), ALIGN_LEFT, COL_BLACK, true);
	sprintf( buf, "%d%c%d", loops/10, get_fraction_sep(), loops%10 );
	display_proportional_clip(x+len, y+LOOP_DATA, buf, ALIGN_LEFT, farbe, true);

	// Added by : Knightly
	PLAYER_COLOR_VAL text_colour, figure_colour;

	text_colour = COL_BLACK;
	figure_colour = COL_BLUE;

	char status_string[32];
	if ( path_explorer_t::is_processing() )
	{
		tstrncpy(status_string, translator::translate( path_explorer_t::get_current_category_name() ), 15);
		strcat(status_string, " / ");
		strncat(status_string, translator::translate( path_explorer_t::get_current_phase_name() ), 8);
	}
	else
	{
		strcpy(status_string, "stand-by");
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
	display_proportional_clip(x+len, y+PATH_EXPLORE_STATUS, status_string, ALIGN_LEFT, figure_colour, true);

}
