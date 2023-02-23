#
# This file is part of the Simutrans project under the Artistic License.
# (see LICENSE.txt)
#
# file used to generate doxygen documentation of squirrel API
# needs to be copied to trunk/script/api
BEGIN {
	export_types_ai["city_list_x::_get"] = "city_x(integer)"
	export_types_ai["city_x::is_valid"] = "bool()"
	export_types_ai["city_x::get_name"] = "string()"
	export_types_ai["city_x::set_name"] = "void(string)"
	export_types_ai["city_x::get_citizens"] = "array<integer>()"
	export_types_ai["city_x::get_growth"] = "array<integer>()"
	export_types_ai["city_x::get_buildings"] = "array<integer>()"
	export_types_ai["city_x::get_citycars"] = "array<integer>()"
	export_types_ai["city_x::get_transported_pax"] = "array<integer>()"
	export_types_ai["city_x::get_generated_pax"] = "array<integer>()"
	export_types_ai["city_x::get_transported_mail"] = "array<integer>()"
	export_types_ai["city_x::get_generated_mail"] = "array<integer>()"
	export_types_ai["city_x::get_year_citizens"] = "array<integer>()"
	export_types_ai["city_x::get_year_growth"] = "array<integer>()"
	export_types_ai["city_x::get_year_buildings"] = "array<integer>()"
	export_types_ai["city_x::get_year_citycars"] = "array<integer>()"
	export_types_ai["city_x::get_year_transported_pax"] = "array<integer>()"
	export_types_ai["city_x::get_year_generated_pax"] = "array<integer>()"
	export_types_ai["city_x::get_year_transported_mail"] = "array<integer>()"
	export_types_ai["city_x::get_year_generated_mail"] = "array<integer>()"
	export_types_ai["city_x::get_citygrowth_enabled"] = "bool()"
	export_types_ai["city_x::get_pos"] = "coord()"
	export_types_ai["city_x::get_pos_nw"] = "coord()"
	export_types_ai["city_x::get_pos_se"] = "coord()"
	export_types_ai["city_x::change_size"] = "string(integer)"
	export_types_ai["city_x::set_citygrowth_enabled"] = "void(bool)"
	export_types_ai["::get_ops_total"] = "integer()"
	export_types_ai["::get_ops_remaining"] = "integer()"
	export_types_ai["debug::pause"] = "bool()"
	export_types_ai["debug::is_paused"] = "bool()"
	export_types_ai["debug::set_pause_on_error"] = "void(bool)"
	export_types_ai["convoy_x::is_valid"] = "bool()"
	export_types_ai["convoy_x::needs_electrification"] = "bool()"
	export_types_ai["convoy_x::get_name"] = "string()"
	export_types_ai["convoy_x::set_name"] = "void(string)"
	export_types_ai["convoy_x::get_pos"] = "coord3d()"
	export_types_ai["convoy_x::get_owner"] = "player_x()"
	export_types_ai["convoy_x::get_goods_catg_index"] = "array<integer>()"
	export_types_ai["convoy_x::get_waytype"] = "way_types()"
	export_types_ai["convoy_x::get_schedule"] = "schedule_x()"
	export_types_ai["convoy_x::get_capacity"] = "array<integer>()"
	export_types_ai["convoy_x::get_transported_goods"] = "array<integer>()"
	export_types_ai["convoy_x::get_revenue"] = "array<integer>()"
	export_types_ai["convoy_x::get_cost"] = "array<integer>()"
	export_types_ai["convoy_x::get_profit"] = "array<integer>()"
	export_types_ai["convoy_x::get_traveled_distance"] = "array<integer>()"
	export_types_ai["convoy_x::get_way_tolls"] = "array<integer>()"
	export_types_ai["convoy_x::get_distance_traveled_total"] = "integer()"
	export_types_ai["convoy_x::get_line"] = "line_x()"
	export_types_ai["convoy_x::set_line"] = "void(player_x, line_x)"
	export_types_ai["convoy_x::get_vehicles"] = "array<vehicle_desc_x>()"
	export_types_ai["convoy_x::get_speed"] = "integer()"
	export_types_ai["convoy_x::get_loading_limit"] = "integer()"
	export_types_ai["convoy_x::get_loading_level"] = "integer()"
	export_types_ai["convoy_x::get_home_depot"] = "coord3d()"
	export_types_ai["convoy_x::has_obsolete_vehicles"] = "bool()"
	export_types_ai["convoy_x::toggle_withdraw"] = "void(player_x)"
	export_types_ai["convoy_x::is_withdrawn"] = "bool()"
	export_types_ai["convoy_x::is_in_depot"] = "bool()"
	export_types_ai["convoy_x::is_waiting"] = "bool()"
	export_types_ai["convoy_x::is_loading"] = "bool()"
	export_types_ai["convoy_x::destroy"] = "void(player_x)"
	export_types_ai["convoy_x::is_schedule_editor_open"] = "bool()"
	export_types_ai["convoy_x::get_tile_length"] = "integer()"
	export_types_ai["convoy_x::change_schedule"] = "void(player_x, schedule_x)"
	export_types_ai["convoy_x::calc_max_speed"] = "integer(integer, integer, integer)"
	export_types_ai["convoy_x::speed_to_tiles_per_month"] = "integer(integer)"
	export_types_ai["factory_x::is_valid"] = "bool()"
	export_types_ai["factory_x::get_consumers"] = "array<coord>()"
	export_types_ai["factory_x::get_suppliers"] = "array<coord>()"
	export_types_ai["factory_x::get_name"] = "string()"
	export_types_ai["factory_x::get_raw_name"] = "string()"
	export_types_ai["factory_x::set_name"] = "void(string)"
	export_types_ai["factory_x::get_production"] = "array<integer>()"
	export_types_ai["factory_x::get_power"] = "array<integer>()"
	export_types_ai["factory_x::get_boost_electric"] = "array<integer>()"
	export_types_ai["factory_x::get_boost_pax"] = "array<integer>()"
	export_types_ai["factory_x::get_boost_mail"] = "array<integer>()"
	export_types_ai["factory_x::get_pax_generated"] = "array<integer>()"
	export_types_ai["factory_x::get_pax_departed"] = "array<integer>()"
	export_types_ai["factory_x::get_pax_arrived"] = "array<integer>()"
	export_types_ai["factory_x::get_mail_generated"] = "array<integer>()"
	export_types_ai["factory_x::get_mail_departed"] = "array<integer>()"
	export_types_ai["factory_x::get_mail_arrived"] = "array<integer>()"
	export_types_ai["factory_x::get_tile_list"] = "array<tile_x>()"
	export_types_ai["factory_x::get_fields_list"] = "array<tile_x>()"
	export_types_ai["factory_x::get_halt_list"] = "array<halt_x>()"
	export_types_ai["factory_x::is_transformer_connected"] = "bool()"
	export_types_ai["factory_x::get_transformer"] = "powerline_x()"
	export_types_ai["factory_x::get_field_count"] = "integer()"
	export_types_ai["factory_x::get_min_field_count"] = "integer()"
	export_types_ai["factory_x::get_desc"] = "factory_desc_x()"
	export_types_ai["factory_production_x::get_storage"] = "array<integer>()"
	export_types_ai["factory_production_x::get_received"] = "array<integer>()"
	export_types_ai["factory_production_x::get_consumed"] = "array<integer>()"
	export_types_ai["factory_production_x::get_in_transit"] = "array<integer>()"
	export_types_ai["factory_production_x::get_delivered"] = "array<integer>()"
	export_types_ai["factory_production_x::get_produced"] = "array<integer>()"
	export_types_ai["factory_production_x::get_consumption_factor"] = "integer()"
	export_types_ai["factory_production_x::get_production_factor"] = "integer()"
	export_types_ai["obj_desc_x::get_name"] = "string()"
	export_types_ai["obj_desc_x::is_equal"] = "bool(obj_desc_x)"
	export_types_ai["obj_desc_x::is_valid"] = "bool()"
	export_types_ai["obj_desc_time_x::get_intro_date"] = "time_x()"
	export_types_ai["obj_desc_time_x::get_retire_date"] = "time_x()"
	export_types_ai["obj_desc_time_x::is_future"] = "bool(time_x)"
	export_types_ai["obj_desc_time_x::is_retired"] = "bool(time_x)"
	export_types_ai["obj_desc_time_x::is_available"] = "bool(time_x)"
	export_types_ai["obj_desc_transport_x::get_maintenance"] = "integer()"
	export_types_ai["obj_desc_transport_x::get_cost"] = "integer()"
	export_types_ai["obj_desc_transport_x::get_waytype"] = "way_types()"
	export_types_ai["obj_desc_transport_x::get_topspeed"] = "integer()"
	export_types_ai["vehicle_desc_x::can_be_first"] = "bool()"
	export_types_ai["vehicle_desc_x::can_be_last"] = "bool()"
	export_types_ai["vehicle_desc_x::get_successors"] = "array<vehicle_desc_x>()"
	export_types_ai["vehicle_desc_x::get_predecessors"] = "array<vehicle_desc_x>()"
	export_types_ai["vehicle_desc_x::get_available_vehicles"] = "array<vehicle_desc_x>(way_types)"
	export_types_ai["vehicle_desc_x::get_power"] = "integer()"
	export_types_ai["vehicle_desc_x::needs_electrification"] = "bool()"
	export_types_ai["vehicle_desc_x::get_freight"] = "good_desc_x()"
	export_types_ai["vehicle_desc_x::get_capacity"] = "integer()"
	export_types_ai["vehicle_desc_x::get_running_cost"] = "integer()"
	export_types_ai["vehicle_desc_x::get_maintenance"] = "integer()"
	export_types_ai["vehicle_desc_x::get_weight"] = "integer()"
	export_types_ai["vehicle_desc_x::get_length"] = "integer()"
	export_types_ai["vehicle_desc_x::is_coupling_allowed"] = "bool(vehicle_desc_x, vehicle_desc_x)"
	export_types_ai["tree_desc_x::get_price"] = "integer()"
	export_types_ai["building_desc_x::is_attraction"] = "bool()"
	export_types_ai["building_desc_x::get_maintenance"] = "integer()"
	export_types_ai["building_desc_x::get_cost"] = "integer()"
	export_types_ai["building_desc_x::get_capacity"] = "integer()"
	export_types_ai["building_desc_x::can_be_built_underground"] = "bool()"
	export_types_ai["building_desc_x::can_be_built_aboveground"] = "bool()"
	export_types_ai["building_desc_x::enables_pax"] = "bool()"
	export_types_ai["building_desc_x::enables_mail"] = "bool()"
	export_types_ai["building_desc_x::enables_freight"] = "bool()"
	export_types_ai["building_desc_x::get_type"] = "building_desc_x::building_type()"
	export_types_ai["building_desc_x::get_waytype"] = "way_types()"
	export_types_ai["building_desc_x::get_headquarter_level"] = "integer()"
	export_types_ai["building_desc_x::get_building_list"] = "array<building_desc_x>(building_desc_x::building_type)"
	export_types_ai["building_desc_x::get_available_stations"] = "array<building_desc_x>(building_desc_x::building_type, way_types, good_desc_x)"
	export_types_ai["building_desc_x::is_terminus"] = "bool()"
	export_types_ai["factory_desc_x::get_name"] = "string()"
	export_types_ai["factory_desc_x::get_building_desc"] = "building_desc_x()"
	export_types_ai["factory_desc_x::is_electricity_producer"] = "bool()"
	export_types_ai["factory_desc_x::get_productivity_base"] = "integer()"
	export_types_ai["factory_desc_x::get_productivity_range"] = "integer()"
	export_types_ai["factory_desc_x::get_list"] = "table<factory_desc_x>()"
	export_types_ai["way_desc_x::has_double_slopes"] = "bool()"
	export_types_ai["way_desc_x::get_system_type"] = "way_system_types()"
	export_types_ai["way_desc_x::get_available_ways"] = "array<way_desc_x>(way_types, way_system_types)"
	export_types_ai["tunnel_desc_x::get_available_tunnels"] = "array<tunnel_desc_x>(way_types)"
	export_types_ai["bridge_desc_x::has_double_ramp"] = "bool()"
	export_types_ai["bridge_desc_x::has_double_start"] = "bool()"
	export_types_ai["bridge_desc_x::get_max_length"] = "integer()"
	export_types_ai["bridge_desc_x::get_max_height"] = "integer()"
	export_types_ai["bridge_desc_x::get_available_bridges"] = "array<bridge_desc_x>(way_types)"
	export_types_ai["good_desc_x::get_catg_index"] = "integer()"
	export_types_ai["good_desc_x::is_interchangeable"] = "bool(good_desc_x)"
	export_types_ai["good_desc_x::get_weight_per_unit"] = "integer()"
	export_types_ai["good_desc_x::calc_revenue"] = "integer(way_types, integer)"
	export_types_ai["sign_desc_x::is_one_way"] = "bool()"
	export_types_ai["sign_desc_x::is_private_way"] = "bool()"
	export_types_ai["sign_desc_x::is_traffic_light"] = "bool()"
	export_types_ai["sign_desc_x::is_choose_sign"] = "bool()"
	export_types_ai["sign_desc_x::is_signal"] = "bool()"
	export_types_ai["sign_desc_x::is_pre_signal"] = "bool()"
	export_types_ai["sign_desc_x::is_priority_signal"] = "bool()"
	export_types_ai["sign_desc_x::is_longblock_signal"] = "bool()"
	export_types_ai["sign_desc_x::is_end_choose_signal"] = "bool()"
	export_types_ai["sign_desc_x::get_available_signs"] = "array<sign_desc_x>(way_types)"
	export_types_ai["wayobj_desc_x::is_overhead_line"] = "bool()"
	export_types_ai["wayobj_desc_x::get_available_wayobjs"] = "array<wayobj_desc_x>(way_types)"
	export_types_ai["gui::add_message_at"] = "string(player_x, string, coord3d)"
	export_types_ai["halt_x::is_valid"] = "bool()"
	export_types_ai["halt_x::get_name"] = "string()"
	export_types_ai["halt_x::set_name"] = "void(string)"
	export_types_ai["halt_x::get_owner"] = "player_x()"
	export_types_ai["halt_x::_cmp"] = "integer(halt_x)"
	export_types_ai["halt_x::is_connected"] = "integer(halt_x, good_desc_x)"
	export_types_ai["halt_x::accepts_good"] = "bool(good_desc_x)"
	export_types_ai["halt_x::get_arrived"] = "array<integer>()"
	export_types_ai["halt_x::get_departed"] = "array<integer>()"
	export_types_ai["halt_x::get_waiting"] = "array<integer>()"
	export_types_ai["halt_x::get_happy"] = "array<integer>()"
	export_types_ai["halt_x::get_unhappy"] = "array<integer>()"
	export_types_ai["halt_x::get_noroute"] = "array<integer>()"
	export_types_ai["halt_x::get_convoys"] = "array<integer>()"
	export_types_ai["halt_x::get_walked"] = "array<integer>()"
	export_types_ai["halt_x::get_tile_list"] = "array<tile_x>()"
	export_types_ai["halt_x::get_factory_list"] = "array<factory_x>()"
	export_types_ai["halt_x::get_freight_to_dest"] = "integer(good_desc_x, coord)"
	export_types_ai["halt_x::get_freight_to_halt"] = "integer(good_desc_x, halt_x)"
	export_types_ai["halt_x::get_capacity"] = "integer(good_desc_x)"
	export_types_ai["halt_x::get_connections"] = "array<halt_x>(good_desc_x)"
	export_types_ai["halt_x::get_halt"] = "halt_x(coord3d, player_x)"
	export_types_ai["line_x::is_valid"] = "bool()"
	export_types_ai["line_x::get_name"] = "string()"
	export_types_ai["line_x::set_name"] = "void(string)"
	export_types_ai["line_x::get_owner"] = "player_x()"
	export_types_ai["line_x::get_schedule"] = "schedule_x()"
	export_types_ai["line_x::get_goods_catg_index"] = "array<integer>()"
	export_types_ai["line_x::get_capacity"] = "array<integer>()"
	export_types_ai["line_x::get_transported_goods"] = "array<integer>()"
	export_types_ai["line_x::get_convoy_count"] = "array<integer>()"
	export_types_ai["line_x::get_revenue"] = "array<integer>()"
	export_types_ai["line_x::get_cost"] = "array<integer>()"
	export_types_ai["line_x::get_profit"] = "array<integer>()"
	export_types_ai["line_x::get_traveled_distance"] = "array<integer>()"
	export_types_ai["line_x::get_way_tolls"] = "array<integer>()"
	export_types_ai["line_x::get_waytype"] = "way_types()"
	export_types_ai["line_x::change_schedule"] = "void(player_x, schedule_x)"
	export_types_ai["line_x::destroy"] = "void(player_x)"
	export_types_ai["map_object_x::is_valid"] = "bool()"
	export_types_ai["map_object_x::get_owner"] = "player_x()"
	export_types_ai["map_object_x::get_name"] = "string()"
	export_types_ai["map_object_x::get_waytype"] = "way_types()"
	export_types_ai["map_object_x::get_pos"] = "coord3d()"
	export_types_ai["map_object_x::is_removable"] = "string(player_x)"
	export_types_ai["map_object_x::get_type"] = "map_objects()"
	export_types_ai["map_object_x::mark"] = "void()"
	export_types_ai["map_object_x::unmark"] = "void()"
	export_types_ai["map_object_x::is_marked"] = "bool()"
	export_types_ai["tree_x::get_age"] = "integer()"
	export_types_ai["tree_x::get_desc"] = "tree_desc_x()"
	export_types_ai["building_x::get_factory"] = "factory_x()"
	export_types_ai["building_x::get_city"] = "city_x()"
	export_types_ai["building_x::is_townhall"] = "bool()"
	export_types_ai["building_x::is_headquarter"] = "bool()"
	export_types_ai["building_x::is_monument"] = "bool()"
	export_types_ai["building_x::get_passenger_level"] = "integer()"
	export_types_ai["building_x::get_mail_level"] = "integer()"
	export_types_ai["building_x::get_desc"] = "building_desc_x()"
	export_types_ai["building_x::get_tile_list"] = "array<tile_x>()"
	export_types_ai["building_x::is_same_building"] = "bool(building_x)"
	export_types_ai["depot_x::append_vehicle"] = "void(player_x, convoy_x, vehicle_desc_x)"
	export_types_ai["depot_x::start_convoy"] = "void(player_x, convoy_x)"
	export_types_ai["depot_x::start_all_convoys"] = "void(player_x)"
	export_types_ai["depot_x::get_convoy_list"] = "array<convoy_x>()"
	export_types_ai["depot_x::get_depot_list"] = "array<depot_x>(player_x, way_types)"
	export_types_ai["way_x::has_sidewalk"] = "bool()"
	export_types_ai["way_x::is_electrified"] = "bool()"
	export_types_ai["way_x::has_sign"] = "bool()"
	export_types_ai["way_x::has_signal"] = "bool()"
	export_types_ai["way_x::has_wayobj"] = "bool()"
	export_types_ai["way_x::is_crossing"] = "bool()"
	export_types_ai["way_x::get_desc"] = "way_desc_x()"
	export_types_ai["way_x::get_max_speed"] = "integer()"
	export_types_ai["way_x::get_transported_goods"] = "array<integer>()"
	export_types_ai["way_x::get_convoys_passed"] = "array<integer>()"
	export_types_ai["label_x::create"] = "string(coord, player_x, string)"
	export_types_ai["label_x::set_text"] = "void(string)"
	export_types_ai["label_x::get_text"] = "string()"
	export_types_ai["sign_x::get_desc"] = "sign_desc_x()"
	export_types_ai["sign_x::can_pass"] = "bool(player_x)"
	export_types_ai["powerline_x::is_connected"] = "bool(powerline_x)"
	export_types_ai["powerline_x::get_factory"] = "factory_x()"
	export_types_ai["field_x::is_deletable"] = "bool()"
	export_types_ai["field_x::get_factory"] = "factory_x()"
	export_types_ai["wayobj_x::get_desc"] = "wayobj_desc_x()"
	export_types_ai["bridge_x::get_desc"] = "bridge_desc_x()"
	export_types_ai["tunnel_x::get_desc"] = "tunnel_desc_x()"
	export_types_ai["player_x::is_valid"] = "bool()"
	export_types_ai["player_x::get_headquarter_level"] = "integer()"
	export_types_ai["player_x::get_headquarter_pos"] = "coord()"
	export_types_ai["player_x::get_name"] = "string()"
	export_types_ai["player_x::set_name"] = "void(string)"
	export_types_ai["player_x::get_construction"] = "array<integer>()"
	export_types_ai["player_x::get_vehicle_maint"] = "array<integer>()"
	export_types_ai["player_x::get_new_vehicles"] = "array<integer>()"
	export_types_ai["player_x::get_income"] = "array<integer>()"
	export_types_ai["player_x::get_maintenance"] = "array<integer>()"
	export_types_ai["player_x::get_assets"] = "array<integer>()"
	export_types_ai["player_x::get_cash"] = "array<integer>()"
	export_types_ai["player_x::get_net_wealth"] = "array<integer>()"
	export_types_ai["player_x::get_profit"] = "array<integer>()"
	export_types_ai["player_x::get_operating_profit"] = "array<integer>()"
	export_types_ai["player_x::get_margin"] = "array<integer>()"
	export_types_ai["player_x::get_transported"] = "array<integer>()"
	export_types_ai["player_x::get_powerline"] = "array<integer>()"
	export_types_ai["player_x::get_transported_pax"] = "array<integer>()"
	export_types_ai["player_x::get_transported_mail"] = "array<integer>()"
	export_types_ai["player_x::get_transported_goods"] = "array<integer>()"
	export_types_ai["player_x::get_convoys"] = "array<integer>()"
	export_types_ai["player_x::get_way_tolls"] = "array<integer>()"
	export_types_ai["player_x::get_current_cash"] = "float()"
	export_types_ai["player_x::get_current_net_wealth"] = "integer()"
	export_types_ai["player_x::get_current_maintenance"] = "integer()"
	export_types_ai["player_x::create_line"] = "void(way_types)"
	export_types_ai["player_x::get_type"] = "integer()"
	export_types_ai["schedule_entry_x::constructor"] = "void(coord3d, integer, integer)"
	export_types_ai["coord3d::get_halt"] = "halt_x(player_x)"
	export_types_ai["settings::get_industry_increase_every"] = "integer()"
	export_types_ai["settings::set_industry_increase_every"] = "void(integer)"
	export_types_ai["settings::get_traffic_level"] = "integer()"
	export_types_ai["settings::set_traffic_level"] = "void(integer)"
	export_types_ai["settings::get_start_time"] = "time_x()"
	export_types_ai["settings::get_station_coverage"] = "integer()"
	export_types_ai["settings::get_passenger_factor"] = "integer()"
	export_types_ai["settings::get_factory_worker_radius"] = "integer()"
	export_types_ai["settings::get_factory_worker_minimum_towns"] = "integer()"
	export_types_ai["settings::get_factory_worker_maximum_towns"] = "integer()"
	export_types_ai["settings::avoid_overcrowding"] = "bool()"
	export_types_ai["settings::no_routing_over_overcrowding"] = "bool()"
	export_types_ai["settings::separate_halt_capacities"] = "bool()"
	export_types_ai["settings::obsolete_vehicles_allowed"] = "bool()"
	export_types_ai["settings::get_max_road_convoi_length"] = "integer()"
	export_types_ai["settings::get_max_rail_convoi_length"] = "integer()"
	export_types_ai["settings::get_max_ship_convoi_length"] = "integer()"
	export_types_ai["settings::get_max_air_convoi_length"] = "integer()"
	export_types_ai["settings::get_drive_on_left"] = "bool()"
	export_types_ai["settings::get_pay_for_total_distance_mode"] = "integer()"
	export_types_ai["settings::get_underground_view_level"] = "integer()"
	export_types_ai["dir::is_single"] = "bool(dir)"
	export_types_ai["dir::is_twoway"] = "bool(dir)"
	export_types_ai["dir::is_threeway"] = "bool(dir)"
	export_types_ai["dir::is_curve"] = "bool(dir)"
	export_types_ai["dir::is_straight"] = "bool(dir)"
	export_types_ai["dir::double"] = "dir(dir)"
	export_types_ai["dir::backward"] = "dir(dir)"
	export_types_ai["dir::to_slope"] = "slope(dir)"
	export_types_ai["slope::to_dir"] = "dir(slope)"
	export_types_ai["::translate"] = "string(string)"
	export_types_ai["::double_to_string"] = "string(float, integer)"
	export_types_ai["::integer_to_string"] = "string(integer)"
	export_types_ai["::money_to_string"] = "string(integer)"
	export_types_ai["::coord_to_string"] = "string(coord)"
	export_types_ai["::coord3d_to_string"] = "string(coord3d)"
	export_types_ai["::get_month_name"] = "string(integer)"
	export_types_ai["::difftick_to_string"] = "string(integer)"
	export_types_ai["tile_x::is_valid"] = "bool()"
	export_types_ai["tile_x::find_object"] = "map_object_x(map_objects)"
	export_types_ai["tile_x::remove_object"] = "string(player_x, map_objects)"
	export_types_ai["tile_x::get_halt"] = "halt_x()"
	export_types_ai["tile_x::is_water"] = "bool()"
	export_types_ai["tile_x::is_bridge"] = "bool()"
	export_types_ai["tile_x::is_tunnel"] = "bool()"
	export_types_ai["tile_x::is_empty"] = "bool()"
	export_types_ai["tile_x::is_ground"] = "bool()"
	export_types_ai["tile_x::is_crossing"] = "bool()"
	export_types_ai["tile_x::get_slope"] = "slope()"
	export_types_ai["tile_x::get_text"] = "string()"
	export_types_ai["tile_x::has_way"] = "bool(way_types)"
	export_types_ai["tile_x::has_ways"] = "bool()"
	export_types_ai["tile_x::has_two_ways"] = "bool()"
	export_types_ai["tile_x::get_way"] = "way_x(way_types)"
	export_types_ai["tile_x::get_neighbour"] = "tile_x(way_types, dir)"
	export_types_ai["tile_x::get_depot"] = "depot_x()"
	export_types_ai["tile_x::can_remove_all_objects"] = "string(player_x)"
	export_types_ai["tile_x::is_marked"] = "bool()"
	export_types_ai["tile_x::unmark"] = "void()"
	export_types_ai["tile_x::mark"] = "void()"
	export_types_ai["tile_x::get_convoys"] = "array<convoy_x>()"
	export_types_ai["square_x::is_valid"] = "bool()"
	export_types_ai["square_x::get_halt"] = "halt_x()"
	export_types_ai["square_x::get_player_halt"] = "halt_x(player_x)"
	export_types_ai["square_x::get_tile_at_height"] = "tile_x(integer)"
	export_types_ai["square_x::get_ground_tile"] = "tile_x()"
	export_types_ai["square_x::get_halt_list"] = "array<halt_x>()"
	export_types_ai["square_x::get_climate"] = "climates()"
	export_types_ai["world::is_coord_valid"] = "bool(coord)"
	export_types_ai["world::find_nearest_city"] = "city_x(coord)"
	export_types_ai["world::get_season"] = "integer()"
	export_types_ai["world::get_player"] = "player_x(integer)"
	export_types_ai["world::get_time"] = "time_ticks_x()"
	export_types_ai["world::get_citizens"] = "array<integer>()"
	export_types_ai["world::get_growth"] = "array<integer>()"
	export_types_ai["world::get_towns"] = "array<integer>()"
	export_types_ai["world::get_factories"] = "array<integer>()"
	export_types_ai["world::get_convoys"] = "array<integer>()"
	export_types_ai["world::get_citycars"] = "array<integer>()"
	export_types_ai["world::get_ratio_pax"] = "array<integer>()"
	export_types_ai["world::get_generated_pax"] = "array<integer>()"
	export_types_ai["world::get_ratio_mail"] = "array<integer>()"
	export_types_ai["world::get_generated_mail"] = "array<integer>()"
	export_types_ai["world::get_ratio_goods"] = "array<integer>()"
	export_types_ai["world::get_transported_goods"] = "array<integer>()"
	export_types_ai["world::get_year_citizens"] = "array<integer>()"
	export_types_ai["world::get_year_growth"] = "array<integer>()"
	export_types_ai["world::get_year_towns"] = "array<integer>()"
	export_types_ai["world::get_year_factories"] = "array<integer>()"
	export_types_ai["world::get_year_convoys"] = "array<integer>()"
	export_types_ai["world::get_year_citycars"] = "array<integer>()"
	export_types_ai["world::get_year_ratio_pax"] = "array<integer>()"
	export_types_ai["world::get_year_generated_pax"] = "array<integer>()"
	export_types_ai["world::get_year_ratio_mail"] = "array<integer>()"
	export_types_ai["world::get_year_generated_mail"] = "array<integer>()"
	export_types_ai["world::get_year_ratio_goods"] = "array<integer>()"
	export_types_ai["world::get_year_transported_goods"] = "array<integer>()"
	export_types_ai["world::use_timeline"] = "bool()"
	export_types_ai["attraction_list_x::_get"] = "building_x(integer)"
	export_types_ai["label_list_x::_get"] = "label_x(integer)"
	export_types_ai["label_list_x::get_count"] = "integer()"
	export_types_ai["::get_pakset_name"] = "string()"
	export_types_ai["::get_version_number"] = "string()"
	export_types_ai["simple_heap_x::clear"] = "void()"
	export_types_ai["simple_heap_x::len"] = "integer()"
	export_types_ai["simple_heap_x::is_empty"] = "bool()"
	export_types_ai["simple_heap_x::insert"] = "void(integer, integer)"
	export_types_ai["way_planner_x::set_build_types"] = "void(way_desc_x)"
	export_types_ai["way_planner_x::is_allowed_step"] = "bool(tile_x, tile_x)"
	export_types_ai["bridge_planner_x::find_end"] = "coord3d(player_x, coord3d, dir, bridge_desc_x, integer)"
	export_types_ai["command_x::get_flags"] = "integer()"
	export_types_ai["command_x::set_flags"] = "void(integer)"
	export_types_ai["command_x::build_way"] = "string(player_x, coord3d, coord3d, way_desc_x, bool)"
	export_types_ai["command_x::build_road"] = "string(player_x, coord3d, coord3d, way_desc_x, bool, bool)"
	export_types_ai["command_x::build_depot"] = "string(player_x, coord3d, building_desc_x)"
	export_types_ai["command_x::build_station"] = "string(player_x, coord3d, building_desc_x, dir)"
	export_types_ai["command_x::build_bridge"] = "string(player_x, coord3d, coord3d, bridge_desc_x)"
	export_types_ai["command_x::build_bridge_at"] = "string(player_x, coord3d, bridge_desc_x)"
	export_types_ai["command_x::set_slope"] = "string(player_x, coord3d, slope)"
	export_types_ai["command_x::restore_slope"] = "string(player_x, coord3d)"
	export_types_ai["command_x::can_set_slope"] = "string(player_x, coord3d, slope)"
	export_types_ai["command_x::slope_get_price"] = "integer(slope)"
	export_types_ai["command_x::build_sign_at"] = "string(player_x, coord3d, sign_desc_x)"
	export_types_ai["command_x::build_wayobj"] = "string(player_x, coord3d, coord3d, wayobj_desc_x)"
	export_types_ai["command_x::change_climate_at"] = "string(player_x, coord3d, integer)"
}
