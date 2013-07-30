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

#define DIALOG_WIDTH (210)
#define L_COLUMN_EDIT (DIALOG_WIDTH - edit_width - D_MARGIN_RIGHT)

/**
 * set the climate borders
 * @author prissi
 */
climate_gui_t::climate_gui_t(settings_t* const sets_par) :
	gui_frame_t( translator::translate("Climate Control") )
{
	const scr_coord_val edit_width = display_get_char_max_width("-0123456789")*4 + button_t::gui_arrow_left_size.x + button_t::gui_arrow_right_size.x + 4;
	const scr_coord_val label_width = L_COLUMN_EDIT - D_MARGIN_LEFT-D_H_SPACE;
	koord cursor(D_MARGIN_LEFT,D_MARGIN_TOP);
	sint16 labelnr=0;

	sets = sets_par;

	// Water level
	water_level.init( sets->get_grundwasser(), -10, 0, gui_numberinput_t::AUTOLINEAR, false );
	water_level.set_pos( koord(L_COLUMN_EDIT,cursor.y) );
	water_level.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	water_level.add_listener( this );
	add_komponente( &water_level );
	numberinput_lbl[labelnr].init( "Water level", cursor );
	numberinput_lbl[labelnr].align_to(&water_level,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	// Mountain height
	mountain_height.init( (int)sets->get_max_mountain_height(), 0, 320, 10, false );
	mountain_height.set_pos( koord(L_COLUMN_EDIT,cursor.y) );
	mountain_height.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	mountain_height.add_listener( this );
	add_komponente( &mountain_height );
	numberinput_lbl[labelnr].init( "Mountain height", cursor );
	numberinput_lbl[labelnr].align_to(&mountain_height,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	// Mountain roughness
	mountain_roughness.init( (int)(sets->get_map_roughness()*20.0 + 0.5)-8, 0, 10, gui_numberinput_t::AUTOLINEAR, false );
	mountain_roughness.set_pos( koord(L_COLUMN_EDIT,cursor.y) );
	mountain_roughness.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	mountain_roughness.add_listener( this );
	add_komponente( &mountain_roughness );
	numberinput_lbl[labelnr].init( "Map roughness", cursor );
	numberinput_lbl[labelnr].align_to(&mountain_roughness,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT+D_V_SPACE;

	// summer snowline always starting above highest climate
	numberinput_lbl[labelnr].init( "Summer snowline", cursor );
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	sprintf( snowline_txt ,"%d", sets->get_climate_borders()[arctic_climate] );
	summer_snowline.init( snowline_txt, cursor, COL_WHITE);
	summer_snowline.align_to(&mountain_roughness,ALIGN_RIGHT,koord(button_t::gui_arrow_right_size.x,0));
	add_komponente( &summer_snowline );
	cursor.y += LINESPACE+D_V_SPACE;

	// Winter snowline
	snowline_winter.init( sets->get_winter_snowline(), -10, 24, gui_numberinput_t::AUTOLINEAR, false );
	snowline_winter.set_pos( koord(L_COLUMN_EDIT, cursor.y) );
	snowline_winter.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	snowline_winter.add_listener( this );
	add_komponente( &snowline_winter );
	numberinput_lbl[labelnr].init( "Winter snowline", cursor );
	numberinput_lbl[labelnr].align_to(&snowline_winter,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate-1;  i<=rocky_climate-1;  i++  ) {

		climate_borders_ui[i].init( sets->get_climate_borders()[i+1], 0, 24, gui_numberinput_t::AUTOLINEAR, false );
		climate_borders_ui[i].set_pos( koord(L_COLUMN_EDIT, cursor.y) );
		climate_borders_ui[i].set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
		climate_borders_ui[i].add_listener( this );
		add_komponente( climate_borders_ui+i );
		if(sets->get_climate_borders()[i]>arctic) {
			arctic = sets->get_climate_borders()[i];
		}
		numberinput_lbl[labelnr].init( grund_besch_t::get_climate_name_from_bit((climate)(i+1)), cursor );
		numberinput_lbl[labelnr].align_to(&climate_borders_ui[i],ALIGN_CENTER_V);
		add_komponente( numberinput_lbl+labelnr );
		labelnr++;
		cursor.y += D_EDIT_HEIGHT;
	}
	snowline_winter.set_limits( water_level.get_value(), arctic );
	snowline_winter.set_value( snowline_winter.get_value() );
	cursor.y += D_V_SPACE;

	no_tree.init( button_t::square_state, "no tree", cursor );
	no_tree.set_width(DIALOG_WIDTH-D_MARGINS_X);
	no_tree.pressed = sets->get_no_trees();
	no_tree.add_listener( this );
	add_komponente( &no_tree );
	cursor.y += D_BUTTON_SQUARE + D_V_SPACE;

	river_n.init( sets->get_river_number(), 0, 1024, gui_numberinput_t::POWER2, false );
	river_n.set_pos( koord(L_COLUMN_EDIT, cursor.y) );
	river_n.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	river_n.add_listener(this);
	add_komponente( &river_n );
	numberinput_lbl[labelnr].init( "Number of rivers", cursor );
	numberinput_lbl[labelnr].align_to(&river_n,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	river_min.init( sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
	river_min.set_pos( koord(L_COLUMN_EDIT, cursor.y) );
	river_min.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	river_min.add_listener(this);
	add_komponente( &river_min );
	numberinput_lbl[labelnr].init( "minimum length of rivers", cursor );
	numberinput_lbl[labelnr].align_to(&river_min,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	river_max.init( sets->get_max_river_length(), sets->get_min_river_length()+16, 1024, gui_numberinput_t::AUTOLINEAR, false );
	river_max.set_pos( koord(L_COLUMN_EDIT, cursor.y) );
	river_max.set_groesse( koord(edit_width, D_EDIT_HEIGHT) );
	river_max.add_listener(this);
	add_komponente( &river_max );
	numberinput_lbl[labelnr].init( "maximum length of rivers", cursor );
	numberinput_lbl[labelnr].align_to(&river_max,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_komponente( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	set_fenstergroesse( koord(DIALOG_WIDTH, D_TITLEBAR_HEIGHT+cursor.y+D_MARGIN_BOTTOM) );
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

	// all climate borders from here on

	// Arctic starts at maximum end of climate
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
	snowline_winter.set_limits( water_level.get_value(), arctic );
	snowline_winter.set_value( snowline_winter.get_value() );

	sprintf( snowline_txt ,"%d", sets->get_climate_borders()[arctic_climate] );
	return true;
}


void climate_gui_t::update_river_number( sint16 new_river_number )
{
	river_n.set_value( new_river_number );
}
