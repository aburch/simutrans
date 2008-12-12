/*
 * Spieleinstellungen
 *
 * Hj. Malthaner
 *
 * April 2000
 */

#include "einstellungen.h"
#include "umgebung.h"
#include "../simconst.h"
#include "../simtypes.h"
#include "../simdebug.h"
#include "../utils/simstring.h"
#include "../vehicle/simvehikel.h"
#include "../player/simplay.h"
#include "loadsave.h"

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

	scroll_multi = 1;

	verkehr_level = 7;

	show_pax = true;

	// default climate zones
	static sint16 borders[MAX_CLIMATES] = { 0, 0, 0, 3, 6, 8, 10, 10 };
	memcpy( climate_borders, borders, sizeof(sint16)*MAX_CLIMATES );
	winter_snowline = 7;	// not mediterran
	grundwasser = -2*Z_TILE_STEP;            //25-Nov-01        Markus Weber    Added

	max_mountain_height = 160;                  //can be 0-160.0  01-Dec-01        Markus Weber    Added
	map_roughness = 0.6;                        //can be 0-1      01-Dec-01        Markus Weber    Added

	// some settigns more
	allow_player_change = true;

	beginner_mode = false;
	beginner_price_factor = 1500;

	rotation = 0;

	origin_x = origin_y = 0;

	// passenger manipulation factor (=16 about old value)
	passenger_factor = 16;

	default_electric_promille = 330;

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
	just_in_time=true;

	fussgaenger = true;
	stadtauto_duration = 120;	// ten years

	// to keep names consistent
	numbered_stations = false;

	city_road_type = strdup( "city_road" );

	max_route_steps = 1000000;
	max_transfers = 7;
	max_hops = 300;

	/* multiplier for steps on diagonal:
	 * 1024: TT-like, faktor 2, vehicle will be too long and too fast
	 * 724: correct one, faktor sqrt(2)
	 */
	pak_diagonal_multiplier = 724;

	setze_name_language_iso( "en" );


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
		password[i] = NULL;
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
	cst_signal=-50000;
	cst_tunnel=-1000000;
	cst_third_rail=-8000;
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
	way_count_90_curve=50;
	way_count_slope=10;
	way_count_tunnel=8;
	way_max_bridge_len=15;
	way_count_leaving_road=25;
}



void einstellungen_t::rdwr(loadsave_t *file)
{
	if(file->get_version() < 86000) {
		uint32 dummy;

		file->rdwr_long(groesse_x, " ");
		groesse_y = groesse_x;

		file->rdwr_long(nummer, "\n");

		// to be compatible with previous savegames
		dummy = 0;
		file->rdwr_long(dummy, " ");	//dummy!
		land_industry_chains = 6;
		electric_promille = 330;
		tourist_attractions = 12;

		// now towns
		mittlere_einwohnerzahl = 1600;
		dummy =  anzahl_staedte;
		file->rdwr_long(dummy, "\n");
		dummy &= 127;
		if(dummy>63) {
			dbg->warning("einstellungen_t::rdwr()","This game was saved with too many cities! (%i of maximum 63). Simutrans may crash!",dummy);
		}
		anzahl_staedte = dummy;

		// rest
		file->rdwr_long(dummy, " ");	// scroll ingnored
		file->rdwr_long(verkehr_level, "\n");
		file->rdwr_long(show_pax, "\n");
		dummy = grundwasser;
		file->rdwr_long(dummy, "\n");
		grundwasser = (sint16)(dummy/16)*Z_TILE_STEP;
		file->rdwr_double(max_mountain_height, "\n");
		file->rdwr_double(map_roughness, "\n");

		station_coverage_size = 3;
		beginner_mode = false;
		rotation = 0;
	}
	else {
		// newer versions
		file->rdwr_long(groesse_x, " ");
		file->rdwr_long(nummer, "\n");

		// industries
		file->rdwr_long(land_industry_chains, " ");
		if(file->get_version()<99018) {
			uint32 dummy;	// was city chains
			file->rdwr_long(dummy, " ");
		}
		else {
			file->rdwr_long(electric_promille, " ");
		}
		file->rdwr_long(tourist_attractions, "\n");

		// now towns
		file->rdwr_long(mittlere_einwohnerzahl, " ");
		file->rdwr_long(anzahl_staedte, " ");

		// rest
		if(file->get_version() < 101000) {
			uint32 dummy;	// was scroll dir
			file->rdwr_long(dummy, " ");
		}
		file->rdwr_long(verkehr_level, "\n");
		file->rdwr_long(show_pax, "\n");
		sint32 dummy = grundwasser/Z_TILE_STEP;
		file->rdwr_long(dummy, "\n");
		if(file->get_version() < 99005) {
			grundwasser = (sint16)(dummy/16)*Z_TILE_STEP;
		}
		else {
			grundwasser = (sint16)dummy*Z_TILE_STEP;
		}
		file->rdwr_double(max_mountain_height, "\n");
		file->rdwr_double(map_roughness, "\n");

		if(file->get_version() >= 86003) {
			dummy = station_coverage_size;
			file->rdwr_long(dummy, " ");
			station_coverage_size = (uint16)dummy;
		}

		if(file->get_version() >= 86006) {
			// handle also size on y direction
			file->rdwr_long(groesse_y, " ");
		}
		else {
			groesse_y = groesse_x;
		}

		if(file->get_version() >= 86011) {
			// some more settings
			file->rdwr_byte(allow_player_change, " ");
			file->rdwr_byte(use_timeline, " " );
			file->rdwr_short(starting_year, "\n");
		}
		else {
			allow_player_change = 1;
			use_timeline = 1;
			starting_year = 1930;
		}

		if(file->get_version()>=88005) {
			file->rdwr_short(bits_per_month,"b");
		}
		else {
			bits_per_month = 18;
		}

		if(file->get_version()>=89003) {
			file->rdwr_bool(beginner_mode,"\n");
		}
		else {
			beginner_mode = false;
		}
		if(file->get_version()>=89004) {
			file->rdwr_bool(just_in_time,"\n");
		}
		// rotation of the map with respect to the original value
		if(file->get_version()>=99015) {
			file->rdwr_byte(rotation,"\n");
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
				file->rdwr_short(climate_borders[i], "c");
			}
			file->rdwr_short(winter_snowline, "c");
		}

		// since vehicle will need realignment afterwards!
		if(file->get_version()<=99018) {
			vehikel_basis_t::set_diagonal_multiplier( pak_diagonal_multiplier, 1024 );
		}
		else {
			uint16 old_multiplier = pak_diagonal_multiplier;
			file->rdwr_short( old_multiplier, "m" );
			vehikel_basis_t::set_diagonal_multiplier( pak_diagonal_multiplier, old_multiplier );
		}

		// since vehicle will need realignment afterwards!
		if(file->get_version()>=101000) {
			// game mechanics
			file->rdwr_short( origin_x, "ox" );
			file->rdwr_short( origin_y, "oy" );

			file->rdwr_long( passenger_factor, "" );
			file->rdwr_long( default_electric_promille, "" );

			file->rdwr_short( factory_spacing, "" );
			file->rdwr_bool( crossconnect_factories, "" );
			file->rdwr_short( crossconnect_factor, "" );

			file->rdwr_bool( fussgaenger, "" );
			file->rdwr_long( stadtauto_duration , "" );

			file->rdwr_bool( numbered_stations, "" );
			file->rdwr_str( city_road_type, "" );
			file->rdwr_long( max_route_steps , "" );
			file->rdwr_long( max_transfers , "" );
			file->rdwr_long( max_hops , "" );

			file->rdwr_long( beginner_price_factor , "" );

			// name of stops
			file->rdwr_str( language_code_names, "en" );

			// restore AI state
			for(  int i=0;  i<15;  i++  ) {
				file->rdwr_bool( automaten[i], "" );
				file->rdwr_byte( spieler_type[i], "" );
				file->rdwr_str( password[i], "" );
			}

			// cost section ...
			file->rdwr_bool( freeplay, "" );
			file->rdwr_longlong( starting_money, "" );
			file->rdwr_long( maint_building, "" );

			file->rdwr_longlong( cst_multiply_dock, "" );
			file->rdwr_longlong( cst_multiply_station, "" );
			file->rdwr_longlong( cst_multiply_roadstop, "" );
			file->rdwr_longlong( cst_multiply_airterminal, "" );
			file->rdwr_longlong( cst_multiply_post, "" );
			file->rdwr_longlong( cst_multiply_headquarter, "" );
			file->rdwr_longlong( cst_depot_rail, "" );
			file->rdwr_longlong( cst_depot_road, "" );
			file->rdwr_longlong( cst_depot_ship, "" );
			file->rdwr_longlong( cst_depot_air, "" );
			file->rdwr_longlong( cst_signal, "" );
			file->rdwr_longlong( cst_tunnel, "" );
			file->rdwr_longlong( cst_third_rail, "" );
			// alter landscape
			file->rdwr_longlong( cst_buy_land, "" );
			file->rdwr_longlong( cst_alter_land, "" );
			file->rdwr_longlong( cst_set_slope, "" );
			file->rdwr_longlong( cst_found_city, "" );
			file->rdwr_longlong( cst_multiply_found_industry, "" );
			file->rdwr_longlong( cst_remove_tree, "" );
			file->rdwr_longlong( cst_multiply_remove_haus, "" );
			file->rdwr_longlong( cst_multiply_remove_field, "" );
			// cost for transformers
			file->rdwr_longlong( cst_transformer, "" );
			file->rdwr_longlong( cst_maintain_transformer, "" );
			// wayfinder
			file->rdwr_long( way_count_straight, "" );
			file->rdwr_long( way_count_curve, "" );
			file->rdwr_long( way_count_double_curve, "" );
			file->rdwr_long( way_count_90_curve, "" );
			file->rdwr_long( way_count_slope, "" );
			file->rdwr_long( way_count_tunnel, "" );
			file->rdwr_long( way_max_bridge_len, "" );
			file->rdwr_long( way_count_leaving_road, "" );
		}
		else {
			// name of stops
			setze_name_language_iso( umgebung_t::language_iso );

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
				password[i] = NULL;
			}
		}
	}
}




// read the settings from this file
void einstellungen_t::parse_simuconf( tabfile_t &simuconf, sint16 &disp_width, sint16 &disp_height, sint16 &fullscreen, cstring_t &objfilename )
{
	tabfileobj_t contents;

	simuconf.read(contents);

	umgebung_t::max_convoihandles = contents.get_int("convoys", umgebung_t::max_convoihandles );
	umgebung_t::max_linehandles = contents.get_int("lines", umgebung_t::max_linehandles );
	umgebung_t::max_halthandles = contents.get_int("stations", umgebung_t::max_halthandles );

	// routing stuff
	max_route_steps = contents.get_int("max_route_steps", max_route_steps );
	max_hops = contents.get_int("max_hops", max_hops );
	max_transfers = contents.get_int("max_transfers", max_transfers );
	passenger_factor = contents.get_int("passenger_factor", passenger_factor ); /* this can manipulate the passenger generation */

	fussgaenger = contents.get_int("random_pedestrians", fussgaenger ) != 0;
	show_pax = contents.get_int("stop_pedestrians", show_pax ) != 0;
	stadtauto_duration = contents.get_int("default_citycar_life", stadtauto_duration);	// ten normal years

	umgebung_t::water_animation = contents.get_int("water_animation_ms", umgebung_t::water_animation);
	umgebung_t::ground_object_probability = contents.get_int("random_grounds_probability", umgebung_t::ground_object_probability);
	umgebung_t::moving_object_probability = contents.get_int("random_wildlife_probability", umgebung_t::moving_object_probability);
	umgebung_t::drive_on_left = contents.get_int("drive_left", umgebung_t::drive_on_left );

	umgebung_t::verkehrsteilnehmer_info = contents.get_int("pedes_and_car_info", umgebung_t::verkehrsteilnehmer_info) != 0;
	umgebung_t::tree_info = contents.get_int("tree_info", umgebung_t::tree_info) != 0;
	umgebung_t::ground_info = contents.get_int("ground_info", umgebung_t::ground_info) != 0;
	umgebung_t::townhall_info = contents.get_int("townhall_info", umgebung_t::townhall_info) != 0;
	umgebung_t::single_info = contents.get_int("only_single_info", umgebung_t::single_info);

	umgebung_t::window_buttons_right = contents.get_int("window_buttons_right", umgebung_t::window_buttons_right);
	umgebung_t::window_frame_active = contents.get_int("window_frame_active", umgebung_t::window_frame_active);

	umgebung_t::show_tooltips = contents.get_int("show_tooltips", umgebung_t::show_tooltips);
	umgebung_t::tooltip_color = contents.get_int("tooltip_background_color", umgebung_t::tooltip_color);
	umgebung_t::tooltip_textcolor = contents.get_int("tooltip_text_color", umgebung_t::tooltip_textcolor);

	starting_money = contents.get_int("starting_money", starting_money );
	maint_building = contents.get_int("maintenance_building", maint_building);

	numbered_stations = contents.get_int("numbered_stations", numbered_stations ) != 0;
	station_coverage_size = contents.get_int("station_coverage", station_coverage_size );

	// display stuff
	umgebung_t::show_names = contents.get_int("show_names", umgebung_t::show_names);
	umgebung_t::show_month = contents.get_int("show_month", umgebung_t::show_month);
	umgebung_t::max_acceleration = contents.get_int("fast_forward", umgebung_t::max_acceleration);

	// time stuff
	bits_per_month = contents.get_int("bits_per_month", bits_per_month);
	use_timeline = contents.get_int("use_timeline", use_timeline);
	starting_year = contents.get_int("starting_year", starting_year);

	umgebung_t::intercity_road_length = contents.get_int("intercity_road_length", umgebung_t::intercity_road_length);
	cstring_t *test = new cstring_t(ltrim(contents.get("intercity_road_type")));
	if(test->len()>0) {
		delete umgebung_t::intercity_road_type;
		umgebung_t::intercity_road_type = test;
	}
	else {
		delete test;
	}
	const char *str = ltrim(contents.get("city_road_type"));
	if(str[0]>0) {
		delete city_road_type;
		city_road_type = strdup( str );
	}

	pak_diagonal_multiplier = contents.get_int("diagonal_multiplier", pak_diagonal_multiplier);

	umgebung_t::autosave = (contents.get_int("autosave", umgebung_t::autosave));

	factory_spacing = contents.get_int("factory_spacing", factory_spacing );
	crossconnect_factories = contents.get_int("crossconnect_factories", crossconnect_factories ) != 0;
	crossconnect_factor = contents.get_int("crossconnect_factories_percentage", crossconnect_factor );
	default_electric_promille = contents.get_int("electric_promille", default_electric_promille);

	just_in_time = contents.get_int("just_in_time", just_in_time) != 0;
	beginner_price_factor = contents.get_int("beginner_price_factor", beginner_price_factor ); /* this manipulates the good prices in beginner mode */
	beginner_mode = contents.get_int("first_beginner", beginner_mode ); /* start in beginner mode */

	/* now the cost section */
	cst_multiply_dock = contents.get_int("cost_multiply_dock", cst_multiply_dock/(-100) ) * -100;
	cst_multiply_station = contents.get_int("cost_multiply_station", cst_multiply_station/(-100) ) * -100;
	cst_multiply_roadstop = contents.get_int("cost_multiply_roadstop", cst_multiply_roadstop/(-100) ) * -100;
	cst_multiply_airterminal = contents.get_int("cost_multiply_airterminal", cst_multiply_airterminal/(-100) ) * -100;
	cst_multiply_post = contents.get_int("cost_multiply_post", cst_multiply_post/(-100) ) * -100;
	cst_multiply_headquarter = contents.get_int("cost_multiply_headquarter", cst_multiply_headquarter/(-100) ) * -100;
	cst_depot_air = contents.get_int("cost_depot_air", cst_depot_air/(-100) ) * -100;
	cst_depot_rail = contents.get_int("cost_depot_rail", cst_depot_rail/(-100) ) * -100;
	cst_depot_road = contents.get_int("cost_depot_road", cst_depot_road/(-100) ) * -100;
	cst_depot_ship = contents.get_int("cost_depot_ship", cst_depot_ship/(-100) ) * -100;
	cst_signal = contents.get_int("cost_signal", cst_signal/(-100) ) * -100;
	cst_tunnel = contents.get_int("cost_tunnel", cst_tunnel/(-100) ) * -100;
	cst_third_rail = contents.get_int("cost_third_rail", cst_third_rail/(-100) ) * -100;

	// alter landscape
	cst_buy_land = contents.get_int("cost_buy_land", cst_buy_land/(-100) ) * -100;
	cst_alter_land = contents.get_int("cost_alter_land", cst_alter_land/(-100) ) * -100;
	cst_set_slope = contents.get_int("cost_set_slope", cst_set_slope/(-100) ) * -100;
	cst_found_city = contents.get_int("cost_found_city", cst_found_city/(-100) ) * -100;
	cst_multiply_found_industry = contents.get_int("cost_multiply_found_industry", cst_multiply_found_industry/(-100) ) * -100;
	cst_remove_tree = contents.get_int("cost_remove_tree", cst_remove_tree/(-100) ) * -100;
	cst_multiply_remove_haus = contents.get_int("cost_multiply_remove_haus", cst_multiply_remove_haus/(-100) ) * -100;
	cst_multiply_remove_field = contents.get_int("cost_multiply_remove_field", cst_multiply_remove_field/(-100) ) * -100;
	// powerlines
	cst_transformer = contents.get_int("cost_transformer", cst_transformer/(-100) ) * -100;
	cst_maintain_transformer = contents.get_int("cost_maintain_transformer", cst_maintain_transformer/(-100) ) * -100;

	/* now the way builder */
	way_count_straight = contents.get_int("way_straight", way_count_straight);
	way_count_curve = contents.get_int("way_curve", way_count_curve);
	way_count_double_curve = contents.get_int("way_double_curve", way_count_double_curve);
	way_count_90_curve = contents.get_int("way_90_curve", way_count_90_curve);
	way_count_slope = contents.get_int("way_slope", way_count_slope);
	way_count_tunnel = contents.get_int("way_tunnel", way_count_tunnel);
	way_max_bridge_len = contents.get_int("way_max_bridge_len", way_max_bridge_len);
	way_count_leaving_road = contents.get_int("way_leaving_road", way_count_leaving_road);

	/*
	* Selection of savegame format through inifile
	*/
	str = contents.get("saveformat");
	while (*str == ' ') str++;
	if (strcmp(str, "binary") == 0) {
		loadsave_t::set_savemode(loadsave_t::binary);
	} else if(strcmp(str, "zipped") == 0) {
		loadsave_t::set_savemode(loadsave_t::zipped);
	} else if(strcmp(str, "text") == 0) {
		loadsave_t::set_savemode(loadsave_t::text);
	}

	/*
	 * Default resolution
	 */
	disp_width = contents.get_int("display_width", disp_width);
	disp_height = contents.get_int("display_height", disp_height);
	fullscreen = contents.get_int("fullscreen", fullscreen);
	umgebung_t::fps = contents.get_int("frames_per_second",umgebung_t::fps);

	// Default pak file path
	objfilename = ltrim(contents.get_string("pak_file_path", "" ));

	print("Reading simuconf.tab successful!\n");

	simuconf.close();
}
