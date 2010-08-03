/*
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include <string>
#include "einstellungen.h"
#include "umgebung.h"
#include "../simconst.h"
#include "../simtools.h"
#include "../simtypes.h"
#include "../simdebug.h"
#include "../bauer/wegbauer.h"
#include "../besch/weg_besch.h"
#include "../utils/simstring.h"
#include "../vehicle/simvehikel.h"
#include "../player/simplay.h"
#include "loadsave.h"
#include "tabfile.h"


#define NEVER 0xFFFFU


einstellungen_t::einstellungen_t() :
	filename(""),
	heightfield("")
{
	groesse_x = 256;
	groesse_y = 256;

	nummer = 33;

	/* new setting since version 0.85.01
	 * @author prissi
	 */
	land_industry_chains = 4;
	tourist_attractions = 16;

	anzahl_staedte = 16;
	mittlere_einwohnerzahl = 1600;

	station_coverage_size = 2;

	verkehr_level = 5;

	show_pax = true;

	// default climate zones
	set_default_climates( );
	winter_snowline = 7;	// not mediterran
	grundwasser = -2*Z_TILE_STEP;            //25-Nov-01        Markus Weber    Added

	max_mountain_height = 160;                  //can be 0-160.0  01-Dec-01        Markus Weber    Added
	map_roughness = 0.6;                        //can be 0-1      01-Dec-01        Markus Weber    Added

	river_number = 16;
	min_river_length = 16;
	max_river_length = 256;

	// forest setting ...
	forest_base_size = 36; 	// Base forest size - minimal size of forest - map independent
	forest_map_size_divisor = 38;	// Map size divisor - smaller it is the larger are individual forests
	forest_count_divisor = 16;	// Forest count divisor - smaller it is, the more forest are generated
	forest_inverse_spare_tree_density = 5;	// Determins how often are spare trees going to be planted (works inversly)
	max_no_of_trees_on_square = 3;	// Number of trees on square 2 - minimal usable, 3 good, 5 very nice looking
	tree_climates = 0;	// bit set, if this climate is to be covered with trees entirely
	no_tree_climates = 0;	// bit set, if this climate is to be void of random trees
	no_trees = false;	// if set, no trees at all, may be useful for low end engines

	// some settigns more
	allow_player_change = true;
	use_timeline = 2;
	starting_year = 1930;
	starting_month = 0;
	bits_per_month = 20;

	beginner_mode = false;
	beginner_price_factor = 1500;

	rotation = 0;

	origin_x = origin_y = 0;

	// passenger manipulation factor (=16 about old value)
	passenger_factor = 16;

	// town growth factors
	passenger_multiplier = 40;
	mail_multiplier = 20;
	goods_multiplier = 20;
	electricity_multiplier = 0;

	// Also there are size dependen factors (0 causes crash !)
	growthfactor_small = 400;
	growthfactor_medium = 200;
	growthfactor_large = 100;

	minimum_city_distance = 16;
	industry_increase = 2000;

	factory_worker_percentage = 33;
	tourist_percentage = 16;
	factory_worker_radius = 77;
	// try to have at least a single town connected to a factory
	factory_worker_minimum_towns = 1;
	// not more than four towns should supply to a factory
	factory_worker_maximum_towns = 4;

	electric_promille = 330;

#ifdef OTTD_LIKE
	/* prissi: crossconnect all factories (like OTTD and similar games) */
	crossconnect_factories=true;
	crossconnect_factor=100;
#else
	/* prissi: crossconnect a certain number */
	crossconnect_factories=false;
	crossconnect_factor=33;
#endif

	/* minimum spacing between two factories */
	factory_spacing = 6;

	/* prissi: do not distribute goods to overflowing factories */
	just_in_time = true;

	fussgaenger = true;
	stadtauto_duration = 36;	// three years

	// to keep names consistent
	numbered_stations = false;

	num_city_roads = 0;
	num_intercity_roads = 0;

	max_route_steps = 1000000;
	max_transfers = 9;
	max_hops = 2000;
	no_routing_over_overcrowding = false;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, faktor 2, vehicle will be too long and too fast
	 * 724: correct one, faktor sqrt(2)
	 */
	pak_diagonal_multiplier = 724;

	strcpy( language_code_names, "en" );

	// default AIs active
	for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
		if(  i<2  ) {
			automaten[i] = true;
			spieler_type[i] = spieler_t::HUMAN;
		}
		else if(  i==3  ) {
			automaten[i] = true;
			spieler_type[i] = spieler_t::AI_PASSENGER;
		}
		else if(  i==6  ) {
			automaten[i] = true;
			spieler_type[i] = spieler_t::AI_GOODS;
		}
		else {
			automaten[i] = false;
			spieler_type[i] = spieler_t::EMPTY;
		}
	}

	/* the big cost section */
	freeplay = false;
	starting_money = 20000000;
	maint_building = 5000;	// normal buildings

	// stop buildings
	cst_multiply_dock=-50000;
	cst_multiply_station=-60000;
	cst_multiply_roadstop=-40000;
	cst_multiply_airterminal=-300000;
	cst_multiply_post=-30000;
	cst_multiply_headquarter=-100000;
	cst_depot_rail=-100000;
	cst_depot_road=-130000;
	cst_depot_ship=-250000;
	cst_depot_air=-500000;
	// alter landscape
	cst_buy_land=-10000;
	cst_alter_land=-100000;
	cst_set_slope=-250000;
	cst_found_city=-500000000;
	cst_multiply_found_industry=-2000000;
	cst_remove_tree=-10000;
	cst_multiply_remove_haus=-100000;
	cst_multiply_remove_field=-500000;
	// cost for transformers
	cst_transformer=-250000;
	cst_maintain_transformer=-2000;

	// costs for the way searcher
	way_count_straight=1;
	way_count_curve=2;
	way_count_double_curve=6;
	way_count_90_curve=15;
	way_count_slope=10;
	way_count_tunnel=8;
	way_max_bridge_len=15;
	way_count_leaving_road=25;

	// defualt: joined capacities
	seperate_halt_capacities = false;

	// this will pay for distance to next change station
	pay_for_total_distance = TO_PREVIOUS;

	avoid_overcrowding = false;

	allow_buying_obsolete_vehicles = true;

	// default: load also private extensions of the pak file
	with_private_paks = true;

	// some network thing to keep client in sync
	random_counter = 0;	// will be set when actually saving
	frames_per_second = 20;
	frames_per_step = 4;
}



void einstellungen_t::set_default_climates()
{
	static sint16 borders[MAX_CLIMATES] = { 0, 0, 0, 3, 6, 8, 10, 10 };
	memcpy( climate_borders, borders, sizeof(sint16)*MAX_CLIMATES );
}



void einstellungen_t::rdwr(loadsave_t *file)
{
	xml_tag_t e( file, "einstellungen_t" );

	if(file->get_version() < 86000) {
		uint32 dummy;

		file->rdwr_long(groesse_x );
		groesse_y = groesse_x;

		file->rdwr_long(nummer );

		// to be compatible with previous savegames
		dummy = 0;
		file->rdwr_long(dummy );	//dummy!
		land_industry_chains = 6;
		tourist_attractions = 12;

		// now towns
		mittlere_einwohnerzahl = 1600;
		dummy =  anzahl_staedte;
		file->rdwr_long(dummy );
		dummy &= 127;
		if(dummy>63) {
			dbg->warning("einstellungen_t::rdwr()","This game was saved with too many cities! (%i of maximum 63). Simutrans may crash!",dummy );
		}
		anzahl_staedte = dummy;

		// rest
		file->rdwr_long(dummy );	// scroll ingnored
		file->rdwr_long(verkehr_level );
		file->rdwr_long(show_pax );
		dummy = grundwasser;
		file->rdwr_long(dummy );
		grundwasser = (sint16)(dummy/16)*Z_TILE_STEP;
		file->rdwr_double(max_mountain_height );
		file->rdwr_double(map_roughness );

		station_coverage_size = 3;
		beginner_mode = false;
		rotation = 0;
	}
	else {
		// newer versions
		file->rdwr_long(groesse_x );
		file->rdwr_long(nummer );

		// industries
		file->rdwr_long(land_industry_chains );
		if(file->get_version()<99018) {
			uint32 dummy;	// was city chains
			file->rdwr_long(dummy );
		}
		else {
			file->rdwr_long( electric_promille );
		}
		file->rdwr_long(tourist_attractions );

		// now towns
		file->rdwr_long(mittlere_einwohnerzahl );
		file->rdwr_long(anzahl_staedte );

		// rest
		if(file->get_version() < 101000) {
			uint32 dummy;	// was scroll dir
			file->rdwr_long(dummy );
		}
		file->rdwr_long(verkehr_level );
		file->rdwr_long(show_pax );
		sint32 dummy = grundwasser/Z_TILE_STEP;
		file->rdwr_long(dummy );
		if(file->get_version() < 99005) {
			grundwasser = (sint16)(dummy/16)*Z_TILE_STEP;
		}
		else {
			grundwasser = (sint16)dummy*Z_TILE_STEP;
		}
		file->rdwr_double(max_mountain_height );
		file->rdwr_double(map_roughness );

		if(file->get_version() >= 86003) {
			dummy = station_coverage_size;
			file->rdwr_long(dummy );
			station_coverage_size = (uint16)dummy;
		}

		if(file->get_version() >= 86006) {
			// handle also size on y direction
			file->rdwr_long(groesse_y );
		}
		else {
			groesse_y = groesse_x;
		}

		if(file->get_version() >= 86011) {
			// some more settings
			file->rdwr_byte(allow_player_change );
			file->rdwr_byte(use_timeline );
			file->rdwr_short(starting_year );
		}
		else {
			allow_player_change = 1;
			use_timeline = 1;
			starting_year = 1930;
		}

		if(file->get_version()>=88005) {
			file->rdwr_short(bits_per_month );
		}
		else {
			bits_per_month = 18;
		}

		if(file->get_version()>=89003) {
			file->rdwr_bool(beginner_mode );
		}
		else {
			beginner_mode = false;
		}
		if(file->get_version()>=89004) {
			file->rdwr_bool(just_in_time );
		}
		// rotation of the map with respect to the original value
		if(file->get_version()>=99015) {
			file->rdwr_byte(rotation );
		}
		else {
			rotation = 0;
		}

		// clear the name when loading ...
		if(file->is_loading()) {
			filename = "";
		}

		// climate corders
		if(file->get_version()>=91000) {
			for(  int i=0;  i<8;  i++ ) {
				file->rdwr_short(climate_borders[i] );
			}
			file->rdwr_short(winter_snowline );
		}

		// since vehicle will need realignment afterwards!
		if(file->get_version()<=99018) {
			vehikel_basis_t::set_diagonal_multiplier( pak_diagonal_multiplier, 1024 );
		}
		else {
			uint16 old_multiplier = pak_diagonal_multiplier;
			file->rdwr_short(old_multiplier );
			vehikel_basis_t::set_diagonal_multiplier( pak_diagonal_multiplier, old_multiplier );
			// since vehicle will need realignment afterwards!
		}

		if(file->get_version()>=101000) {
			// game mechanics
			file->rdwr_short(origin_x );
			file->rdwr_short(origin_y );

			file->rdwr_long(passenger_factor );

			// town grow stuff
			if(file->get_version()>102001) {
				file->rdwr_long(passenger_multiplier );
				file->rdwr_long(mail_multiplier );
				file->rdwr_long(goods_multiplier );
				file->rdwr_long(electricity_multiplier );
				file->rdwr_long(growthfactor_small );
				file->rdwr_long(growthfactor_medium );
				file->rdwr_long(growthfactor_large );
				file->rdwr_short(factory_worker_percentage );
				file->rdwr_short(tourist_percentage );
				file->rdwr_short(factory_worker_radius );
			}

			file->rdwr_long(electric_promille );

			file->rdwr_short(factory_spacing );
			file->rdwr_bool(crossconnect_factories );
			file->rdwr_short(crossconnect_factor );

			file->rdwr_bool(fussgaenger );
			file->rdwr_long(stadtauto_duration );

			file->rdwr_bool(numbered_stations );
			if(  file->get_version()<=102002  ) {
				if(  file->is_loading()  ) {
					num_city_roads = 1;
					city_roads[0].intro = 0;
					city_roads[0].retire = 0;
					// intercity roads were not saved in old savegames
					num_intercity_roads = 0;
				}
				file->rdwr_str(city_roads[0].name, lengthof(city_roads[0].name) );
			}
			else {
				// several roads ...
				file->rdwr_short(num_city_roads );
				if(  num_city_roads>=10  ) {
					dbg->fatal( "einstellungen_t::rdwr()", "Too many (%i) city roads!", num_city_roads );
				}
				for(  int i=0;  i<num_city_roads;  i++  ) {
					file->rdwr_str(city_roads[i].name, lengthof(city_roads[i].name) );
					file->rdwr_short(city_roads[i].intro );
					file->rdwr_short(city_roads[i].retire );
				}
				// several intercity roads ...
				file->rdwr_short(num_intercity_roads );
				if(  num_intercity_roads>=10  ) {
					dbg->fatal( "einstellungen_t::rdwr()", "Too many (%i) intercity roads!", num_intercity_roads );
				}
				for(  int i=0;  i<num_intercity_roads;  i++  ) {
					file->rdwr_str(intercity_roads[i].name, lengthof(intercity_roads[i].name) );
					file->rdwr_short(intercity_roads[i].intro );
					file->rdwr_short(intercity_roads[i].retire );
				}
			}
			file->rdwr_long(max_route_steps );
			file->rdwr_long(max_transfers );
			file->rdwr_long(max_hops );

			file->rdwr_long(beginner_price_factor );

			// name of stops
			file->rdwr_str(language_code_names, lengthof(language_code_names) );

			// restore AI state
			for(  int i=0;  i<15;  i++  ) {
				file->rdwr_bool(automaten[i] );
				file->rdwr_byte(spieler_type[i] );
				if(  file->get_version()<=102002  ) {
					char dummy[2] = { 0, 0 };
					file->rdwr_str(dummy, lengthof(dummy) );
				}
			}

			// cost section ...
			file->rdwr_bool(freeplay );
			if(  file->get_version()>102002  ) {
				file->rdwr_longlong(starting_money );
				// these must be saved, since new player will get different amounts eventually
				for(  int i=0;  i<10;  i++  ) {
					file->rdwr_short(startingmoneyperyear[i].year );
					file->rdwr_longlong(startingmoneyperyear[i].money );
					file->rdwr_bool(startingmoneyperyear[i].interpol );
				}
			}
			else {
				// compatibility code
				sint64 save_starting_money = starting_money;
				if(file->is_saving()) {
					if(save_starting_money==0) save_starting_money = get_starting_money(starting_year );
					if(save_starting_money==0) save_starting_money = umgebung_t::default_einstellungen.get_starting_money(starting_year );
					if(save_starting_money==0) save_starting_money = 20000000;
				}
				file->rdwr_longlong(save_starting_money );
				if(file->is_loading()) {
					if(save_starting_money==0) save_starting_money = umgebung_t::default_einstellungen.get_starting_money(starting_year );
					if(save_starting_money==0) save_starting_money = 20000000;
					starting_money = save_starting_money;
				}
			}
			file->rdwr_long(maint_building );

			file->rdwr_longlong(cst_multiply_dock );
			file->rdwr_longlong(cst_multiply_station );
			file->rdwr_longlong(cst_multiply_roadstop );
			file->rdwr_longlong(cst_multiply_airterminal );
			file->rdwr_longlong(cst_multiply_post );
			file->rdwr_longlong(cst_multiply_headquarter );
			file->rdwr_longlong(cst_depot_rail );
			file->rdwr_longlong(cst_depot_road );
			file->rdwr_longlong(cst_depot_ship );
			file->rdwr_longlong(cst_depot_air );
			if(  file->get_version()<=102001  ) {
				sint64 dummy64 = 100000;
				file->rdwr_longlong(dummy64 );
				file->rdwr_longlong(dummy64 );
				file->rdwr_longlong(dummy64 );
			}
			// alter landscape
			file->rdwr_longlong(cst_buy_land );
			file->rdwr_longlong(cst_alter_land );
			file->rdwr_longlong(cst_set_slope );
			file->rdwr_longlong(cst_found_city );
			file->rdwr_longlong(cst_multiply_found_industry );
			file->rdwr_longlong(cst_remove_tree );
			file->rdwr_longlong(cst_multiply_remove_haus );
			file->rdwr_longlong(cst_multiply_remove_field );
			// cost for transformers
			file->rdwr_longlong(cst_transformer );
			file->rdwr_longlong(cst_maintain_transformer );
			// wayfinder
			file->rdwr_long(way_count_straight );
			file->rdwr_long(way_count_curve );
			file->rdwr_long(way_count_double_curve );
			file->rdwr_long(way_count_90_curve );
			file->rdwr_long(way_count_slope );
			file->rdwr_long(way_count_tunnel );
			file->rdwr_long(way_max_bridge_len );
			file->rdwr_long(way_count_leaving_road );
		}
		else {
			// name of stops
			set_name_language_iso( umgebung_t::language_iso );

			// default AIs active
			for(  int i=0;  i<MAX_PLAYER_COUNT;  i++  ) {
				if(  i<2  ) {
					spieler_type[i] = spieler_t::HUMAN;
				}
				else if(  i==3  ) {
					spieler_type[i] = spieler_t::AI_PASSENGER;
				}
				else if(  i<8  ) {
					spieler_type[i] = spieler_t::AI_GOODS;
				}
				else {
					spieler_type[i] = spieler_t::EMPTY;
				}
				automaten[i] = false;
			}
		}

		if(file->get_version()>101000) {
			file->rdwr_bool( seperate_halt_capacities );
			file->rdwr_byte( pay_for_total_distance );

			file->rdwr_short(starting_month );

			file->rdwr_short( river_number );
			file->rdwr_short( min_river_length );
			file->rdwr_short( max_river_length );
		}

		if(file->get_version()>102000) {
			file->rdwr_bool( avoid_overcrowding );
		}
		if(file->get_version()>102001) {
			file->rdwr_bool( no_routing_over_overcrowding );
			file->rdwr_bool( with_private_paks );
		}
		if(file->get_version()>=102003) {
			// network stuff
			random_counter = get_random_seed( );
			file->rdwr_long( random_counter );
			if(  !umgebung_t::networkmode  ||  umgebung_t::server  ) {
				frames_per_second = clamp(umgebung_t::fps,5,100 );	// update it on the server to the current setting
				frames_per_step = umgebung_t::network_frames_per_step;
			}
			file->rdwr_long( frames_per_second );
			file->rdwr_long( frames_per_step );
			if(  !umgebung_t::networkmode  ||  umgebung_t::server  ) {
				frames_per_second = umgebung_t::fps;	// update it on the server to the current setting
				frames_per_step = umgebung_t::network_frames_per_step;
			}
			file->rdwr_bool( allow_buying_obsolete_vehicles );
			file->rdwr_long( factory_worker_minimum_towns );
			file->rdwr_long( factory_worker_maximum_towns );
			// forest stuff
			file->rdwr_byte( forest_base_size );
			file->rdwr_byte( forest_map_size_divisor );
			file->rdwr_byte( forest_count_divisor );
			file->rdwr_short( forest_inverse_spare_tree_density );
			file->rdwr_byte( max_no_of_trees_on_square );
			file->rdwr_short( tree_climates );
			file->rdwr_short( no_tree_climates );
			file->rdwr_bool( no_trees );
			file->rdwr_long( minimum_city_distance );
			file->rdwr_long( industry_increase );
		}
	}
}




// read the settings from this file
void einstellungen_t::parse_simuconf( tabfile_t &simuconf, sint16 &disp_width, sint16 &disp_height, sint16 &fullscreen, std::string &objfilename )
{
	tabfileobj_t contents;

	simuconf.read(contents );

	umgebung_t::water_animation = contents.get_int("water_animation_ms", umgebung_t::water_animation );
	umgebung_t::ground_object_probability = contents.get_int("random_grounds_probability", umgebung_t::ground_object_probability );
	umgebung_t::moving_object_probability = contents.get_int("random_wildlife_probability", umgebung_t::moving_object_probability );
	umgebung_t::drive_on_left = contents.get_int("drive_left", umgebung_t::drive_on_left );

	umgebung_t::verkehrsteilnehmer_info = contents.get_int("pedes_and_car_info", umgebung_t::verkehrsteilnehmer_info) != 0;
	umgebung_t::tree_info = contents.get_int("tree_info", umgebung_t::tree_info) != 0;
	umgebung_t::ground_info = contents.get_int("ground_info", umgebung_t::ground_info) != 0;
	umgebung_t::townhall_info = contents.get_int("townhall_info", umgebung_t::townhall_info) != 0;
	umgebung_t::single_info = contents.get_int("only_single_info", umgebung_t::single_info );

	umgebung_t::window_buttons_right = contents.get_int("window_buttons_right", umgebung_t::window_buttons_right );
	umgebung_t::left_to_right_graphs = contents.get_int("left_to_right_graphs", umgebung_t::left_to_right_graphs );
	umgebung_t::window_frame_active = contents.get_int("window_frame_active", umgebung_t::window_frame_active );
	umgebung_t::front_window_bar_color = contents.get_int("front_window_bar_color", umgebung_t::front_window_bar_color );
	umgebung_t::front_window_text_color = contents.get_int("front_window_text_color", umgebung_t::front_window_text_color );
	umgebung_t::bottom_window_bar_color = contents.get_int("bottom_window_bar_color", umgebung_t::bottom_window_bar_color );
	umgebung_t::bottom_window_text_color = contents.get_int("bottom_window_text_color", umgebung_t::bottom_window_text_color );

	umgebung_t::show_tooltips = contents.get_int("show_tooltips", umgebung_t::show_tooltips );
	umgebung_t::tooltip_color = contents.get_int("tooltip_background_color", umgebung_t::tooltip_color );
	umgebung_t::tooltip_textcolor = contents.get_int("tooltip_text_color", umgebung_t::tooltip_textcolor );
	umgebung_t::tooltip_delay = contents.get_int("tooltip_delay", umgebung_t::tooltip_delay );
	umgebung_t::tooltip_duration = contents.get_int("tooltip_duration", umgebung_t::tooltip_duration );
	umgebung_t::toolbar_max_width = contents.get_int("toolbar_max_width", umgebung_t::toolbar_max_width );
	umgebung_t::toolbar_max_height = contents.get_int("toolbar_max_height", umgebung_t::toolbar_max_height );
	umgebung_t::cursor_overlay_color = contents.get_int("cursor_overlay_color", umgebung_t::cursor_overlay_color );

	// display stuff
	umgebung_t::show_names = contents.get_int("show_names", umgebung_t::show_names );
	umgebung_t::show_month = contents.get_int("show_month", umgebung_t::show_month );
	umgebung_t::max_acceleration = contents.get_int("fast_forward", umgebung_t::max_acceleration );

	// network stuff
	umgebung_t::server_frames_ahead = contents.get_int("server_frames_ahead", umgebung_t::server_frames_ahead );
	umgebung_t::server_ms_ahead = contents.get_int("network_ms_ahead", umgebung_t::server_ms_ahead );
	umgebung_t::network_frames_per_step = contents.get_int("server_frames_per_step", umgebung_t::network_frames_per_step );
	umgebung_t::server_sync_steps_between_checks = contents.get_int("server_frames_between_checks", umgebung_t::server_sync_steps_between_checks );

	// up to ten rivers are possible
	for(  int i = 0;  i<10;  i++  ) {
		char name[32];
		sprintf( name, "river_type[%i]", i );
		const char *test = ltrim(contents.get(name) );
		if(*test) {
			const int add_river = i<umgebung_t::river_types ? i : umgebung_t::river_types;
			free( (void *)(umgebung_t::river_type[add_river]) );
			umgebung_t::river_type[add_river] = NULL;
			umgebung_t::river_type[add_river] = strdup( test );
			if(  add_river==umgebung_t::river_types  ) {
				umgebung_t::river_types++;
			}
		}
	}

	// old syntax for single city road
	const char *str = ltrim(contents.get("city_road_type") );
	if(str[0]==0) {
		// old fallback value
		str = "city_road";
	}
	num_city_roads = 1;
	tstrncpy(city_roads[0].name, str, lengthof(city_roads[0].name) );
	rtrim( city_roads[0].name );
	// default her: always available
	city_roads[0].intro = 1;
	city_roads[0].retire = NEVER;

	// new: up to ten city_roads are possible
	if(  *contents.get("city_road[0]")  ) {
		// renew them always when a table is encountered ...
		num_city_roads = 0;
		for(  int i = 0;  i<10;  i++  ) {
			char name[256];
			sprintf( name, "city_road[%i]", i );
			// format is "city_road[%i]=name_of_road,using from (year), using to (year)"
			const char *test = ltrim(contents.get(name) );
			if(*test) {
				const char *p = test;
				while(*p  &&  *p!=','  ) {
					p++;
				}
				tstrncpy( city_roads[num_city_roads].name, test, (unsigned)(p-test)+1 );
				// default her: intro/retire=0 -> set later to intro/retire of way-besch
				city_roads[num_city_roads].intro = 0;
				city_roads[num_city_roads].retire = 0;
				if(  *p==','  ) {
					++p;
					city_roads[num_city_roads].intro = atoi(p)*12;
					while(*p  &&  *p!=','  ) {
						p++;
					}
				}
				if(  *p==','  ) {
					city_roads[num_city_roads].retire = atoi(p+1)*12;
				}
				num_city_roads ++;
			}
		}
	}
	if (num_city_roads == 0) {
		// take fallback value: "city_road"
		tstrncpy(city_roads[0].name, "city_road", lengthof(city_roads[0].name) );
		rtrim( city_roads[0].name );
		// default her: always available
		city_roads[0].intro = 1;
		city_roads[0].retire = NEVER;
		num_city_roads = 1;
	}

	// intercity road
	// old syntax for single intercity road
	str = ltrim(contents.get("intercity_road_type") );
	if(str[0]==0) {
		str = "asphalt_road";
	}
	num_intercity_roads = 1;
	tstrncpy(intercity_roads[0].name, str, lengthof(intercity_roads[0].name) );
	rtrim( intercity_roads[0].name );
	// default her: always available
	intercity_roads[0].intro = 0;
	intercity_roads[0].retire = NEVER;

	// new: up to ten intercity_roads are possible
	if(  *contents.get("intercity_road[0]")  ) {
		// renew them always when a table is encountered ...
		num_intercity_roads = 0;
		for(  int i = 0;  i<10;  i++  ) {
			char name[256];
			sprintf( name, "intercity_road[%i]", i );
			// format is "intercity_road[%i]=name_of_road,using from (year), using to (year)"
			const char *test = ltrim(contents.get(name) );
			if(*test) {
				const char *p = test;
				while(*p  &&  *p!=','  ) {
					p++;
				}
				tstrncpy( intercity_roads[num_intercity_roads].name, test, (unsigned)(p-test)+1 );
				// default her: intro/retire=0 -> set later to intro/retire of way-besch
				intercity_roads[num_intercity_roads].intro = 0;
				intercity_roads[num_intercity_roads].retire = 0;
				if(  *p==','  ) {
					++p;
					intercity_roads[num_intercity_roads].intro = atoi(p)*12;
					while(*p  &&  *p!=','  ) {
						p++;
					}
				}
				if(  *p==','  ) {
					intercity_roads[num_intercity_roads].retire = atoi(p+1)*12;
				}
				num_intercity_roads ++;
			}
		}
	}

	umgebung_t::autosave = (contents.get_int("autosave", umgebung_t::autosave) );
	umgebung_t::fps = contents.get_int("frames_per_second",umgebung_t::fps );

	// routing stuff
	max_route_steps = contents.get_int("max_route_steps", max_route_steps );
	max_hops = contents.get_int("max_hops", max_hops );
	max_transfers = contents.get_int("max_transfers", max_transfers );
	minimum_city_distance = contents.get_int("minimum_city_distance", minimum_city_distance );
	industry_increase = contents.get_int("industry_increase_every", industry_increase );
	passenger_factor = contents.get_int("passenger_factor", passenger_factor ); /* this can manipulate the passenger generation */
	factory_worker_percentage = contents.get_int("factory_worker_percentage", factory_worker_percentage );
	factory_worker_radius = contents.get_int("factory_worker_radius", factory_worker_radius );
	factory_worker_minimum_towns = contents.get_int("factory_worker_minimum_towns", factory_worker_minimum_towns );
	factory_worker_maximum_towns = contents.get_int("factory_worker_maximum_towns", factory_worker_maximum_towns );
	tourist_percentage = contents.get_int("tourist_percentage", tourist_percentage );
	seperate_halt_capacities = contents.get_int("seperate_halt_capacities", seperate_halt_capacities ) != 0;
	pay_for_total_distance = contents.get_int("pay_for_total_distance", pay_for_total_distance );
	avoid_overcrowding = contents.get_int("avoid_overcrowding", avoid_overcrowding )!=0;
	no_routing_over_overcrowding = contents.get_int("no_routing_over_overcrowded", no_routing_over_overcrowding )!=0;

	// city stuff
	passenger_multiplier = contents.get_int("passenger_multiplier", passenger_multiplier );
	mail_multiplier = contents.get_int("mail_multiplier", mail_multiplier );
	goods_multiplier = contents.get_int("goods_multiplier", goods_multiplier );
	electricity_multiplier = contents.get_int("electricity_multiplier", electricity_multiplier );

	growthfactor_small = contents.get_int("growthfactor_villages", growthfactor_small );
	growthfactor_medium = contents.get_int("growthfactor_cities", growthfactor_medium );
	growthfactor_large = contents.get_int("growthfactor_capitals", growthfactor_large );

	fussgaenger = contents.get_int("random_pedestrians", fussgaenger ) != 0;
	show_pax = contents.get_int("stop_pedestrians", show_pax ) != 0;
	verkehr_level = contents.get_int("citycar_level", verkehr_level );	// ten normal years
	stadtauto_duration = contents.get_int("default_citycar_life", stadtauto_duration );	// ten normal years
	allow_buying_obsolete_vehicles = contents.get_int("allow_buying_obsolete_vehicles", allow_buying_obsolete_vehicles );

	// starting money
	starting_money = contents.get_int64("starting_money", starting_money );
	/* up to ten blocks year, money, interpolation={0,1} are possible:
	 * starting_money[i]=y,m,int
	 * y .. year
	 * m .. money (in 1/100 Cr)
	 * int .. interpolation: 0 - no interpolation, !=0 linear interpolated
	 * (m) is the starting money for player start after (y), if (i)!=0, the starting money
	 * is linearly interpolated between (y) and the next greater year given in another entry.
	 * starting money for given year is:
	 */
	int j=0;
	for(  int i = 0;  i<10;  i++  ) {
		char name[32];
		sprintf( name, "starting_money[%i]", i );
		int *test = contents.get_ints(name );
		if ((test[0]>1) && (test[0]<=3)) {
			// insert sorted by years
			int k=0;
			for (k=0; k<i; k++) {
				if (startingmoneyperyear[k].year > test[1]) {
					for (int l=j; l>=k; l--)
						memcpy( &startingmoneyperyear[l+1], &startingmoneyperyear[l], sizeof(yearmoney) );
					break;
				}
			}
			startingmoneyperyear[k].year = test[1];
			startingmoneyperyear[k].money = test[2];
			if (test[0]==3) {
				startingmoneyperyear[k].interpol = test[3]!=0;
			}
			else {
				startingmoneyperyear[k].interpol = false;
			}
			j++;
		}
		else {
			// invalid entry
		}
		delete [] test;
	}
	// at least one found => use this now!
	if(  j>0  &&  startingmoneyperyear[0].money>0  ) {
		starting_money = 0;
	}
	// fill remaining entries
	for(  int i=j+1; i<10; i++  ) {
		startingmoneyperyear[i].year = 0;
		startingmoneyperyear[i].money = 0;
		startingmoneyperyear[i].interpol = 0;
	}

	maint_building = contents.get_int("maintenance_building", maint_building );

	numbered_stations = contents.get_int("numbered_stations", numbered_stations );
	station_coverage_size = contents.get_int("station_coverage", station_coverage_size );

	// time stuff
	bits_per_month = contents.get_int("bits_per_month", bits_per_month );
	use_timeline = contents.get_int("use_timeline", use_timeline );
	starting_year = contents.get_int("starting_year", starting_year );
	starting_month = contents.get_int("starting_month", starting_month+1)-1;

	river_number = contents.get_int("river_number", river_number );
	min_river_length = contents.get_int("river_min_length", min_river_length );
	max_river_length = contents.get_int("river_max_length", max_river_length );

	// forest stuff (now part of simuconf.tab)
	forest_base_size = contents.get_int("forest_base_size", forest_base_size );
	forest_map_size_divisor = contents.get_int("forest_map_size_divisor", forest_map_size_divisor );
	forest_count_divisor = contents.get_int("forest_count_divisor", forest_count_divisor );
	forest_inverse_spare_tree_density = contents.get_int("forest_inverse_spare_tree_density", forest_inverse_spare_tree_density );
	max_no_of_trees_on_square = contents.get_int("max_no_of_trees_on_square", max_no_of_trees_on_square );
	tree_climates = contents.get_int("tree_climates", tree_climates );
	no_tree_climates = contents.get_int("no_tree_climates", no_tree_climates );
	no_trees	= contents.get_int("no_trees", no_trees );

	pak_diagonal_multiplier = contents.get_int("diagonal_multiplier", pak_diagonal_multiplier );

	factory_spacing = contents.get_int("factory_spacing", factory_spacing );
	crossconnect_factories = contents.get_int("crossconnect_factories", crossconnect_factories ) != 0;
	crossconnect_factor = contents.get_int("crossconnect_factories_percentage", crossconnect_factor );
	electric_promille = contents.get_int("electric_promille", electric_promille );

	just_in_time = contents.get_int("just_in_time", just_in_time) != 0;
	beginner_price_factor = contents.get_int("beginner_price_factor", beginner_price_factor ); /* this manipulates the good prices in beginner mode */
	beginner_mode = contents.get_int("first_beginner", beginner_mode ); /* start in beginner mode */

	/* now the cost section */
	cst_multiply_dock = contents.get_int64("cost_multiply_dock", cst_multiply_dock/(-100) ) * -100;
	cst_multiply_station = contents.get_int64("cost_multiply_station", cst_multiply_station/(-100) ) * -100;
	cst_multiply_roadstop = contents.get_int64("cost_multiply_roadstop", cst_multiply_roadstop/(-100) ) * -100;
	cst_multiply_airterminal = contents.get_int64("cost_multiply_airterminal", cst_multiply_airterminal/(-100) ) * -100;
	cst_multiply_post = contents.get_int64("cost_multiply_post", cst_multiply_post/(-100) ) * -100;
	cst_multiply_headquarter = contents.get_int64("cost_multiply_headquarter", cst_multiply_headquarter/(-100) ) * -100;
	cst_depot_air = contents.get_int64("cost_depot_air", cst_depot_air/(-100) ) * -100;
	cst_depot_rail = contents.get_int64("cost_depot_rail", cst_depot_rail/(-100) ) * -100;
	cst_depot_road = contents.get_int64("cost_depot_road", cst_depot_road/(-100) ) * -100;
	cst_depot_ship = contents.get_int64("cost_depot_ship", cst_depot_ship/(-100) ) * -100;

	// alter landscape
	cst_buy_land = contents.get_int64("cost_buy_land", cst_buy_land/(-100) ) * -100;
	cst_alter_land = contents.get_int64("cost_alter_land", cst_alter_land/(-100) ) * -100;
	cst_set_slope = contents.get_int64("cost_set_slope", cst_set_slope/(-100) ) * -100;
	cst_found_city = contents.get_int64("cost_found_city", cst_found_city/(-100) ) * -100;
	cst_multiply_found_industry = contents.get_int64("cost_multiply_found_industry", cst_multiply_found_industry/(-100) ) * -100;
	cst_remove_tree = contents.get_int64("cost_remove_tree", cst_remove_tree/(-100) ) * -100;
	cst_multiply_remove_haus = contents.get_int64("cost_multiply_remove_haus", cst_multiply_remove_haus/(-100) ) * -100;
	cst_multiply_remove_field = contents.get_int64("cost_multiply_remove_field", cst_multiply_remove_field/(-100) ) * -100;
	// powerlines
	cst_transformer = contents.get_int64("cost_transformer", cst_transformer/(-100) ) * -100;
	cst_maintain_transformer = contents.get_int64("cost_maintain_transformer", cst_maintain_transformer/(-100) ) * -100;

	/* now the way builder */
	way_count_straight = contents.get_int("way_straight", way_count_straight );
	way_count_curve = contents.get_int("way_curve", way_count_curve );
	way_count_double_curve = contents.get_int("way_double_curve", way_count_double_curve );
	way_count_90_curve = contents.get_int("way_90_curve", way_count_90_curve );
	way_count_slope = contents.get_int("way_slope", way_count_slope );
	way_count_tunnel = contents.get_int("way_tunnel", way_count_tunnel );
	way_max_bridge_len = contents.get_int("way_max_bridge_len", way_max_bridge_len );
	way_count_leaving_road = contents.get_int("way_leaving_road", way_count_leaving_road );

	/*
	 * Selection of savegame format through inifile
	 */
	str = contents.get("saveformat" );
	while (*str == ' ') str++;
	if (strcmp(str, "binary") == 0) {
		loadsave_t::set_savemode(loadsave_t::binary );
	} else if(strcmp(str, "zipped") == 0) {
		loadsave_t::set_savemode(loadsave_t::zipped );
	} else if(strcmp(str, "xml") == 0) {
		loadsave_t::set_savemode(loadsave_t::xml );
	} else if(strcmp(str, "xml_zipped") == 0) {
		loadsave_t::set_savemode(loadsave_t::xml_zipped );
	} else if(strcmp(str, "bzip2") == 0) {
		loadsave_t::set_savemode(loadsave_t::bzip2 );
	} else if(strcmp(str, "xml_bzip2") == 0) {
		loadsave_t::set_savemode(loadsave_t::xml_bzip2 );
	}

	/*
	 * Default resolution
	 */
	disp_width = contents.get_int("display_width", disp_width );
	disp_height = contents.get_int("display_height", disp_height );
	fullscreen = contents.get_int("fullscreen", fullscreen );

	with_private_paks = contents.get_int("with_private_paks", with_private_paks)!=0;

	// Default pak file path
	objfilename = ltrim(contents.get_string("pak_file_path", "" ) );

	printf("Reading simuconf.tab successful!\n" );

	simuconf.close( );
}


sint64 einstellungen_t::get_starting_money(sint16 year) const
{
	if(  starting_money>0  ) {
		return starting_money;
	}

	// search entry with startingmoneyperyear[i].year > year
	int i;
	bool found = false;
	for(  i=0;  i<10;  i++  ) {
		if(startingmoneyperyear[i].year!=0) {
			if (startingmoneyperyear[i].year>year) {
				found = true;
				break;
			}
		}
		else {
			// year is behind the latest given date
			return startingmoneyperyear[i>0 ? i-1 : 0].money;
		}
	}
	if (i==0) {
		// too early: use first entry
		return startingmoneyperyear[0].money;
	}
	else {
		// now: startingmoneyperyear[i-1].year <= year < startingmoneyperyear[i].year
		if (startingmoneyperyear[i-1].interpol) {
			// linear interpolation
			return startingmoneyperyear[i-1].money +
				(startingmoneyperyear[i].money-startingmoneyperyear[i-1].money)
				* (year-startingmoneyperyear[i-1].year) /(startingmoneyperyear[i].year-startingmoneyperyear[i-1].year );
		}
		else {
			// no interpolation
			return startingmoneyperyear[i-1].money;
		}

	}
}


/**
 * returns newest way-besch for road_timeline_t arrays
 * @param road_timeline_t must be an array with at least num_roads elements, no range checks!
 */
static const weg_besch_t *get_timeline_road_type( uint16 year, uint16 num_roads, road_timeline_t* roads)
{
	const weg_besch_t *besch = NULL;
	const weg_besch_t *test;
	for(  int i=0;  i<num_roads;  i++  ) {
		test = wegbauer_t::get_besch( roads[i].name, 0 );
		if(  test  ) {
			// return first available for no timeline
			if(  year==0  ) {
				return test;
			}
			if(  roads[i].intro==0  ) {
				// fill in real intro date
				roads[i].intro = test->get_intro_year_month( );
			}
			if(  roads[i].retire==0  ) {
				// fill in real retire date
				roads[i].retire = test->get_retire_year_month( );
				if(  roads[i].retire==0  ) {
					roads[i].retire = NEVER;
				}
			}
			// find newest available ...
			if(  year>=roads[i].intro  &&  year<roads[i].retire  ) {
				if(  besch==0  ||  besch->get_intro_year_month()<test->get_intro_year_month()  ) {
					besch = test;
				}
			}
		}
	}
	return besch;
}


const weg_besch_t *einstellungen_t::get_city_road_type( uint16 year )
{
	return get_timeline_road_type(year, num_city_roads, city_roads );
}


const weg_besch_t *einstellungen_t::get_intercity_road_type( uint16 year )
{
	return get_timeline_road_type(year, num_intercity_roads, intercity_roads );
}
