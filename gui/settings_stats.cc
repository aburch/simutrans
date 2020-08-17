/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "welt.h"
#include "../gui/simwin.h"
#include "../simversion.h"
#include "../dataobj/settings.h"
#include "../dataobj/environment.h"
#include "../dataobj/translator.h"
#include "../player/finance.h" // MAX_PLAYER_HISTORY_YEARS
#include "../vehicle/simvehicle.h"
#include "../path_explorer.h"
#include "settings_stats.h"

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
	"0.102.3",
	"0.102.5",
	"0.110.0",
	"0.110.1",
	"0.110.5",
	"0.110.6",
	"0.110.7",
	"0.111.0",
	"0.111.1",
	"0.111.2",
	"0.111.3",
	"0.111.4",
	"0.112.0",
	"0.112.1",
	"0.112.2",
	"0.112.3",
	"0.112.5",
	"0.112.6",
	"0.112.7",
	"0.120.0",
	"0.120.1",
	"0.120.2",
	"0.120.3",
	"0.120.4",
	"0.120.5",
	"0.120.7",
	"0.120.1.2",
	"0.120.2.1"
};

static const char *version_ex[] =
{
	"", /*Ex version 0 has no Ex string at all*/
	".1",
	".2",
	".3",
	".4",
	".5",
	".6",
	".7",
	".8",
	".9",
	".10",
	".11",
	".12",
	".13",
	".14",
	".15",
	".16",
	".17"
};

static const char *revision_ex[] =
{
	"0", /*Ex version 0 has no Ex string at all*/
	"1",
	"2",
	"3",
	"4",
	"5",
	"6",
	"7",
	"8",
	"9",
	"10",
	"11",
	"12",
	"13",
	"14",
	"15",
	"16",
	"17",
	"18",
	"19",
	"20",
	"21",
	"22",
	"23",
	"24",
	"25",
	"26",
	"27",
	"28",
	"29",
	"30"
};

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
	while(  !table.empty()  ) {
		delete table.remove_first();
	}
	others.set_count(0);
}


gui_component_table_t& settings_stats_t::new_table(const scr_coord& pos, coordinate_t columns, coordinate_t rows)
{
	gui_component_table_t& tbl = * new gui_component_table_t();
	tbl.set_default_cell_size(scr_size(12,12));
	tbl.set_owns_cell_components(false);
	tbl.set_grid_width(koord(4,0));
	tbl.set_grid_visible(false);
	tbl.set_grid_size(coordinates_t(columns, rows));
	tbl.set_pos(pos);
	table.append(&tbl);
	add_component(&tbl);
	return tbl;
}


gui_label_t& settings_stats_t::new_label(const scr_coord& pos, const char *text)
{
	gui_label_t& lb = * new gui_label_t();
	lb.set_text_pointer(text);
	lb.set_pos(pos);
	lb.set_size(scr_size(proportional_string_width(text), 10));
	label.append(&lb);
	return lb;
}


gui_textarea_t& settings_stats_t::new_textarea(const scr_coord& pos, const char* text)
{
	gui_textarea_t& ta = * new gui_textarea_t(text);
	ta.set_pos(pos);
	others.append(&ta);
	return ta;
}


gui_numberinput_t& settings_stats_t::new_numinp(const scr_coord& pos, sint32 value, sint32 min_value, sint32 max_value, sint32 mode, bool wrap)
{
	gui_numberinput_t& ni = * new gui_numberinput_t();
	ni.init(value, min_value, max_value, mode, wrap);
	ni.set_pos(pos);
	ni.set_size(scr_size(37+proportional_string_width("0")*max(1,(sint16)(log10((double)(max_value)+1.0)+0.5)), D_BUTTON_HEIGHT ));
	numinp.append(&ni);
	return ni;
}


button_t& settings_stats_t::new_button(const scr_coord& pos, const char *text, bool pressed)
{
	button_t& bt = * new button_t();
	bt.init(button_t::square_automatic, text, pos);
	bt.set_size(scr_size(16 + proportional_string_width(text), D_BUTTON_HEIGHT));
	bt.pressed = pressed;
	return bt;
}


void settings_stats_t::set_cell_component(gui_component_table_t &tbl, gui_component_t &c, coordinate_t x, coordinate_t y)
{
	tbl.set_cell_component(x, y, &c);
	tbl.set_column_width(x, max(tbl.get_column_width(x), c.get_pos().x + c.get_size().w));
	tbl.set_row_height(y, max(tbl.get_row_height(y), c.get_pos().y + c.get_size().h));
}


#define INIT_TABLE_END(tbl) \
	ypos += (tbl).get_table_height();\
	width = max(width, (tbl).get_pos().x * 2 + (tbl).get_table_width());\
	tbl.set_size(tbl.get_table_size());


void settings_extended_general_stats_t::init( settings_t *sets )
{
	INIT_INIT;
	{
		gui_component_table_t &tbl = new_table(scr_coord(0, ypos), 3, 2);
		int row = 0;
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("revenue of")), 0, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("above\nminutes")), 1, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("get\nrevenue $")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 3), "travelling post office"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_tpo_min_minutes(), 0, 14400, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_tpo_revenue(), 0, 10000, 1), 2, row);
		INIT_TABLE_END(tbl);
	}
	SEPERATOR;

#ifdef MULTI_THREAD
	world()->await_private_car_threads();
#endif

	INIT_NUM( "city_threshold_size", sets->get_city_threshold_size(), 1000, 100000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "capital_threshold_size", sets->get_capital_threshold_size(), 10000, 1000000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "city_threshold_percentage", sets->get_city_threshold_percentage(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM( "capital_threshold_percentage", sets->get_capital_threshold_percentage(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM( "max_small_city_size", sets->get_max_small_city_size(), 1000, 100000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_city_size", sets->get_max_city_size(), 10000, 1000000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "congestion_density_factor", sets->get_congestion_density_factor(), 0, 1024, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "quick_city_growth", sets->get_quick_city_growth());
	INIT_BOOL( "assume_everywhere_connected_by_road", sets->get_assume_everywhere_connected_by_road());
	INIT_NUM( "max_route_tiles_to_process_in_a_step", sets->get_max_route_tiles_to_process_in_a_step(), 0, 65535, gui_numberinput_t::AUTOLINEAR, false);
	INIT_BOOL("toll_free_public_roads", sets->get_toll_free_public_roads());
	INIT_NUM( "spacing_shift_mode", sets->get_spacing_shift_mode(), 0, 2 , gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "spacing_shift_divisor", sets->get_spacing_shift_divisor(), 1, 32767 , gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "allow_routing_on_foot", sets->get_allow_routing_on_foot());
	INIT_BOOL("allow_airports_without_control_towers", sets->get_allow_airports_without_control_towers());
	INIT_NUM("global_power_factor_percent", sets->get_global_power_factor_percent(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("global_force_factor_percent", sets->get_global_force_factor_percent(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("enforce_weight_limits", sets->get_enforce_weight_limits(), 0, 3, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("max_diversion_tiles", sets->get_max_diversion_tiles(), 0, 65535, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("way_degradation_fraction", sets->get_way_degradation_fraction(), 0, 40, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("sighting_distance_meters", sets->get_sighting_distance_meters(), 0, 7500, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("tilting_min_radius_effect", sets->get_tilting_min_radius_effect(), 0, 10000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("assumed_curve_radius_45_degrees", sets->get_assumed_curve_radius_45_degrees(), 0, 10000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("max_speed_drive_by_sight_kmh", sets->get_max_speed_drive_by_sight_kmh(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("time_interval_seconds_to_clear", sets->get_time_interval_seconds_to_clear(), 0, 10000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("time_interval_seconds_to_caution", sets->get_time_interval_seconds_to_caution(), 0, 10000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("town_road_speed_limit", sets->get_town_road_speed_limit(), 0, 500, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("minimum_staffing_percentage_consumer_industry", sets->get_minimum_staffing_percentage_consumer_industry(), 0, 100, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("minimum_staffing_percentage_full_production_producer_industry", sets->get_minimum_staffing_percentage_full_production_producer_industry(), 0, 100, gui_numberinput_t::AUTOLINEAR, false);

	SEPERATOR;
	INIT_NUM("population_per_level", sets->get_population_per_level(), gui_numberinput_t::PLAIN, 1000, 1, false);
	INIT_NUM("visitor_demand_per_level", sets->get_visitor_demand_per_level(), 1, 1000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("jobs_per_level", sets->get_jobs_per_level(), 1, 1000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("mail_per_level", sets->get_mail_per_level(), 1, 1000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("power_revenue_factor_percentage", sets->get_power_revenue_factor_percentage(), 0, 10000, gui_numberinput_t::PLAIN, false);
	SEPERATOR;
	INIT_NUM("forge_cost_road", sets->get_forge_cost_road(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_track", sets->get_forge_cost_track(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_water", sets->get_forge_cost_water(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_monorail", sets->get_forge_cost_monorail(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_maglev", sets->get_forge_cost_maglev(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_tram", sets->get_forge_cost_tram(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_narrowgauge", sets->get_forge_cost_narrowgauge(), 0, 1000000, gui_numberinput_t::PLAIN, false);
	INIT_NUM("forge_cost_air", sets->get_forge_cost_air(), 0, 1000000, gui_numberinput_t::PLAIN, false);

	SEPERATOR;
	INIT_BOOL("rural_industries_no_staff_shortage", sets->rural_industries_no_staff_shortage);
	INIT_NUM("auto_connect_industries_and_attractions_by_road", sets->auto_connect_industries_and_attractions_by_road, 0, 65535, gui_numberinput_t::PLAIN, false);

	SEPERATOR;
	INIT_NUM("parallel_ways_forge_cost_percentage_road", sets->get_parallel_ways_forge_cost_percentage_road(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_track", sets->get_parallel_ways_forge_cost_percentage_track(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_water", sets->get_parallel_ways_forge_cost_percentage_water(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_monorail", sets->get_parallel_ways_forge_cost_percentage_monorail(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_maglev", sets->get_parallel_ways_forge_cost_percentage_maglev(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_tram", sets->get_parallel_ways_forge_cost_percentage_tram(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_narrowgauge", sets->get_parallel_ways_forge_cost_percentage_narrowgauge(), 0, 100, gui_numberinput_t::PLAIN, false);
	INIT_NUM("parallel_ways_forge_cost_percentage_air", sets->get_parallel_ways_forge_cost_percentage_air(), 0, 100, gui_numberinput_t::PLAIN, false);

	SEPERATOR;
	{
		gui_component_table_t &tbl = new_table(scr_coord(0, ypos), 2, 9);
		int row = 8;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(overheadlines_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_other"), 1, row);
		row = 0;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(road_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_road"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(track_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_rail"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(water_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_water"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(monorail_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_monorail"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(maglev_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_maglev"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(tram_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_tram"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(narrowgauge_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_narrowgauge"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_default_increase_maintenance_after_years(air_wt), 0, 1000, 1), 0, row);
		set_cell_component(tbl, new_label(scr_coord(2, 0), "default_increase_maintenance_after_years_air"), 1, row);
		INIT_TABLE_END(tbl);
	}

	SEPERATOR;

	INIT_NUM("path_explorer_time_midpoint", sets->get_path_explorer_time_midpoint(), 1, 2048, gui_numberinput_t::PLAIN, false);
	INIT_BOOL("save_path_explorer_data", sets->get_save_path_explorer_data());

	SEPERATOR;

	INIT_BOOL("show_future_vehicle_information", sets->get_show_future_vehicle_info());

	SEPERATOR;

	INIT_NUM("industry_density_proportion_override", sets->get_industry_density_proportion_override(), 0, 65535, gui_numberinput_t::AUTOLINEAR, false);

	clear_dirty();
	height = ypos;
	set_size(settings_stats_t::get_size());
}


void settings_extended_general_stats_t::read(settings_t *sets)
{
#ifdef MULTI_THREAD
	world()->await_private_car_threads();
#endif
	READ_INIT;

	READ_NUM( sets->set_tpo_min_minutes );
	READ_NUM( sets->set_tpo_revenue );

	READ_NUM( sets->set_city_threshold_size );
	READ_NUM( sets->set_capital_threshold_size );
	READ_NUM( sets->set_city_threshold_percentage );
	READ_NUM(sets->set_capital_threshold_percentage);
	READ_NUM( sets->set_max_small_city_size );
	READ_NUM( sets->set_max_city_size );
	READ_NUM( sets->set_congestion_density_factor );
	READ_BOOL( sets->set_quick_city_growth );
	READ_BOOL( sets->set_assume_everywhere_connected_by_road );
	READ_NUM( sets->set_max_route_tiles_to_process_in_a_step );
	READ_BOOL_VALUE(sets->toll_free_public_roads);
	READ_NUM( sets->set_spacing_shift_mode );
	READ_NUM( sets->set_spacing_shift_divisor);
	READ_BOOL( sets->set_allow_routing_on_foot);
	READ_BOOL( sets->set_allow_airports_without_control_towers );
	READ_NUM( sets->set_global_power_factor_percent );
	READ_NUM( sets->set_global_force_factor_percent );
	READ_NUM( sets->set_enforce_weight_limits );
	READ_NUM_VALUE( sets->max_diversion_tiles );
	READ_NUM_VALUE( sets->way_degradation_fraction );
	READ_NUM_VALUE( sets->sighting_distance_meters );
	sets->sighting_distance_tiles = sets->sighting_distance_meters / sets->meters_per_tile;
	READ_NUM_VALUE( sets->tilting_min_radius_effect );
	READ_NUM_VALUE( sets->assumed_curve_radius_45_degrees );
	READ_NUM_VALUE( sets->max_speed_drive_by_sight_kmh );
	sets->max_speed_drive_by_sight = kmh_to_speed(sets->max_speed_drive_by_sight_kmh);
	READ_NUM_VALUE( sets->time_interval_seconds_to_clear );
	READ_NUM_VALUE( sets->time_interval_seconds_to_caution );
	READ_NUM_VALUE( sets->town_road_speed_limit );
	READ_NUM_VALUE(sets->minimum_staffing_percentage_consumer_industry);
	READ_NUM_VALUE(sets->minimum_staffing_percentage_full_production_producer_industry);

	READ_NUM_VALUE(sets->population_per_level);
	READ_NUM_VALUE(sets->visitor_demand_per_level);
	READ_NUM_VALUE(sets->jobs_per_level);
	READ_NUM_VALUE(sets->mail_per_level);
	READ_NUM_VALUE(sets->power_revenue_factor_percentage);

	READ_NUM_VALUE(sets->forge_cost_road);
	READ_NUM_VALUE(sets->forge_cost_track);
	READ_NUM_VALUE(sets->forge_cost_water);
	READ_NUM_VALUE(sets->forge_cost_monorail);
	READ_NUM_VALUE(sets->forge_cost_maglev);
	READ_NUM_VALUE(sets->forge_cost_tram);
	READ_NUM_VALUE(sets->forge_cost_narrowgauge);
	READ_NUM_VALUE(sets->forge_cost_air);

	READ_BOOL_VALUE(sets->rural_industries_no_staff_shortage);
	READ_NUM_VALUE(sets->auto_connect_industries_and_attractions_by_road);

	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_road);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_track);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_water);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_monorail);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_maglev);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_tram);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_narrowgauge);
	READ_NUM_VALUE(sets->parallel_ways_forge_cost_percentage_air);

	uint16 default_increase_maintenance_after_years_other;
	READ_NUM_VALUE( default_increase_maintenance_after_years_other );
	for(uint8 i = road_wt; i <= air_wt; i ++)
	{
		switch(i)
		{
		case road_wt:
		case track_wt:
		case water_wt:
		case monorail_wt:
		case maglev_wt:
		case tram_wt:
		case narrowgauge_wt:
		case air_wt:
			sets->set_default_increase_maintenance_after_years((waytype_t)i, (*numiter++)->get_value());
			break;
		default:
			sets->set_default_increase_maintenance_after_years((waytype_t)i, default_increase_maintenance_after_years_other);
		}
	}

	READ_NUM_VALUE(sets->path_explorer_time_midpoint);
	READ_BOOL_VALUE(sets->save_path_explorer_data);

	READ_BOOL_VALUE(sets->show_future_vehicle_info);

	READ_NUM_VALUE(sets->industry_density_proportion_override);

	path_explorer_t::set_absolute_limits_external();
}


void settings_extended_revenue_stats_t::init( settings_t *sets )
{
	INIT_INIT;
	INIT_NUM( "passenger_trips_per_month_hundredths", sets->get_passenger_trips_per_month_hundredths(), 0, 4096, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "mail_packets_per_month_hundredths", sets->get_mail_packets_per_month_hundredths(), 0, 4096, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "passenger_routing_packet_size", sets->get_passenger_routing_packet_size(), 1, 64, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("max_onward_trips", sets->get_max_onward_trips(), 0, 32, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("onward_trip_chance_percent", sets->get_onward_trip_chance_percent(), 0, 100, gui_numberinput_t::PROGRESS, false );
	INIT_NUM("commuting_trip_chance_percent", sets->get_commuting_trip_chance_percent(), 0, 100, gui_numberinput_t::PROGRESS, false );
	INIT_NUM( "max_alternative_destinations_visiting", sets->get_max_alternative_destinations_visiting(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_alternative_destinations_commuting", sets->get_max_alternative_destinations_commuting(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_alternative_destinations_per_visitor_demand_millionths", sets->get_max_alternative_destinations_per_visitor_demand_millionths(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_alternative_destinations_per_job_millionths", sets->get_max_alternative_destinations_per_job_millionths(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("min_alternative_destinations_visiting", sets->get_min_alternative_destinations_visiting(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("min_alternative_destinations_commuting", sets->get_min_alternative_destinations_commuting(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("min_alternative_destinations_per_visitor_demand_millionths", sets->get_min_alternative_destinations_per_visitor_demand_millionths(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("min_alternative_destinations_per_job_millionths", sets->get_min_alternative_destinations_per_job_millionths(), 0, 65534, gui_numberinput_t::AUTOLINEAR, false);
	INIT_NUM("passenger_max_wait", sets->get_passenger_max_wait(), 0, 311040, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("min_wait_airport", sets->get_min_wait_airport(), 0, 311040, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("random_mode_commuting", sets->get_random_mode_commuting(), 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("random_mode_visiting", sets->get_random_mode_visiting(), 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM("tolerance_modifier_percentage", sets->get_tolerance_modifier_percentage(), 0, 8192, gui_numberinput_t::AUTOLINEAR, false );
	{
		gui_component_table_t &tbl = new_table(scr_coord(0, ypos), 6, 4);
		int row = 0;
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("passenger\ndistribution")), 0, 0);
		// Note: "Waiting" here should be "journey time": these are translated in en.tab, but should be correct in other translations, too.
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("waiting\ntolerance\nmin. min")), 4, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("waiting\ntolerance\nmax. min")), 5, 0);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 3), "commuting"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_min_commuting_tolerance() / 10, 1, 5256000, 1), 4, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_range_commuting_tolerance() / 10, 1, 5256000, 1), 5, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "visiting"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_min_visiting_tolerance() / 10, 1, 5256000, 1), 4, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_range_visiting_tolerance() / 10, 1, 5256000, 1), 5, row);
		INIT_TABLE_END(tbl);
	}
	SEPERATOR;
	{
		gui_component_table_t &tbl = new_table(scr_coord(0, ypos), 3, 6);
		int row = 0;
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("comfort expectance\nfor travelling")), 0, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("duration\nin minutes")), 1, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("min comfort\nrating")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 3), "short time"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_tolerable_comfort_short_minutes(), 0, 120, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_tolerable_comfort_short(), 0, 255, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "median short time"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_median_short_minutes(), 0, 720, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_median_short(), 0, 255, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "median median time"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_median_median_minutes(), 0, 1440, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_median_median(), 0, 255, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "median long time"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_median_long_minutes(), 0, 1440*7, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_median_long(), 0, 255, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "long time"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_long_minutes(), 0, 1440*30, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_tolerable_comfort_long(), 0, 255, 1), 2, row);
		INIT_TABLE_END(tbl);
	}
	SEPERATOR;
	{
		gui_component_table_t &tbl = new_table(scr_coord(0, ypos), 3, 3);
		int row = 0;
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("comfort impact\nlimitations")), 0, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("differential")), 1, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("percent")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 3), "max luxury bonus"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_max_luxury_bonus_differential(), 0, 250, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_max_luxury_bonus_percent(), 0, 1000, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "max discomfort penalty"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_max_discomfort_penalty_differential(), 0, 250, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_max_discomfort_penalty_percent(), 0, 1000, 1), 2, row);
		INIT_TABLE_END(tbl);
	}
	SEPERATOR;
	{
		gui_component_table_t &tbl = new_table(scr_coord(0, ypos), 3, 8);
		int row = 0;
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("catering bonus\nfor travelling")), 0, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("duration\nin minutes")), 1, 0);
		set_cell_component(tbl, new_textarea(scr_coord(2, 0), translator::translate("max catering\nrevenue $")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 3), "min traveltime"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 3), sets->get_catering_min_minutes(), 0, 14400, 1), 1, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "catering level 1"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level1_minutes(), 0, 14400, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level1_max_revenue(), 0, 10000, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "catering level 2"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level2_minutes(), 0, 14400, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level2_max_revenue(), 0, 10000, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "catering level 3"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level3_minutes(), 0, 14400, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level3_max_revenue(), 0, 10000, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "catering level 4"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level4_minutes(), 0, 14400, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level4_max_revenue(), 0, 10000, 1), 2, row);
		row++;
		set_cell_component(tbl, new_label(scr_coord(2, 0), "catering level 5"), 0, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level5_minutes(), 0, 14400, 1), 1, row);
		set_cell_component(tbl, new_numinp(scr_coord(0, 0), sets->get_catering_level5_max_revenue(), 0, 10000, 1), 2, row);
		INIT_TABLE_END(tbl);
	}
	SEPERATOR;
	INIT_NUM("max_comfort_preference_percentage", sets->get_max_comfort_preference_percentage(), 100, 65535, gui_numberinput_t::AUTOLINEAR, false);

	clear_dirty();
	height = ypos;
	set_size(settings_stats_t::get_size());
}


void settings_extended_revenue_stats_t::read(settings_t *sets)
{
	READ_INIT
	(void)booliter;

	READ_NUM_VALUE( sets->passenger_trips_per_month_hundredths );
	READ_NUM_VALUE( sets->mail_packets_per_month_hundredths );
	READ_NUM_VALUE( sets->passenger_routing_packet_size );
	READ_NUM_VALUE( sets->max_onward_trips );
	READ_NUM_VALUE( sets->onward_trip_chance_percent );
	READ_NUM_VALUE( sets->commuting_trip_chance_percent );
	READ_NUM_VALUE( sets->max_alternative_destinations_visiting );
	READ_NUM_VALUE( sets->max_alternative_destinations_commuting );
	READ_NUM_VALUE( sets->max_alternative_destinations_per_visitor_demand_millionths );
	READ_NUM_VALUE( sets->max_alternative_destinations_per_job_millionths );
	READ_NUM_VALUE(sets->min_alternative_destinations_visiting);
	READ_NUM_VALUE(sets->min_alternative_destinations_commuting);
	READ_NUM_VALUE(sets->min_alternative_destinations_per_visitor_demand_millionths);
	READ_NUM_VALUE(sets->min_alternative_destinations_per_job_millionths);
	READ_NUM_VALUE( sets->passenger_max_wait );
	READ_NUM_VALUE( sets->min_wait_airport );
	READ_NUM_VALUE( sets->random_mode_commuting );
	READ_NUM_VALUE( sets->random_mode_visiting );
	READ_NUM_VALUE( sets->tolerance_modifier_percentage );

	READ_NUM_VALUE_TENTHS( (sets->min_commuting_tolerance) );
	READ_NUM_VALUE_TENTHS( sets->range_commuting_tolerance);
	READ_NUM_VALUE_TENTHS( sets->min_visiting_tolerance );
	READ_NUM_VALUE_TENTHS( sets->range_visiting_tolerance );
	READ_NUM_VALUE( sets->tolerable_comfort_short_minutes );
	READ_NUM_VALUE( sets->tolerable_comfort_short );
	READ_NUM_VALUE( sets->tolerable_comfort_median_short_minutes );
	READ_NUM_VALUE( sets->tolerable_comfort_median_short );
	READ_NUM_VALUE( sets->tolerable_comfort_median_median_minutes );
	READ_NUM_VALUE( sets->tolerable_comfort_median_median );
	READ_NUM_VALUE( sets->tolerable_comfort_median_long_minutes );
	READ_NUM_VALUE( sets->tolerable_comfort_median_long );
	READ_NUM_VALUE( sets->tolerable_comfort_long_minutes );
	READ_NUM_VALUE( sets->tolerable_comfort_long );

	READ_NUM_VALUE( sets->max_luxury_bonus_differential );
	READ_NUM_VALUE( sets->max_luxury_bonus_percent );
	READ_NUM_VALUE( sets->max_discomfort_penalty_differential );
	READ_NUM_VALUE( sets->max_discomfort_penalty_percent );

	READ_NUM_VALUE( sets->catering_min_minutes );
	READ_NUM_VALUE( sets->catering_level1_minutes );
	READ_NUM_VALUE( sets->catering_level1_max_revenue );
	READ_NUM_VALUE( sets->catering_level2_minutes );
	READ_NUM_VALUE( sets->catering_level2_max_revenue );
	READ_NUM_VALUE( sets->catering_level3_minutes );
	READ_NUM_VALUE( sets->catering_level3_max_revenue );
	READ_NUM_VALUE( sets->catering_level4_minutes );
	READ_NUM_VALUE( sets->catering_level4_max_revenue );
	READ_NUM_VALUE( sets->catering_level5_minutes );
	READ_NUM_VALUE( sets->catering_level5_max_revenue );

	READ_NUM_VALUE(sets->max_comfort_preference_percentage);

	// And convert to the form used in-game...
	sets->cache_catering_revenues();
	sets->cache_comfort_tables();
}

bool settings_general_stats_t::action_triggered(gui_action_creator_t *comp, value_t v)
{
	assert( comp==&savegame || comp==&savegame_ex || comp ==&savegame_ex_rev); (void)comp;

	if(  v.i==-1  )
	{
		if(comp==&savegame)
		{
			savegame.set_selection( 0 );
		}
		else if( comp==&savegame_ex )
		{
			savegame_ex.set_selection( 0 );
		}
		else if( comp == &savegame_ex_rev )
		{
			savegame_ex_rev.set_selection( 0 );
		}
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
	savegame.set_pos( scr_coord(0, ypos) );
	savegame.set_size( scr_size(70, D_BUTTON_HEIGHT) );
	for(  uint32 i=0;  i<lengthof(version);  i++  ) {
		savegame.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( version[i]+2, SYSCOL_TEXT ) );
		if(  strcmp(version[i],env_t::savegame_version_str)==0  ) {
			savegame.set_selection( i );
		}
	}
	savegame.set_focusable( false );
	add_component( &savegame );
	savegame.add_listener( this );
	INIT_LB( "savegame version" );
	label.back()->set_pos( scr_coord( 70 + 6, label.back()->get_pos().y + 2 ) );
	SEPERATOR
	INIT_BOOL( "drive_left", sets->is_drive_left() );
	INIT_BOOL( "signals_on_left", sets->is_signals_left() );
	SEPERATOR
	INIT_NUM( "autosave", env_t::autosave, 0, 12, gui_numberinput_t::AUTOLINEAR, false );
	//INIT_NUM( "frames_per_second",env_t::fps, 10, 30, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "fast_forward", env_t::max_acceleration, 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_BOOL( "numbered_stations", sets->get_numbered_stations() );
	INIT_NUM( "show_names", env_t::show_names, 0, 7, gui_numberinput_t::AUTOLINEAR, true );
	SEPERATOR
	INIT_NUM( "bits_per_month", sets->get_bits_per_month(), 16, 48, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "use_timeline", sets->get_use_timeline(), 0, 3, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "starting_year", sets->get_starting_year(), 0, 2999, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "starting_month", sets->get_starting_month(), 0, 11, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "show_month", env_t::show_month, 0, 9, gui_numberinput_t::AUTOLINEAR, true );
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

	clear_dirty();

	SEPERATOR
	// comboboxes for Extended savegame version and revision
	savegame_ex.set_pos( scr_coord(2,ypos-2) );
	savegame_ex.set_size( scr_size(70,D_BUTTON_HEIGHT) );
	for(  uint32 i=0;  i<lengthof(version_ex);  i++  )
	{
		if(i == 0)
		{
			savegame_ex.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( "0", SYSCOL_TEXT ) );
		}
		else
		{
			savegame_ex.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( version_ex[i]+1, SYSCOL_TEXT ) );
		}
		if(  strcmp(version_ex[i],EXTENDED_VER_NR)==0  )
		{
			savegame_ex.set_selection( i );
		}
	}
	savegame_ex.set_focusable( false );
	add_component( &savegame_ex );
	savegame_ex.add_listener( this );
	INIT_LB( "savegame Extended version" );
	label.back()->set_pos( scr_coord( 76, label.back()->get_pos().y ) );
	clear_dirty();

	ypos+=5;
	height = ypos;

	savegame_ex_rev.set_pos( scr_coord(2,ypos-2) );
	savegame_ex_rev.set_size( scr_size(70,D_BUTTON_HEIGHT) );
	for(  uint32 i=0;  i<lengthof(revision_ex);  i++  )
	{
		if(i == 0)
		{
			savegame_ex_rev.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( "0", SYSCOL_TEXT ) );
		}
		else
		{
			savegame_ex_rev.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( revision_ex[i], SYSCOL_TEXT ) );
		}
		if(  strcmp(revision_ex[i],QUOTEME(EX_SAVE_MINOR))==0  )
		{
			savegame_ex_rev.set_selection( i );
		}
	}
	savegame_ex_rev.set_focusable( false );
	add_component( &savegame_ex_rev );
	savegame_ex_rev.add_listener( this );
	INIT_LB( "savegame Extended revision" );
	label.back()->set_pos( scr_coord( 76, label.back()->get_pos().y ) );
	clear_dirty();

	ypos+=105;
	height = ypos;

	set_size( settings_stats_t::get_size() );
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

	sets->calc_job_replenishment_ticks();

	const int selected_ex = savegame_ex.get_selection();
	if (selected_ex >= 0 &&  selected_ex < (sint32)lengthof(version_ex)) {
		env_t::savegame_ex_version_str = version_ex[ selected_ex ];
	}

	const int selected_ex_rev = savegame_ex_rev.get_selection();
	if (selected_ex_rev >= 0 &&  selected_ex_rev < (sint32)lengthof(revision_ex)) {
		env_t::savegame_ex_revision_str = revision_ex[ selected_ex_rev ];
	}
}

void settings_display_stats_t::init(settings_t const* const)
{
	INIT_INIT
	INIT_NUM( "frames_per_second",env_t::fps, 10, 60, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "simple_drawing_tile_size",env_t::simple_drawing_default, 2, 256, gui_numberinput_t::POWER2, false );
	INIT_BOOL( "simple_drawing_fast_forward",env_t::simple_drawing_fast_forward );
	INIT_NUM( "water_animation_ms", env_t::water_animation, 0, 1000, 25, false );
	SEPERATOR
	INIT_BOOL( "window_buttons_right", env_t::window_buttons_right );
	INIT_BOOL( "window_frame_active", env_t::window_frame_active );
	INIT_NUM( "front_window_bar_color", env_t::front_window_bar_color, 0, 6, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "front_window_text_color", env_t::front_window_text_color, 208, 240, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "bottom_window_bar_color", env_t::bottom_window_bar_color, 0, 6, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "bottom_window_text_color", env_t::bottom_window_text_color, 208, 240, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_BOOL( "show_tooltips", env_t::show_tooltips );
	INIT_NUM( "tooltip_background_color", env_t::tooltip_color, 0, 255, 1, 0 );
	INIT_NUM( "tooltip_text_color", env_t::tooltip_textcolor, 0, 255, 1, 0 );
	INIT_NUM( "tooltip_delay", env_t::tooltip_delay, 0, 10000, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "tooltip_duration", env_t::tooltip_duration, 0, 30000, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_NUM( "cursor_overlay_color", env_t::cursor_overlay_color, 0, 255, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_BOOL( "left_to_right_graphs", env_t::left_to_right_graphs );
	SEPERATOR
	INIT_BOOL( "player_finance_display_account", env_t::player_finance_display_account );

	clear_dirty();
	height = ypos;
	set_size( settings_stats_t::get_size() );
}

void settings_display_stats_t::read(settings_t* const)
{
	READ_INIT
	// all visual stuff
	READ_NUM_VALUE( env_t::fps );
	READ_NUM_VALUE( env_t::simple_drawing_default );
	READ_BOOL_VALUE( env_t::simple_drawing_fast_forward );
	READ_NUM_VALUE( env_t::water_animation );

	READ_BOOL_VALUE( env_t::window_buttons_right );
	READ_BOOL_VALUE( env_t::window_frame_active );
	READ_NUM_VALUE( env_t::front_window_bar_color );
	READ_NUM_VALUE( env_t::front_window_text_color );
	READ_NUM_VALUE( env_t::bottom_window_bar_color );
	READ_NUM_VALUE( env_t::bottom_window_text_color );

	READ_BOOL_VALUE( env_t::show_tooltips );
	READ_NUM_VALUE( env_t::tooltip_color );
	READ_NUM_VALUE( env_t::tooltip_textcolor );
	READ_NUM_VALUE( env_t::tooltip_delay );
	READ_NUM_VALUE( env_t::tooltip_duration );

	READ_NUM_VALUE( env_t::cursor_overlay_color );
	READ_BOOL_VALUE( env_t::left_to_right_graphs );

	READ_BOOL_VALUE( env_t::player_finance_display_account );
}

void settings_routing_stats_t::init(settings_t const* const sets)
{
	INIT_INIT
	INIT_BOOL( "separate_halt_capacities", sets->is_separate_halt_capacities() );
	INIT_BOOL( "avoid_overcrowding", sets->is_avoid_overcrowding() );
	SEPERATOR
	INIT_NUM( "max_route_steps", sets->get_max_route_steps(), 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
	INIT_NUM( "max_choose_route_steps", sets->get_max_choose_route_steps(), 0, 0x7FFFFFFFul, gui_numberinput_t::POWER2, false );
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
	height = ypos;
	set_size( settings_stats_t::get_size() );
}

void settings_routing_stats_t::read(settings_t* const sets)
{
	READ_INIT
	const sint32 old_route_steps = sets->max_route_steps;
	// routing of goods
	READ_BOOL_VALUE( sets->separate_halt_capacities );
	READ_BOOL_VALUE( sets->avoid_overcrowding );
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

	if(old_route_steps != sets->max_route_steps)
	{
		route_t::TERM_NODES();
		route_t::INIT_NODES(sets->max_route_steps, koord::invalid);
	}
}


void settings_economy_stats_t::init(settings_t const* const sets)
{
	INIT_INIT
	INIT_NUM( "remove_dummy_player_months", sets->get_remove_dummy_player_months(), 0, MAX_PLAYER_HISTORY_YEARS*12, 12, false );
	INIT_NUM( "unprotect_abandoned_player_months", sets->get_unprotect_abandoned_player_months(), 0, MAX_PLAYER_HISTORY_YEARS*12, 12, false );
	SEPERATOR
	INIT_COST( "starting_money", sets->get_starting_money(sets->get_starting_year()), 1, 0x7FFFFFFFul, 10000, false );
	INIT_BOOL_NEW( "first_beginner", sets->get_beginner_mode() );
	INIT_NUM( "beginner_price_factor", sets->get_beginner_price_factor(), 1, 25000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "allow_buying_obsolete_vehicles", sets->get_allow_buying_obsolete_vehicles(), 0, 2, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "used_vehicle_reduction", sets->get_used_vehicle_reduction(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "toll_runningcost_percentage", sets->get_way_toll_runningcost_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "toll_waycost_percentage", sets->get_way_toll_waycost_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "toll_revenue_percentage", sets->get_way_toll_revenue_percentage(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL("disable_make_way_public", sets->get_disable_make_way_public());
	SEPERATOR

	INIT_NUM( "ai_construction_speed", sets->get_default_ai_construction_speed(), 0, 1000000000, 1000, false );
	SEPERATOR

	INIT_NUM( "just_in_time", sets->get_just_in_time(), 0, 255, gui_numberinput_t::PLAIN, false );
	INIT_NUM( "maximum_intransit_percentage", sets->get_factory_maximum_intransit_percentage(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "crossconnect_factories", sets->is_crossconnect_factories() );
	INIT_NUM( "crossconnect_factories_percentage", sets->get_crossconnect_factor(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "industry_increase_every", sets->get_industry_increase_every(), 0, 100000, 100, false );
	INIT_NUM( "min_factory_spacing", sets->get_min_factory_spacing(), 1, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_factory_spacing_percent", sets->get_max_factory_spacing_percent(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_factory_spacing", sets->get_max_factory_spacing(), 1, 65535, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "electric_promille", sets->get_electric_promille(), 0, 2000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "allow_underground_transformers", sets->get_allow_underground_transformers() );
	INIT_NUM( "way_height_clearance", sets->get_way_height_clearance(), 1, 2, gui_numberinput_t::AUTOLINEAR, true );
	SEPERATOR

	INIT_NUM( "city_isolation_factor", sets->get_city_isolation_factor(), 1, 20000, 1, false );
	INIT_NUM( "special_building_distance", sets->get_special_building_distance(), 1, 150, 1, false );
	INIT_NUM( "factory_arrival_periods", sets->get_factory_arrival_periods(), 1, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "factory_enforce_demand", sets->get_factory_enforce_demand() );
	SEPERATOR
	INIT_NUM( "passenger_multiplier", sets->get_passenger_multiplier(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "mail_multiplier", sets->get_mail_multiplier(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "goods_multiplier", sets->get_goods_multiplier(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	//INIT_NUM( "electricity_multiplier", sets->get_electricity_multiplier(), 0, 10000, 10, false );
	SEPERATOR
	INIT_NUM( "growthfactor_villages", sets->get_growthfactor_small(), 1, 10000, 10, false );
	INIT_NUM( "growthfactor_cities", sets->get_growthfactor_medium(), 1, 10000, 10, false );
	INIT_NUM( "growthfactor_capitals", sets->get_growthfactor_large(), 1, 10000, 10, false );
	SEPERATOR
	INIT_BOOL( "random_pedestrians", sets->get_random_pedestrians() );
	INIT_BOOL( "stop_pedestrians", sets->get_show_pax() );
	INIT_NUM( "citycar_level", sets->get_traffic_level(), 0, 16, 1, false );
	INIT_NUM( "default_citycar_life", sets->get_stadtauto_duration(), 1, 1200, 12, false );

	clear_dirty();
	height = ypos;
	set_size( settings_stats_t::get_size() );
}

void settings_economy_stats_t::read(settings_t* const sets)
{
	READ_INIT
	sint64 start_money_temp;
	READ_NUM_VALUE( sets->remove_dummy_player_months );
	READ_NUM_VALUE( sets->unprotect_abandoned_player_months );
	READ_COST_VALUE( start_money_temp );
	if(  sets->get_starting_money(sets->get_starting_year())!=start_money_temp  ) {
		// because this will render the table based values invalid, we do this only when needed
		sets->starting_money = start_money_temp;
	}
	READ_BOOL_VALUE_NEW( sets->beginner_mode );
	READ_NUM_VALUE( sets->beginner_price_factor );
	READ_NUM_VALUE( sets->allow_buying_obsolete_vehicles );
	READ_NUM_VALUE( sets->used_vehicle_reduction );
	READ_NUM_VALUE( sets->way_toll_runningcost_percentage );
	READ_NUM_VALUE( sets->way_toll_waycost_percentage );
	READ_NUM_VALUE( sets->way_toll_revenue_percentage );
	READ_BOOL_VALUE(sets->disable_make_way_public);

	READ_NUM_VALUE( sets->default_ai_construction_speed );
	env_t::default_ai_construction_speed = sets->get_default_ai_construction_speed();

	READ_NUM_VALUE( sets->just_in_time );
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

	READ_NUM_VALUE( sets->city_isolation_factor );
	READ_NUM_VALUE( sets->special_building_distance );
	READ_NUM_VALUE( sets->factory_arrival_periods );
	READ_BOOL_VALUE( sets->factory_enforce_demand );
	READ_NUM_VALUE( sets->passenger_multiplier );
	READ_NUM_VALUE( sets->mail_multiplier );
	READ_NUM_VALUE( sets->goods_multiplier );
	//READ_NUM_VALUE( sets->electricity_multiplier );
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
	INIT_COST( "cost_buy_land", -sets->cst_buy_land, 0, 100000000, 10, false );
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
	INIT_NUM("cost_make_public_months", sets->cst_make_public_months, 0, 36000, gui_numberinput_t::AUTOLINEAR, false);
	clear_dirty();
	height = ypos;
	set_size( settings_stats_t::get_size() );
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

	clear_dirty();
	set_size( settings_stats_t::get_size() );
}


#include "../descriptor/ground_desc.h"


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
	INIT_LB( "Summer snowline" );
	INIT_NUM( "Winter snowline", sets->get_winter_snowline(), sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate;  i!=arctic_climate;  i++  ) {
		INIT_NUM( ground_desc_t::get_climate_name_from_bit((climate)i), sets->get_climate_borders()[i], sets->get_groundwater(), 127, gui_numberinput_t::AUTOLINEAR, false );
		if(sets->get_climate_borders()[i+1]>arctic) {
			arctic = sets->get_climate_borders()[i+1];
		}
	}
	numinp.at(3)->set_limits( 0, arctic );
	buf.clear();
	buf.printf( "%s %i", translator::translate( "Summer snowline" ), arctic );
	label.at(3)->set_text( buf );
	SEPERATOR
	INIT_BOOL( "lake", sets->get_lake() );
	INIT_NUM_NEW( "Number of rivers", sets->get_river_number(), 0, 1024, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "minimum length of rivers", sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM_NEW( "maximum length of rivers", sets->get_max_river_length(), sets->get_min_river_length()+16, 8196, gui_numberinput_t::AUTOLINEAR, false );
	// add listener to all of them
	FOR(slist_tpl<gui_numberinput_t*>, const n, numinp) {
		n->add_listener(this);
	}
	// the following are independent and thus need no listener
	SEPERATOR
	INIT_BOOL( "no tree", sets->get_no_trees() );
	INIT_NUM_NEW( "forest_base_size", sets->get_forest_base_size(), 10, 255, 1, false );
	INIT_NUM_NEW( "forest_map_size_divisor", sets->get_forest_map_size_divisor(), 2, 255, 1, false );
	INIT_NUM_NEW( "forest_count_divisor", sets->get_forest_count_divisor(), 2, 255, 1, false );
	INIT_NUM_NEW( "forest_inverse_spare_tree_density", sets->get_forest_inverse_spare_tree_density(), 33, 10000, 10, false );
	INIT_NUM( "max_no_of_trees_on_square", sets->get_max_no_of_trees_on_square(), 1, 5, 1, true );
	INIT_NUM_NEW( "tree_climates", sets->get_tree_climates(), 0, 255, 1, false );
	INIT_NUM_NEW( "no_tree_climates", sets->get_no_tree_climates(), 0, 255, 1, false );

	clear_dirty();
	height = ypos;
	set_size( settings_stats_t::get_size() );
}


void settings_climates_stats_t::read(settings_t* const sets)
{
	READ_INIT
	READ_NUM_VALUE_NEW( env_t::pak_height_conversion_factor );
	READ_NUM_VALUE_NEW( sets->groundwater );
	READ_NUM_VALUE_NEW( sets->max_mountain_height );
	double n;
	READ_NUM_VALUE_NEW( n );
	if(  new_world  ) {
		sets->map_roughness = (n+8.0)/20.0;
	}
	READ_NUM_VALUE( sets->winter_snowline );
	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate;  i!=arctic_climate;  i++  ) {
		sint16 ch;
		READ_NUM_VALUE( ch );
		sets->climate_borders[i] = ch;
		if(  ch>arctic  ) {
			arctic = ch;
		}
	}
	numinp.at(3)->set_limits( 0, arctic );
	buf.clear();
	buf.printf( "%s %i", translator::translate( "Summer snowline" ), arctic );
	label.at(3)->set_text( buf );
	READ_BOOL_VALUE( sets->lake );
	READ_NUM_VALUE_NEW( sets->river_number );
	READ_NUM_VALUE_NEW( sets->min_river_length );
	READ_NUM_VALUE_NEW( sets->max_river_length );
	READ_BOOL_VALUE( sets->no_trees );
	READ_NUM_VALUE_NEW( sets->forest_base_size );
	READ_NUM_VALUE_NEW( sets->forest_map_size_divisor );
	READ_NUM_VALUE_NEW( sets->forest_count_divisor );
	READ_NUM_VALUE_NEW( sets->forest_inverse_spare_tree_density );
	READ_NUM_VALUE( sets->max_no_of_trees_on_square );
	READ_NUM_VALUE_NEW( sets->tree_climates );
	READ_NUM_VALUE_NEW( sets->no_tree_climates );
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
