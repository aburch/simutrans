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
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../dings/baum.h"
#include "../dings/zeiger.h"
#include "../display/simgraph.h"
#include "../simmenu.h"
#include "../player/simplay.h"
#include "../utils/simstring.h"

#include "themeselector.h"
#include "simwin.h"

// Local params
#define L_DIALOG_WIDTH (220)


color_gui_t::color_gui_t(karte_t *welt) :
gui_frame_t( translator::translate("Helligk. u. Farben") )
{
	koord cursor = koord( D_MARGIN_LEFT, D_MARGIN_TOP );
	this->welt = welt;

	// Use one of the edit controls to calculate width
	inp_underground_level.set_width_by_len(3);
	const scr_coord_val edit_width = inp_underground_level.get_groesse().x;
	const scr_coord_val label_width = L_DIALOG_WIDTH - D_MARGINS_X - D_H_SPACE - edit_width;

	// Max Kielland: No need to put right aligned controls in place here.
	// They will be positioned in the resize window function.

	// Show thememanager
	buttons[23].set_pos( cursor );
	buttons[23].set_typ(button_t::roundbox_state);
	buttons[23].set_text("Select a theme for display");
	buttons[23].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	cursor.y += D_BUTTON_HEIGHT + D_V_SPACE;

	// Show grid checkbox
	buttons[17].set_pos( cursor );
	buttons[17].set_typ(button_t::square_state);
	buttons[17].set_text("show grid");
	buttons[17].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Underground view checkbox
	buttons[16].set_pos( cursor );
	buttons[16].set_typ(button_t::square_state);
	buttons[16].set_text("underground mode");
	buttons[16].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Show slice map view checkbox
	buttons[20].set_pos( cursor );
	buttons[20].set_typ(button_t::square_state);
	buttons[20].set_text("sliced underground mode");
	buttons[20].set_width( label_width );

	// underground slice edit
	inp_underground_level.set_pos( cursor );
	inp_underground_level.set_width( edit_width );
	inp_underground_level.align_to(&buttons[20], ALIGN_CENTER_V);
	inp_underground_level.set_value( grund_t::underground_mode==grund_t::ugm_level ? grund_t::underground_level : welt->get_zeiger()->get_pos().z);
	inp_underground_level.set_limits(welt->get_grundwasser()-10, 32);
	inp_underground_level.add_listener(this);
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Day/night change checkbox
	buttons[9].set_pos( cursor );
	buttons[9].set_typ(button_t::square_state);
	buttons[9].set_text("8WORLD_CHOOSE");
	buttons[9].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[9].pressed = environment_t::night_shift;
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Brightness label
	brightness_label.init("1LIGHT_CHOOSE",cursor);
	brightness_label.set_width( label_width );
	add_komponente(&brightness_label);
	cursor.y += LINESPACE + D_V_SPACE;

	// brightness edit
	brightness.set_pos( cursor );
	brightness.set_width( edit_width );
	brightness.align_to(&brightness_label, ALIGN_CENTER_V);
	brightness.set_value( environment_t::daynight_level );
	brightness.set_limits( 0, 9 );
	brightness.add_listener(this);

	// Scroll inverse checkbox
	buttons[6].set_pos( cursor );
	buttons[6].set_typ(button_t::square_state);
	buttons[6].set_text("4LIGHT_CHOOSE");
	buttons[6].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[6].pressed = environment_t::scroll_multi < 0;
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Scroll speed label
	scrollspeed_label.init("3LIGHT_CHOOSE",cursor);
	scrollspeed_label.set_width( label_width );
	add_komponente(&scrollspeed_label);
	cursor.y += LINESPACE;

	// Scroll speed edit
	scrollspeed.set_pos( cursor );
	scrollspeed.set_width( edit_width );
	scrollspeed.align_to(&scrollspeed_label, ALIGN_CENTER_V);
	scrollspeed.set_value( abs(environment_t::scroll_multi) );
	scrollspeed.set_limits( 1, 9 );
	scrollspeed.add_listener(this);

	// Divider 1
	divider1.set_pos( cursor );
	add_komponente(&divider1);
	cursor.y += D_DIVIDER_HEIGHT;

	// Transparent instead of hidden checkbox
	buttons[10].set_pos( cursor );
	buttons[10].set_typ(button_t::square_state);
	buttons[10].set_text("hide transparent");
	buttons[10].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[10].pressed = environment_t::hide_with_transparency;
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Hide trees checkbox
	buttons[11].set_pos( cursor );
	buttons[11].set_typ(button_t::square_state);
	buttons[11].set_text("hide trees");
	buttons[11].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Hide buildings arrows
	buttons[12].set_pos( cursor );
	buttons[12].set_typ(button_t::arrowleft);
	buttons[13].set_pos( cursor );
	buttons[13].set_typ(button_t::arrowright);

	// Hide buildings label
	hide_buildings_label.set_pos( cursor + koord (buttons[12].get_groesse().x + D_H_SPACE,0));
	hide_buildings_label.align_to(&buttons[12], ALIGN_CENTER_V);
	add_komponente(&hide_buildings_label);
	cursor.y += buttons[12].get_groesse().y + D_V_SPACE;

	// Hide buildings and trees under mouse cursor checkbox
	buttons[21].set_pos( cursor );
	buttons[21].set_typ( button_t::square_state );
	buttons[21].set_text( "Smart hide objects" );
	buttons[21].set_width( label_width );
	buttons[21].set_tooltip( "hide objects under cursor" );

	// Smart hide objects edit
	cursor_hide_range.set_pos( cursor );
	cursor_hide_range.set_width( edit_width );
	cursor_hide_range.align_to(&buttons[21], ALIGN_CENTER_V);
	cursor_hide_range.set_value(environment_t::cursor_hide_range);
	cursor_hide_range.set_limits( 0, 10 );
	cursor_hide_range.add_listener(this);
	cursor.y += D_BUTTON_SQUARE;

	// Divider 2
	divider2.set_pos( cursor );
	add_komponente(&divider2);
	cursor.y += D_DIVIDER_HEIGHT;

	// Transparent station coverage
	buttons[14].set_pos( cursor );
	buttons[14].set_typ(button_t::square_state);
	buttons[14].set_text("transparent station coverage");
	buttons[14].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[14].pressed = environment_t::use_transparency_station_coverage;
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Show station coverage
	buttons[15].set_pos( cursor );
	buttons[15].set_typ(button_t::square_state);
	buttons[15].set_text("show station coverage");
	buttons[15].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Show station names arrow
	buttons[18].set_pos( cursor );
	buttons[18].set_typ(button_t::arrowright);
	buttons[18].set_tooltip("show station names");
	cursor.y += buttons[18].get_groesse().y + D_V_SPACE;

	// Show waiting bars checkbox
	buttons[19].set_pos( cursor );
	buttons[19].set_typ(button_t::square_state);
	buttons[19].set_text("show waiting bars");
	buttons[19].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[19].pressed = environment_t::show_names&2;
	cursor.y += D_BUTTON_SQUARE;

	// Divider 3
	divider3.set_pos( cursor );
	add_komponente(&divider3);
	cursor.y += D_DIVIDER_HEIGHT;

	// Pedestrians in towns checkbox
	buttons[8].set_pos( cursor );
	buttons[8].set_typ(button_t::square_state);
	buttons[8].set_text("6LIGHT_CHOOSE");
	buttons[8].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[8].pressed = welt->get_settings().get_random_pedestrians();
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Pedestrians at stops checkbox
	buttons[7].set_pos( cursor );
	buttons[7].set_typ(button_t::square_state);
	buttons[7].set_text("5LIGHT_CHOOSE");
	buttons[7].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	buttons[7].pressed = welt->get_settings().get_show_pax();
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Traffic density label
	traffic_density_label.init("6WORLD_CHOOSE",cursor);
	traffic_density_label.set_width( label_width );
	add_komponente(&traffic_density_label);

	// Traffic density edit
	traffic_density.set_pos( cursor );
	traffic_density.set_width( edit_width );
	traffic_density.align_to(&traffic_density_label, ALIGN_CENTER_V);
	traffic_density.set_value(welt->get_settings().get_verkehr_level());
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	// Convoy tooltip left/right arrows
	buttons[0].set_pos( cursor );
	buttons[0].set_typ(button_t::arrowleft);
	buttons[1].set_pos( cursor );
	buttons[1].set_typ(button_t::arrowright);

	// Convoy tooltip label
	convoy_tooltip_label.init("", cursor + koord (buttons[0].get_groesse().x + D_H_SPACE,0) );
	convoy_tooltip_label.align_to(&buttons[0], ALIGN_CENTER_V);
	add_komponente(&convoy_tooltip_label);
	cursor.y += buttons[0].get_groesse().y + D_V_SPACE;

	// Show schedule's stop checkbox
	buttons[22].set_pos( cursor );
	buttons[22].set_typ( button_t::square_state );
	buttons[22].set_text( "Highlite schedule" );
	buttons[22].set_width( L_DIALOG_WIDTH - D_MARGINS_X );
	cursor.y += D_BUTTON_SQUARE;

	// Divider 4
	divider4.set_pos( cursor );
	add_komponente(&divider4);
	cursor.y += D_DIVIDER_HEIGHT;

	// add controls to info container
	label_container.set_pos( cursor );
	koord label_cursor = koord(0,0);

	// Frame time label
	frame_time_label.init("Frame time:", label_cursor, COL_BLACK );
	sprintf(frame_time_buf," ***** ms" );
	frame_time_value_label.init( frame_time_buf, koord(0, label_cursor.y), SYSCOL_TEXT_HIGHLIGHT );
	label_container.add_komponente( &frame_time_label );
	value_container.add_komponente( &frame_time_value_label );
	label_cursor.y += LINESPACE;

	// Idle time label
	idle_time_label.init("Idle:", label_cursor, COL_BLACK);
	sprintf(idle_time_buf," ***** ms" );
	idle_time_value_label.init( idle_time_buf, koord(0, label_cursor.y), SYSCOL_TEXT_HIGHLIGHT );
	label_container.add_komponente( &idle_time_label );
	value_container.add_komponente( &idle_time_value_label );
	label_cursor.y += LINESPACE;

	// FPS label
	fps_label.init("FPS:", label_cursor, COL_BLACK );
	sprintf(fps_buf," *** fps*" );
	fps_value_label.init( fps_buf, koord(0, label_cursor.y), SYSCOL_TEXT_HIGHLIGHT );
	label_container.add_komponente( &fps_label );
	value_container.add_komponente( &fps_value_label );
	label_cursor.y += LINESPACE;

	// Simloops label
	simloops_label.init("Sim:", label_cursor, COL_BLACK );
	sprintf(simloops_buf," ********" );
	simloops_value_label.init( simloops_buf, koord(0, label_cursor.y), SYSCOL_TEXT_HIGHLIGHT );
	label_container.add_komponente( &simloops_label );
	value_container.add_komponente( &simloops_value_label );
	label_cursor.y += LINESPACE;

	// Align all values with labels
	scr_rect bounds = label_container.get_min_boundaries();
	label_container.set_groesse( koord( bounds.get_width(), bounds.get_height() ) );
	value_container.set_pos( label_container.get_pos() + koord( bounds.get_width()+D_H_SPACE, 0 ) );
//	value_container.align_to( &label_container, ALIGN_EXTERIOR_H | ALIGN_LEFT | ALIGN_TOP, koord( D_H_SPACE, 0 ) );
	value_container.set_groesse( koord( L_DIALOG_WIDTH - D_MARGINS_X - label_container.get_groesse().x - D_H_SPACE, bounds.get_height() ) );

	for(  int i = 0;  i < COLORS_MAX_BUTTONS;  i++  ) {
		buttons[i].add_listener(this);
	}

	// add buttons for sensible keyboard tab order
	add_komponente( buttons+23 );
	add_komponente( buttons+17 );
	add_komponente( buttons+16 );
	add_komponente( &inp_underground_level );
	add_komponente( buttons+20 );
	add_komponente( buttons+9 );
	add_komponente( &brightness );
	add_komponente( buttons+6 );
	add_komponente( &scrollspeed );
	add_komponente( buttons+10 );
	add_komponente( buttons+11 );
	add_komponente( &cursor_hide_range );
	add_komponente( buttons+21);
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
	add_komponente( buttons+22);

	// unused buttons
	// add_komponente( buttons+2 );
	// add_komponente( buttons+3 );
	// add_komponente( buttons+4 );
	// add_komponente( buttons+5 );

	add_komponente( &label_container );
	add_komponente( &value_container );
	cursor.y += label_container.get_groesse().y;

	set_resizemode(gui_frame_t::horizonal_resize);
	set_min_windowsize( koord(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM) );
	set_fenstergroesse( koord(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM) );
}


void color_gui_t::set_fenstergroesse(koord groesse)
{
	scr_coord_val column;
	scr_coord_val delta_w = groesse.x - D_MARGINS_X;

	for(  int i=0;  i<COLORS_MAX_BUTTONS;  i++  ) {
		if(  buttons[i].get_type() == button_t::square_state  ) {
			// resize buttons too to fix text
			buttons[i].set_groesse( koord(delta_w,buttons[i].get_groesse().y) );
		}
	}

	gui_frame_t::set_fenstergroesse(groesse);

	column = groesse.x - D_MARGIN_RIGHT - inp_underground_level.get_groesse().x;
	inp_underground_level.set_pos ( koord( column, inp_underground_level.get_pos().y ) );
	brightness.set_pos            ( koord( column, brightness.get_pos().y            ) );
	scrollspeed.set_pos           ( koord( column, scrollspeed.get_pos().y           ) );
	traffic_density.set_pos       ( koord( column, traffic_density.get_pos().y       ) );
	cursor_hide_range.set_pos     ( koord( column, cursor_hide_range.get_pos().y     ) );

	column = groesse.x - D_MARGIN_RIGHT - gui_theme_t::gui_arrow_right_size.x;
	buttons[1].set_pos            ( koord( column, buttons[1].get_pos().y            ) );
	buttons[13].set_pos           ( koord( column, buttons[13].get_pos().y           ) );

	column = groesse.x - D_MARGINS_X;
	divider1.set_width            ( column );
	divider2.set_width            ( column );
	divider3.set_width            ( column );
	divider4.set_width            ( column );

}


bool color_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{

	// Brightness edit
	if(&brightness==komp) {
	  environment_t::daynight_level = (sint8)v.i;
	} else

	// Traffic density edit
	if(&traffic_density==komp) {
		if(  !environment_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			static char level[16];
			sprintf(level, "%li", v.i);
			werkzeug_t::simple_tool[WKZ_TRAFFIC_LEVEL&0xFFF]->set_default_param( level );
			welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TRAFFIC_LEVEL&0xFFF], welt->get_active_player() );
		}
		else {
			traffic_density.set_value(welt->get_settings().get_verkehr_level());
		}
	} else

	// Scroll speed edit
	if(&scrollspeed==komp) {
		environment_t::scroll_multi = (sint16)( buttons[6].pressed ? -v.i : v.i );
	} else

	// Smart hide objects edit
	if(&cursor_hide_range==komp) {
		environment_t::cursor_hide_range = cursor_hide_range.get_value();
	} else

	// Convoy tooltip left arrows
	if((buttons+0)==komp) {
		environment_t::show_vehicle_states = (environment_t::show_vehicle_states+2)%3;
	} else

	// Convoy tooltip right arrow
	if((buttons+1)==komp) {
		environment_t::show_vehicle_states = (environment_t::show_vehicle_states+1)%3;
	} else

	// Scroll inverse checkbox
	if((buttons+6)==komp) {
		buttons[6].pressed ^= 1;
		environment_t::scroll_multi = -environment_t::scroll_multi;
	} else

	// Pedestrians at stops checkbox
	if((buttons+7)==komp) {
		if(  !environment_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TOOGLE_PAX&0xFFF], welt->get_active_player() );
		}
	} else

	// Pedestrians in towns checkbox
	if((buttons+8)==komp) {
		if(  !environment_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_werkzeug( werkzeug_t::simple_tool[WKZ_TOOGLE_PEDESTRIANS&0xFFF], welt->get_active_player() );
		}
	} else

	// Day/night change checkbox
	if((buttons+9)==komp) {
		environment_t::night_shift = !environment_t::night_shift;
		buttons[9].pressed ^= 1;
	} else

	// Transparent instead of hidden checkbox
	if((buttons+10)==komp) {
		environment_t::hide_with_transparency = !environment_t::hide_with_transparency;
		buttons[10].pressed ^= 1;
		baum_t::recalc_outline_color();
	} else

	// Hide trees checkbox
	if((buttons+11)==komp) {
		environment_t::hide_trees = !environment_t::hide_trees;
		baum_t::recalc_outline_color();
	} else

	// Hide buildings left arrows
	if((buttons+12)==komp) {
		environment_t::hide_buildings = (environment_t::hide_buildings+2)%3;
	} else

	// Hide buildings right arrows
	if((buttons+13)==komp) {
		environment_t::hide_buildings = (environment_t::hide_buildings+1)%3;
	} else

	// Transparent station coverage
	if((buttons+14)==komp) {
		environment_t::use_transparency_station_coverage = !environment_t::use_transparency_station_coverage;
		buttons[14].pressed ^= 1;
	} else

	// Show station coverage
	if((buttons+15)==komp) {
		environment_t::station_coverage_show = environment_t::station_coverage_show==0 ? 0xFF : 0;
	} else

	// Underground view checkbox
	if((buttons+16)==komp) {

		// see simwerkz.cc::wkz_show_underground_t::init
		grund_t::set_underground_mode(buttons[16].pressed ? grund_t::ugm_none : grund_t::ugm_all, inp_underground_level.get_value());
		buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;

		// calc new images
		welt->update_map();

		// renew toolbar
		werkzeug_t::update_toolbars(welt);
	} else

	// Show grid checkbox
	if((buttons+17)==komp) {
		grund_t::toggle_grid();
	} else

	// Show station names arrow
	if((buttons+18)==komp) {
		if(  environment_t::show_names&1  ) {
			if(  (environment_t::show_names>>2) == 2  ) {
				environment_t::show_names &= 2;
			}
			else {
				environment_t::show_names += 4;
			}
		}
		else {
			environment_t::show_names &= 2;
			environment_t::show_names |= 1;
		}
	} else

	// Show waiting bars checkbox
	if((buttons+19)==komp) {
		environment_t::show_names ^= 2;
	} else

	// Show slice map view checkbox
	if((buttons+20)==komp) {

		// see simwerkz.cc::wkz_show_underground_t::init
		grund_t::set_underground_mode(buttons[20].pressed ? grund_t::ugm_none : grund_t::ugm_level, inp_underground_level.get_value());
		buttons[20].pressed = grund_t::underground_mode == grund_t::ugm_level;

		// calc new images
		welt->update_map();

		// renew toolbar
		werkzeug_t::update_toolbars(welt);
	} else

	// Hide buildings and trees under mouse cursor checkbox
	if((buttons+21)==komp) {

		// see simwerkz.cc::wkz_hide_under_cursor_t::init
		environment_t::hide_under_cursor = !environment_t::hide_under_cursor  &&  environment_t::cursor_hide_range>0;
		buttons[21].pressed = environment_t::hide_under_cursor;

		// renew toolbar
		werkzeug_t::update_toolbars(welt);
	} else

	// Show schedule's stop checkbox
	if((buttons+22)==komp) {
		environment_t::visualize_schedule = !environment_t::visualize_schedule;
		buttons[22].pressed = environment_t::visualize_schedule;
	} else

	// underground slice edit
	if (komp == &inp_underground_level) {
		if(grund_t::underground_mode==grund_t::ugm_level) {
			grund_t::underground_level = inp_underground_level.get_value();

			// calc new images
			welt->update_map();
		}
	} else

	if((buttons+23)==komp) {
		create_win(new themeselector_t(), w_info, magic_themes);
	}

	welt->set_dirty();
	return true;
}


void color_gui_t::zeichnen(koord pos, koord gr)
{
	const int x = pos.x;
	const int y = pos.y + D_TITLEBAR_HEIGHT; // compensate for title bar

	// Update button states that was changed with keyboard ...
	buttons[ 7].pressed = welt->get_settings().get_show_pax();
	buttons[ 8].pressed = welt->get_settings().get_random_pedestrians();
	buttons[11].pressed = environment_t::hide_trees;
	buttons[21].pressed = environment_t::hide_under_cursor;
	buttons[15].pressed = environment_t::station_coverage_show;
	buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[17].pressed = grund_t::show_grid;
	//buttons[18].pressed = environment_t::show_names&1;
	buttons[19].pressed = (environment_t::show_names&2)!=0;
	buttons[20].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[22].pressed = environment_t::visualize_schedule;

	// Update label buffers
	hide_buildings_label.set_text( environment_t::hide_buildings==0 ? "no buildings hidden" : (environment_t::hide_buildings==1 ? "hide city building" : "hide all building") );
	convoy_tooltip_label.set_text( environment_t::show_vehicle_states==0 ? "convoi error tooltips" : (environment_t::show_vehicle_states==1 ? "convoi mouseover tooltips" : "all convoi tooltips") );
	sprintf(frame_time_buf," %ld ms", get_frame_time() );
	sprintf(idle_time_buf, " %d ms", welt->get_schlaf_zeit() );

	// fps_label
	uint8  color;
	uint32 loops;
	uint32 target_fps = welt->is_fast_forward() ? 10 : environment_t::fps;
	loops = welt->get_realFPS();
	color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops < (target_fps*3)/4  ) {
		color = ( loops <= target_fps/2 ) ? COL_RED : COL_YELLOW;
	}
	fps_value_label.set_color(color);
	sprintf(fps_buf," %d fps", loops );
#ifdef DEBUG
	if(  environment_t::simple_drawing  ) {
		strcat( fps_buf, "*" );
	}
#endif

	//simloops_label
	loops = welt->get_simloops();
	color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops <= 30  ) {
		color = (loops<=20) ? COL_RED : COL_YELLOW;
	}
	simloops_value_label.set_color(color);
	sprintf(simloops_buf,  " %d%c%d", loops/10, get_fraction_sep(), loops%10 );

	// All components are updated, now draw them...
	gui_frame_t::zeichnen(pos, gr);

	// Draw user defined components (not a component object)
	if(  environment_t::show_names&1  ) {

		PLAYER_COLOR_VAL pc = welt->get_active_player() ? welt->get_active_player()->get_player_color1()+4 : COL_ORANGE; // Why +4?
		const char *text = translator::translate("show station names");

		switch( environment_t::show_names >> 2 ) {
			case 0:
				display_ddd_proportional_clip( x+buttons[18].get_pos().x+buttons[18].get_groesse().x+D_H_SPACE, y+buttons[18].get_pos().y+buttons[18].get_groesse().y/2, proportional_string_width(text)+7, 0, pc, COL_BLACK, text, 1 );
				break;
			case 1:
				display_outline_proportional( x+buttons[18].get_pos().x+buttons[18].get_groesse().x+D_H_SPACE, y+buttons[18].get_pos().y, pc+1, COL_BLACK, text, 1 );
				break;
			case 2:
				display_outline_proportional( x+buttons[18].get_pos().x+buttons[18].get_groesse().x+D_H_SPACE+LINESPACE+D_H_SPACE, y+buttons[18].get_pos().y,   COL_YELLOW,  COL_BLACK,   text, 1 );
				display_ddd_box_clip(         x+buttons[18].get_pos().x+buttons[18].get_groesse().x+D_H_SPACE,                     y+buttons[18].get_pos().y,   LINESPACE,   LINESPACE,   pc-2, pc+2 );
				display_fillbox_wh(           x+buttons[18].get_pos().x+buttons[18].get_groesse().x+D_H_SPACE+1,                   y+buttons[18].get_pos().y+1, LINESPACE-2, LINESPACE-2, pc,   true);
				break;
		}
	}

}
