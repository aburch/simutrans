/** @file changelog.h Documenting the timeline of api evolution */

/** @page changelog Changelog
 *
 * @section api-trunk Current trunk
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
 *
 * @section api-112-1 Releases 112.0 and 112.1
 *
 * Initial release and minor changes.
 *
 */
