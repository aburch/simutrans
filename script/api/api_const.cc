#include "api.h"

/** @file api_const.cc exports constants */

#include "../api_param.h"
#include "../../simobj.h"
#include "../../simmenu.h"

using namespace script_api;

#define begin_enum(name) sq_pushconsttable(vm);
#define end_enum() sq_setconsttable(vm);
#define enum_slot create_slot<SQInteger>

void export_global_constants(HSQUIRRELVM vm)
{
	/**
	 * Constants to identify tools, ie actions a player can perform to
	 * alter the state of the game.
	 */
	begin_enum("tool_ids");
	/// remover tool
	enum_slot(vm, "tool_remover",   WKZ_REMOVER | GENERAL_TOOL);
	/// raise land tool
	enum_slot(vm, "tool_raise_land", WKZ_RAISE_LAND | GENERAL_TOOL);
	/// lower land tool
	enum_slot(vm, "tool_lower_land", WKZ_LOWER_LAND | GENERAL_TOOL);
	/// artificial slope
	enum_slot(vm, "tool_setslope", WKZ_SETSLOPE | GENERAL_TOOL);
	/// restore natural slope
	enum_slot(vm, "tool_restoreslope", WKZ_RESTORESLOPE | GENERAL_TOOL);
	/// set marker
	enum_slot(vm, "tool_set_marker", WKZ_MARKER | GENERAL_TOOL);
	/// clear block reservation
	enum_slot(vm, "tool_clear_reservation", WKZ_CLEAR_RESERVATION | GENERAL_TOOL);
	/// build transformer
	enum_slot(vm, "tool_build_transformer", WKZ_TRANSFORMER | GENERAL_TOOL);
	/// add city
	enum_slot(vm, "tool_add_city", WKZ_ADD_CITY | GENERAL_TOOL);
	/// change city size
	enum_slot(vm, "tool_change_city_size", WKZ_CHANGE_CITY_SIZE | GENERAL_TOOL);
	/// plant a tree
	enum_slot(vm, "tool_plant_tree", WKZ_PLANT_TREE | GENERAL_TOOL);
	// not needed? enum__slot(vm, "tool_fahrplan_add", WKZ_FAHRPLAN_ADD | GENERAL_TOOL);
	// not needed? enum__slot(vm, "tool_fahrplan_ins", WKZ_FAHRPLAN_INS | GENERAL_TOOL);
	/// build ways
	enum_slot(vm, "tool_build_way", WKZ_WEGEBAU | GENERAL_TOOL);
	/// build bridges
	enum_slot(vm, "tool_build_bridge", WKZ_BRUECKENBAU | GENERAL_TOOL);
	/// build tunnel
	enum_slot(vm, "tool_build_tunnel", WKZ_TUNNELBAU | GENERAL_TOOL);
	/// remove way
	enum_slot(vm, "tool_remove_way", WKZ_WAYREMOVER | GENERAL_TOOL);
	/// build overhead wires
	enum_slot(vm, "tool_build_wayobj", WKZ_WAYOBJ | GENERAL_TOOL);
	/// build stations
	enum_slot(vm, "tool_build_station", WKZ_STATION | GENERAL_TOOL);
	/// build signals and road signs
	enum_slot(vm, "tool_build_roadsign", WKZ_ROADSIGN | GENERAL_TOOL);
	/// build depot
	enum_slot(vm, "tool_build_depot", WKZ_DEPOT | GENERAL_TOOL);
	/// build city houses
	enum_slot(vm, "tool_build_house", WKZ_BUILD_HAUS | GENERAL_TOOL);
	/// create industry chain with end consumer not in cities
	enum_slot(vm, "tool_land_chain", WKZ_LAND_CHAIN | GENERAL_TOOL);
	/// create industry chain with end consumer in cities
	enum_slot(vm, "tool_city_chain", WKZ_CITY_CHAIN | GENERAL_TOOL);
	/// build a factory
	enum_slot(vm, "tool_build_factory", WKZ_BUILD_FACTORY | GENERAL_TOOL);
	/// link factories
	enum_slot(vm, "tool_link_factory", WKZ_LINK_FACTORY | GENERAL_TOOL);
	/// build headquarter
	enum_slot(vm, "tool_headquarter", WKZ_HEADQUARTER | GENERAL_TOOL);
	/// lock map: switching players not allowed anymore
	enum_slot(vm, "tool_lock_game", WKZ_LOCK_GAME | GENERAL_TOOL);
	/// add city car
	enum_slot(vm, "tool_add_citycar", WKZ_ADD_CITYCAR | GENERAL_TOOL);
	/// create forest
	enum_slot(vm, "tool_forest", WKZ_FOREST | GENERAL_TOOL);
	/// move stop tool
	enum_slot(vm, "tool_stop_mover", WKZ_STOP_MOVER | GENERAL_TOOL);
	/// make stop public
	enum_slot(vm, "tool_make_stop_public", WKZ_MAKE_STOP_PUBLIC | GENERAL_TOOL);
	/// remove way objects like overheadwires
	enum_slot(vm, "tool_remove_wayobj", WKZ_REMOVE_WAYOBJ | GENERAL_TOOL);
	// not needed? enum__slot(vm, "tool_sliced_and_underground_view", WKZ_SLICED_AND_UNDERGROUND_VIEW | GENERAL_TOOL);
	/// buy a house
	enum_slot(vm, "tool_buy_house", WKZ_BUY_HOUSE | GENERAL_TOOL);
	/// build city road with pavement
	enum_slot(vm, "tool_build_cityroad", WKZ_CITYROAD | GENERAL_TOOL);

	// simple tools
	/// increase industry density
	enum_slot(vm, "tool_increase_industry", WKZ_INCREASE_INDUSTRY | SIMPLE_TOOL);
	/// switch player
	enum_slot(vm, "tool_switch_player", WKZ_SWITCH_PLAYER | SIMPLE_TOOL);
	/// step year forward
	enum_slot(vm, "tool_step_year", WKZ_STEP_YEAR | SIMPLE_TOOL);
	/// fill area with trees
	enum_slot(vm, "tool_fill_trees", WKZ_FILL_TREES | SIMPLE_TOOL);
	/// set traffic level
	enum_slot(vm, "tool_set_traffic_level", WKZ_TRAFFIC_LEVEL | SIMPLE_TOOL);

	// tools to open certain windows
	/// open factory editor window
	enum_slot(vm, "dialog_edit_factory", WKZ_EDIT_FACTORY | DIALOGE_TOOL);
	/// open tourist attraction editor window
	enum_slot(vm, "dialog_edit_attraction", WKZ_EDIT_ATTRACTION | DIALOGE_TOOL);
	/// open house editor window
	enum_slot(vm, "dialog_edit_house", WKZ_EDIT_HOUSE | DIALOGE_TOOL);
	/// open tree editor window
	enum_slot(vm, "dialog_edit_tree", WKZ_EDIT_TREE | DIALOGE_TOOL);
	/// open map enlargement window
	enum_slot(vm, "dialog_enlarge_map", WKZ_ENLARGE_MAP | DIALOGE_TOOL);

	end_enum();

	/**
	 * Constants for different way types.
	 */
	begin_enum("way_types");
	/// catch all value: used to forbid tools for all waytypes
	enum_slot(vm, "wt_all", invalid_wt);
	/// road
	enum_slot(vm, "wt_road", road_wt);
	/// rail
	enum_slot(vm, "wt_rail", track_wt);
	/// water
	enum_slot(vm, "wt_water", water_wt);
	/// monorail
	enum_slot(vm, "wt_monorail", monorail_wt);
	/// maglev
	enum_slot(vm, "wt_maglev", maglev_wt);
	/// trams
	enum_slot(vm, "wt_tram", tram_wt);
	/// narrow gauge
	enum_slot(vm, "wt_narrowgauge", narrowgauge_wt);
	/// aircrafts and airports
	enum_slot(vm, "wt_air", air_wt);
	/// powerlines
	enum_slot(vm, "wt_power", powerline_wt);
	/// invalid
	enum_slot(vm, "wt_invalid", invalid_wt);
	end_enum();

	/**
	 * Constants for different system types of ways.
	 * System type could take also other values than the ones defined here.
	 */
	begin_enum("way_system_types");
	/// flat ways
	enum_slot(vm, "st_flat", weg_t::type_flat);
	/// elevated ways
	enum_slot(vm, "st_elevated", weg_t::type_elevated);
	/// tram tracks (here way type has to be wt_tram)
	enum_slot(vm, "st_tram", weg_t::type_tram);
	end_enum();

	// players
	begin_enum("");
	/// constant to forbid/allow tools for all players (except public player)
	enum_slot(vm, "player_all", PLAYER_UNOWNED);
	end_enum();

	/**
	 * Types of map objects.
	 */
	begin_enum("map_objects");
	/// tree
	enum_slot(vm, "mo_tree", obj_t::baum);
	/// pointer (bulldozers etc)
	enum_slot(vm, "mo_pointer", obj_t::zeiger);
	/// cloude and smoke
	enum_slot(vm, "mo_cloud", obj_t::wolke);
	/// building (houses, halts, factories ...)
	enum_slot(vm, "mo_building", obj_t::gebaeude);
	/// signal
	enum_slot(vm, "mo_signal", obj_t::signal);
	/// bridge
	enum_slot(vm, "mo_bridge", obj_t::bruecke);
	/// tunnel
	enum_slot(vm, "mo_tunnel", obj_t::tunnel);
	/// depot: rail
	enum_slot(vm, "mo_depot_rail", obj_t::bahndepot);
	/// depot: road
	enum_slot(vm, "mo_depot_road", obj_t::strassendepot);
	/// depot: ship
	enum_slot(vm, "mo_depot_water", obj_t::schiffdepot);
	/// powerline
	enum_slot(vm, "mo_powerline", obj_t::leitung);
	/// transformer at powerplant
	enum_slot(vm, "mo_transformer_s", obj_t::pumpe);
	/// transformer at factory
	enum_slot(vm, "mo_transformer_c", obj_t::senke);
	/// road-sign
	enum_slot(vm, "mo_roadsign", obj_t::roadsign);
	/// bridge pillar
	enum_slot(vm, "mo_pillar", obj_t::pillar);
	/// depot: airplanes
	enum_slot(vm, "mo_depot_air", obj_t::airdepot);
	/// depot: monorail
	enum_slot(vm, "mo_depot_monorail", obj_t::monoraildepot);
	/// depot: tram
	enum_slot(vm, "mo_depot_tram", obj_t::tramdepot);
	/// depot: maglev
	enum_slot(vm, "mo_depot_maglev", obj_t::maglevdepot);
	/// way object (overhead wires)
	enum_slot(vm, "mo_wayobj", obj_t::wayobj);
	/// way
	enum_slot(vm, "mo_way", obj_t::way	);
	/// text label
	enum_slot(vm, "mo_label", obj_t::label);
	/// field
	enum_slot(vm, "mo_field", obj_t::field);
	/// crossing
	enum_slot(vm, "mo_crossing", obj_t::crossing);
	/// decorative objects (rocks, lakes ...)
	enum_slot(vm, "mo_groundobj", obj_t::groundobj);
	/// depot: narrowgauge
	enum_slot(vm, "mo_depot_narrowgauge", obj_t::narrowgaugedepot);
	/// pedestrian
	enum_slot(vm, "mo_pedestrian", obj_t::fussgaenger);
	/// city car - not player owned
	enum_slot(vm, "mo_city_car", obj_t::verkehr);
	/// road vehicle
	enum_slot(vm, "mo_car", obj_t::automobil);
	/// rail vehicle
	enum_slot(vm, "mo_train", obj_t::waggon);
	/// monorail vehicle
	enum_slot(vm, "mo_monorail", obj_t::monorailwaggon);
	/// maglev vehicle
	enum_slot(vm, "mo_maglev", obj_t::maglevwaggon);
	/// narrowgauge vehicle
	enum_slot(vm, "mo_narrowgauge", obj_t::narrowgaugewaggon);
	/// ship
	enum_slot(vm, "mo_ship", obj_t::schiff);
	/// airplane
	enum_slot(vm, "mo_airplane", obj_t::aircraft);
	/// moving object (sheep ...)
	enum_slot(vm, "mo_moving_object", obj_t::movingobj);

	end_enum();

}
