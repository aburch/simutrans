/*
 * dialog for setting the climate border and other map related parameters
 *
 * prissi
 *
 * August 2006
 */

#include "climates.h"
#include "karte.h"
#include "welt.h"

#include "../besch/grund_besch.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../simwin.h"

#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"

#include "../simgraph.h"

#include "../utils/simstring.h"


#define START_HEIGHT (28)

#define LEFT_ARROW (130)
#define RIGHT_ARROW (180)
#define TEXT_RIGHT (D_MARGIN_LEFT+165)


/**
 * set the climate borders
 * @author prissi
 */
climate_gui_t::climate_gui_t(settings_t* const sets) :
	gui_frame_t( translator::translate("Climate Control") )
{
	this->sets = sets;

	sint16 labelnr=0;

	// mountian/water stuff
	sint16 y = D_MARGIN_TOP;
	numberinput_lbl[labelnr].init( "Water level", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	water_level.init( sets->get_grundwasser(), -10, 0, gui_numberinput_t::AUTOLINEAR, false );
	water_level.set_pos( koord(LEFT_ARROW,y) );
	water_level.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	water_level.add_listener( this );
	add_komponente( &water_level );
	y += D_BUTTON_HEIGHT;

	numberinput_lbl[labelnr].init( "Mountain height", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	mountain_height.init( (int)sets->get_max_mountain_height(), 0, 320, 10, false );
	mountain_height.set_pos( koord(LEFT_ARROW,y) );
	mountain_height.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	mountain_height.add_listener( this );
	add_komponente( &mountain_height );
	y += D_BUTTON_HEIGHT;

	numberinput_lbl[labelnr].init( "Map roughness", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

#ifndef DOUBLE_GROUNDS
	mountain_roughness.init( (int)(sets->get_map_roughness()*20.0 + 0.5)-8, 0, 7, gui_numberinput_t::AUTOLINEAR, false );
#else
	mountain_roughness.init( (int)(sets->get_map_roughness()*20.0 + 0.5)-8, 0, 10, gui_numberinput_t::AUTOLINEAR, false );
#endif
	mountain_roughness.set_pos( koord(LEFT_ARROW,y) );
	mountain_roughness.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	mountain_roughness.add_listener( this );
	add_komponente( &mountain_roughness );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	// summer snowline alsway startig above highest climate
	numberinput_lbl[labelnr].init( "Summer snowline", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	sprintf( snowline_txt ,"%d", sets->get_climate_borders()[arctic_climate] );
	summer_snowline.init( NULL, koord( TEXT_RIGHT, y+2 ), COL_WHITE, gui_label_t::right );
	summer_snowline.set_text_pointer( snowline_txt );
	add_komponente( &summer_snowline );
	y += D_BUTTON_HEIGHT;

	// artic starts at maximum end of climate
	numberinput_lbl[labelnr].init( "Winter snowline", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	snowline_winter.init( sets->get_winter_snowline(), 0, 24, gui_numberinput_t::AUTOLINEAR, false );
	snowline_winter.set_pos( koord(LEFT_ARROW,y) );
	snowline_winter.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	snowline_winter.add_listener( this );
	add_komponente( &snowline_winter );
	y += D_BUTTON_HEIGHT+D_V_SPACE;

	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate-1;  i<=rocky_climate-1;  i++  ) {

		numberinput_lbl[labelnr].init( grund_besch_t::get_climate_name_from_bit((climate)(i+1)), koord( D_MARGIN_LEFT, y+2 ) );
		add_komponente( numberinput_lbl+labelnr );
		labelnr++;

		climate_borders_ui[i].init( sets->get_climate_borders()[i+1], 0, 24, gui_numberinput_t::AUTOLINEAR, false );
		climate_borders_ui[i].set_pos( koord(LEFT_ARROW,y) );
		climate_borders_ui[i].set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
		climate_borders_ui[i].add_listener( this );
		add_komponente( climate_borders_ui+i );
		if(sets->get_climate_borders()[i]>arctic) {
			arctic = sets->get_climate_borders()[i];
		}
		y += D_BUTTON_HEIGHT;
	}
	snowline_winter.set_limits( 0, arctic );
	y += 5;

	no_tree.init( button_t::square_state, "no tree", koord(10,y) ); // right align
	no_tree.pressed = sets->get_no_trees();
	no_tree.add_listener( this );
	add_komponente( &no_tree );
	y += D_BUTTON_HEIGHT+4;

	// and finally river stuff
	numberinput_lbl[labelnr].init( "Number of rivers", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	river_n.init( sets->get_river_number(), 0, 1024, gui_numberinput_t::POWER2, false );
	river_n.set_pos( koord(LEFT_ARROW,y) );
	river_n.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	river_n.add_listener(this);
	add_komponente( &river_n );
	y += D_BUTTON_HEIGHT;

	numberinput_lbl[labelnr].init( "minimum length of rivers", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	river_min.init( sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
	river_min.set_pos( koord(LEFT_ARROW,y) );
	river_min.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	river_min.add_listener(this);
	add_komponente( &river_min );
	y += D_BUTTON_HEIGHT;

	numberinput_lbl[labelnr].init( "maximum length of rivers", koord( D_MARGIN_LEFT, y+2 ) );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;

	river_max.init( sets->get_max_river_length(), sets->get_min_river_length()+16, 1024, gui_numberinput_t::AUTOLINEAR, false );
	river_max.set_pos( koord(LEFT_ARROW,y) );
	river_max.set_groesse( koord(RIGHT_ARROW-LEFT_ARROW+10, D_BUTTON_HEIGHT) );
	river_max.add_listener(this);
	add_komponente( &river_max );
	y += D_BUTTON_HEIGHT;

	set_fenstergroesse( koord(RIGHT_ARROW+D_BUTTON_HEIGHT+D_MARGIN_RIGHT, y+D_MARGIN_BOTTOM+D_TITLEBAR_HEIGHT) );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool climate_gui_t::action_triggered( gui_action_creator_t *komp, value_t v)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	if(komp==&no_tree) {
		no_tree.pressed ^= 1;
		sets->set_no_trees(no_tree.pressed);
	}
	else if(komp==&water_level) {
		sets->grundwasser = (sint16)v.i;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==&mountain_height) {
		sets->max_mountain_height = v.i;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==&mountain_roughness) {
		sets->map_roughness = (double)(v.i+8)/20.0;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(komp==&river_n) {
		sets->river_number = (sint16)v.i;
	}
	else if(komp==&river_min) {
		sets->min_river_length = (sint16)v.i;
		river_max.set_limits(v.i+16,1024);
	}
	else if(komp==&river_max) {
		sets->max_river_length = (sint16)v.i;
		river_min.set_limits(0,max(16,v.i)-16);
	}
	else if(komp==&snowline_winter) {
		sets->winter_snowline = (sint16)v.i;
	}
	else {
		// all climate borders from here on

		// artic starts at maximum end of climate
		sint16 arctic = 0;
		for(  int i=desert_climate;  i<=rocky_climate;  i++  ) {
			if(  komp==climate_borders_ui+i-1  ) {
				sets->climate_borders[i] = (sint16)v.i;
			}
			if(sets->climate_borders[i]>arctic) {
				arctic = sets->climate_borders[i];
			}
		}
		sets->climate_borders[arctic_climate] = arctic;

		// correct summer snowline too
		if(arctic<sets->get_winter_snowline()) {
			sets->winter_snowline = arctic;
		}
		snowline_winter.set_limits( 0, arctic );
	}
	sprintf( snowline_txt ,"%d", sets->get_climate_borders()[arctic_climate] );
	return true;
}


void climate_gui_t::update_river_number( sint16 new_river_number )
{
	river_n.set_value( new_river_number );
}
