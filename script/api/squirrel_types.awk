# file used to generate doxygen documentation of squirrel API
# needs to be copied to trunk/script/api
BEGIN {
	export_types["city_x::get_name"] = "string()"
	export_types["city_x::get_citizens"] = "array<integer>()"
	export_types["city_x::get_growth"] = "array<integer>()"
	export_types["city_x::get_buildings"] = "array<integer>()"
	export_types["city_x::get_citycars"] = "array<integer>()"
	export_types["city_x::get_transported_pax"] = "array<integer>()"
	export_types["city_x::get_generated_pax"] = "array<integer>()"
	export_types["city_x::get_transported_mail"] = "array<integer>()"
	export_types["city_x::get_generated_mail"] = "array<integer>()"
	export_types["city_x::get_year_citizens"] = "array<integer>()"
	export_types["city_x::get_year_growth"] = "array<integer>()"
	export_types["city_x::get_year_buildings"] = "array<integer>()"
	export_types["city_x::get_year_citycars"] = "array<integer>()"
	export_types["city_x::get_year_transported_pax"] = "array<integer>()"
	export_types["city_x::get_year_generated_pax"] = "array<integer>()"
	export_types["city_x::get_year_transported_mail"] = "array<integer>()"
	export_types["city_x::get_year_generated_mail"] = "array<integer>()"
	export_types["city_x::get_citygrowth_enabled"] = "bool()"
	export_types["city_x::get_pos"] = "coord()"
	export_types["city_x::get_pos_nw"] = "coord()"
	export_types["city_x::get_pos_se"] = "coord()"
	export_types["city_x::change_size"] = "void(integer)"
	export_types["city_x::set_citygrowth_enabled"] = "void(bool)"
	export_types["city_x::set_name"] = "void(string)"
	export_types["convoy_x::needs_electrification"] = "bool()"
	export_types["convoy_x::get_name"] = "string()"
	export_types["convoy_x::get_pos"] = "coord3d()"
	export_types["convoy_x::get_owner"] = "player_x()"
	export_types["convoy_x::get_goods_catg_index"] = "array<integer>()"
	export_types["convoy_x::get_waytype"] = "way_types()"
	export_types["convoy_x::get_schedule"] = "schedule_x()"
	export_types["convoy_x::get_capacity"] = "array<integer>()"
	export_types["convoy_x::get_transported_goods"] = "array<integer>()"
	export_types["convoy_x::get_revenue"] = "array<integer>()"
	export_types["convoy_x::get_cost"] = "array<integer>()"
	export_types["convoy_x::get_profit"] = "array<integer>()"
	export_types["convoy_x::get_traveled_distance"] = "array<integer>()"
	export_types["convoy_x::get_way_tolls"] = "array<integer>()"
	export_types["convoy_x::get_distance_traveled_total"] = "integer()"
	export_types["factory_x::get_consumers"] = "array<coord>()"
	export_types["factory_x::get_suppliers"] = "array<coord>()"
	export_types["factory_x::get_name"] = "string()"
	export_types["factory_x::set_name"] = "void(string)"
	export_types["factory_x::get_production"] = "array<integer>()"
	export_types["factory_x::get_power"] = "array<integer>()"
	export_types["factory_x::get_boost_electric"] = "array<integer>()"
	export_types["factory_x::get_boost_pax"] = "array<integer>()"
	export_types["factory_x::get_boost_mail"] = "array<integer>()"
	export_types["factory_x::get_pax_generated"] = "array<integer>()"
	export_types["factory_x::get_pax_departed"] = "array<integer>()"
	export_types["factory_x::get_pax_arrived"] = "array<integer>()"
	export_types["factory_x::get_mail_generated"] = "array<integer>()"
	export_types["factory_x::get_mail_departed"] = "array<integer>()"
	export_types["factory_x::get_mail_arrived"] = "array<integer>()"
	export_types["factory_production_x::get_storage"] = "array<integer>()"
	export_types["factory_production_x::get_received"] = "array<integer>()"
	export_types["factory_production_x::get_consumed"] = "array<integer>()"
	export_types["factory_production_x::get_in_transit"] = "array<integer>()"
	export_types["factory_production_x::get_delivered"] = "array<integer>()"
	export_types["factory_production_x::get_produced"] = "array<integer>()"
	export_types["obj_desc_x::get_name"] = "string()"
	export_types["obj_desc_x::is_equal"] = "bool(obj_desc_x)"
	export_types["obj_desc_time_x::get_intro_date"] = "time_x()"
	export_types["obj_desc_time_x::get_retire_date"] = "time_x()"
	export_types["obj_desc_time_x::is_future"] = "bool(time_x)"
	export_types["obj_desc_time_x::is_retired"] = "bool(time_x)"
	export_types["obj_desc_time_x::is_available"] = "bool(time_x)"
	export_types["obj_desc_transport_x::get_maintenance"] = "integer()"
	export_types["obj_desc_transport_x::get_cost"] = "integer()"
	export_types["obj_desc_transport_x::get_waytype"] = "way_types()"
	export_types["obj_desc_transport_x::get_topspeed"] = "integer()"
	export_types["building_desc_x::is_attraction"] = "bool()"
	export_types["building_desc_x::get_size"] = "coord(integer)"
	export_types["building_desc_x::get_maintenance"] = "integer()"
	export_types["building_desc_x::get_cost"] = "integer()"
	export_types["building_desc_x::get_capacity"] = "integer()"
	export_types["building_desc_x::can_be_built_underground"] = "bool()"
	export_types["building_desc_x::can_be_built_aboveground"] = "bool()"
	export_types["building_desc_x::enables_pax"] = "bool()"
	export_types["building_desc_x::enables_mail"] = "bool()"
	export_types["building_desc_x::enables_freight"] = "bool()"
	export_types["building_desc_x::get_type"] = "building_desc_x::building_type()"
	export_types["building_desc_x::get_waytype"] = "way_types()"
	export_types["building_desc_x::get_headquarter_level"] = "integer()"
	export_types["::get_building_list"] = "array<building_desc_x>(building_desc_x::building_type)"
	export_types["way_desc_x::has_double_slopes"] = "bool()"
	export_types["way_desc_x::get_system_type"] = "integer()"
	export_types["good_desc_x::get_catg_index"] = "integer()"
	export_types["::open_info_win"] = "bool()"
	export_types["::add_message_at"] = "void(string, coord)"
	export_types["::add_message"] = "void(string)"
	export_types["halt_x::get_name"] = "string()"
	export_types["halt_x::get_owner"] = "player_x()"
	export_types["halt_x::_cmp"] = "integer(halt_x)"
	export_types["halt_x::is_connected"] = "integer(halt_x, good_desc_x)"
	export_types["halt_x::accepts_good"] = "bool(good_desc_x)"
	export_types["halt_x::get_arrived"] = "array<integer>()"
	export_types["halt_x::get_departed"] = "array<integer>()"
	export_types["halt_x::get_waiting"] = "array<integer>()"
	export_types["halt_x::get_happy"] = "array<integer>()"
	export_types["halt_x::get_unhappy"] = "array<integer>()"
	export_types["halt_x::get_noroute"] = "array<integer>()"
	export_types["halt_x::get_convoys"] = "array<integer>()"
	export_types["halt_x::get_walked"] = "array<integer>()"
	export_types["line_x::get_name"] = "string()"
	export_types["line_x::get_owner"] = "player_x()"
	export_types["line_x::get_schedule"] = "schedule_x()"
	export_types["line_x::get_goods_catg_index"] = "array<integer>()"
	export_types["line_x::get_capacity"] = "array<integer>()"
	export_types["line_x::get_transported_goods"] = "array<integer>()"
	export_types["line_x::get_convoy_count"] = "array<integer>()"
	export_types["line_x::get_revenue"] = "array<integer>()"
	export_types["line_x::get_cost"] = "array<integer>()"
	export_types["line_x::get_profit"] = "array<integer>()"
	export_types["line_x::get_traveled_distance"] = "array<integer>()"
	export_types["line_x::get_way_tolls"] = "array<integer>()"
	export_types["map_object_x::get_owner"] = "player_x()"
	export_types["map_object_x::get_name"] = "string()"
	export_types["map_object_x::get_waytype"] = "way_types()"
	export_types["map_object_x::get_pos"] = "coord3d()"
	export_types["map_object_x::is_removable"] = "string(player_x)"
	export_types["map_object_x::get_type"] = "map_objects()"
	export_types["tree_x::get_age"] = "integer()"
	export_types["tree_x::get_desc"] = "tree_desc_x()"
	export_types["building_x::get_factory"] = "factory_x()"
	export_types["building_x::get_city"] = "city_x()"
	export_types["building_x::is_townhall"] = "bool()"
	export_types["building_x::is_headquarter"] = "bool()"
	export_types["building_x::is_monument"] = "bool()"
	export_types["building_x::get_passenger_level"] = "integer()"
	export_types["building_x::get_mail_level"] = "integer()"
	export_types["building_x::get_desc"] = "building_desc_x()"
	export_types["building_x::is_same_building"] = "bool(building_x)"
	export_types["way_x::has_sidewalk"] = "bool()"
	export_types["way_x::is_electrified"] = "bool()"
	export_types["way_x::has_sign"] = "bool()"
	export_types["way_x::has_signal"] = "bool()"
	export_types["way_x::has_wayobj"] = "bool()"
	export_types["way_x::is_crossing"] = "bool()"
	export_types["way_x::get_desc"] = "way_desc_x()"
	export_types["::create"] = "label_x(coord, player_x, string)"
	export_types["label_x::set_text"] = "void(string)"
	export_types["player_x::get_headquarter_level"] = "integer()"
	export_types["player_x::get_headquarter_pos"] = "coord()"
	export_types["player_x::get_name"] = "string()"
	export_types["player_x::get_construction"] = "array<integer>()"
	export_types["player_x::get_vehicle_maint"] = "array<integer>()"
	export_types["player_x::get_new_vehicles"] = "array<integer>()"
	export_types["player_x::get_income"] = "array<integer>()"
	export_types["player_x::get_maintenance"] = "array<integer>()"
	export_types["player_x::get_assets"] = "array<integer>()"
	export_types["player_x::get_cash"] = "array<integer>()"
	export_types["player_x::get_net_wealth"] = "array<integer>()"
	export_types["player_x::get_profit"] = "array<integer>()"
	export_types["player_x::get_operating_profit"] = "array<integer>()"
	export_types["player_x::get_margin"] = "array<integer>()"
	export_types["player_x::get_transported"] = "array<integer>()"
	export_types["player_x::get_powerline"] = "array<integer>()"
	export_types["player_x::get_transported_pax"] = "array<integer>()"
	export_types["player_x::get_transported_mail"] = "array<integer>()"
	export_types["player_x::get_transported_goods"] = "array<integer>()"
	export_types["player_x::get_convoys"] = "array<integer>()"
	export_types["player_x::get_way_tolls"] = "array<integer>()"
	export_types["player_x::book_cash"] = "void(integer)"
	export_types["player_x::is_active"] = "bool()"
	export_types["::translate"] = "string(string)"
	export_types["::load_language_file"] = "string(string)"
	export_types["::double_to_string"] = "string(float, integer)"
	export_types["::integer_to_string"] = "string(integer)"
	export_types["::money_to_string"] = "string(integer)"
	export_types["::coord_to_string"] = "string(coord)"
	export_types["::coord3d_to_string"] = "string(coord3d)"
	export_types["::get_month_name"] = "string(integer)"
	export_types["::forbid_tool"] = "void(integer, integer)"
	export_types["::allow_tool"] = "void(integer, integer)"
	export_types["::forbid_way_tool"] = "void(integer, integer, way_types)"
	export_types["::allow_way_tool"] = "void(integer, integer, way_types)"
	export_types["::forbid_way_tool_rect"] = "void(integer, integer, way_types, coord, coord, string)"
	export_types["::allow_way_tool_rect"] = "void(integer, integer, way_types, coord, coord)"
	export_types["::forbid_way_tool_cube"] = "void(integer, integer, way_types, coord3d, coord3d, string)"
	export_types["::allow_way_tool_cube"] = "void(integer, integer, way_types, coord3d, coord3d)"
	export_types["::clear"] = "void()"
	export_types["::get_forbidden_text"] = "string()"
	export_types["coord3d::get_halt"] = "halt_x(player_x)"
	export_types["settings::get_industry_increase_every"] = "integer()"
	export_types["settings::set_industry_increase_every"] = "void(integer)"
	export_types["settings::get_traffic_level"] = "integer()"
	export_types["settings::set_traffic_level"] = "void(integer)"
	export_types["settings::get_start_time"] = "time_x()"
	export_types["settings::get_station_coverage"] = "integer()"
#	export_types["settings::get_passenger_factor"] = "integer()"
#	export_types["settings::get_factory_worker_radius"] = "integer()"
#	export_types["settings::get_factory_worker_minimum_towns"] = "integer()"
#	export_types["settings::get_factory_worker_maximum_towns"] = "integer()"
	export_types["settings::avoid_overcrowding"] = "bool()"
#	export_types["settings::no_routing_over_overcrowding"] = "bool()"
	export_types["settings::separate_halt_capacities"] = "bool()"
	export_types["tile_x::find_object"] = "map_object_x(map_objects)"
	export_types["tile_x::remove_object"] = "string(player_x, map_objects)"
	export_types["tile_x::get_halt"] = "halt_x()"
	export_types["tile_x::is_water"] = "bool()"
	export_types["tile_x::is_bridge"] = "bool()"
	export_types["tile_x::is_tunnel"] = "bool()"
	export_types["tile_x::is_empty"] = "bool()"
	export_types["tile_x::is_ground"] = "bool()"
	export_types["tile_x::has_way"] = "bool(way_types)"
	export_types["tile_x::has_ways"] = "bool()"
	export_types["tile_x::has_two_ways"] = "bool()"
	export_types["square_x::get_halt"] = "halt_x()"
	export_types["square_x::get_player_halt"] = "halt_x(player_x)"
	export_types["square_x::get_tile_at_height"] = "tile_x(integer)"
	export_types["square_x::get_ground_tile"] = "tile_x()"
	export_types["world::is_coord_valid"] = "bool(coord)"
	export_types["world::find_nearest_city"] = "city_x(coord)"
	export_types["world::get_season"] = "integer()"
	export_types["world::remove_player"] = "bool(player_x)"
	export_types["world::get_time"] = "time_ticks_x()"
	export_types["world::get_citizens"] = "array<integer>()"
	export_types["world::get_growth"] = "array<integer>()"
	export_types["world::get_towns"] = "array<integer>()"
	export_types["world::get_factories"] = "array<integer>()"
	export_types["world::get_convoys"] = "array<integer>()"
	export_types["world::get_citycars"] = "array<integer>()"
	export_types["world::get_ratio_pax"] = "array<integer>()"
	export_types["world::get_generated_pax"] = "array<integer>()"
	export_types["world::get_ratio_mail"] = "array<integer>()"
	export_types["world::get_generated_mail"] = "array<integer>()"
	export_types["world::get_ratio_goods"] = "array<integer>()"
	export_types["world::get_transported_goods"] = "array<integer>()"
	export_types["world::get_year_citizens"] = "array<integer>()"
	export_types["world::get_year_growth"] = "array<integer>()"
	export_types["world::get_year_towns"] = "array<integer>()"
	export_types["world::get_year_factories"] = "array<integer>()"
	export_types["world::get_year_convoys"] = "array<integer>()"
	export_types["world::get_year_citycars"] = "array<integer>()"
	export_types["world::get_year_ratio_pax"] = "array<integer>()"
	export_types["world::get_year_generated_pax"] = "array<integer>()"
	export_types["world::get_year_ratio_mail"] = "array<integer>()"
	export_types["world::get_year_generated_mail"] = "array<integer>()"
	export_types["world::get_year_ratio_goods"] = "array<integer>()"
	export_types["world::get_year_transported_goods"] = "array<integer>()"
	export_types["attraction_list_x::_get"] = "building_x(integer)"
}