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

#include "themeselector.h"
#include "loadfont_frame.h"
#include "simwin.h"


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
				display_fillbox_wh_clip_rgb(           p.x+1,                   p.y+1, LINESPACE-2, LINESPACE-2, pc,   true);
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

color_gui_t::color_gui_t() :
gui_frame_t( translator::translate("Helligk. u. Farben") )
{
	set_table_layout(1,0);

	// Show thememanager
	buttons[23].init(button_t::roundbox_state | button_t::flexible, "Select a theme for display");
	add_component(buttons+23);

	// Change font
	buttons[25].init(button_t::roundbox_state | button_t::flexible, "Select display font");
	add_component(buttons+25);

	add_table(2,0);
	// Show grid checkbox
	buttons[17].init(button_t::square_state, "show grid");
	add_component(buttons+17, 2);

	// Underground view checkbox
	buttons[16].init(button_t::square_state, "underground mode");
	add_component(buttons+16, 2);

	// Show slice map view checkbox
	buttons[20].init(button_t::square_state, "sliced underground mode");
	add_component(buttons+20);

	// underground slice edit
	inp_underground_level.set_value( grund_t::underground_mode==grund_t::ugm_level ? grund_t::underground_level : welt->get_zeiger()->get_pos().z);
	inp_underground_level.set_limits(welt->get_groundwater()-10, 32);
	inp_underground_level.add_listener(this);
	add_component( &inp_underground_level );

	// Day/night change checkbox
	buttons[9].init(button_t::square_state, "8WORLD_CHOOSE");
	buttons[9].pressed = env_t::night_shift;
	add_component(buttons+9, 2);

	// Brightness label
	new_component<gui_label_t>("1LIGHT_CHOOSE");

	// brightness edit
	brightness.set_value( env_t::daynight_level );
	brightness.set_limits( 0, 9 );
	brightness.add_listener(this);
	add_component(&brightness);

	// Scroll inverse checkbox
	buttons[6].init(button_t::square_state, "4LIGHT_CHOOSE");
	buttons[6].pressed = env_t::scroll_multi < 0;
	add_component(buttons+6, 2);

	// Scroll speed label
	new_component<gui_label_t>("3LIGHT_CHOOSE");

	// Scroll speed edit
	scrollspeed.set_value( abs(env_t::scroll_multi) );
	scrollspeed.set_limits( 1, 9 );
	scrollspeed.add_listener(this);
	add_component(&scrollspeed);

	// Divider line
	new_component_span<gui_divider_t>(2);

	// Transparent instead of hidden checkbox
	buttons[10].init(button_t::square_state, "hide transparent");
	buttons[10].pressed = env_t::hide_with_transparency;
	add_component(buttons+10, 2);

	// Hide trees checkbox
	buttons[11].init(button_t::square_state, "hide trees");
	add_component(buttons+11, 2);

	// Hide buildings
	hide_buildings.set_focusable(false);
	hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("no buildings hidden"), SYSCOL_TEXT );
	hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("hide city building"), SYSCOL_TEXT );
	hide_buildings.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("hide all building"), SYSCOL_TEXT );
	hide_buildings.set_selection( env_t::hide_buildings );
	add_component(&hide_buildings, 2);
	hide_buildings.add_listener(this);

	buttons[21].set_tooltip( "hide objects under cursor" );
	buttons[21].init( button_t::square_state ,  "Smart hide objects" );
	add_component(buttons+21);

	// Smart hide objects edit
	cursor_hide_range.set_value(env_t::cursor_hide_range);
	cursor_hide_range.set_limits( 0, 10 );
	cursor_hide_range.add_listener(this);
	add_component(&cursor_hide_range);

	// Divider 2
	new_component_span<gui_divider_t>(2);

	// Transparent station coverage
	buttons[14].init(button_t::square_state, "transparent station coverage");
	buttons[14].pressed = env_t::use_transparency_station_coverage;
	add_component(buttons+14, 2);

	// Show station coverage
	buttons[15].init(button_t::square_state, "show station coverage");
	add_component(buttons+15, 2);

	// Show station names arrow
	add_table(2,1);
	{
		buttons[18].set_typ(button_t::arrowright);
		buttons[18].set_tooltip("show station names");
		add_component(buttons+18);
		new_component<gui_label_stationname_t>("show station names");
	}
	end_table();
	new_component<gui_empty_t>();

	// Show waiting bars checkbox
	buttons[19].init(button_t::square_state, "show waiting bars");
	buttons[19].pressed = env_t::show_names&2;
	add_component(buttons+19, 2);

	// Divider 3
	new_component_span<gui_divider_t>(2);

	// Pedestrians in towns checkbox
	buttons[8].init(button_t::square_state, "6LIGHT_CHOOSE");
	buttons[8].pressed = welt->get_settings().get_random_pedestrians();
	add_component(buttons+8, 2);

	// Pedestrians at stops checkbox
	buttons[7].init(button_t::square_state, "5LIGHT_CHOOSE");
	buttons[7].pressed = welt->get_settings().get_show_pax();
	add_component(buttons+7, 2);

	// Traffic density label
	new_component<gui_label_t>("6WORLD_CHOOSE");

	// Traffic density edit
	traffic_density.set_value(welt->get_settings().get_traffic_level());
	traffic_density.set_limits( 0, 16 );
	traffic_density.add_listener(this);
	add_component(&traffic_density);

	// Convoy tooltip
	convoy_tooltip.set_focusable(false);
	convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("convoi error tooltips"), SYSCOL_TEXT );
	convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("convoi mouseover tooltips"), SYSCOL_TEXT );
	convoy_tooltip.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("all convoi tooltips"), SYSCOL_TEXT );
	convoy_tooltip.set_selection( env_t::show_vehicle_states );
	add_component(&convoy_tooltip, 2);
	convoy_tooltip.add_listener(this);

	// Show schedule's stop checkbox
	buttons[22].init( button_t::square_state ,  "Highlite schedule" );
	add_component(buttons+22, 2);

	// convoi booking message options
	money_booking.set_focusable( false );
	money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Show all revenue messages"), SYSCOL_TEXT );
	money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Show only player's revenue"), SYSCOL_TEXT );
	money_booking.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Show no revenue messages"), SYSCOL_TEXT );
	money_booking.set_selection( env_t::show_money_message );
	add_component(&money_booking, 2);
	money_booking.add_listener(this);
	
	// ribi arrow
	buttons[26].init(button_t::square_state, "show connected directions");
	buttons[26].pressed = strasse_t::show_masked_ribi;
	add_component(buttons+26, 2);

	// Toggle simple drawing for debugging
#ifdef DEBUG
	buttons[24].init(button_t::square_state, "Simple drawing");
	buttons[24].pressed = env_t::simple_drawing;
	add_component(buttons+24, 2);
#endif

	// Divider 4
	new_component_span<gui_divider_t>(2);

	end_table();

	// add controls to info container
	container_bottom = add_table(2,0);
	container_bottom->set_alignment(ALIGN_LEFT);
	// Frame time label
	new_component<gui_label_t>("Frame time:");
	add_component( &frame_time_value_label );
	// Idle time label
	new_component<gui_label_t>("Idle:");
	add_component( &idle_time_value_label );
	// FPS label
	new_component<gui_label_t>("FPS:");
	add_component( &fps_value_label );
	// Simloops label
	new_component<gui_label_t>("Sim:");
	add_component( &simloops_value_label );
	// empty row with inf-length second column to have space for changing labels in 2nd column
	new_component<gui_empty_t>();
	new_component<gui_fill_t>();
	end_table();

	for(  int i = 0;  i < COLORS_MAX_BUTTONS;  i++  ) {
		buttons[i].add_listener(this);
	}

	update_labels();

	reset_min_windowsize();
	set_windowsize(get_min_windowsize() );
}


bool color_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{

	// Brightness edit
	if(&brightness==komp) {
	  env_t::daynight_level = (sint8)v.i;
	} else

	// Traffic density edit
	if(&traffic_density==komp) {
		if(  !env_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			static char level[16];
			sprintf(level, "%li", v.i);
			tool_t::simple_tool[TOOL_TRAFFIC_LEVEL&0xFFF]->set_default_param( level );
			welt->set_tool( tool_t::simple_tool[TOOL_TRAFFIC_LEVEL&0xFFF], welt->get_active_player() );
		}
		else {
			traffic_density.set_value(welt->get_settings().get_traffic_level());
		}
	} else

	// Scroll speed edit
	if(&scrollspeed==komp) {
		env_t::scroll_multi = (sint16)( buttons[6].pressed ? -v.i : v.i );
	} else

	// Smart hide objects edit
	if(&cursor_hide_range==komp) {
		env_t::cursor_hide_range = cursor_hide_range.get_value();
	} else

	// Convoy tooltip
	if (&convoy_tooltip == komp) {
		env_t::show_vehicle_states = v.i;
	} else

	// Scroll inverse checkbox
	if((buttons+6)==komp) {
		buttons[6].pressed ^= 1;
		env_t::scroll_multi = -env_t::scroll_multi;
	} else

	// Pedestrians at stops checkbox
	if((buttons+7)==komp) {
		if(  !env_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_tool( tool_t::simple_tool[TOOL_TOOGLE_PAX&0xFFF], welt->get_active_player() );
		}
	} else

	// Pedestrians in towns checkbox
	if((buttons+8)==komp) {
		if(  !env_t::networkmode  ||  welt->get_active_player_nr()==1  ) {
			welt->set_tool( tool_t::simple_tool[TOOL_TOOGLE_PEDESTRIANS&0xFFF], welt->get_active_player() );
		}
	} else

	// Day/night change checkbox
	if((buttons+9)==komp) {
		env_t::night_shift = !env_t::night_shift;
		buttons[9].pressed ^= 1;
	} else

	// Transparent instead of hidden checkbox
	if((buttons+10)==komp) {
		env_t::hide_with_transparency = !env_t::hide_with_transparency;
		buttons[10].pressed ^= 1;
		baum_t::recalc_outline_color();
	} else

	// Hide trees checkbox
	if((buttons+11)==komp) {
		env_t::hide_trees = !env_t::hide_trees;
		baum_t::recalc_outline_color();
	} else

	// Hide building
	if(&hide_buildings == komp) {
		env_t::hide_buildings = v.i;
	} else

	// Transparent station coverage
	if((buttons+14)==komp) {
		env_t::use_transparency_station_coverage = !env_t::use_transparency_station_coverage;
		buttons[14].pressed ^= 1;
	} else

	// Show station coverage
	if((buttons+15)==komp) {
		env_t::station_coverage_show = env_t::station_coverage_show==0 ? 0xFF : 0;
	} else

	// Underground view checkbox
	if((buttons+16)==komp) {

		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode(buttons[16].pressed ? grund_t::ugm_none : grund_t::ugm_all, inp_underground_level.get_value());
		buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;

		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
	} else

	// Show grid checkbox
	if((buttons+17)==komp) {
		grund_t::toggle_grid();
	} else

	// Show station names arrow
	if((buttons+18)==komp) {
		if(  env_t::show_names&1  ) {
			if(  (env_t::show_names>>2) == 2  ) {
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
	} else

	// Show waiting bars checkbox
	if((buttons+19)==komp) {
		env_t::show_names ^= 2;
	} else

	// Show slice map view checkbox
	if((buttons+20)==komp) {

		// see simtool.cc::tool_show_underground_t::init
		grund_t::set_underground_mode(buttons[20].pressed ? grund_t::ugm_none : grund_t::ugm_level, inp_underground_level.get_value());
		buttons[20].pressed = grund_t::underground_mode == grund_t::ugm_level;

		// calc new images
		welt->update_underground();

		// renew toolbar
		tool_t::update_toolbars();
	} else

	// Hide buildings and trees under mouse cursor checkbox
	if((buttons+21)==komp) {

		// see simtool.cc::tool_hide_under_cursor_t::init
		env_t::hide_under_cursor = !env_t::hide_under_cursor  &&  env_t::cursor_hide_range>0;
		buttons[21].pressed = env_t::hide_under_cursor;

		// renew toolbar
		tool_t::update_toolbars();
	} else

	// Show schedule's stop checkbox
	if((buttons+22)==komp) {
		env_t::visualize_schedule = !env_t::visualize_schedule;
		buttons[22].pressed = env_t::visualize_schedule;
	} else

	// underground slice edit
	if (komp == &inp_underground_level) {
		if(grund_t::underground_mode==grund_t::ugm_level) {
			grund_t::underground_level = inp_underground_level.get_value();

			// calc new images
			welt->update_underground();
		}
	} else

	if((buttons+23)==komp) {
		create_win(new themeselector_t(), w_info, magic_themes);
	}
	if((buttons+24)==komp) {
		env_t::simple_drawing = !env_t::simple_drawing;
		buttons[24].pressed = env_t::simple_drawing;
	}
	if((buttons+25)==komp) {
		create_win(new loadfont_frame_t(), w_info, magic_font);
	}
	if((buttons+26)==komp  &&  skinverwaltung_t::ribi_arrow) {
		strasse_t::show_masked_ribi ^= 1;
		welt->set_dirty();
	}
	if(  &money_booking==komp  ) {
		env_t::show_money_message = v.i;
	}
	welt->set_dirty();
	return true;
}


void color_gui_t::update_labels()
{
	// Update label buffers
	frame_time_value_label.buf().printf(" %d ms", get_frame_time() );
	frame_time_value_label.update();
	idle_time_value_label.buf().printf(" %d ms", welt->get_idle_time() );
	idle_time_value_label.update();

	// fps_label
	uint32 target_fps = welt->is_fast_forward() ? 10 : env_t::fps;
	uint32 loops = welt->get_realFPS();
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
	loops = welt->get_simloops();
	color = SYSCOL_TEXT_HIGHLIGHT;
	if(  loops <= 30  ) {
		color = color_idx_to_rgb((loops<=20) ? COL_RED : COL_YELLOW);
	}
	simloops_value_label.set_color(color);
	simloops_value_label.buf().printf(" %d%c%d", loops/10, get_fraction_sep(), loops%10 );
	simloops_value_label.update();

	// realign container - necessary if strings changed length
	container_bottom->set_size( container_bottom->get_size() );
}


void color_gui_t::draw(scr_coord pos, scr_size size)
{
	// Update button states that was changed with keyboard ...
	buttons[ 7].pressed = welt->get_settings().get_show_pax();
	buttons[ 8].pressed = welt->get_settings().get_random_pedestrians();
	buttons[11].pressed = env_t::hide_trees;
	buttons[21].pressed = env_t::hide_under_cursor;
	buttons[15].pressed = env_t::station_coverage_show;
	buttons[16].pressed = grund_t::underground_mode == grund_t::ugm_all;
	buttons[17].pressed = grund_t::show_grid;
	buttons[19].pressed = (env_t::show_names&2)!=0;
	buttons[20].pressed = grund_t::underground_mode == grund_t::ugm_level;
	buttons[22].pressed = env_t::visualize_schedule;
	buttons[24].pressed = env_t::simple_drawing;
	buttons[24].enable(welt->is_paused());
	buttons[26].enable(skinverwaltung_t::ribi_arrow!=NULL);

	update_labels();

	// All components are updated, now draw them...
	gui_frame_t::draw(pos, size);
}
