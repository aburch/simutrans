/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "welt.h"
#include "karte.h"

#include "../simdebug.h"
#include "../simworld.h"
#include "../gui/simwin.h"
#include "../display/simimg.h"
#include "../simmesg.h"
#include "../simskin.h"
#include "../simversion.h"

#include "../bauer/hausbauer.h"
#include "../bauer/wegbauer.h"

#include "../descriptor/building_desc.h"

#include "../dataobj/height_map_loader.h"
#include "../dataobj/settings.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../boden/wege/schiene.h"
#include "../obj/baum.h"
#include "../simcity.h"
#include "../vehicle/simvehicle.h"
#include "../player/simplay.h"
#include "../simconvoi.h"

#include "../simcolor.h"

#include "../display/simgraph.h"

#include "../sys/simsys.h"
#include "../utils/simstring.h"
#include "../utils/simrandom.h"

#include "sprachen.h"
#include "climates.h"
#include "settings_frame.h"
#include "loadsave_frame.h"
#include "load_relief_frame.h"
#include "messagebox.h"
#include "scenario_frame.h"


#define L_MAP_PREVIEW_SIZE (70)
#define L_MAP_PREVIEW_WIDTH L_MAP_PREVIEW_SIZE
#define L_MAP_PREVIEW_HEIGHT L_MAP_PREVIEW_SIZE
#define L_PREVIEW_SIZE_MIN (16)
#define L_DIALOG_WIDTH     (270)
#define L_EDIT_WIDTH       (edit_width)
#define L_COLUMN1_X (L_DIALOG_WIDTH - L_EDIT_WIDTH - D_H_SPACE - L_MAP_PREVIEW_WIDTH - D_MARGIN_RIGHT)
#define L_COLUMN2_X (L_DIALOG_WIDTH - L_EDIT_WIDTH - D_MARGIN_RIGHT)
#define L_BUTTON_WIDTH	( (L_DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT - D_V_SPACE) / 2 )
#define L_BUTTON_COLUMN_2 (L_DIALOG_WIDTH - L_BUTTON_WIDTH - D_MARGIN_RIGHT)
#define L_BUTTON_NARROW ( (L_DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT - D_V_SPACE + 2) / 3 )
#define L_BUTTON_WIDE   (((L_DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT - D_V_SPACE) * 2) / 3 )

uint32 welt_gui_t::max_map_dimension_fixed = 32766;


welt_gui_t::welt_gui_t(settings_t* const sets_par) :
	gui_frame_t( translator::translate("Neue Welt" ) ),
	map(0,0)
{
	// Coordinates are relative to parent (TITLEHEIGHT already subtracted)
	const scr_coord_val digit_width = display_get_char_max_width("0123456789");
	scr_coord cursor(D_MARGIN_LEFT,D_MARGIN_TOP);
	scr_coord_val edit_width = digit_width * 5 + D_ARROW_LEFT_WIDTH + D_ARROW_RIGHT_WIDTH;
	scr_size edit_size(edit_width, D_EDIT_HEIGHT);
	scr_coord_val label_width = L_COLUMN1_X - D_MARGIN_LEFT - D_H_SPACE;

	sets = sets_par;
	sets->beginner_mode = env_t::default_settings.get_beginner_mode();

	double size = sqrt((double)sets->get_size_x()*sets->get_size_y());
	city_density       = sets->get_city_count()      ? size / sets->get_city_count()      : 0.0;
	industry_density   = sets->get_factory_count()       ? (double)sets->get_factory_count()  / (double)sets->get_city_count()      : 0.0;
	attraction_density = sets->get_tourist_attractions() ? (double)sets->get_tourist_attractions() / (double)sets->get_city_count() : 0.0;
	river_density      = sets->get_river_number()        ? size / sets->get_river_number()        : 0.0;

	// find earliest start and end date ...
	uint16 game_start = 4999;
	uint16 game_ends = 0;

	// first check town halls
	FOR(vector_tpl<building_desc_t const*>, const desc, *hausbauer_t::get_list(building_desc_t::townhall)) {
		uint16 intro_year = (desc->get_intro_year_month()+11)/12;
		if(  intro_year<game_start  ) {
			game_start = intro_year;
		}
		uint16 retire_year = (desc->get_retire_year_month()+11)/12;
		if(  retire_year>game_ends  ) {
			game_ends = retire_year;
		}
	}

	// then streets
	game_start = max( game_start, (way_builder_t::get_earliest_way(road_wt)->get_intro_year_month()+11)/12 );
	game_ends  = min( game_ends,  (way_builder_t::get_latest_way(road_wt)->get_retire_year_month()+11)/12 );

	loaded_heightfield = load_heightfield = false;
	load = start = close = scenario = quit = false;
	sets->heightfield = "";

	//******************************************************************
	// Component creation

	//const scr_coord_val L_CLIENT_WIDTH  = L_DIALOG_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT;
	const scr_coord_val L_MAP_X         = L_DIALOG_WIDTH - L_MAP_PREVIEW_WIDTH - D_MARGIN_RIGHT;
	const scr_coord_val L_LEFT_OF_MAP_W = L_MAP_X - D_H_SPACE - D_MARGIN_LEFT;

	// Map preview
	map_preview.init(scr_coord(L_MAP_X, cursor.y), scr_size(L_MAP_PREVIEW_WIDTH, L_MAP_PREVIEW_HEIGHT));
	add_component( &map_preview );

	// World setting label
	world_title_label.init("1WORLD_CHOOSE",cursor);
	world_title_label.set_width(L_LEFT_OF_MAP_W);
	add_component( &world_title_label );
	cursor.y += LINESPACE; // label

	// divider 1
	divider_1.init(cursor,L_LEFT_OF_MAP_W);
	add_component( &divider_1 );
	cursor.y += D_DIVIDER_HEIGHT;

	// Map number edit
	inp_map_number.init( abs(sets->get_map_number()), 0, 0x7FFFFFFF, 1, true );
	inp_map_number.set_pos(scr_coord(L_COLUMN1_X - digit_width * 5, cursor.y));
	inp_map_number.set_size(edit_size + scr_size(digit_width * 5, 0));
	inp_map_number.add_listener( this );
	add_component( &inp_map_number );

	// Map number label
	map_number_label.init("2WORLD_CHOOSE",cursor);
	map_number_label.set_width( label_width );
	map_number_label.align_to(&inp_map_number,ALIGN_CENTER_V);
	add_component( &map_number_label );
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	// Random map button
	random_map.init(button_t::roundbox,"Random map",cursor,scr_size(L_LEFT_OF_MAP_W, D_BUTTON_HEIGHT));
	random_map.set_tooltip("chooses a random map");
	random_map.add_listener( this );
	add_component( &random_map );
	cursor.y += D_BUTTON_HEIGHT;
	cursor.y += D_V_SPACE;

	// Load height map button
	load_map.init(button_t::roundbox,"Lade Relief",cursor,scr_size(L_LEFT_OF_MAP_W, D_BUTTON_HEIGHT));
	load_map.set_tooltip("load height data from file");
	load_map.add_listener( this );
	add_component( &load_map );
	cursor.y += D_BUTTON_HEIGHT;
	cursor.y += D_V_SPACE;

	label_width = L_COLUMN2_X - D_MARGIN_LEFT - D_H_SPACE;

	// Map size label
	size_label.init("",cursor);
	size_label.set_width( label_width );
	add_component( &size_label );
	cursor.y += LINESPACE;

	// Map X size edit
	inp_x_size.init( sets->get_size_x(), 8, max_map_dimension_fixed, sets->get_size_x()>=512 ? 128 : 64, false );
	inp_x_size.set_pos( scr_coord(L_COLUMN2_X,cursor.y) );
	inp_x_size.set_size(edit_size);
	inp_x_size.add_listener(this);
	add_component( &inp_x_size );
	lbl_x_size.init("West To East",cursor);
	lbl_x_size.set_width( label_width );
	lbl_x_size.align_to(&inp_x_size,ALIGN_CENTER_V);
	add_component( &lbl_x_size );
	info_x_size.init("", scr_coord(L_COLUMN1_X,cursor.y));
	info_x_size.set_align(gui_label_t::right);
	info_x_size.set_width(L_COLUMN2_X - L_COLUMN1_X - D_H_SPACE);
	info_x_size.align_to(&inp_x_size,ALIGN_CENTER_V);
	add_component( &info_x_size );
	cursor.y += D_EDIT_HEIGHT;

	// Map size Y edit
	inp_y_size.init( sets->get_size_y(), 8, max_map_dimension_fixed, sets->get_size_y()>=512 ? 128 : 64, false );
	inp_y_size.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_y_size.set_size(edit_size);
	inp_y_size.add_listener(this);
	add_component( &inp_y_size );
	lbl_y_size.init("North To South",cursor);
	lbl_y_size.set_width( label_width );
	lbl_y_size.align_to(&inp_y_size,ALIGN_CENTER_V);
	add_component( &lbl_y_size );
	info_y_size.init("", scr_coord(L_COLUMN1_X,cursor.y));
	info_y_size.set_align(gui_label_t::right);
	info_y_size.set_width(L_COLUMN2_X - L_COLUMN1_X - D_H_SPACE);
	info_y_size.align_to(&inp_y_size,ALIGN_CENTER_V);
	add_component( &info_y_size );
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	// Number of towns edit
	inp_number_of_towns.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_number_of_towns.set_size(edit_size);
	inp_number_of_towns.add_listener(this);
	inp_number_of_towns.set_limits(0,2048);
	inp_number_of_towns.set_value(abs(sets->get_city_count()) );
	add_component( &inp_number_of_towns );

	// Number of towns label
	cities_label.init("5WORLD_CHOOSE",cursor);
	cities_label.set_width( label_width );
	cities_label.align_to(&inp_number_of_towns,ALIGN_CENTER_V);
	add_component( &cities_label );
	cursor.y += D_EDIT_HEIGHT;

	// number of big cities
	inp_number_of_big_cities.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_number_of_big_cities.set_size(edit_size);
	inp_number_of_big_cities.add_listener(this);
	inp_number_of_big_cities.set_limits(0,sets->get_city_count() );
	inp_number_of_big_cities.set_value(env_t::number_of_big_cities );
	add_component( &inp_number_of_big_cities );
	lbl_number_of_big_cities.init("Number of big cities:", cursor);
	lbl_number_of_big_cities.set_width( label_width );
	lbl_number_of_big_cities.align_to(&inp_number_of_big_cities, ALIGN_CENTER_V);
	add_component( &lbl_number_of_big_cities );
	cursor.y += D_EDIT_HEIGHT;

	// number of city clusters
	inp_number_of_clusters.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_number_of_clusters.set_size(edit_size);
	inp_number_of_clusters.add_listener(this);
	inp_number_of_clusters.set_limits(0,sets->get_city_count()/3 );
	inp_number_of_clusters.set_value(env_t::number_of_clusters);
	add_component( &inp_number_of_clusters );
	lbl_number_of_clusters.init("Number of city clusters:", cursor);
	lbl_number_of_clusters.set_width( label_width );
	lbl_number_of_clusters.align_to(&inp_number_of_clusters, ALIGN_CENTER_V);
	add_component( &lbl_number_of_clusters );
	cursor.y += D_EDIT_HEIGHT;

	// city cluster size
	inp_cluster_size.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_cluster_size.set_size(edit_size);
	inp_cluster_size.add_listener(this);
	inp_cluster_size.set_limits(1,9999);
	inp_cluster_size.set_value(env_t::cluster_size);
	add_component( &inp_cluster_size );
	lbl_cluster_size.init("City cluster size:", cursor);
	lbl_cluster_size.set_width( label_width );
	lbl_cluster_size.align_to(&inp_cluster_size, ALIGN_CENTER_V);
	add_component( &lbl_cluster_size );
	cursor.y += D_EDIT_HEIGHT;

	// Town size edit
	inp_town_size.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_town_size.set_size(edit_size);
	inp_town_size.add_listener(this);
	inp_town_size.set_limits(0,999999);
	inp_town_size.set_increment_mode(50);
	inp_town_size.set_value( sets->get_mean_einwohnerzahl() );
	add_component( &inp_town_size );

	// Town size label
	median_label.init("Median Citizen per town",cursor);
	median_label.set_width( label_width );
	median_label.align_to(&inp_town_size,ALIGN_CENTER_V);
	add_component( &median_label );
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	// Intercity road length edit
	inp_intercity_road_len.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_intercity_road_len.set_size(edit_size);
	inp_intercity_road_len.add_listener(this);
	inp_intercity_road_len.set_limits(0,9999);
	inp_intercity_road_len.set_value( env_t::intercity_road_length );
	inp_intercity_road_len.set_increment_mode( env_t::intercity_road_length>=1000 ? 100 : 20 );
	add_component( &inp_intercity_road_len );

	// Intercity road length label
	intercity_label.init("Intercity road len:",cursor);
	intercity_label.set_width( label_width );
	intercity_label.align_to(&inp_intercity_road_len,ALIGN_CENTER_V);
	add_component( &intercity_label );
	cursor.y += D_EDIT_HEIGHT;

	// Factories edit
	inp_other_industries.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_other_industries.set_size(edit_size);
	inp_other_industries.add_listener(this);
	inp_other_industries.set_limits(0,16384);
	inp_other_industries.set_value(abs(sets->get_factory_count()) );
	add_component( &inp_other_industries );

	// Factories label
	factories_label.init("No. of Factories",cursor);
	factories_label.set_width( label_width );
	factories_label.align_to(&inp_other_industries,ALIGN_CENTER_V);
	add_component( &factories_label );
	cursor.y += D_EDIT_HEIGHT;

	// Tourist attr. edit
	inp_tourist_attractions.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_tourist_attractions.set_size(edit_size);
	inp_tourist_attractions.add_listener(this);
	inp_tourist_attractions.set_limits(0,999);
	inp_tourist_attractions.set_value(abs(sets->get_tourist_attractions()) );
	add_component( &inp_tourist_attractions );

	// Tourist attr. label
	tourist_label.init("Tourist attractions",cursor);
	tourist_label.set_width( label_width );
	tourist_label.align_to(&inp_tourist_attractions,ALIGN_CENTER_V);
	add_component( &tourist_label );
	cursor.y += D_EDIT_HEIGHT;
	cursor.y += D_V_SPACE;

	use_intro_dates.set_pos( cursor );
	use_intro_dates.set_typ( button_t::square_state );
	use_intro_dates.pressed = sets->get_use_timeline()&1;
	use_intro_dates.add_listener( this );
	add_component( &use_intro_dates );

	// Timeline year edit
	inp_intro_date.set_pos(scr_coord(L_COLUMN2_X,cursor.y) );
	inp_intro_date.set_size(edit_size);
	inp_intro_date.add_listener(this);
	inp_intro_date.set_limits(game_start,game_ends);
	inp_intro_date.set_increment_mode(10);
	inp_intro_date.set_value(abs(sets->get_starting_year()) );
	add_component( &inp_intro_date );

	// Use timeline checkbox
	use_intro_dates.init(button_t::square_state,"Use timeline start year", cursor);
	use_intro_dates.set_width( label_width );
	use_intro_dates.align_to(&inp_intro_date,ALIGN_CENTER_V);
	use_intro_dates.pressed = sets->get_use_timeline()&1;
	use_intro_dates.add_listener( this );
	add_component( &use_intro_dates );
	cursor.y += D_EDIT_HEIGHT;

	// Use beginner mode checkbox
	use_beginner_mode.init(button_t::square_state, "Use beginner mode", cursor);
	use_beginner_mode.set_width( label_width );
	use_beginner_mode.set_tooltip("Higher transport fees, crossconnect all factories");
	use_beginner_mode.pressed = sets->get_beginner_mode();
	use_beginner_mode.add_listener( this );
	add_component( &use_beginner_mode );
	cursor.y += D_CHECKBOX_HEIGHT;

	// divider 2
	divider_2.init(cursor,L_DIALOG_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	add_component( &divider_2 );
	cursor.y += D_DIVIDER_HEIGHT;

	// Map settings button
	open_setting_gui.init(button_t::roundbox, "Setting", cursor, scr_size(L_BUTTON_NARROW, D_BUTTON_HEIGHT));
	open_setting_gui.add_listener( this );

	open_setting_gui.pressed = win_get_magic( magic_settings_frame_t );
	open_setting_gui.add_listener( this );
	add_component( &open_setting_gui );

	// Landscape settings button
	open_climate_gui.init(button_t::roundbox,"Climate Control", scr_coord(L_DIALOG_WIDTH - D_MARGIN_RIGHT - L_BUTTON_WIDE, cursor.y), scr_size(L_BUTTON_WIDE, D_BUTTON_HEIGHT));
	open_climate_gui.pressed = win_get_magic( magic_climate );
	open_climate_gui.add_listener( this );
	add_component( &open_climate_gui );
	cursor.y += D_BUTTON_HEIGHT;

	// divider 3
	divider_3.init(cursor,L_DIALOG_WIDTH-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	add_component( &divider_3 );
	cursor.y += D_DIVIDER_HEIGHT;

	// load game
	load_game.init(button_t::roundbox, "Load game", cursor, scr_size(L_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	load_game.add_listener( this );
	add_component( &load_game );

	// load scenario
	load_scenario.init(button_t::roundbox,"Load scenario", scr_coord(L_BUTTON_COLUMN_2,cursor.y), scr_size(L_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	load_scenario.add_listener( this );
	add_component( &load_scenario );
	cursor.y += D_BUTTON_HEIGHT;
	cursor.y += D_V_SPACE;

	// start game
	start_game.init(button_t::roundbox, "Starte Spiel", cursor, scr_size(L_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	start_game.add_listener( this );
	add_component( &start_game );

	// quit game
	quit_game.init(button_t::roundbox,"Beenden", scr_coord(L_BUTTON_COLUMN_2,cursor.y), scr_size(L_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	quit_game.add_listener( this );
	add_component( &quit_game );
	cursor.y += D_BUTTON_HEIGHT;

	set_windowsize( scr_size(L_DIALOG_WIDTH, D_TITLEBAR_HEIGHT + cursor.y + D_MARGIN_BOTTOM) );
	resize(scr_coord(0,0));

	update_preview();
}


/**
 * Calculates preview from height map
 * @param filename name of heightfield file
 * @author Hajo/prissi
 */
bool welt_gui_t::update_from_heightfield(const char *filename)
{
	DBG_MESSAGE("welt_gui_t::update_from_heightfield()", "%s", filename);

	sint16 w, h;
	sint8 *h_field=NULL;
	height_map_loader_t hml(sets);
	if(hml.get_height_data_from_file(filename, (sint8)sets->get_groundwater(), h_field, w, h, false )) {
		sets->set_size_x(w);
		sets->set_size_y(h);
		update_densities();

		inp_x_size.set_value(sets->get_size_x());
		inp_y_size.set_value(sets->get_size_y());

		resize_preview();

		const int mx = sets->get_size_x()/map_size.w;
		const int my = sets->get_size_y()/map_size.h;
		for(  int y=0;  y<map_size.h;  y++  ) {
			for(  int x=0;  x<map_size.w;  x++  ) {
				map.at(x,y) = reliefkarte_t::calc_hoehe_farbe( h_field[x*mx+y*my*w], sets->get_groundwater()-1 );
			}
		}
		map_preview.set_map_data(&map,map_size);
		return true;
	}
	return false;
}


// sets the new values for the number input filed for the densities
void welt_gui_t::update_densities()
{
	if(  city_density!=0.0  ) {
		inp_number_of_towns.set_value( max( 1, (sint32)(0.5+sqrt((double)sets->get_size_x()*sets->get_size_y())/city_density) ) );
		sets->set_city_count( inp_number_of_towns.get_value() );
	}
	if(  industry_density!=0.0  ) {
		inp_other_industries.set_value(max(1, (sint32)(double)sets->get_city_count() * industry_density));
		sets->set_factory_count( inp_other_industries.get_value() );
	}
	if(  attraction_density!=0.0  ) {
		inp_tourist_attractions.set_value( max( 1, (sint32)((double)sets->get_city_count() *attraction_density) ) );
		sets->set_tourist_attractions( inp_tourist_attractions.get_value() );
	}
	if(  river_density!=0.0  ) {
		sets->river_number = max( 1, (sint32)(0.5+sqrt((double)sets->get_size_x()*sets->get_size_y())/river_density) );
		if(  climate_gui_t *climate_gui = (climate_gui_t *)win_get_magic( magic_climate )  ) {
			climate_gui->update_river_number( sets->get_river_number() );
		}
	}
}


/**
 * Calculate the new Map-Preview. Initialize the new RNG!
 * @author Hj. Malthaner
 */
void welt_gui_t::update_preview(bool load_heightfield)
{
	if(  loaded_heightfield  ||  load_heightfield) {
		loaded_heightfield = true;
		update_from_heightfield(sets->heightfield.c_str());
	}
	else {
		resize_preview();

		setsimrand( 0xFFFFFFFF, sets->get_map_number() );

		const sint32 max_size = max(sets->get_size_x(), sets->get_size_y());
		const int mx = sets->get_size_x()/map_size.w;
		const int my = sets->get_size_y()/map_size.h;
		for(  int y=0;  y<map_size.h;  y++  ) {
			for(  int x=0;  x<map_size.w;  x++  ) {
				map.at(x,y) = reliefkarte_t::calc_hoehe_farbe(karte_t::perlin_hoehe( sets, koord(x*mx,y*my), koord::invalid, max_size ), sets->get_groundwater());
			}
		}
		sets->heightfield = "";
	}
	map_preview.set_map_data(&map,map_size);
	map_preview.set_size(map_size+scr_size(2,2));
}


void welt_gui_t::resize_preview()
{
	const float world_aspect = (float)sets->get_size_x() / (float)sets->get_size_y();

	if(  world_aspect > 1.0  ) {
		map_size.w = L_MAP_PREVIEW_WIDTH-2;
		map_size.h = (scr_coord_val) max( (int)((float)map_size.w / world_aspect), L_PREVIEW_SIZE_MIN-2);
	}
	else {
		map_size.h = L_MAP_PREVIEW_HEIGHT-2;
		map_size.w = (scr_coord_val) max( (int)((float)map_size.h * world_aspect), L_PREVIEW_SIZE_MIN-2);
	}
	map.resize( map_size.w, map_size.h );
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool welt_gui_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	// check for changed map (update preview for any event)
	int knr = inp_map_number.get_value(); //

	if(comp==&inp_map_number) {
		sets->heightfield = "";
		loaded_heightfield = false;
	}
	else if(comp==&inp_x_size) {
		if(  !loaded_heightfield  ) {
			sets->set_size_x( v.i );
			inp_x_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
			update_densities();
		}
		else {
			inp_x_size.set_value(sets->get_size_x()); // can't change size with heightfield loaded
		}
	}
	else if(comp==&inp_y_size) {
		if(  !loaded_heightfield  ) {
			sets->set_size_y( v.i );
			inp_y_size.set_increment_mode( v.i>=64 ? (v.i>=512 ? 128 : 64) : 8 );
			update_densities();
		}
		else {
			inp_y_size.set_value(sets->get_size_y()); // can't change size with heightfield loaded
		}
	}
	else if(comp==&inp_number_of_towns) {
		sets->set_city_count( v.i );
		city_density = sets->get_city_count() ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_city_count() : 0.0;
		if (v.i == 0) {
			env_t::number_of_big_cities = 0;
			inp_number_of_big_cities.set_limits(0,0);
			inp_number_of_big_cities.set_value(0);

		}
		else {
			inp_number_of_big_cities.set_limits(0, v.i);
		}

		if (env_t::number_of_big_cities > unsigned(v.i)) {
			env_t::number_of_big_cities = v.i;
			inp_number_of_big_cities.set_value( env_t::number_of_big_cities );
		}

		inp_number_of_clusters.set_limits(0, v.i/4);
		if (env_t::number_of_clusters > unsigned(v.i)/4) {
			env_t::number_of_clusters = v.i/4;
			inp_number_of_clusters.set_value(v.i/4);
		}
		update_densities();
	}
	else if(comp==&inp_number_of_big_cities) {
		env_t::number_of_big_cities = v.i;
	}
	else if(comp == &inp_number_of_clusters) {
		env_t::number_of_clusters = v.i;
	}
	else if(comp == &inp_cluster_size) {
		env_t::cluster_size = v.i;
	}
	else if(comp==&inp_town_size) {
		sets->set_mean_einwohnerzahl( v.i );
	}
	else if(comp==&inp_intercity_road_len) {
		env_t::intercity_road_length = v.i;
		inp_intercity_road_len.set_increment_mode( v.i>=1000 ? 100 : 20 );
	}
	else if(comp==&inp_other_industries) {
		sets->set_factory_count( v.i );
		industry_density = sets->get_factory_count() ? (double)sets->get_factory_count() / (double)sets->get_city_count() : 0.0;
	}
	else if(comp==&inp_tourist_attractions) {
		sets->set_tourist_attractions( v.i );
		attraction_density = sets->get_tourist_attractions() ? (double)sets->get_factory_count() / (double)sets->get_tourist_attractions() : 0.0;
	}
	else if(comp==&inp_intro_date) {
		sets->set_starting_year( (sint16)(v.i) );
	}
	else if(comp==&random_map) {
		knr = simrand(2147483647, "bool welt_gui_t::action_triggered");
		inp_map_number.set_value(knr);
		sets->heightfield = "";
		loaded_heightfield = false;
	}
	else if(comp==&load_map) {
		// load relief
		loaded_heightfield = false;
		sets->heightfield = "";
		sets->groundwater = -2;
		load_relief_frame_t* lrf = new load_relief_frame_t(sets);
		create_win((display_get_width() - lrf->get_windowsize().w-10), 40, lrf, w_info, magic_load_t );
		knr = sets->get_map_number();	// otherwise using cancel would not show the normal generated map again
	}
	else if(comp==&use_intro_dates) {
		// 0,1 should force setting to new game as well. don't allow to change
		// 2,3 allow to change
		if(sets->get_use_timeline()&2) {
			// don't change bit1. bit1 affects loading saved game
			sets->set_use_timeline( sets->get_use_timeline()^1 );
			use_intro_dates.pressed = sets->get_use_timeline()&1;
		}
	}
	else if(comp==&use_beginner_mode) {
		sets->beginner_mode = sets->get_beginner_mode()^1;
		use_beginner_mode.pressed = sets->get_beginner_mode();
	}
	else if(comp==&open_setting_gui) {
		gui_frame_t *sg = win_get_magic( magic_settings_frame_t );
		if(  sg  ) {
			destroy_win( sg );
			open_setting_gui.pressed = false;
		}
		else {
			create_win(10, 40, new settings_frame_t(sets), w_info, magic_settings_frame_t );
			open_setting_gui.pressed = true;
		}
	}
	else if(comp==&open_climate_gui) {
		gui_frame_t *climate_gui = win_get_magic( magic_climate );
		if(  climate_gui  ) {
			destroy_win( climate_gui );
			open_climate_gui.pressed = false;
		}
		else {
			climate_gui_t *cg = new climate_gui_t(sets);
			create_win((display_get_width() - cg->get_windowsize().w-10), 40, cg, w_info, magic_climate );
			open_climate_gui.pressed = true;
		}
	}
	else if(comp==&load_game) {
		welt->get_message()->clear();
		create_win( new loadsave_frame_t(true), w_info, magic_load_t);
	}
	else if(comp==&load_scenario) {
		destroy_all_win(true);
		welt->get_message()->clear();
		create_win( new scenario_frame_t(), w_info, magic_load_t );
	}
	else if(comp==&start_game) {
		destroy_all_win(true);
		welt->get_message()->clear();
		create_win(200, 100, new news_img("Erzeuge neue Karte.\n", skinverwaltung_t::neueweltsymbol->get_image_id(0)), w_info, magic_none);
		if(loaded_heightfield) {
			welt->load_heightfield(&env_t::default_settings);
		}
		else {
			env_t::default_settings.heightfield = "";
			welt->init( &env_t::default_settings, 0 );
		}
		destroy_all_win(true);
		welt->step_month( env_t::default_settings.get_starting_month() );
		welt->set_pause(false);
		// save setting ...
		loadsave_t file;
		if(file.wr_open("default.sve",loadsave_t::binary,"settings only",SAVEGAME_VER_NR, EXTENDED_VER_NR, EXTENDED_REVISION_NR)) {
			// save default setting
			env_t::default_settings.rdwr(&file);
			welt->set_scale();
			file.close();
		}
	}
	else if(comp==&quit_game) {
		destroy_all_win(true);
		env_t::quit_simutrans = true;
	}

	if(knr>=0) {
		sets->map_number = knr;
		if(!loaded_heightfield) {
			update_preview();
		}
	}
	return true;
}


bool  welt_gui_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);

	if(ev->ev_class==INFOWIN  &&  ev->ev_code==WIN_CLOSE) {
		close = true;
	}

	return true;
}


void welt_gui_t::draw(scr_coord pos, scr_size size)
{
	cbuffer_t buf;

	// Update child controls before redraw
	if(!loaded_heightfield  && sets->heightfield.size()!=0) {
		if(update_from_heightfield(sets->heightfield.c_str())) {
			loaded_heightfield = true;
		}
		else {
			loaded_heightfield = false;
			sets->heightfield = "";
		}
	}

	// simulate toggle buttons
	open_setting_gui.pressed = win_get_magic( magic_settings_frame_t );
	open_climate_gui.pressed = false;

	if(  win_get_magic( magic_climate )  ) {
		open_climate_gui.pressed = true;
		// check if number was directly changed
		sint16 new_river_number = max( 1, (sint32)(0.5+sqrt((double)sets->get_size_x()*sets->get_size_y())/river_density) );
		if(  sets->get_river_number() != new_river_number  ) {
			river_density = sets->get_river_number() ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_river_number() : 0.0;
		}
	}

	use_intro_dates.pressed = sets->get_use_timeline()&1;
	use_beginner_mode.pressed = sets->get_beginner_mode();

	// Calculate map memory
	const uint sx = sets->get_size_x();
	const uint sy = sets->get_size_y();
	const uint64 memory = (
		(uint64)sizeof(karte_t) +
		sizeof(player_t) * 8 +
		sizeof(convoi_t) * 1000 +
		(sizeof(schiene_t) + sizeof(vehicle_t)) * 10 * (sx + sy) +
		sizeof(stadt_t) * sets->get_city_count() +
		(
			sizeof(grund_t) +
			sizeof(planquadrat_t) +
			sizeof(baum_t)*(1-sets->get_no_trees()) + /* only one since a lot will be water */
			sizeof(void*)*2
		) * (uint64)sx * (uint64)sy
	) / (1024ll * 1024ll);

	const double tile_km = sets->get_meters_per_tile() / 1000.0;
	buf.printf("%s (%d MByte, %.3f km/%s):", translator::translate("Size"), (uint32)memory, tile_km, translator::translate("tile"));
	size_label.set_text(buf);

	cbuffer_t buf_x;
	buf_x.printf("%.2f km", tile_km * inp_x_size.get_value());
	info_x_size.set_text(buf_x, false);

	cbuffer_t buf_y;
	buf_y.printf("%.2f km", tile_km * inp_y_size.get_value());
	info_y_size.set_text(buf_y, false);

	// draw child controls
	gui_frame_t::draw(pos, size);

}
