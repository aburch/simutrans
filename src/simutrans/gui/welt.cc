/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "welt.h"
#include "minimap.h"

#include "../simdebug.h"
#include "../world/simworld.h"
#include "simwin.h"
#include "../display/simimg.h"
#include "../simmesg.h"
#include "../simskin.h"
#include "../simversion.h"

#include "../builder/hausbauer.h"
#include "../builder/wegbauer.h"

#include "../descriptor/building_desc.h"

#include "../dataobj/height_map_loader.h"
#include "../dataobj/settings.h"
#include "../dataobj/loadsave.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

// just for their structure size ...
#include "../obj/way/schiene.h"
#include "../obj/baum.h"
#include "../world/simcity.h"
#include "../vehicle/vehicle.h"
#include "../player/simplay.h"
#include "../simconvoi.h"

#include "../simcolor.h"

#include "../display/simgraph.h"

#include "../sys/simsys.h"
#include "../utils/simstring.h"
#include "../utils/simrandom.h"

#include "components/gui_divider.h"

#include "sprachen.h"
#include "climates.h"
#include "settings_frame.h"
#include "loadsave_frame.h"
#include "load_relief_frame.h"
#include "messagebox.h"
#include "scenario_frame.h"


// Local adjustment
#define L_PREVIEW_SIZE_MIN (16)


welt_gui_t::welt_gui_t(settings_t* const sets_par) :
	gui_frame_t( translator::translate("Neue Welt" ) ),
	map(0,0)
{
	sets = sets_par;
	sets->beginner_mode = env_t::default_settings.get_beginner_mode();

	city_density       = ( sets->get_city_count()      ) ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_city_count()      : 0.0;
	industry_density   = ( sets->get_factory_count()       ) ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_factory_count()       : 0.0;
	attraction_density = ( sets->get_tourist_attractions() ) ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_tourist_attractions() : 0.0;
	river_density      = ( sets->get_river_number()        ) ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_river_number()        : 0.0;

	// find earliest start and end date ...
	uint16 game_start = 4999;
	uint16 game_ends = 0;

	// first check town halls
	for(building_desc_t const* const desc : *hausbauer_t::get_list(building_desc_t::townhall)) {
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
	set_table_layout(1,0);
	// top part: preview, maps size
	new_component<gui_label_t>("1WORLD_CHOOSE");
	new_component<gui_divider_t>();
	add_table(3,1);
	{
		// input fields
		add_table(2,3);
		{
			new_component<gui_label_t>("2WORLD_CHOOSE");
			inp_map_number.init( abs(sets->get_map_number()), 0, 0x7FFFFFFF, 1, true );
			inp_map_number.add_listener( this );
			add_component( &inp_map_number );

			// Map size label
			size_label.init();
			size_label.buf().printf(translator::translate("Size (%d MB):"), 9999);
			size_label.update();
			add_component( &size_label );

			// Map X size edit
			inp_x_size.init( sets->get_size_x(), 8, 32766, sets->get_size_x()>=512 ? 128 : 64, false );
			inp_x_size.add_listener(this);
			add_component( &inp_x_size );

			new_component<gui_empty_t>(&size_label);

			// Map size Y edit
			inp_y_size.init( sets->get_size_y(), 8, 32766, sets->get_size_y()>=512 ? 128 : 64, false );
			inp_y_size.add_listener(this);
			add_component( &inp_y_size );
		}
		end_table();

		new_component<gui_fill_t>();
		// Map preview (will be initialized in update_preview
		add_component( &map_preview );
	}
	end_table();

	// two buttons
	add_table(2,0)->set_force_equal_columns(true);
	{
		// Random map button
		random_map.init(button_t::roundbox | button_t::flexible, "Random map");
		random_map.set_tooltip("chooses a random map");
		random_map.add_listener( this );
		add_component( &random_map );

		// Load height map button
		load_map.init(button_t::roundbox | button_t::flexible, "Lade Relief");
		load_map.set_tooltip("load height data from file");
		load_map.add_listener( this );
		add_component( &load_map );
	}
	end_table();

	// specify map parameters
	add_table(2,0);
	{
		// Number of towns
		new_component<gui_label_t>("5WORLD_CHOOSE");
		inp_number_of_towns.add_listener(this);
		inp_number_of_towns.init(abs(sets->get_city_count()), 0, 999);
		add_component( &inp_number_of_towns );

		// Town size
		new_component<gui_label_t>("Median Citizen per town");
		inp_town_size.add_listener(this);
		inp_town_size.set_limits(0,999999);
		inp_town_size.set_increment_mode(50);
		inp_town_size.set_value( sets->get_mean_citizen_count() );
		add_component( &inp_town_size );

		// Intercity road length
		new_component<gui_label_t>("Intercity road len:");
		inp_intercity_road_len.add_listener(this);
		inp_intercity_road_len.set_limits(0,9999);
		inp_intercity_road_len.set_value( env_t::intercity_road_length );
		inp_intercity_road_len.set_increment_mode( env_t::intercity_road_length>=1000 ? 100 : 20 );
		add_component( &inp_intercity_road_len );

		// Factories
		new_component<gui_label_t>("No. of Factories");
		inp_other_industries.add_listener(this);
		inp_other_industries.set_limits(0,999);
		inp_other_industries.set_value(abs(sets->get_factory_count()) );
		add_component( &inp_other_industries );

		// Tourist attr.
		new_component<gui_label_t>("Tourist attractions");
		inp_tourist_attractions.add_listener(this);
		inp_tourist_attractions.set_limits(0,999);
		inp_tourist_attractions.set_value(abs(sets->get_tourist_attractions()) );
		add_component( &inp_tourist_attractions );

		// Use timeline checkbox
		use_intro_dates.init(button_t::square_state, "Use timeline start year");
		use_intro_dates.pressed = sets->get_use_timeline()&1;
		use_intro_dates.add_listener( this );
		add_component( &use_intro_dates );

		// Timeline year edit
		inp_intro_date.add_listener(this);
		inp_intro_date.set_limits(game_start,game_ends);
		inp_intro_date.set_increment_mode(10);
		inp_intro_date.set_value(abs(sets->get_starting_year()) );
		add_component( &inp_intro_date );


		// Use beginner mode checkbox
		use_beginner_mode.init(button_t::square_state, "Use beginner mode");
		use_beginner_mode.set_tooltip("Higher transport fees, crossconnect all factories");
		use_beginner_mode.pressed = sets->get_beginner_mode();
		use_beginner_mode.add_listener( this );
		add_component( &use_beginner_mode );
	}
	end_table();

	new_component<gui_divider_t>();

	add_table(2,1)->set_force_equal_columns(true);
	{
		// Map settings button
		open_setting_gui.init(button_t::roundbox | button_t::flexible, "Setting");
		open_setting_gui.pressed = win_get_magic( magic_settings_frame_t );
		open_setting_gui.add_listener( this );
		add_component( &open_setting_gui );

		// Landscape settings button
		open_climate_gui.init(button_t::roundbox | button_t::flexible,"Climate Control");
		open_climate_gui.pressed = win_get_magic( magic_climate );
		open_climate_gui.add_listener( this );
		add_component( &open_climate_gui );
	}
	end_table();

	new_component<gui_divider_t>();

	add_table(2,2)->set_force_equal_columns(true);
	{
		// load game
		load_game.init(button_t::roundbox | button_t::flexible, "Load game");
		load_game.add_listener( this );
		add_component( &load_game );

		// load scenario
		load_scenario.init(button_t::roundbox | button_t::flexible,"Load scenario");
		load_scenario.add_listener( this );
		add_component( &load_scenario );

		// start game
		start_game.init(button_t::roundbox | button_t::flexible, "Starte Spiel");
		start_game.add_listener( this );
		add_component( &start_game );

		// quit game
		quit_game.init(button_t::roundbox | button_t::flexible,"Beenden");
		quit_game.add_listener( this );
		add_component( &quit_game );
	}
	end_table();

	update_preview();
	update_memory(&size_label, sets);

	reset_min_windowsize();
	set_windowsize(get_min_windowsize());
	set_resizemode(gui_frame_t::diagonal_resize);
}


/**
 * Calculates preview from height map
 * @param filename name of heightfield file
 */
bool welt_gui_t::update_from_heightfield(const char *filename)
{
	DBG_MESSAGE("welt_gui_t::update_from_heightfield()", "%s", filename);

	const sint8 min_h = env_t::default_settings.get_minimumheight();
	const sint8 max_h = env_t::default_settings.get_maximumheight();

	height_map_loader_t hml(min_h, max_h, env_t::height_conv_mode);
	sint16 w, h;
	sint8 *h_field=NULL;

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
				map.at(x,y) = minimap_t::calc_height_color( h_field[x*mx+y*my*w], sets->get_groundwater()-1 );
			}
		}
		map_preview.set_map_data(&map);
		free(h_field);
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
		inp_other_industries.set_value( max( 1, (sint32)(0.5+sqrt((double)sets->get_size_x()*sets->get_size_y())/industry_density) ) );
		sets->set_factory_count( inp_other_industries.get_value() );
	}
	if(  attraction_density!=0.0  ) {
		inp_tourist_attractions.set_value( max( 1, (sint32)(0.5+sqrt((double)sets->get_size_x()*sets->get_size_y())/attraction_density) ) );
		sets->set_tourist_attractions( inp_tourist_attractions.get_value() );
	}
	if(  river_density!=0.0  ) {
		sets->river_number = max( 1, (sint32)(0.5+sqrt((double)sets->get_size_x()*sets->get_size_y())/river_density) );
		if(  climate_gui_t *climate_gui = (climate_gui_t *)win_get_magic( magic_climate )  ) {
			climate_gui->update_river_number( sets->get_river_number() );
		}
	}
}

void welt_gui_t::update_memory(gui_label_buf_t *label, const settings_t* sets)
{
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
			sizeof(baum_t)*(sets->get_tree_distribution()!=settings_t::TREE_DIST_NONE) + /* only one since a lot will be water */
			sizeof(void*)*2
		) * (uint64)sx * (uint64)sy
	) / (1024ll * 1024ll);

	label->buf().printf(translator::translate("Size (%d MB):"), memory);
	label->update();
}


/**
 * Calculate the new Map-Preview. Initialize the new RNG!
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

		const int mx = sets->get_size_x()/map_size.w;
		const int my = sets->get_size_y()/map_size.h;
		for(  int y=0;  y<map_size.h;  y++  ) {
			for(  int x=0;  x<map_size.w;  x++  ) {
				map.at(x,y) = minimap_t::calc_height_color(karte_t::perlin_hoehe( sets, koord(x*mx,y*my), koord::invalid ), sets->get_groundwater());
			}
		}
		sets->heightfield = "";
	}
	map_preview.set_map_data(&map);
}


void welt_gui_t::resize_preview()
{
	const float world_aspect = (float)sets->get_size_x() / (float)sets->get_size_y();

	if(  world_aspect > 1.0  ) {
		map_size.w = MAP_PREVIEW_SIZE_X-2;
		map_size.h = (sint16) max( (int)((float)map_size.w / world_aspect), L_PREVIEW_SIZE_MIN-2);
	}
	else {
		map_size.h = MAP_PREVIEW_SIZE_Y-2;
		map_size.w = (sint16) max( (int)((float)map_size.h * world_aspect), L_PREVIEW_SIZE_MIN-2);
	}
	map.resize( map_size.w, map_size.h );
}


/**
 * This method is called if an action is triggered
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
			inp_y_size.set_limits( 8, 32766 );
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
			inp_x_size.set_limits( 8, 32766 );
			update_densities();
		}
		else {
			inp_y_size.set_value(sets->get_size_y()); // can't change size with heightfield loaded
		}
	}
	else if(comp==&inp_number_of_towns) {
		sets->set_city_count( v.i );
		city_density = sets->get_city_count() ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_city_count() : 0.0;
	}
	else if(comp==&inp_town_size) {
		sets->set_mean_citizen_count( v.i );
	}
	else if(comp==&inp_intercity_road_len) {
		env_t::intercity_road_length = v.i;
		inp_intercity_road_len.set_increment_mode( v.i>=1000 ? 100 : 20 );
	}
	else if(comp==&inp_other_industries) {
		sets->set_factory_count( v.i );
		industry_density = sets->get_factory_count() ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_factory_count() : 0.0;
	}
	else if(comp==&inp_tourist_attractions) {
		sets->set_tourist_attractions( v.i );
		attraction_density = sets->get_tourist_attractions() ? sqrt((double)sets->get_size_x()*sets->get_size_y()) / sets->get_tourist_attractions() : 0.0;
	}
	else if(comp==&inp_intro_date) {
		sets->set_starting_year( (sint16)(v.i) );
	}
	else if(comp==&random_map) {
		knr = simrand(9999);
		inp_map_number.set_value(knr);
		sets->heightfield = "";
		loaded_heightfield = false;
	}
	else if(comp==&load_map) {
		// load relief
		loaded_heightfield = false;
		sets->heightfield = "";
		load_relief_frame_t* lrf = new load_relief_frame_t(sets);
		create_win(lrf, w_info, magic_load_t );
		win_set_pos(lrf, (display_get_width() - lrf->get_windowsize().w-10), env_t::iconsize.h);
		knr = sets->get_map_number(); // otherwise using cancel would not show the normal generated map again
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
		if(  file.wr_open("default.sve",loadsave_t::binary,0,"settings only",SAVEGAME_VER_NR) == loadsave_t::FILE_STATUS_OK  ) {
			// save default setting
			env_t::default_settings.rdwr(&file);
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
	// Coordinates are relative to parent (TITLEHEIGHT **NOT** subtracted)
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

	update_memory(&size_label, sets);
	// draw child controls
	gui_frame_t::draw(pos, size);
}
