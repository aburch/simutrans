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
#include "../simgraph.h"
#include "../simmenu.h"

#include "../utils/simstring.h"

// y coordinates
#define GRID_MODE						(0*13+6)
#define UNDERGROUND						(1*13+6)
#define SLICE							(2*13+6)
#define DAY_NIGHT						(3*13+6)
#define BRIGHTNESS						(4*13+6)
#define SCROLL_INVERS					(5*13+6)
#define SCROLL_SPEED					(6*13+6)

#define SEPERATE1						(7*13+6)


#define USE_TRANSPARENCY				(7*13+6+4)
#define HIDE_TREES						(8*13+6+4)
#define HIDE_CITY_HOUSES				(9*13+6+4)

#define SEPERATE2						(10*13+6+4)

#define USE_TRANSPARENCY_STATIONS		(10*13+6+2*4)
#define SHOW_STATION_COVERAGE			(11*13+6+2*4)
#define SHOW_STATION_SIGNS				(12*13+6+2*4)
#define SHOW_STATION_GOODS				(13*13+6+2*4)

#define SEPERATE3						(14*13+6+2*4)

#define CITY_WALKER						(14*13+6+3*4)
#define STOP_WALKER						(15*13+6+3*4)
#define DENS_TRAFFIC					(16*13+6+3*4)
#define CONVOI_TOOLTIPS					(17*13+6+3*4)

#define SEPERATE4						(18*13+6+3*4)

#define FPS_DATA						(18*13+6+4*4)
#define IDLE_DATA						(19*13+6+4*4)
#define FRAME_DATA						(20*13+6+4*4)
#define LOOP_DATA						(21*13+6+4*4)

#define SEPERATE5						(22*13+6+4*4)

#define CENTRALIZED_SEARCH				(22*13+6+5*4)
#define USE_PERFORMANCE_COUNTER			(23*13+6+5*4)
#define PHASE_SORT_ELIGIBLE				(24*13+6+5*4)
#define PHASE_FILL_MATRIX				(25*13+6+5*4)
#define PHASE_PATH_EXPLORE				(26*13+6+5*4)
#define PHASE_WARE_REROUTE				(27*13+6+5*4)
#define PATH_EXPLORE_STATUS				(28*13+6+5*4)

#define BOTTOM							(29*13+6+12+6*4)


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

	// other settings
	buttons[6].set_pos( koord(10,SCROLL_INVERS) );
	buttons[6].set_typ(button_t::square_state);
	buttons[6].set_text("4LIGHT_CHOOSE");
	buttons[6].pressed = umgebung_t::scroll_multi < 0;
	buttons[6].set_tooltip("The main game window can be scrolled by right-clicking and dragging the ground.");

	buttons[7].set_pos( koord(10,STOP_WALKER) );
	buttons[7].set_typ(button_t::square_state);
	buttons[7].set_text("5LIGHT_CHOOSE");
	buttons[7].pressed = welt->get_einstellungen()->get_show_pax();
	buttons[7].set_tooltip("Pedestrians will appear near stops whenver a passenger vehicle unloads there.");

	buttons[8].set_pos( koord(10,CITY_WALKER) );
	buttons[8].set_typ(button_t::square_state);
	buttons[8].set_text("6LIGHT_CHOOSE");
	buttons[8].pressed = welt->get_einstellungen()->get_random_pedestrians();
	buttons[8].set_tooltip("Pedestrians will appear randomly in towns.");

	buttons[9].set_pos( koord(10,DAY_NIGHT) );
	buttons[9].set_typ(button_t::square_state);
	buttons[9].set_text("8WORLD_CHOOSE");
	buttons[9].pressed = umgebung_t::night_shift;
	buttons[9].set_tooltip("Whether the lighting in the main game window simulates a periodic transition between day and night.");

	buttons[10].set_pos( koord(10,USE_TRANSPARENCY) );
	buttons[10].set_typ(button_t::square_state);
	buttons[10].set_text("hide transparent");
	buttons[10].pressed = umgebung_t::hide_with_transparency;
	buttons[10].set_tooltip("All hidden items (such as trees and buildings) will appear as transparent.");

	buttons[11].set_pos( koord(10,HIDE_TREES) );
	buttons[11].set_typ(button_t::square_state);
	buttons[11].set_text("hide trees");
	buttons[11].set_tooltip("Trees will be miniaturised or made transparent in the main game window.");

	// left right for hide messages
	buttons[12].set_pos( koord(10,HIDE_CITY_HOUSES) );
	buttons[12].set_typ(button_t::arrowleft);
	buttons[13].set_pos( koord(RIGHT_WIDTH-10-10,HIDE_CITY_HOUSES) );
	buttons[13].set_typ(button_t::arrowright);

	buttons[14].set_pos( koord(10,USE_TRANSPARENCY_STATIONS) );
	buttons[14].set_typ(button_t::square_state);
	buttons[14].set_text("transparent station coverage");
	buttons[14].pressed = umgebung_t::use_transparency_station_coverage;
	buttons[14].set_tooltip("The display of the station coverage can either be a transparent rectangle or a series of boxes.");

	buttons[15].set_pos( koord(10,SHOW_STATION_COVERAGE) );
	buttons[15].set_typ(button_t::square_state);
	buttons[15].set_text("show station coverage");
	buttons[15].set_tooltip("Show from how far that passengers or goods will come to use your stops. Toggle with the v key.");

	buttons[16].set_pos( koord(10,UNDERGROUND) );
	buttons[16].set_typ(button_t::square_state);
	buttons[16].set_text("underground mode");
	buttons[16].set_tooltip("See under the ground, to build tunnels and underground railways/metros. Toggle with SHIFT + U");

	buttons[17].set_pos( koord(10,GRID_MODE) );
	buttons[17].set_typ(button_t::square_state);
	buttons[17].set_text("show grid");
	buttons[17].set_tooltip("Shows the borderlines of each tile in the main game window. Can be useful for construction. Toggle with the # key.");

	buttons[18].set_pos( koord(10,SHOW_STATION_SIGNS) );
	buttons[18].set_typ(button_t::square_state);
	buttons[18].set_text("show station names");
	buttons[18].pressed = umgebung_t::show_names&1;
	buttons[18].set_tooltip("Shows the names of the individual stations in the main game window.");

	buttons[19].set_pos( koord(10,SHOW_STATION_GOODS) );
	buttons[19].set_typ(button_t::square_state);
	buttons[19].set_text("show waiting bars");
	buttons[19].pressed = umgebung_t::show_names&1;
	buttons[19].set_tooltip("Shows a bar graph representing the number of passengers/mail/goods waiting at stops.");

	buttons[20].set_pos( koord(10,CENTRALIZED_SEARCH) );
	buttons[20].set_typ(button_t::square_state);
	buttons[20].set_text("Centralised path searching");
	buttons[20].pressed = welt->get_einstellungen()->get_default_path_option() == 2;
	buttons[20].set_tooltip("Use centralized instead of distributed path searching system.");

	buttons[21].set_pos( koord(10,SLICE) );
	buttons[21].set_typ(button_t::square_state);
	buttons[21].set_text("sliced underground mode");
	buttons[21].set_tooltip("See under the ground, one layer at a time. Toggle with CTRL + U. Move up/down in layers with HOME and END.");

	// Added by : Knightly
	buttons[22].set_pos( koord(10,USE_PERFORMANCE_COUNTER) );
	buttons[22].set_typ(button_t::square_state);
	buttons[22].set_text("Use performance counter");
	buttons[22].pressed = umgebung_t::default_einstellungen.get_system_time_option() == 1;
	buttons[22].set_tooltip("Read-only option : need to be set in simuconf.tab");
	buttons[22].set_read_only(true);
#if ( !WIN32 && !SDL )
	buttons[22].disable();
#endif

	inp_underground_level.set_pos(koord(NUMBER_INP, SLICE) );
	inp_underground_level.set_groesse( koord(50,12));
	inp_underground_level.set_limits(welt->get_grundwasser(), 20);
	inp_underground_level.set_value( grund_t::underground_level );
	add_komponente(&inp_underground_level);
	inp_underground_level.add_listener(this);

	// left/right for convoi tooltips
	buttons[0].set_pos( koord(10,CONVOI_TOOLTIPS) );
	buttons[0].set_typ(button_t::arrowleft);
	buttons[1].set_pos( koord(RIGHT_WIDTH-10-10,CONVOI_TOOLTIPS) );
	buttons[1].set_typ(button_t::arrowright);

	for(int i=0;  i<MAX_BUTTONS;  i++ ) {
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


	len = 15+display_proportional_clip(x+10, y+PHASE_SORT_ELIGIBLE, translator::translate("Sort eligible halts:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_SORT_ELIGIBLE, ntos(path_explorer_t::get_limit_sort_eligible(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_FILL_MATRIX, translator::translate("Fill path matrix:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_FILL_MATRIX, ntos(path_explorer_t::get_limit_fill_matrix(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_PATH_EXPLORE, translator::translate("Path exploration:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_PATH_EXPLORE, ntos(path_explorer_t::get_limit_path_explore(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PHASE_WARE_REROUTE, translator::translate("Goods re-route:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PHASE_WARE_REROUTE, ntos(path_explorer_t::get_limit_ware_reroute(), "%lu"), ALIGN_LEFT, figure_colour, true);

	len = 15+display_proportional_clip(x+10, y+PATH_EXPLORE_STATUS, translator::translate("Status:"), ALIGN_LEFT, text_colour, true);
	display_proportional_clip(x+len, y+PATH_EXPLORE_STATUS, translator::translate( path_explorer_t::is_processing() ? "Processing ..." : "Stand-by" ), ALIGN_LEFT, figure_colour, true);

}
