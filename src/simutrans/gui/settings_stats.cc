/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "welt.h"
#include "simwin.h"
#include "../simversion.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../player/finance.h" // MAX_PLAYER_HISTORY_YEARS
#include "../vehicle/vehicle_base.h"
#include "settings_stats.h"
#include "components/gui_divider.h"


/* stuff not set here ....
INIT_NUM( "intercity_road_length", env_t::intercity_road_length);
INIT_NUM( "diagonal_multiplier", pak_diagonal_multiplier);
*/


static char const* const version[] =
{
	"0.99.17",
	"0.100.0",
	"0.101.0",
	"0.102.1",
	"0.102.2",
	"0.102.5",
	"0.110.0",
	"0.110.1",
	"0.111.0",
	"0.111.1",
	"0.111.2",
	"0.111.3",
	"0.111.4",
	"0.112.0",
	"0.112.2",
	"0.120.1",
	"0.120.2",
	"0.120.3",
	"0.120.4",
	"0.120.5",
	"122.0"
};

bool settings_general_stats_t::action_triggered(gui_action_creator_t *comp, value_t v)
{
	assert( comp==&savegame ); (void)comp;

	if(  v.i==-1  ) {
		savegame.set_selection( 0 );
	}
	return true;
}

/* Nearly automatic lists with controls:
 * BEWARE: The init exit pair MUST match in the same order or else!!!
 */
void settings_general_stats_t::init(settings_t const* const sets)
{
	INIT_INIT

	// combobox for savegame version
	savegame.clear_elements();
	for(  uint32 i=0;  i<lengthof(version);  i++  ) {
		savegame.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( version[i]+2, SYSCOL_TEXT ) ;
		if(  strcmp(version[i],env_t::savegame_version_str)==0  ) {
			savegame.set_selection( i );
		}
	}
	savegame.set_focusable( false );
	add_component( &savegame );
	savegame.add_listener( this );
	INIT_LB( "savegame version" );
	SEPERATOR
	INIT_BOOL( "drive_left", sets->is_drive_left() );
	INIT_BOOL( "signals_on_left", sets->is_signals_left() );
	SEPERATOR
	INIT_NUM( "autosave", env_t::autosave, 0, 12, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "fast_forward", env_t::max_acceleration, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_BOOL( "numbered_stations", sets->get_numbered_stations() );
	INIT_NUM( "show_names", env_t::show_names, 0, 3, gui_numberinput_t::AUTOLINEAR, true );
	SEPERATOR
	INIT_NUM( "bits_per_month", sets->get_bits_per_month(), 16, 24, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "use_timeline", sets->get_use_timeline(), 0, 3, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "starting_year", sets->get_starting_year(), 0, 2999, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "starting_month", sets->get_starting_month(), 0, 11, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "show_month", env_t::show_month, 0, 7, gui_numberinput_t::AUTOLINEAR, true );
	SEPERATOR
	INIT_NUM( "random_grounds_probability", env_t::ground_object_probability, 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	INIT_NUM( "random_wildlife_probability", env_t::moving_object_probability, 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	SEPERATOR
	INIT_BOOL( "pedes_and_car_info", env_t::road_user_info );
	INIT_BOOL( "tree_info", env_t::tree_info );
	INIT_BOOL( "ground_info", env_t::ground_info );
	INIT_BOOL( "townhall_info", env_t::townhall_info );
	INIT_BOOL( "only_single_info", env_t::single_info );
	SEPERATOR
	INIT_NUM( "compass_map_position", env_t::compass_map_position, 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "compass_screen_position", env_t::compass_screen_position, 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "world_maximum_height", sets->get_maximumheight(), 16, 127, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "world_minimum_height", sets->get_minimumheight(), -127, -12, gui_numberinput_t::AUTOLINEAR, false );

	INIT_END
	clear_dirty();
}

void settings_general_stats_t::read(settings_t* const sets)
{
	READ_INIT

	int selected = savegame.get_selection();
	if(  0 <= selected  &&  (uint32)selected < lengthof(version)  ) {
		env_t::savegame_version_str = version[ selected ];
	}

	READ_BOOL_VALUE( sets->drive_on_left );
	vehicle_base_t::set_overtaking_offsets( sets->drive_on_left );
	READ_BOOL_VALUE( sets->signals_on_left );

	READ_NUM_VALUE( env_t::autosave );
	READ_NUM_VALUE( env_t::max_acceleration );

	READ_BOOL_VALUE( sets->numbered_stations );
	READ_NUM_VALUE( env_t::show_names );

	READ_NUM_VALUE( sets->bits_per_month );
	READ_NUM_VALUE( sets->use_timeline );
	READ_NUM_VALUE_NEW( sets->starting_year );
	READ_NUM_VALUE_NEW( sets->starting_month );
	READ_NUM_VALUE( env_t::show_month );

	READ_NUM_VALUE( env_t::ground_object_probability );
	READ_NUM_VALUE( env_t::moving_object_probability );

	READ_BOOL_VALUE( env_t::road_user_info );
	READ_BOOL_VALUE( env_t::tree_info );
	READ_BOOL_VALUE( env_t::ground_info );
	READ_BOOL_VALUE( env_t::townhall_info );
	READ_BOOL_VALUE( env_t::single_info );

	READ_NUM_VALUE( env_t::compass_map_position );
	READ_NUM_VALUE( env_t::compass_screen_position );

	READ_NUM_VALUE( sets->world_maximum_height );
	READ_NUM_VALUE( sets->world_minimum_height );
}

void settings_display_stats_t::init(settings_t const* const)
{
	INIT_INIT
	INIT_NUM( "frames_per_second",env_t::fps, env_t::min_fps, env_t::max_fps, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "fast_forward_frames_per_second", env_t::ff_fps, env_t::min_fps, env_t::max_fps, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM( "simple_drawing_tile_size",env_t::simple_drawing_default, 2, 256, gui_numberinput_t::POWER2, false );
	INIT_BOOL( "simple_drawing_fast_forward",env_t::simple_drawing_fast_forward );
	INIT_NUM( "water_animation_ms", env_t::water_animation, 0, 1000, 25, false );
	INIT_NUM( "follow_convoi_underground", env_t::follow_convoi_underground, 0, 2, 1, true );
	SEPERATOR
	INIT_BOOL( "window_buttons_right", env_t::window_buttons_right );
	INIT_BOOL( "window_frame_active", env_t::window_frame_active );
	INIT_NUM( "default_window_title_color", env_t::default_window_title_color_rgb, 0, 16777215, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "front_window_text_color", env_t::front_window_text_color_rgb, 0, 16777215, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "bottom_window_text_color", env_t::bottom_window_text_color_rgb, 0, 16777215, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "bottom_window_darkness", env_t::bottom_window_darkness, 0, 100, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_BOOL( "show_tooltips", env_t::show_tooltips );
	INIT_NUM( "tooltip_background_color", env_t::tooltip_color_rgb, 0, 16777215, 1, 0 );
	INIT_NUM( "tooltip_text_color", env_t::tooltip_textcolor_rgb, 0, 16777215, 1, 0 );
	INIT_NUM( "tooltip_delay", env_t::tooltip_delay, 0, 10000, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "tooltip_duration", env_t::tooltip_duration, 0, 30000, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_NUM( "cursor_overlay_color", env_t::cursor_overlay_color_rgb, 0, 16777215, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_BOOL( "left_to_right_graphs", env_t::left_to_right_graphs );
	SEPERATOR
	INIT_BOOL( "player_finance_display_account", env_t::player_finance_display_account );

	INIT_END
}

void settings_display_stats_t::read(settings_t* const)
{
	READ_INIT
	// all visual stuff
	READ_NUM_VALUE( env_t::fps );
	READ_NUM_VALUE( env_t::ff_fps );
	READ_NUM_VALUE( env_t::simple_drawing_default );
	READ_BOOL_VALUE( env_t::simple_drawing_fast_forward );
	READ_NUM_VALUE( env_t::water_animation );
	READ_NUM_VALUE( env_t::follow_convoi_underground );

	READ_BOOL_VALUE( env_t::window_buttons_right );
	READ_BOOL_VALUE( env_t::window_frame_active );
	READ_NUM_VALUE( env_t::default_window_title_color_rgb );
	READ_NUM_VALUE( env_t::front_window_text_color_rgb );
	READ_NUM_VALUE( env_t::bottom_window_text_color_rgb );
	READ_NUM_VALUE( env_t::bottom_window_darkness );

	READ_BOOL_VALUE( env_t::show_tooltips );
	READ_NUM_VALUE( env_t::tooltip_color_rgb );
	READ_NUM_VALUE( env_t::tooltip_textcolor_rgb );
	READ_NUM_VALUE( env_t::tooltip_delay );
	READ_NUM_VALUE( env_t::tooltip_duration );

	READ_NUM_VALUE( env_t::cursor_overlay_color_rgb );
	READ_BOOL_VALUE( env_t::left_to_right_graphs );

	READ_BOOL_VALUE( env_t::player_finance_display_account );
}

void settings_routing_stats_t::init(settings_t const* const sets)
{
	INIT_INIT
	INIT_BOOL( "separate_halt_capacities", sets->is_separate_halt_capacities() );
	INIT_BOOL( "avoid_overcrowding", sets->is_avoid_overcrowding() );
	INIT_BOOL( "no_routing_over_overcrowded", sets->is_no_routing_over_overcrowding() );
	INIT_NUM( "station_coverage", sets->get_station_coverage(), 1, 8, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "allow_merge_distant_halt", sets->get_allow_merge_distant_halt(), 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	SEPERATOR
	INIT_NUM( "max_route_steps", sets->get_max_route_steps(), 1, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	INIT_NUM( "max_choose_route_steps", sets->get_max_choose_route_steps(), 1, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
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
	SEPERATOR
	INIT_BOOL( "stop_halt_as_scheduled", sets->get_stop_halt_as_scheduled() );

	INIT_END
}

void settings_routing_stats_t::read(settings_t* const sets)
{
	READ_INIT
	// routing of goods
	READ_BOOL_VALUE( sets->separate_halt_capacities );
	READ_BOOL_VALUE( sets->avoid_overcrowding );
	READ_BOOL_VALUE( sets->no_routing_over_overcrowding );
	READ_NUM_VALUE( sets->station_coverage_size );
	READ_NUM_VALUE( sets->allow_merge_distant_halt );
	READ_NUM_VALUE( sets->max_route_steps );
	READ_NUM_VALUE( sets->max_choose_route_steps );
	READ_NUM_VALUE( sets->max_hops );
	READ_NUM_VALUE( sets->max_transfers );
	// routing on ways
	READ_NUM_VALUE( sets->way_count_straight );
	READ_NUM_VALUE( sets->way_count_curve );
	READ_NUM_VALUE( sets->way_count_double_curve );
	READ_NUM_VALUE( sets->way_count_90_curve );
	READ_NUM_VALUE( sets->way_count_slope );
	READ_NUM_VALUE( sets->way_count_tunnel );
	READ_NUM_VALUE( sets->way_max_bridge_len );
	READ_NUM_VALUE( sets->way_count_leaving_road );

	READ_BOOL_VALUE( sets->stop_halt_as_scheduled );
}


void settings_economy_stats_t::init(settings_t const* const sets)
{
	INIT_INIT
	INIT_NUM( "remove_dummy_player_months", sets->get_remove_dummy_player_months(), 0, MAX_PLAYER_HISTORY_YEARS*12, 12, false );
	INIT_NUM( "unprotect_abandoned_player_months", sets->get_unprotect_abandoned_player_months(), 0, MAX_PLAYER_HISTORY_YEARS*12, 12, false );
	INIT_NUM( "ai_construction_speed", sets->get_default_ai_construction_speed(), 0, 1000000000, 1000, false );
	SEPERATOR

	INIT_COST( "starting_money", sets->get_starting_money(sets->get_starting_year()), 1, 0x7FFFFFFFul, 10000, false );
	INIT_NUM( "pay_for_total_distance", sets->get_pay_for_total_distance_mode(), 0, 2, gui_numberinput_t::AUTOLINEAR, true );
	INIT_NUM( "bonus_basefactor", sets->get_bonus_basefactor(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL_NEW( "first_beginner", sets->get_beginner_mode() );
	INIT_NUM( "beginner_price_factor", sets->get_beginner_price_factor(), 1, 25000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR

	INIT_BOOL( "allow_buying_obsolete_vehicles", sets->get_allow_buying_obsolete_vehicles() );
	INIT_NUM( "used_vehicle_reduction", sets->get_used_vehicle_reduction(), 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_rail_convoi_length", sets->get_max_rail_convoi_length(), 1, 254, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_road_convoi_length", sets->get_max_road_convoi_length(), 1, 254, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_ship_convoi_length", sets->get_max_ship_convoi_length(), 1, 254, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_air_convoi_length", sets->get_max_air_convoi_length(), 1, 254, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR

	INIT_NUM( "toll_runningcost_percentage", sets->get_way_toll_runningcost_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "toll_waycost_percentage", sets->get_way_toll_waycost_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL("disable_make_way_public", sets->get_disable_make_way_public());
	SEPERATOR

	INIT_NUM( "just_in_time", sets->get_just_in_time(), 0, 2, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "maximum_intransit_percentage", sets->get_factory_maximum_intransit_percentage(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "crossconnect_factories", sets->is_crossconnect_factories() );
	INIT_NUM( "crossconnect_factories_percentage", sets->get_crossconnect_factor(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "industry_increase_every", sets->get_industry_increase_every(), 0, 100000, 100, false );
	INIT_NUM( "min_factory_spacing", sets->get_min_factory_spacing(), 1, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_factory_spacing_percent", sets->get_max_factory_spacing_percent(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_factory_spacing", sets->get_max_factory_spacing(), 1, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "electric_promille", sets->get_electric_promille(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "allow_underground_transformers", sets->get_allow_underground_transformers() );
	INIT_NUM( "way_height_clearance", sets->get_way_height_clearance(), 1, 2, gui_numberinput_t::AUTOLINEAR, true );
	SEPERATOR

	INIT_NUM( "passenger_factor",  sets->get_passenger_factor(), 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "minimum_city_distance", sets->get_minimum_city_distance(), 1, 20000, 10, false );
	INIT_NUM( "special_building_distance", sets->get_special_building_distance(), 1, 150, 1, false );
	INIT_NUM( "factory_worker_radius", sets->get_factory_worker_radius(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "factory_worker_minimum_towns", sets->get_factory_worker_minimum_towns(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "factory_worker_maximum_towns", sets->get_factory_worker_maximum_towns(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "factory_arrival_periods", sets->get_factory_arrival_periods(), 1, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "factory_enforce_demand", sets->get_factory_enforce_demand() );
	INIT_NUM( "factory_worker_percentage", sets->get_factory_worker_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "tourist_percentage", sets->get_tourist_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "locality_factor[0].year", sets->locality_factor_per_year[0].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[0].factor", sets->locality_factor_per_year[0].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[1].year", sets->locality_factor_per_year[1].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[1].factor", sets->locality_factor_per_year[1].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[2].year", sets->locality_factor_per_year[2].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[2].factor", sets->locality_factor_per_year[2].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[3].year", sets->locality_factor_per_year[3].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[3].factor", sets->locality_factor_per_year[3].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[4].year", sets->locality_factor_per_year[4].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[4].factor", sets->locality_factor_per_year[4].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[5].year", sets->locality_factor_per_year[5].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[5].factor", sets->locality_factor_per_year[5].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[6].year", sets->locality_factor_per_year[6].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[6].factor", sets->locality_factor_per_year[6].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[7].year", sets->locality_factor_per_year[7].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[7].factor", sets->locality_factor_per_year[7].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[8].year", sets->locality_factor_per_year[8].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[8].factor", sets->locality_factor_per_year[8].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
	INIT_NUM( "locality_factor[9].year", sets->locality_factor_per_year[9].year, 0, 2999, 10, false );
	INIT_NUM( "locality_factor[9].factor", sets->locality_factor_per_year[9].factor, 1, 0x7FFFFFFFu, gui_numberinput_t::POWER2, false );
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
	INIT_NUM( "citycar_level", sets->get_traffic_level(), 0, 16, 1, false );
	INIT_NUM( "default_citycar_life", sets->get_stadtauto_duration(), 1, 1200, 12, false );

	INIT_END
}

void settings_economy_stats_t::read(settings_t* const sets)
{
	READ_INIT
	sint64 start_money_temp;
	READ_NUM_VALUE( sets->remove_dummy_player_months );
	READ_NUM_VALUE( sets->unprotect_abandoned_player_months );
	READ_NUM_VALUE( sets->default_ai_construction_speed );
	env_t::default_ai_construction_speed = sets->get_default_ai_construction_speed();
	READ_COST_VALUE( start_money_temp );
	if(  sets->get_starting_money(sets->get_starting_year())!=start_money_temp  ) {
		// because this will render the table based values invalid, we do this only when needed
		sets->starting_money = start_money_temp;
	}
	READ_NUM_VALUE( sets->pay_for_total_distance );
	READ_NUM_VALUE( sets->bonus_basefactor );
	READ_BOOL_VALUE_NEW( sets->beginner_mode );
	READ_NUM_VALUE( sets->beginner_price_factor );

	READ_BOOL_VALUE( sets->allow_buying_obsolete_vehicles );
	READ_NUM_VALUE( sets->used_vehicle_reduction );
	READ_NUM_VALUE( sets->max_rail_convoi_length );
	READ_NUM_VALUE( sets->max_road_convoi_length );
	READ_NUM_VALUE( sets->max_ship_convoi_length );
	READ_NUM_VALUE( sets->max_air_convoi_length );

	READ_NUM_VALUE( sets->way_toll_runningcost_percentage );
	READ_NUM_VALUE( sets->way_toll_waycost_percentage );
	READ_BOOL_VALUE(sets->disable_make_way_public);

	READ_NUM_VALUE( env_t::just_in_time );
	sets->just_in_time = env_t::just_in_time;
	READ_NUM_VALUE( sets->factory_maximum_intransit_percentage );
	READ_BOOL_VALUE( sets->crossconnect_factories );
	READ_NUM_VALUE( sets->crossconnect_factor );
	READ_NUM_VALUE( sets->industry_increase );
	READ_NUM_VALUE( sets->min_factory_spacing );
	READ_NUM_VALUE( sets->max_factory_spacing_percentage );
	READ_NUM_VALUE( sets->max_factory_spacing );
	READ_NUM_VALUE( sets->electric_promille );
	READ_BOOL_VALUE( sets->allow_underground_transformers );
	READ_NUM_VALUE( sets->way_height_clearance );

	READ_NUM_VALUE( sets->passenger_factor );
	READ_NUM_VALUE( sets->minimum_city_distance );
	READ_NUM_VALUE( sets->special_building_distance );
	READ_NUM_VALUE( sets->factory_worker_radius );
	READ_NUM_VALUE( sets->factory_worker_minimum_towns );
	READ_NUM_VALUE( sets->factory_worker_maximum_towns );
	READ_NUM_VALUE( sets->factory_arrival_periods );
	READ_BOOL_VALUE( sets->factory_enforce_demand );
	READ_NUM_VALUE( sets->factory_worker_percentage );
	READ_NUM_VALUE( sets->tourist_percentage );
	for(  int i=0;  i<10;  i++  ) {
		READ_NUM_VALUE( sets->locality_factor_per_year[i].year );
		READ_NUM_VALUE( sets->locality_factor_per_year[i].factor );
	}
	READ_NUM_VALUE( sets->passenger_multiplier );
	READ_NUM_VALUE( sets->mail_multiplier );
	READ_NUM_VALUE( sets->goods_multiplier );
//	READ_NUM_VALUE( sets->set_electricity_multiplier );
	READ_NUM_VALUE( sets->growthfactor_small );
	READ_NUM_VALUE( sets->growthfactor_medium );
	READ_NUM_VALUE( sets->growthfactor_large );
	READ_BOOL( sets->set_random_pedestrians );
	READ_BOOL( sets->set_show_pax );
	READ_NUM( sets->set_traffic_level );
	READ_NUM_VALUE( sets->stadtauto_duration );
}


void settings_costs_stats_t::init(settings_t const* const sets)
{
	INIT_INIT
	INIT_NUM( "maintenance_building", sets->maint_building, 1, 100000000, 100, false );
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
	INIT_COST( "cost_alter_climate", -sets->cst_alter_climate, 1, 100000000, 10, false );
	INIT_COST( "cost_found_city", -sets->cst_found_city, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_found_industry", -sets->cst_multiply_found_industry, 1, 100000000, 10, false );
	INIT_COST( "cost_remove_tree", -sets->cst_remove_tree, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_remove_haus", -sets->cst_multiply_remove_haus, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_remove_field", -sets->cst_multiply_remove_field, 1, 100000000, 10, false );
	INIT_COST( "cost_transformer", -sets->cst_transformer, 1, 100000000, 10, false );
	INIT_COST( "cost_maintain_transformer", -sets->cst_maintain_transformer, 1, 100000000, 10, false );
	INIT_NUM("cost_make_public_months", sets->cst_make_public_months, 0, 36000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_END
}


void settings_costs_stats_t::read(settings_t* const sets)
{
	READ_INIT
	(void)booliter;
	READ_NUM_VALUE( sets->maint_building );
	READ_COST_VALUE( sets->cst_multiply_dock )*(-1);
	READ_COST_VALUE( sets->cst_multiply_station )*(-1);
	READ_COST_VALUE( sets->cst_multiply_roadstop )*(-1);
	READ_COST_VALUE( sets->cst_multiply_airterminal )*(-1);
	READ_COST_VALUE( sets->cst_multiply_post )*(-1);
	READ_COST_VALUE( sets->cst_multiply_headquarter )*(-1);
	READ_COST_VALUE( sets->cst_depot_air )*(-1);
	READ_COST_VALUE( sets->cst_depot_rail )*(-1);
	READ_COST_VALUE( sets->cst_depot_road )*(-1);
	READ_COST_VALUE( sets->cst_depot_ship )*(-1);
	READ_COST_VALUE( sets->cst_buy_land )*(-1);
	READ_COST_VALUE( sets->cst_alter_land )*(-1);
	READ_COST_VALUE( sets->cst_set_slope )*(-1);
	READ_COST_VALUE( sets->cst_alter_climate )*(-1);
	READ_COST_VALUE( sets->cst_found_city )*(-1);
	READ_COST_VALUE( sets->cst_multiply_found_industry )*(-1);
	READ_COST_VALUE( sets->cst_remove_tree )*(-1);
	READ_COST_VALUE( sets->cst_multiply_remove_haus )*(-1);
	READ_COST_VALUE( sets->cst_multiply_remove_field )*(-1);
	READ_COST_VALUE( sets->cst_transformer )*(-1);
	READ_COST_VALUE( sets->cst_maintain_transformer )*(-1);
	READ_NUM_VALUE(sets->cst_make_public_months);
}


#include "../descriptor/ground_desc.h"

static char const * const climate_generate_string[] =
{
	"height based",
	"temperature-humidity based"
};

static char const * const tree_generate_string[] =
{
	"none",
	"random",
	"rainfall"
};

void settings_climates_stats_t::init(settings_t* const sets)
{
	int mountain_height_start = (int)sets->get_max_mountain_height();
	int mountain_roughness_start = (int)(sets->get_map_roughness()*20.0 + 0.5)-8;

	local_sets = sets;
	INIT_INIT
	INIT_NUM_NEW( "height_map_conversion_version", env_t::pak_height_conversion_factor, 1, 2, 0, false );
	SEPERATOR
	INIT_NUM_NEW( "Water level", sets->get_groundwater(), -20*(ground_desc_t::double_grounds?2:1), 20, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "Mountain height", mountain_height_start, 0, min(1000,100*(11-mountain_roughness_start)), 10, false );
	INIT_NUM_NEW( "Map roughness", mountain_roughness_start, 0, min(10, 11-((mountain_height_start+99)/100)), gui_numberinput_t::AUTOLINEAR, false );

	SEPERATOR
	INIT_NUM_NEW( "Wind direction", sets->wind_direction, 0, 3, 1, true );
	// combobox for climate generator
	climate_generate.clear_elements();
	for(  uint32 i=0;  i<lengthof(climate_generate_string);  i++  ) {
		climate_generate.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( climate_generate_string[i], SYSCOL_TEXT ) ;
	}
	climate_generate.set_selection( sets->get_climate_generator() );
	climate_generate.set_focusable( false );
	add_component( &climate_generate );
	INIT_LB( "climate generator" );

	add_table(3,0);
	{
		// other climate borders ...
		for(  int i=desert_climate;  i<=arctic_climate;  i++  ) {
			INIT_NUM( ground_desc_t::get_climate_name_from_bit((climate)i), sets->get_climate_borders(i,0), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
			gui_numberinput_t *ni = new_component<gui_numberinput_t>();
			ni->init( sets->get_climate_borders(i,1), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
			numinp.append( ni );
		}
	}
	end_table();
	new_component<gui_empty_t>();

	INIT_NUM_NEW( "climate area percentage", sets->get_patch_size_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );

	SEPERATOR
	INIT_NUM_NEW( "lake_height", sets->get_lakeheight(), 0, 127, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "Number of rivers", sets->get_river_number(), 0, 1024, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "minimum length of rivers", sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "maximum length of rivers", sets->get_max_river_length(), sets->get_min_river_length()+16, 8196, gui_numberinput_t::AUTOLINEAR, false );
	// add listener to all of them
	for(gui_numberinput_t* const n : numinp) {
		n->add_listener(this);
	}
	// the following are independent and thus need no listener
	SEPERATOR
	// combobox for trees generator
	tree_generate.clear_elements();
	for(  uint32 i=0;  i<lengthof(tree_generate_string);  i++  ) {
		tree_generate.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( tree_generate_string[i], SYSCOL_TEXT ) ;
	}
	tree_generate.set_selection( sets->get_tree_distribution() );
	tree_generate.set_focusable( false );
	add_component( &tree_generate, 2);
	INIT_NUM_NEW( "forest_base_size", sets->get_forest_base_size(), 10, 255, 1, false );
	INIT_NUM_NEW( "forest_map_size_divisor", sets->get_forest_map_size_divisor(), 2, 255, 1, false );
	INIT_NUM_NEW( "forest_count_divisor", sets->get_forest_count_divisor(), 2, 255, 1, false );
	INIT_NUM_NEW( "forest_inverse_spare_tree_density", sets->get_forest_inverse_spare_tree_density(), 33, 10000, 10, false );
	INIT_NUM( "max_no_of_trees_on_square", sets->get_max_no_of_trees_on_square(), 1, 5, 1, true );
	INIT_NUM_NEW( "tree_climates", sets->get_tree_climates(), 0, 255, 1, false );
	INIT_NUM_NEW( "no_tree_climates", sets->get_no_tree_climates(), 0, 255, 1, false );

	INIT_END
}


void settings_climates_stats_t::read(settings_t* const sets)
{
	sets->climate_generator = (settings_t::climate_generate_t)max( 0, climate_generate.get_selection() );
	sets->tree_distribution = ::clamp(tree_generate.get_selection(), (int)settings_t::TREE_DIST_NONE, (int)settings_t::TREE_DIST_COUNT-1 );
	READ_INIT
	READ_NUM_VALUE_NEW( env_t::pak_height_conversion_factor );
	READ_NUM_VALUE_NEW( sets->groundwater );
	READ_NUM_VALUE_NEW( sets->max_mountain_height );
	READ_NUM_VALUE_NEW( sets->wind_direction );
	double n = 0;
	READ_NUM_VALUE_NEW( n );
	if(  new_world  ) {
		sets->map_roughness = (n+8.0)/20.0;
	}
	// other climate borders ...
	for(  int i=desert_climate;  i<=arctic_climate;  i++  ) {
		sint16 ch;
		READ_NUM_VALUE( ch );
		sets->climate_borders[i][0] = ch;
		READ_NUM_VALUE( ch );
		sets->climate_borders[i][1] = ch;
	}
	READ_NUM_VALUE_NEW( sets->patch_size_percentage );
	READ_NUM_VALUE_NEW( sets->lake_height );
	READ_NUM_VALUE_NEW( sets->river_number );
	READ_NUM_VALUE_NEW( sets->min_river_length );
	READ_NUM_VALUE_NEW( sets->max_river_length );
	READ_NUM_VALUE_NEW( sets->forest_base_size );
	READ_NUM_VALUE_NEW( sets->forest_map_size_divisor );
	READ_NUM_VALUE_NEW( sets->forest_count_divisor );
	READ_NUM_VALUE_NEW( sets->forest_inverse_spare_tree_density );
	READ_NUM_VALUE( sets->max_no_of_trees_on_square );
	READ_NUM_VALUE_NEW( sets->tree_climates );
	READ_NUM_VALUE_NEW( sets->no_tree_climates );

	(void)booliter; // silence warning
}


bool settings_climates_stats_t::action_triggered(gui_action_creator_t *comp, value_t)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	read( local_sets );
	uint i = 0;
	FORX(slist_tpl<gui_numberinput_t*>, const n, numinp, ++i) {
		if (n == comp && i < 3 && welt_gui) {
			// update world preview
			welt_gui->update_preview();
		}
	}
	return true;
}
