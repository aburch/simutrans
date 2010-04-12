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


gui_component_table_t& settings_stats_t::new_table(koord &pos, coordinate_t columns, coordinate_t rows)
{
	gui_component_table_t& tbl = * new gui_component_table_t();
	tbl.set_default_cell_size(koord(BUTTON_HEIGHT, BUTTON_HEIGHT));
	tbl.set_owns_cell_components(false);
	tbl.set_grid_width(0);
	tbl.set_size(coordinates_t(columns, rows));
	tbl.set_pos(pos);
	table.append(&tbl);
	add_komponente(&tbl);
	return tbl;
}


gui_label_t& settings_stats_t::new_label(koord &pos, const char *text)
{
	gui_label_t& lb = * new gui_label_t();
	lb.set_text_pointer(text);
	lb.set_pos(pos);
	lb.set_groesse(koord(proportional_string_width(text), 10));
	label.append(&lb);
	return lb;
}


gui_textarea_t& settings_stats_t::new_textarea(koord &pos, const char *text)
{
	gui_textarea_t& ta = * new gui_textarea_t(text);
	ta.set_pos(pos);
	others.append(&ta);
	return ta;
}


gui_numberinput_t& settings_stats_t::new_numinp(koord &pos, sint32 value, sint32 min_value, sint32 max_value, sint32 mode, bool wrap)
{
	gui_numberinput_t& ni = * new gui_numberinput_t();
	ni.init(value, min_value, max_value, mode, wrap);
	ni.set_pos(pos);
	ni.set_groesse(koord(37+proportional_string_width("0")*max(1,(sint16)(log10((double)(max_value)+1.0)+0.5)), BUTTON_HEIGHT ));
	numinp.append(&ni);
	return ni;
}


button_t& settings_stats_t::new_button(koord &pos, const char *text, bool pressed)
{
	button_t& bt = * new button_t();
	bt.init(button_t::square_automatic, text, pos);
	bt.set_groesse(koord(16 + proportional_string_width(text), BUTTON_HEIGHT));
	bt.pressed = pressed;
	return bt;
}


void settings_stats_t::set_cell_component(gui_component_table_t &tbl, gui_komponente_t &c, coordinate_t x, coordinate_t y)
{
	tbl.set_cell_component(x, y, &c);
	tbl.set_column_width(x, max(tbl.get_column_width(x), c.get_pos().x + c.get_groesse().x));
	tbl.set_row_height(y, max(tbl.get_row_height(y), c.get_pos().y + c.get_groesse().y));
}


#define INIT_TABLE_END(tbl) \
	ypos += (tbl).get_table_height();\
	width = max(width, (tbl).get_pos().x * 2 + (tbl).get_table_width());\
	tbl.set_groesse(tbl.get_table_size());


void settings_experimental_revenue_stats_t::init( einstellungen_t *sets )
{
	INIT_INIT;
	INIT_NUM( "distance_per_tile_percent", sets->get_distance_per_tile_percent(), 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "min_bonus_max_distance", sets->get_min_bonus_max_distance(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "median_bonus_distance", sets->get_median_bonus_distance(), 10, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_bonus_min_distance", sets->get_max_bonus_min_distance(), 100, 10000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_bonus_multiplier_percent", sets->get_max_bonus_multiplier_percent(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 6);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), "comfort expectance\nfor travelling"), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), "duration\nin minutes"), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), "min comfort\nrating"), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "short time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_short_minutes(), 0, 120), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_short(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "median short time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_median_short_minutes(), 0, 720), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_median_short(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "median median time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_median_median_minutes(), 0, 1440), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_median_median(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "median long time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_median_long_minutes(), 0, 1440*7), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_median_long(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "long time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_long_minutes(), 0, 1440*30), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_long(), 0, 255), 2, row);
		INIT_TABLE_END(tbl);
	}
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 3);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), "comfort impact\nlimitations"), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), "differential"), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), "percent"), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "max luxury bonus"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_luxury_bonus_differential(), 0, 250), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_luxury_bonus_percent(), 0, 1000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "max discomfort penalty"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_discomfort_penalty_differential(), 0, 250), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_discomfort_penalty_percent(), 0, 1000), 2, row);
		INIT_TABLE_END(tbl);
	}
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 8);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), "catering bonus\nfor travelling"), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), "duration\nin minutes"), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), "max catering\nrevenue"), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "min traveltime"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_min_minutes(), 0, 14400), 1, row);
		gui_numberinput_t &numinp = new_numinp(koord(0, 3), 0, 0, 14400);
		numinp.set_read_only(true);
		set_cell_component(tbl, numinp, 2, row); 
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "catering level 1"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level1_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level1_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "catering level 2"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level2_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level2_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "catering level 3"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level3_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level3_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "catering level 4"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level4_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level4_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "catering level 5"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level5_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_level5_max_revenue(), 0, 10000), 2, row);
		INIT_TABLE_END(tbl);
	}
	clear_dirty();
	set_groesse( koord(width, ypos) );
}


void settings_experimental_revenue_stats_t::read(einstellungen_t *sets)
{
	EXIT_INIT
	EXIT_NUM( sets->set_distance_per_tile_percent );
	EXIT_NUM( sets->set_min_bonus_max_distance );
	EXIT_NUM( sets->set_median_bonus_distance );
	EXIT_NUM( sets->set_max_bonus_min_distance );
	EXIT_NUM( sets->set_max_bonus_multiplier_percent );

	EXIT_NUM( sets->set_tolerable_comfort_short_minutes );
	EXIT_NUM( sets->set_tolerable_comfort_short );
	EXIT_NUM( sets->set_tolerable_comfort_median_short_minutes );
	EXIT_NUM( sets->set_tolerable_comfort_median_short );
	EXIT_NUM( sets->set_tolerable_comfort_median_median_minutes );
	EXIT_NUM( sets->set_tolerable_comfort_median_median );
	EXIT_NUM( sets->set_tolerable_comfort_median_long_minutes );
	EXIT_NUM( sets->set_tolerable_comfort_median_long );
	EXIT_NUM( sets->set_tolerable_comfort_long_minutes );
	EXIT_NUM( sets->set_tolerable_comfort_long );

	EXIT_NUM( sets->set_max_luxury_bonus_differential );
	EXIT_NUM( sets->set_max_luxury_bonus_percent );
	EXIT_NUM( sets->set_max_discomfort_penalty_differential );
	EXIT_NUM( sets->set_max_discomfort_penalty_percent );

	EXIT_NUM( sets->set_catering_min_minutes );
	EXIT_NUM( sets->set_catering_level1_minutes );
	EXIT_NUM( sets->set_catering_level1_max_revenue );
	EXIT_NUM( sets->set_catering_level2_minutes );
	EXIT_NUM( sets->set_catering_level2_max_revenue );
	EXIT_NUM( sets->set_catering_level3_minutes );
	EXIT_NUM( sets->set_catering_level3_max_revenue );
	EXIT_NUM( sets->set_catering_level4_minutes );
	EXIT_NUM( sets->set_catering_level4_max_revenue );
	EXIT_NUM( sets->set_catering_level5_minutes );
	EXIT_NUM( sets->set_catering_level5_max_revenue );
}



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

