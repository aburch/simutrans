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

#define CENTRALISED_SEARCH				(SEPERATE5+7)
#define USE_PERFORMANCE_COUNTER			(CENTRALISED_SEARCH+13)
#define PHASE_REBUILD_CONNEXIONS		(USE_PERFORMANCE_COUNTER+13)
#define PHASE_FILTER_ELIGIBLE			(PHASE_REBUILD_CONNEXIONS+13)
#define PHASE_FILL_MATRIX				(PHASE_FILTER_ELIGIBLE+13)
#define PHASE_EXPLORE_PATHS				(PHASE_FILL_MATRIX+13)
#define PHASE_REROUTE_GOODS				(PHASE_EXPLORE_PATHS+13)
#define PATH_EXPLORE_STATUS				(PHASE_REROUTE_GOODS+13)


#define BOTTOM							(PATH_EXPLORE_STATUS+30)

// x coordinates
#define RIGHT_WIDTH (220)
#define NUMBER_INP (170)


color_gui_t::color_gui_t(karte_t *welt) :
	gui_frame_t("Helligk. u. Farben")
{
	this->welt = welt;

	// brightness
	brightness.set_pos( koord(RIGHT_WIDTH-10-40,BRIGHTNESS) );
	brightness.set_groesse( koord( 40, BUTTON_HEIGHT-1 ) );
	brightness.set_value( umgebung_t::daynight_level );
	brightness.set_limits( 0, 9 );
	brightness.add_listener(this);
	add_komponente(&brightness);

	// scrollspeed
	scrollspeed.set_pos( koord(RIGHT_WIDTH-10-40,SCROLL_SPEED) );
	scrollspeed.set_groesse( koord( 40, BUTTON_HEIGHT-1 ) );
	scrollspeed.set_value( abs(umgebung_t::scroll_multi) );
	scrollspeed.set_limits( 1, 9 );
	scrollspeed.add_listener(this);
	add_komponente(&scrollspeed);

	// traffic density
	traffic_density.set_pos( koord(RIGHT_WIDTH-10-50,DENS_TRAFFIC) );
	traffic_density.set_groesse( koord( 50, BUTTON_HEIGHT-1 ) );
	traffic_density.set_value( welt->get_einstellungen()->get_verkehr_level() );
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);
	add_komponente(&traffic_density);

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
	buttons[b].pressed = welt->get_einstellungen()->get_show_pax();
	buttons[b].set_tooltip("Pedestrians will appear near stops whenver a passenger vehicle unloads there.");

	//8
	buttons[++b].set_pos( koord(10,CITY_WALKER) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("6LIGHT_CHOOSE");
	buttons[b].pressed = welt->get_einstellungen()->get_random_pedestrians();
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
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show station names");
	buttons[b].pressed = umgebung_t::show_names&1;
	buttons[b].set_tooltip("Shows the names of the individual stations in the main game window.");

	//19	
	buttons[++b].set_pos( koord(10,SHOW_STATION_GOODS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("show waiting bars");
	buttons[b].pressed = umgebung_t::show_names&1;
	buttons[b].set_tooltip("Shows a bar graph representing the number of passengers/mail/goods waiting at stops.");

	//20
	buttons[++b].set_pos( koord(10,CENTRALISED_SEARCH) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("Centralised path searching");
	buttons[b].pressed = welt->get_einstellungen()->get_default_path_option() == 2;
	buttons[b].set_tooltip("Use centralised instead of distributed path searching system.");

	//21
	buttons[++b].set_pos( koord(10,SLICE) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("sliced underground mode");
	buttons[b].set_tooltip("See under the ground, one layer at a time. Toggle with CTRL + U. Move up/down in layers with HOME and END.");

	// Added by : Knightly
	//22
	buttons[++b].set_pos( koord(10,USE_PERFORMANCE_COUNTER) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("Use performance counter");
	buttons[b].pressed = umgebung_t::default_einstellungen.get_system_time_option() == 1;
	buttons[b].set_tooltip("Read-only option for Windows/GDI version only: configurable in simuconf.tab");
	buttons[b].set_read_only(true);
#if ( !WIN32 || SDL )
	buttons[b].disable();
#endif
	
	//23
	buttons[++b].set_pos( koord(10, LEFT_TO_RIGHT_GRAPHS) );
	buttons[b].set_typ(button_t::square_state);
	buttons[b].set_text("Inverse graphs");
	buttons[b].pressed = !umgebung_t::left_to_right_graphs;
	buttons[b].set_tooltip("Graphs right to left instead of left to right");
	
	//24
	//buttons[++b].set_pos( koord(10, LEFT_TO_RIGHT_GRAPHS) );
	//buttons[b].set_typ(button_t::square_state);
	//buttons[b].set_text("Inverse graphs (other)");
	//buttons[b].pressed = !umgebung_t::left_to_right_graphs;
	//buttons[b].set_tooltip("Graphs showing non-financial information will appear from right to left instead of left to right");

	inp_underground_level.set_pos(koord(NUMBER_INP, SLICE) );
	inp_underground_level.set_groesse( koord(50,12));
	inp_underground_level.set_limits(welt->get_grundwasser()-10, 32);
	inp_underground_level.set_value( grund_t::underground_mode==grund_t::ugm_level ? grund_t::underground_level : welt->get_zeiger()->get_pos().z);
	add_komponente(&inp_underground_level);
	inp_underground_level.add_listener(this);

	// left/right for convoi tooltips
	buttons[0].set_pos( koord(10,CONVOI_TOOLTIPS) );
	buttons[0].set_typ(button_t::arrowleft);
	buttons[1].set_pos( koord(RIGHT_WIDTH-10-10,CONVOI_TOOLTIPS) );
	buttons[1].set_typ(button_t::arrowright);

	for(int i=0;  i<COLORS_MAX_BUTTONS;  i++ ) {
		buttons[i].add_listener(this);
		add_komponente( buttons+i );
	}

	set_fenstergroesse( koord(RIGHT_WIDTH, BOTTOM) );
}



bool
color_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	einstellungen_t * sets = welt->get_einstellungen();

	if(&brightness==komp) {
		umgebung_t::daynight_level = v.i;
	} else if(&traffic_density==komp) {
		sets->set_verkehr_level( v.i );
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
		welt->get_einstellungen()->set_show_pax( !welt->get_einstellungen()->get_show_pax() );
		buttons[7].pressed ^= 1;
	} else if((buttons+8)==komp) {
		welt->get_einstellungen()->set_random_pedestrians( !welt->get_einstellungen()->get_random_pedestrians() );
		buttons[8].pressed ^= 1;
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
		umgebung_t::show_names ^= 1;
	} else if((buttons+19)==komp) {
		umgebung_t::show_names ^= 2;
	} else if((buttons+21)==komp) {
		// see simwerkz.cc::wkz_show_underground_t::init
		grund_t::set_underground_mode(buttons[21].pressed ? grund_t::ugm_none : grund_t::ugm_level, inp_underground_level.get_value());
		buttons[21].pressed = grund_t::underground_mode == grund_t::ugm_level;
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
	} else if ((buttons+23)==komp) {
		umgebung_t::left_to_right_graphs ^= 1;
		buttons[23].pressed = !umgebung_t::left_to_right_graphs;
	} else if ((buttons+24)==komp) {
		umgebung_t::left_to_right_graphs ^= 1;
		buttons[24].pressed = !umgebung_t::left_to_right_graphs;
	}

	else if((buttons+20)==komp)
	{
		const uint8 current_option = welt->get_einstellungen()->get_default_path_option();
		if(current_option == 1)
		{
			welt->get_einstellungen()->set_default_path_option(2);
			buttons[20].pressed = true;
			path_explorer_t::full_instant_refresh();
		}
		else
		{
			welt->get_einstellungen()->set_default_path_option(1);
			buttons[20].pressed = false;
			haltestelle_t::prepare_pathing_data_structures();
		}
		
	}

	welt->set_dirty();
	return true;
}


void color_gui_t::zeichnen(koord pos, koord gr)
{
	const int x = pos.x;
	const int y = pos.y+16;	// compensate for title bar
	const einstellungen_t * sets = welt->get_einstellungen();
	char buf[128];

	// can be changed also with keys ...
	buttons[11].pressed = umgebung_t::hide_trees;
	buttons[15].pressed = umgebung_t::station_coverage_show;
	buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[17].pressed = grund_t::show_grid;
	buttons[18].pressed = umgebung_t::show_names&1;
	buttons[19].pressed = (umgebung_t::show_names&2)!=0;
	buttons[21].pressed = grund_t::underground_mode == grund_t::ugm_level;

	gui_frame_t::zeichnen(pos, gr);

	// seperator
	display_ddd_box_clip(x+10, y+SEPERATE1, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE2, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE3, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE4, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);
	display_ddd_box_clip(x+10, y+SEPERATE5, RIGHT_WIDTH-20, 0, MN_GREY0, MN_GREY4);

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
	if ( welt->get_einstellungen()->get_default_path_option() == 2 )
	{
		text_colour = COL_BLACK;
		figure_colour = COL_BLUE;
	}
	else
	{
		text_colour = MN_GREY0;
		figure_colour = COL_LIGHT_BLUE;
	}

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
