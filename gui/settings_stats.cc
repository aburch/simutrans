/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simcity.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "settings_stats.h"


/* stuff not set here ....
INIT_NUM( "intercity_road_length", umgebung_t::intercity_road_length);
INIT_NUM( "river_number", river_number );
INIT_NUM( "river_min_length", min_river_length );
INIT_NUM( "river_max_length", max_river_length );
INIT_NUM( "diagonal_multiplier", pak_diagonal_multiplier);
*/


// just free memory
void settings_stats_t::free_all()
{
	while(  !label.empty()  ) {
		delete label.remove_first();
	}
	while(  !numinp.empty()  ) {
		delete numinp.remove_first();
	}
	while(  !button.empty()  ) {
		delete button.remove_first();
	}
}




/* Nearly automatic lists with controls:
 * BEWARE: The init exit pair MUST match in the same order or else!!!
 */
void settings_general_stats_t::init(einstellungen_t *sets)
{
	INIT_INIT
//	INIT_BOOL( "drive_left", umgebung_t::drive_on_left );	//cannot be switched after loading paks
	INIT_NUM( "autosave", umgebung_t::autosave, 0, 12, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "frames_per_second",umgebung_t::fps, 10, 25, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "fast_forward", umgebung_t::max_acceleration, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_BOOL( "numbered_stations", sets->get_numbered_stations() );
	INIT_NUM( "show_names", umgebung_t::show_names, 0, 3, gui_numberinput_t::AUTOLINEAR, true );
	INIT_NUM( "show_month", umgebung_t::show_month, 0, 4, gui_numberinput_t::AUTOLINEAR, true );
	SEPERATOR
	INIT_NUM( "bits_per_month", sets->get_bits_per_month(), 16, 24, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "use_timeline", sets->get_use_timeline(), 0, 2, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "starting_year", sets->get_starting_year(), 0, 2999, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "starting_month", sets->get_starting_month(), 0, 11, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "water_animation_ms", umgebung_t::water_animation, 0, 1000, 25, false );
	INIT_NUM( "random_grounds_probability", umgebung_t::ground_object_probability, 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	INIT_NUM( "random_wildlife_probability", umgebung_t::moving_object_probability, 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	SEPERATOR
	INIT_BOOL( "pedes_and_car_info", umgebung_t::verkehrsteilnehmer_info );
	INIT_BOOL( "tree_info", umgebung_t::tree_info );
	INIT_BOOL( "ground_info", umgebung_t::ground_info );
	INIT_BOOL( "townhall_info", umgebung_t::townhall_info );
	INIT_BOOL( "only_single_info", umgebung_t::single_info );
	SEPERATOR
	INIT_BOOL( "window_buttons_right", umgebung_t::window_buttons_right );
	INIT_BOOL( "window_frame_active", umgebung_t::window_frame_active );
	SEPERATOR
	INIT_BOOL( "show_tooltips", umgebung_t::show_tooltips );
	INIT_NUM( "tooltip_background_color", umgebung_t::tooltip_color, 0, 255, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "tooltip_text_color", umgebung_t::tooltip_textcolor, 0, 255, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_NUM( "cursor_overlay_color", umgebung_t::cursor_overlay_color, 0, 255, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_BOOL( "left_to_right_graphs", umgebung_t::left_to_right_graphs );

	clear_dirty();
	set_groesse( settings_stats_t::get_groesse() );
}

void settings_general_stats_t::read(einstellungen_t *sets)
{
	EXIT_INIT
//	EXIT_BOOL_VALUE( umgebung_t::drive_on_left );	//cannot be switched after loading paks
	EXIT_NUM_VALUE( umgebung_t::autosave );
	EXIT_NUM_VALUE( umgebung_t::fps );
	EXIT_NUM_VALUE( umgebung_t::max_acceleration );

	EXIT_BOOL( sets->set_numbered_stations );
	EXIT_NUM_VALUE( umgebung_t::show_names );
	EXIT_NUM_VALUE( umgebung_t::show_month );

	EXIT_NUM( sets->set_bits_per_month );
	EXIT_NUM( sets->set_use_timeline );
	EXIT_NUM( sets->set_starting_year );
	EXIT_NUM( sets->set_starting_month );

	EXIT_NUM_VALUE( umgebung_t::water_animation );
	EXIT_NUM_VALUE( umgebung_t::ground_object_probability );
	EXIT_NUM_VALUE( umgebung_t::moving_object_probability );

	EXIT_BOOL_VALUE( umgebung_t::verkehrsteilnehmer_info );
	EXIT_BOOL_VALUE( umgebung_t::tree_info );
	EXIT_BOOL_VALUE( umgebung_t::ground_info );
	EXIT_BOOL_VALUE( umgebung_t::townhall_info );
	EXIT_BOOL_VALUE( umgebung_t::single_info );

	EXIT_BOOL_VALUE( umgebung_t::window_buttons_right );
	EXIT_BOOL_VALUE( umgebung_t::window_frame_active );

	EXIT_BOOL_VALUE( umgebung_t::show_tooltips );
	EXIT_NUM_VALUE( umgebung_t::tooltip_color );
	EXIT_NUM_VALUE( umgebung_t::tooltip_textcolor );

	EXIT_NUM_VALUE( umgebung_t::cursor_overlay_color );
	EXIT_BOOL_VALUE( umgebung_t::left_to_right_graphs );
}




void settings_routing_stats_t::init(einstellungen_t *sets)
{
	INIT_INIT
	INIT_BOOL( "seperate_halt_capacities", sets->is_seperate_halt_capacities() );
	INIT_BOOL( "avoid_overcrowding", sets->is_avoid_overcrowding() );
	INIT_BOOL( "no_routing_over_overcrowded", sets->is_no_routing_over_overcrowding() );
	INIT_NUM( "station_coverage", sets->get_station_coverage(), 1, 8, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "max_route_steps", sets->get_max_route_steps(), 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	INIT_NUM( "max_hops", sets->get_max_hops(), 100, 65000, gui_numberinput_t::POWER2, false );
	INIT_NUM( "max_transfers", sets->get_max_transfers(), 1, 100, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "way_straight", sets->way_count_straight, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_curve", sets->way_count_curve, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_double_curve", sets->way_count_double_curve, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_90_curve", sets->way_count_90_curve, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_slope", sets->way_count_slope, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_tunnel", sets->way_count_tunnel, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_max_bridge_len", sets->way_max_bridge_len, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "way_leaving_road", sets->way_count_leaving_road, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );

	clear_dirty();
	set_groesse( settings_stats_t::get_groesse() );
}

void settings_routing_stats_t::read(einstellungen_t *sets)
{
	EXIT_INIT
	EXIT_BOOL( sets->set_seperate_halt_capacities );
	EXIT_BOOL( sets->set_avoid_overcrowding );
	EXIT_BOOL( sets->set_no_routing_over_overcrowding );
	EXIT_NUM( sets->set_station_coverage );
	EXIT_NUM( sets->set_max_route_steps );
	EXIT_NUM( sets->set_max_hops );
	EXIT_NUM( sets->set_max_transfers );
	EXIT_NUM_VALUE( sets->way_count_straight );
	EXIT_NUM_VALUE( sets->way_count_curve );
	EXIT_NUM_VALUE( sets->way_count_double_curve );
	EXIT_NUM_VALUE( sets->way_count_90_curve );
	EXIT_NUM_VALUE( sets->way_count_slope );
	EXIT_NUM_VALUE( sets->way_count_tunnel );
	EXIT_NUM_VALUE( sets->way_max_bridge_len );
	EXIT_NUM_VALUE( sets->way_count_leaving_road );
}




void settings_economy_stats_t::init(einstellungen_t *sets)
{
	INIT_INIT
	INIT_COST( "starting_money", sets->get_starting_money(sets->get_starting_year()), 1, 0x7FFFFFFFul, 10000, false );
	INIT_NUM( "pay_for_total_distance", sets->get_pay_for_total_distance_mode(), 0, 2, gui_numberinput_t::AUTOLINEAR, true );
	INIT_BOOL( "first_beginner", sets->get_beginner_mode() );
	INIT_NUM( "beginner_price_factor", sets->get_beginner_price_factor(), 1, 25000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "allow_buying_obsolete_vehicles", sets->get_allow_buying_obsolete_vehicles() );
	SEPERATOR
	INIT_BOOL( "just_in_time", sets->get_just_in_time() );
	INIT_BOOL( "crossconnect_factories", sets->is_crossconnect_factories() );
	INIT_NUM( "crossconnect_factories_percentage", sets->get_crossconnect_factor(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "industry_increase_every", stadt_t::get_industry_increase(), 0, 100000, 100, false );
	INIT_NUM( "factory_spacing", sets->get_factory_spacing(), 1, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "electric_promille", sets->get_electric_promille(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "passenger_factor",  sets->get_passenger_factor(), 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "minimum_city_distance", stadt_t::get_minimum_city_distance(), 1, 20000, 10, false );
	INIT_NUM( "factory_worker_radius", sets->get_factory_worker_radius(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "factory_worker_percentage", sets->get_factory_worker_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "tourist_percentage", sets->get_tourist_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "passenger_multiplier", sets->get_passenger_multiplier(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "mail_multiplier", sets->get_mail_multiplier(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "goods_multiplier", sets->get_goods_multiplier(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
//	INIT_NUM( "electricity_multiplier", sets->get_electricity_multiplier(), 0, 10000, 10, false );
	SEPERATOR
	INIT_NUM( "growthfactor_villages", sets->get_growthfactor_small(), 1, 10000, 10, false );
	INIT_NUM( "growthfactor_cities", sets->get_growthfactor_medium(), 1, 10000, 10, false );
	INIT_NUM( "growthfactor_capitals", sets->get_growthfactor_large(), 1, 10000, 10, false );
	SEPERATOR
	INIT_BOOL( "random_pedestrians", sets->get_random_pedestrians() );
	INIT_BOOL( "stop_pedestrians", sets->get_show_pax() );
	INIT_NUM( "citycar_level", sets->get_verkehr_level(), 0, 16, 12, false );
	INIT_NUM( "default_citycar_life", sets->get_stadtauto_duration(), 1, 1200, 12, false );

	clear_dirty();
	set_groesse( settings_stats_t::get_groesse() );
}

void settings_economy_stats_t::read( einstellungen_t *sets )
{
	EXIT_INIT
	if(  sets->get_starting_money(sets->get_starting_year())!=((sint64)(numinp.at(read_numinp)->get_value())*100)  ) {
		// because this will render the table based values invalid, we do this only when needed
		EXIT_COST( sets->set_starting_money );
	}
	else {
		// skip this
		read_numinp++;
	}
	EXIT_NUM( sets->set_pay_for_total_distance_mode );
	EXIT_BOOL( sets->set_beginner_mode );
	EXIT_NUM( sets->set_beginner_price_factor );
	EXIT_BOOL( sets->set_allow_buying_obsolete_vehicles );

	EXIT_BOOL( sets->set_just_in_time );
	EXIT_BOOL( sets->set_crossconnect_factories );
	EXIT_NUM( sets->set_crossconnect_factor );
	EXIT_NUM( stadt_t::set_industry_increase );
	EXIT_NUM( sets->set_factory_spacing );
	EXIT_NUM( sets->set_electric_promille );
	EXIT_NUM( sets->set_passenger_factor );
	EXIT_NUM( stadt_t::set_minimum_city_distance );
	EXIT_NUM( sets->set_factory_worker_radius );
	EXIT_NUM( sets->set_factory_worker_percentage );
	EXIT_NUM( sets->set_tourist_percentage );
	EXIT_NUM( sets->set_passenger_multiplier );
	EXIT_NUM( sets->set_mail_multiplier );
	EXIT_NUM( sets->set_goods_multiplier );
//	EXIT_NUM( sets->set_electricity_multiplier );
	EXIT_NUM( sets->set_growthfactor_small );
	EXIT_NUM( sets->set_growthfactor_medium );
	EXIT_NUM( sets->set_growthfactor_large );
	EXIT_BOOL( sets->set_random_pedestrians );
	EXIT_BOOL( sets->set_show_pax );
	EXIT_NUM( sets->set_verkehr_level );
	EXIT_NUM( sets->set_stadtauto_duration );
}



void settings_costs_stats_t::init(einstellungen_t *sets)
{
	INIT_INIT
	INIT_NUM( "maintenance_building", sets->maint_building/100, 1, 100000000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_COST( "cost_multiply_dock", -sets->cst_multiply_dock, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_station", -sets->cst_multiply_station, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_roadstop", -sets->cst_multiply_roadstop, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_airterminal", -sets->cst_multiply_airterminal, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_post", -sets->cst_multiply_post, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_headquarter", -sets->cst_multiply_headquarter, 1, 100000000, 10, false );
	INIT_COST( "cost_depot_air", -sets->cst_depot_air, 1, 100000000, 10, false );
	INIT_COST( "cost_depot_rail", -sets->cst_depot_rail, 1, 100000000, 10, false );
	INIT_COST( "cost_depot_road", -sets->cst_depot_road, 1, 100000000, 10, false );
	INIT_COST( "cost_depot_ship", -sets->cst_depot_ship, 1, 100000000, 10, false );
	INIT_COST( "cost_buy_land", -sets->cst_buy_land, 1, 100000000, 10, false );
	INIT_COST( "cost_alter_land", -sets->cst_alter_land, 1, 100000000, 10, false );
	INIT_COST( "cost_set_slope", -sets->cst_set_slope, 1, 100000000, 10, false );
	INIT_COST( "cost_found_city", -sets->cst_found_city, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_found_industry", -sets->cst_multiply_found_industry, 1, 100000000, 10, false );
	INIT_COST( "cost_remove_tree", -sets->cst_remove_tree, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_remove_haus", -sets->cst_multiply_remove_haus, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_remove_field", -sets->cst_multiply_remove_field, 1, 100000000, 10, false );
	INIT_COST( "cost_transformer", -sets->cst_transformer, 1, 100000000, 10, false );
	INIT_COST( "cost_maintain_transformer", -sets->cst_maintain_transformer, 1, 100000000, 10, false );
	set_groesse( settings_stats_t::get_groesse() );
}

void settings_costs_stats_t::read(einstellungen_t *sets)
{
	EXIT_INIT
	EXIT_NUM_VALUE( sets->maint_building )*100;
	EXIT_COST_VALUE( sets->cst_multiply_dock )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_station )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_roadstop )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_airterminal )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_post )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_headquarter )*(-1);
	EXIT_COST_VALUE( sets->cst_depot_air )*(-1);
	EXIT_COST_VALUE( sets->cst_depot_rail )*(-1);
	EXIT_COST_VALUE( sets->cst_depot_road )*(-1);
	EXIT_COST_VALUE( sets->cst_depot_ship )*(-1);
	EXIT_COST_VALUE( sets->cst_buy_land )*(-1);
	EXIT_COST_VALUE( sets->cst_alter_land )*(-1);
	EXIT_COST_VALUE( sets->cst_set_slope )*(-1);
	EXIT_COST_VALUE( sets->cst_found_city )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_found_industry )*(-1);
	EXIT_COST_VALUE( sets->cst_remove_tree )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_remove_haus )*(-1);
	EXIT_COST_VALUE( sets->cst_multiply_remove_field )*(-1);
	EXIT_COST_VALUE( sets->cst_transformer )*(-1);
	EXIT_COST_VALUE( sets->cst_maintain_transformer )*(-1);

	clear_dirty();
	set_groesse( settings_stats_t::get_groesse() );
}

