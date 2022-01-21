/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/** @file changelog.h Documenting the timeline of api evolution */

/** @page changelog Changelog
 *
 * @section api-trunk Current trunk
 *
 * - Added @ref change_climate_at
 * - Added @ref convoy_x::change_schedule
 *
 * @section api-123 Release 123.0
 *
 * - Changed scripted tools: work, do_work, mark_tiles have additional parameter to send state of ctrl/shift keys
 * - Added @ref factory_desc_x
 * - Added @ref way_x::get_max_speed
 * - Added @ref wayobj_desc_x, @ref wayobj_x, @ref command_x::build_wayobj to work with way-objects
 * - Added @ref factory_x::get_raw_name
 * - Added @ref tree_desc_x::get_price, @ref command_x::slope_get_price
 * - Added @ref way_x::get_transported_goods, @ref way_x::get_convoys_passed
 * - Added @ref tile_x::get_way, @ref tile_x::get_depot
 * - Added @ref get_pakset_name, @ref player_x::get_type
 *
 * @section api-122 Release 122.0
 *
 * - Feature: scripted tools
 * - Added @ref debug::pause, @ref debug::set_pause_on_error
 * - Added more tool ids
 * - Added @ref convoy_x::is_waiting, @ref convoy_x::is_loading
 * - Added @ref rules::gui_needs_update
 * - Added @ref convoy_x::get_tile_length
 * - Added @ref settings::get_pay_for_total_distance_mode, @ref settings::get_drive_on_left
 * - Added @ref halt_x::get_connections
 * - Added @ref command_x::can_set_slope
 * - Added @ref halt_x::get_halt
 * - Changed possible direction parameter of @ref command_x::build_station (useful for flat docks)
 * - Added @ref depot_x::get_depot_list
 * - Added @ref vehicle_desc_x::needs_electrification
 * - Added @ref command_x::build_road
 * - Added fields: @ref field_x
 * - Added powerlines: @ref powerline_x, @ref transformer_x, @ref factory_x::get_transformer, @ref factory_x::is_transformer_connected
 *
 * @section api-121 Release 121.0
 *
 * - Added functions to mark tiles, see @ref tile_x::mark
 * - Added @ref convoy_x::is_schedule_editor_open
 *
 * @section api-120-3 Release 120.3
 *
 * - Changed enable scenario scripts to open info window also on clients, see @ref gui::open_info_win_client
 * - Added sign_x::can_pass
 * - Added methods is_valid to check whether in-game object is still present, see @ref ingame_object
 * - Added tile_x::can_remove_all_objects
 * - Added @ref coord_to_dir, @ref coord.to_dir, @ref dir.to_coord
 * - Added @ref simple_heap_x, @ref way_planner_x, @ref bridge_planner_x
 * - Added map_object_x::is_marked
 * - Added gui::open_info_win_at
 * - Added ::is_convoy_allowed
 * - Added convoy_x::is_in_depot
 * - Added scriptable hyperlinks in scenario info window
 * - Added @ref slope
 * - Added functions to build stuff: command_x::build_way, command_x::build_depot, command_x::build_station, command_x::build_bridge, command_x::build_bridge_at, command_x::set_slope, command_x::restore_slope
 * - Added sign_x, sign_desc_x, command_x::build_sign_at
 * - Added line_x::destroy
 *
 * @section api-120-2-2 Release 120.2.2
 *
 * - Added map_object_x::mark and unmark
 *
 * @section api-120-2 Release 120.2
 *
 * - Feature: scripted AI players
 * - Feature: script calls can be suspended and waken up, less strict checks of 'script take too long'
 * - Feature: add commands to modify game state, see @ref game_cmd
 * - Added @ref climates, square_x::get_climate, world::get_size
 * - Added new classes command_x, vehicle_desc_x, tunnel_desc_x, bridge_desc_x
 * - Added tool_ids::tool_set_climate, tool_ids::tool_change_water_height
 * - Added string::toalnum (converts strings to strings that can be used as table keys)
 * - Changed gui::add_message to take additional player_x parameter
 *
 * @section api-120-1-2 Release 120.1.2
 *
 * - Added label_x::get_text, tile_x::get_text
 * - Added factory_x::get_halt_list, halt_x::get_tile_list, halt_x::get_factory_list, square_x::get_halt_list
 * - Added world.get_player
 * - Removed player_x::is_active
 *
 * @section api-120-1 Release 120.1
 *
 * - Added depot_x, depot_x::get_convoy_list
 * - Added line_x::get_waytype, tile_x::get_slope
 * - Added factory_production_x::get_base_production, factory_production_x::get_base_consumption, factory_x::get_tile_list
 * - Added coord::href, coord3d::href
 * - Added label_x, tile_x::remove_object
 * - Added ::get_debug_text, debug::get_forbidden_text
 * - Added operators and conversion to string to ::coord and ::coord3d classes
 * - Added ::coord_to_string, ::coord3d_to_string
 * - Added settings::get_station_coverage, settings::get_passenger_factor
 * - Added settings::get_factory_worker_radius, settings::get_factory_worker_minimum_towns, settings::get_factory_worker_maximum_towns
 * - Added settings::avoid_overcrowding, settings::no_routing_over_overcrowding, settings::separate_halt_capacities
 *
 * @section api-120-0-1 Release 120.0.1
 *
 * - Added building_desc_x::get_building_list, building_desc_x::get_headquarter_level
 * - Added factory_production_x::get_in_transit
 *
 * @section api-120-0 Release 120.0
 *
 * - Added line_x, line_list_x
 * - Deprecated get_convoy_list()
 * - Added world::get_convoy_list, halt_x::get_convoy_list
 * - Added @ref scenario.api to manage backward compatibility of scripts with the changing api
 * - Added ::include
 * - Changed return type of world::get_time to time_ticks_x
 * - Added building_x::is_same_building.
 * - Added attraction_list_x, world::get_attraction_list
 * - Added ::new_month, ::new_year
 * - Added halt_list_x
 * - Added possibility to attach scenario to an already running game, set map.file to "<attach>".
 * - Added building_desc_x::get_type, convoy_x::get_distance_traveled_total, obj_desc_time_x::is_available
 * - Added obj_desc_time_x, obj_desc_transport_x, building_desc_x, tree_desc_x, way_desc_x
 * - Added time_x
 * - Added @ref way_system_types
 * - Added square_x::get_player_halt
 * - Added way_x::get_dirs,way_x::get_dirs_masked
 * - Deprecated square_x::get_halt, use square_x::get_player_halt or tile_x::get_halt instead!
 *
 * @section api-112-3 Release 112.3
 *
 * - Added good_desc_list_x
 * - Added tile_x::is_bridge, tile_x::is_empty, tile_x::is_ground, tile_x::is_tunnel, tile_x::is_water
 * - Added tile_x::has_way, tile_x::has_ways, tile_x::has_two_ways
 * - Added map_objects, map_object_x, building_x, tree_x, way_x
 * - Added iterator tile_x::objects to loop over all objects on the tile
 * - Added ::dir
 * - Added tile_x::get_way_dirs, tile_x::get_way_dirs_masked, tile_x::get_neighbour
 *
 * @section api-112-2 Release 112.2
 *
 * - Added rules::forbid_way_tool_cube, rules::allow_way_tool_cube
 * - Added settings::get_start_time
 * - Added player_x::book_cash
 * - Added settings::get_traffic_level, settings::set_traffic_level, tool_set_traffic_level
 * - Added gui::add_message, gui::add_message_at
 * - Added hyperlinks to scenario window tabs and map positions are accepted in scenario info window
 * - Added ::double_to_string, ::integer_to_string, ::money_to_string, ::get_month_name
 * - Added factory_list_x
 * - Added factory_x::get_name
 * - Added halt_x::get_name, halt_x::get_owner, halt_x::accepts_good
 * - Added player_x::is_active, world::remove_player
 * - Added schedule_x, schedule_entry_x, ::is_schedule_allowed
 * - Added halt_x::is_connected
 *
 * @section api-112-1 Releases 112.0 and 112.1
 *
 * Initial release and minor changes.
 *
 */
