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
#include "../gui/simwin.h"

#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#include "../display/simgraph.h"

#include "../utils/simstring.h"

//#define START_HEIGHT (40)
//#define LEFT_ARROW (130)
//#define RIGHT_ARROW (180)
//#define TEXT_RIGHT (D_MARGIN_LEFT+165)

#define L_DIALOG_WIDTH (210)
#define L_CLIENT_WIDTH (L_DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT)
#define L_COLUMN_EDIT (L_DIALOG_WIDTH - edit_width - D_MARGIN_RIGHT)

/**
 * set the climate borders
 * @author prissi
 */
climate_gui_t::climate_gui_t(settings_t* const sets_par) :
	gui_frame_t( translator::translate("Climate Control") )
{
	const scr_coord_val edit_width = display_get_char_max_width("-0123456789")*4 + D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH + 4;
	const scr_coord_val label_width = L_CLIENT_WIDTH - edit_width - D_H_SPACE;
	const scr_size edit_size(edit_width, D_EDIT_HEIGHT);
	scr_coord cursor(L_COLUMN_EDIT,D_MARGIN_TOP);
	sint16 labelnr=0;

	sets = sets_par;

	// Water level
	water_level.init( sets->get_grundwasser(), -10*(grund_besch_t::double_grounds?2:1), 0, gui_numberinput_t::AUTOLINEAR, false );
	water_level.set_pos( cursor );
	water_level.set_size( edit_size );
	water_level.add_listener( this );
	add_component( &water_level );
	const gui_label_t& water_level_lbl = numberinput_lbl[labelnr];
	numberinput_lbl[labelnr].init( "Water level", scr_coord(D_MARGIN_LEFT, cursor.y) );
	numberinput_lbl[labelnr].align_to(&water_level,ALIGN_CENTER_V);
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	const scr_coord lbl_offs = water_level_lbl.get_pos() - water_level.get_pos();
	const scr_coord btn_offs(lbl_offs.x, 0);

	// Height and roughness
	int mountain_height_start = (int)sets->get_max_mountain_height();
	int mountain_roughness_start = (int)(sets->get_map_roughness()*20.0 + 0.5)-8;

	// Mountain height
	mountain_height.init( mountain_height_start, 0, min(1000,100*(11-mountain_roughness_start)), 10, false );
	mountain_height.set_pos( cursor );
	mountain_height.set_size( edit_size );
	mountain_height.add_listener( this );
	add_component( &mountain_height );
	numberinput_lbl[labelnr].init( "Mountain height", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	// Mountain roughness
	mountain_roughness.init( mountain_roughness_start, 0, min(10, 11-((mountain_height_start+99)/100)), gui_numberinput_t::AUTOLINEAR, false );
	mountain_roughness.set_pos( cursor );
	mountain_roughness.set_size( edit_size );
	mountain_roughness.add_listener( this );
	add_component( &mountain_roughness );
	numberinput_lbl[labelnr].init( "Map roughness", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_BUTTON_HEIGHT+D_V_SPACE;

	hilly.init( button_t::square_state, "Hilly landscape", cursor + btn_offs); 
	hilly.set_width(L_CLIENT_WIDTH);
	hilly.pressed=env_t::hilly;
	hilly.add_listener( this );
	add_component( &hilly );
	cursor.y += D_BUTTON_HEIGHT;

	cities_ignore_height.init( button_t::square_state, "Cities ignore height", cursor + btn_offs); 
	cities_ignore_height.set_width(L_CLIENT_WIDTH);
	cities_ignore_height.set_tooltip("Cities will be built all over the terrain, rather than preferring lower ground");
	cities_ignore_height.pressed=env_t::cities_ignore_height;
	cities_ignore_height.add_listener( this );
	add_component( &cities_ignore_height );
	cursor.y += D_BUTTON_HEIGHT;

	// Cities like water
	cities_like_water.init( (int)(env_t::cities_like_water), 0, 100, 1, false );
	cities_like_water.set_pos( cursor );
	cities_like_water.set_size( edit_size );
	cities_like_water.add_listener(this);
	add_component( &cities_like_water );
	numberinput_lbl[labelnr].init("Cities like water", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	// Summer snowline always starting above highest climate
	sprintf( snowline_txt ,"%d", sets->get_climate_borders()[arctic_climate] );
	summer_snowline.init( NULL, cursor + scr_coord(D_ARROW_LEFT_WIDTH, lbl_offs.y), SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right );
	summer_snowline.set_width(edit_width - D_ARROW_LEFT_WIDTH - D_ARROW_RIGHT_WIDTH);
	summer_snowline.set_text_pointer( snowline_txt, false );
	add_component( &summer_snowline );
	numberinput_lbl[labelnr].init( "Summer snowline", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_BUTTON_HEIGHT;

	// Winter snowline
	snowline_winter.init( sets->get_winter_snowline(), -5, 32 - sets->get_grundwasser(), gui_numberinput_t::AUTOLINEAR, false );
	snowline_winter.set_pos( cursor );
	snowline_winter.set_size( edit_size );
	snowline_winter.add_listener( this );
	add_component( &snowline_winter );
	numberinput_lbl[labelnr].init( "Winter snowline", cursor + lbl_offs);
	numberinput_lbl[labelnr].set_width( label_width );
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT + D_V_SPACE;

	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate-1;  i<=rocky_climate-1;  i++  ) {

		climate_borders_ui[i].init( sets->get_climate_borders()[i+1], -5, 32 - sets->get_grundwasser(), gui_numberinput_t::AUTOLINEAR, false );
		climate_borders_ui[i].set_pos( cursor );
		climate_borders_ui[i].set_size( edit_size );
		climate_borders_ui[i].add_listener( this );
		add_component( climate_borders_ui+i );
		if(sets->get_climate_borders()[i]>arctic) {
			arctic = sets->get_climate_borders()[i];
		}
		numberinput_lbl[labelnr].init( grund_besch_t::get_climate_name_from_bit((climate)(i+1)), cursor + lbl_offs);
		numberinput_lbl[labelnr].set_width(label_width);
		//numberinput_lbl[labelnr].align_to(&climate_borders_ui[i],ALIGN_CENTER_V);
		add_component( numberinput_lbl+labelnr );
		labelnr++;
		cursor.y += D_EDIT_HEIGHT;
	}
	snowline_winter.set_limits( -5, arctic );
	snowline_winter.set_value( snowline_winter.get_value() );
	cursor.y += D_V_SPACE;

	no_tree.init( button_t::square_state, "no tree", cursor + btn_offs);
	no_tree.set_width(L_CLIENT_WIDTH);
	no_tree.pressed = sets->get_no_trees();
	no_tree.add_listener( this );
	add_component( &no_tree );
	cursor.y += D_CHECKBOX_HEIGHT + D_V_SPACE;

	cities_ignore_height.pressed = env_t::cities_ignore_height;
	cities_like_water.set_value((int)(env_t::cities_like_water));

	lake.init( button_t::square_state, "lake", cursor + btn_offs);
	lake.set_width(L_CLIENT_WIDTH);
	lake.pressed = sets->get_lake();
	lake.add_listener( this );
	add_component( &lake );
	cursor.y += D_CHECKBOX_HEIGHT + D_V_SPACE;

	river_n.init( sets->get_river_number(), 0, 1024, gui_numberinput_t::POWER2, false );
	river_n.set_pos( cursor );
	river_n.set_size( edit_size );
	river_n.add_listener(this);
	add_component( &river_n );
	numberinput_lbl[labelnr].init( "Number of rivers", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	river_min.init( sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
	river_min.set_pos( cursor );
	river_min.set_size( edit_size );
	river_min.add_listener(this);
	add_component( &river_min );
	numberinput_lbl[labelnr].init( "minimum length of rivers", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	river_max.init( sets->get_max_river_length(), sets->get_min_river_length()+16, 8192, gui_numberinput_t::AUTOLINEAR, false );
	river_max.set_pos( cursor );
	river_max.set_size( edit_size );
	river_max.add_listener(this);
	add_component( &river_max );
	numberinput_lbl[labelnr].init( "maximum length of rivers", cursor + lbl_offs );
	numberinput_lbl[labelnr].set_width(label_width);
	add_component( numberinput_lbl+labelnr );
	labelnr++;
	cursor.y += D_EDIT_HEIGHT;

	set_windowsize( scr_size(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT+cursor.y+D_MARGIN_BOTTOM) );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool climate_gui_t::action_triggered( gui_action_creator_t *comp, value_t v)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	if(comp==&no_tree) {
		no_tree.pressed ^= 1;
		sets->set_no_trees(no_tree.pressed);
	}
	else if(comp==&lake) {
		lake.pressed ^= 1;
		sets->set_lake(lake.pressed);
	}
	else if(comp==&water_level) {
		sets->grundwasser = (sint16)v.i;
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
		for(  int i=desert_climate-1;  i<=rocky_climate-1;  i++  ) {
			climate_borders_ui[i].set_limits( -5, 32 - sets->get_grundwasser() );
			climate_borders_ui[i].set_value( climate_borders_ui[i].get_value() );
		}
	}
	else if(comp==&mountain_height) {
		sets->max_mountain_height = v.i;
		mountain_roughness.set_limits(0,min(10,11-((v.i+99)/100)));
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(comp==&mountain_roughness) {
		sets->map_roughness = (double)(v.i+8)/20.0;
		mountain_height.set_limits(0,min(1000,100*(11-v.i)));
		if(  welt_gui  ) {
			welt_gui->update_preview();
		}
	}
	else if(comp==&cities_like_water) {
		env_t::cities_like_water = (uint8) v.i;
	}
	else if(comp==&river_n) {
		sets->river_number = (sint16)v.i;
	}
	else if(comp==&river_min) {
		sets->min_river_length = (sint16)v.i;
		river_max.set_limits(v.i+16,32767);
	}
	else if(comp==&river_max) {
		sets->max_river_length = (sint16)v.i;
		river_min.set_limits(0,max(2,v.i)-2);
	}
	else if(comp==&snowline_winter) {
		sets->winter_snowline = (sint16)v.i;
	}
	else if(comp==&hilly)
	{
		env_t::hilly ^= 1;
		hilly.pressed ^= 1;
		if(  welt_gui  )
		{
			welt_gui->update_preview();
		}
	}
	else if(comp == & cities_ignore_height)
	{
		env_t::cities_ignore_height ^= 1;
		cities_ignore_height.pressed ^= 1;
	}
	else {
	// all climate borders from here on

	// Arctic starts at maximum end of climate
	sint16 arctic = 0;
	for(  int i=desert_climate;  i<=rocky_climate;  i++  ) {
		if(  comp==climate_borders_ui+i-1  ) {
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
	snowline_winter.set_limits( -5, arctic );
	snowline_winter.set_value( snowline_winter.get_value() );

	sprintf( snowline_txt ,"%d", sets->get_climate_borders()[arctic_climate] );
	}
	return true;
}


void climate_gui_t::update_river_number( sint16 new_river_number )
{
	river_n.set_value( new_river_number );
}
