/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "welt.h"
#include "../simwin.h"
#include "../simcity.h"
#include "../dataobj/einstellungen.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "settings_stats.h"


/* stuff not set here ....
INIT_NUM( "intercity_road_length", umgebung_t::intercity_road_length);
INIT_NUM( "diagonal_multiplier", pak_diagonal_multiplier);
*/


gui_component_table_t& settings_stats_t::new_table(koord pos, coordinate_t columns, coordinate_t rows)
{
	gui_component_table_t& tbl = * new gui_component_table_t();
	tbl.set_default_cell_size(koord(12, 12));
	tbl.set_owns_cell_components(false);
	tbl.set_grid_width(koord(4,0));
	tbl.set_grid_visible(false);
	tbl.set_size(coordinates_t(columns, rows));
	tbl.set_pos(pos);
	table.append(&tbl);
	add_komponente(&tbl);
	return tbl;
}


gui_label_t& settings_stats_t::new_label(koord pos, const char *text)
{
	gui_label_t& lb = * new gui_label_t();
	lb.set_text_pointer(text);
	lb.set_pos(pos);
	lb.set_groesse(koord(proportional_string_width(text), 10));
	label.append(&lb);
	return lb;
}


gui_textarea_t& settings_stats_t::new_textarea(koord pos, const char *text)
{
	gui_textarea_t& ta = * new gui_textarea_t(text);
	ta.set_pos(pos);
	others.append(&ta);
	return ta;
}


gui_numberinput_t& settings_stats_t::new_numinp(koord pos, sint32 value, sint32 min_value, sint32 max_value, sint32 mode, bool wrap)
{
	gui_numberinput_t& ni = * new gui_numberinput_t();
	ni.init(value, min_value, max_value, mode, wrap);
	ni.set_pos(pos);
	ni.set_groesse(koord(37+proportional_string_width("0")*max(1,(sint16)(log10((double)(max_value)+1.0)+0.5)), BUTTON_HEIGHT ));
	numinp.append(&ni);
	return ni;
}


button_t& settings_stats_t::new_button(koord pos, const char *text, bool pressed)
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


void settings_experimental_general_stats_t::init( einstellungen_t *sets )
{
	INIT_INIT;
	INIT_NUM( "distance_per_tile_percent", sets->get_distance_per_tile_percent(), 1, 1000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR;
	INIT_NUM( "min_bonus_max_distance", sets->get_min_bonus_max_distance(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "median_bonus_distance", sets->get_median_bonus_distance(), 10, 1000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_bonus_min_distance", sets->get_max_bonus_min_distance(), 100, 10000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_bonus_multiplier_percent", sets->get_max_bonus_multiplier_percent(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 2);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("revenue of")), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("above\nminutes")), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("get\nrevenue $")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "travelling post office"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tpo_min_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tpo_revenue(), 0, 10000), 2, row);
		INIT_TABLE_END(tbl);
	}
	SEPERATOR;
	INIT_NUM( "city_threshold_size", sets->get_city_threshold_size(), 1000, 100000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "capital_threshold_size", sets->get_capital_threshold_size(), 10000, 1000000, gui_numberinput_t::AUTOLINEAR, false );
	INIT_BOOL( "quick_city_growth", sets->get_quick_city_growth());
	INIT_BOOL( "assume_everywhere_connected_by_road", sets->get_assume_everywhere_connected_by_road());
	SEPERATOR;
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 2, 9);
		int row = 8;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(overheadlines_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_other"), 1, row);
		row = 0;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(road_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_road"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(track_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_rail"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(water_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_water"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(monorail_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_monorail"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(maglev_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_maglev"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(tram_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_tram"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(narrowgauge_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_narrowgauge"), 1, row);
		row++;
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_default_increase_maintenance_after_years(air_wt), 0, 1000), 0, row);
		set_cell_component(tbl, new_label(koord(2, 0), "default_increase_maintenance_after_years_air"), 1, row);
		INIT_TABLE_END(tbl);
	}	
	clear_dirty();
	set_groesse( koord(width, ypos) );
}


void settings_experimental_general_stats_t::read(einstellungen_t *sets)
{
	READ_INIT;
	READ_NUM( sets->set_distance_per_tile_percent );

	READ_NUM( sets->set_min_bonus_max_distance );
	READ_NUM( sets->set_median_bonus_distance );
	READ_NUM( sets->set_max_bonus_min_distance );
	READ_NUM( sets->set_max_bonus_multiplier_percent );

	READ_NUM( sets->set_tpo_min_minutes );
	READ_NUM( sets->set_tpo_revenue );

	READ_NUM( sets->set_city_threshold_size );
	READ_NUM( sets->set_capital_threshold_size );
	READ_BOOL( sets->set_quick_city_growth );
	READ_BOOL( sets->set_assume_everywhere_connected_by_road );

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
			numiter.next();
			READ_NUM_ARRAY(sets->set_default_increase_maintenance_after_years, (waytype_t)i);
			break;
		default:
			sets->set_default_increase_maintenance_after_years((waytype_t)i, default_increase_maintenance_after_years_other);
		}
	}
}


void settings_experimental_revenue_stats_t::init( einstellungen_t *sets )
{
	INIT_INIT;
	INIT_NUM( "passenger_routing_packet_size", sets->get_passenger_routing_packet_size(), 1, 100, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "max_alternative_destinations", sets->get_max_alternative_destinations(), 0, 100, gui_numberinput_t::AUTOLINEAR, false );
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 6, 4);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("passenger\ndistribution")), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("min dist.\nkm")), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("max dist.\nkm")), 2, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("chance\npercent")), 3, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("waiting\ntolerance\nmin. min")), 4, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("waiting\ntolerance\nmax. min")), 5, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "local"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_local_passengers_max_distance(), 0, 1000), 2, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_passenger_routing_local_chance(), 0, 100), 3, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_min_local_tolerance() / 10, 30, 600), 4, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_local_tolerance() / 10, 30, 600), 5, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "mid range"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_midrange_passengers_min_distance(), 0, 1000), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_midrange_passengers_max_distance(), 0, 10000), 2, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_passenger_routing_midrange_chance(), 0, 100), 3, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_min_midrange_tolerance() / 10, 60, 1200), 4, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_max_midrange_tolerance() / 10, 60, 1200), 5, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "long dist."), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_longdistance_passengers_min_distance(), 0, 1000), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_min_longdistance_tolerance() / 10, 120, 4800), 4, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_max_longdistance_tolerance() / 10, 120, 4800), 5, row);
		INIT_TABLE_END(tbl);
	}
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 6);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("comfort expectance\nfor travelling")), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("duration\nin minutes")), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("min comfort\nrating")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "short time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_short_minutes(), 0, 120), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_tolerable_comfort_short(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "median short time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_median_short_minutes(), 0, 720), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_median_short(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "median median time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_median_median_minutes(), 0, 1440), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_median_median(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "median long time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_median_long_minutes(), 0, 1440*7), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_median_long(), 0, 255), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "long time"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_long_minutes(), 0, 1440*30), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_tolerable_comfort_long(), 0, 255), 2, row);
		INIT_TABLE_END(tbl);
	}
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 3);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("comfort impact\nlimitations")), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("differential")), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("percent")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "max luxury bonus"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_luxury_bonus_differential(), 0, 250), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_max_luxury_bonus_percent(), 0, 1000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "max discomfort penalty"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_max_discomfort_penalty_differential(), 0, 250), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_max_discomfort_penalty_percent(), 0, 1000), 2, row);
		INIT_TABLE_END(tbl);
	}
	{
		gui_component_table_t &tbl = new_table(koord(0, ypos), 3, 8);
		int row = 0;
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("catering bonus\nfor travelling")), 0, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("duration\nin minutes")), 1, 0);
		set_cell_component(tbl, new_textarea(koord(2, 0), translator::translate("max catering\nrevenue $")), 2, 0);
		row++;
		set_cell_component(tbl, new_label(koord(2, 3), "min traveltime"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 3), sets->get_catering_min_minutes(), 0, 14400), 1, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "catering level 1"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level1_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level1_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "catering level 2"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level2_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level2_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "catering level 3"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level3_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level3_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "catering level 4"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level4_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level4_max_revenue(), 0, 10000), 2, row);
		row++;
		set_cell_component(tbl, new_label(koord(2, 0), "catering level 5"), 0, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level5_minutes(), 0, 14400), 1, row);
		set_cell_component(tbl, new_numinp(koord(0, 0), sets->get_catering_level5_max_revenue(), 0, 10000), 2, row);
		INIT_TABLE_END(tbl);
	}
	clear_dirty();
	set_groesse( koord(width, ypos) );
}


void settings_experimental_revenue_stats_t::read(einstellungen_t *sets)
{
	READ_INIT
	READ_NUM_VALUE( sets->passenger_routing_packet_size );
	READ_NUM_VALUE( sets->max_alternative_destinations );

	READ_NUM_VALUE( sets->local_passengers_max_distance );
	READ_NUM_VALUE( sets->passenger_routing_local_chance );
	READ_NUM_VALUE_TENTHS( (sets->min_local_tolerance) );
	READ_NUM_VALUE_TENTHS( sets->max_local_tolerance);
	READ_NUM_VALUE( sets->midrange_passengers_min_distance );
	READ_NUM_VALUE( sets->midrange_passengers_max_distance );
	READ_NUM_VALUE( sets->passenger_routing_midrange_chance );
	READ_NUM_VALUE_TENTHS( sets->min_midrange_tolerance );
	READ_NUM_VALUE_TENTHS( sets->max_midrange_tolerance );
	READ_NUM_VALUE( sets->longdistance_passengers_min_distance );
	READ_NUM_VALUE_TENTHS( sets->min_longdistance_tolerance );
	READ_NUM_VALUE_TENTHS( sets->max_longdistance_tolerance);
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
	INIT_NUM( "front_window_bar_color", umgebung_t::front_window_bar_color, 0, 6, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "front_window_text_color", umgebung_t::front_window_text_color, 208, 240, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "bottom_window_bar_color", umgebung_t::bottom_window_bar_color, 0, 6, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "bottom_window_text_color", umgebung_t::bottom_window_text_color, 208, 240, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_BOOL( "show_tooltips", umgebung_t::show_tooltips );
	INIT_NUM( "tooltip_background_color", umgebung_t::tooltip_color, 0, 255, 1, 0 );
	INIT_NUM( "tooltip_text_color", umgebung_t::tooltip_textcolor, 0, 255, 1, 0 );
	INIT_NUM( "tooltip_delay", umgebung_t::tooltip_delay, 0, 10000, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_NUM( "tooltip_duration", umgebung_t::tooltip_duration, 0, 30000, gui_numberinput_t::AUTOLINEAR, 0 );
	SEPERATOR
	INIT_NUM( "cursor_overlay_color", umgebung_t::cursor_overlay_color, 0, 255, gui_numberinput_t::AUTOLINEAR, 0 );
	INIT_BOOL( "left_to_right_graphs", umgebung_t::left_to_right_graphs );

	clear_dirty();
	set_groesse( koord(width, ypos) );
}

void settings_general_stats_t::read(einstellungen_t *sets)
{
	READ_INIT
//	READ_BOOL_VALUE( umgebung_t::drive_on_left );	//cannot be switched after loading paks
	READ_NUM_VALUE( umgebung_t::autosave );
	READ_NUM_VALUE( umgebung_t::fps );
	READ_NUM_VALUE( umgebung_t::max_acceleration );

	READ_BOOL_VALUE( sets->numbered_stations );
	READ_NUM_VALUE( umgebung_t::show_names );
	READ_NUM_VALUE( umgebung_t::show_month );

	READ_NUM_VALUE( sets->bits_per_month );
	READ_NUM_VALUE( sets->use_timeline );
	READ_NUM_VALUE( sets->starting_year );
	READ_NUM_VALUE( sets->starting_month );

	READ_NUM_VALUE( umgebung_t::water_animation );
	READ_NUM_VALUE( umgebung_t::ground_object_probability );
	READ_NUM_VALUE( umgebung_t::moving_object_probability );

	READ_BOOL_VALUE( umgebung_t::verkehrsteilnehmer_info );
	READ_BOOL_VALUE( umgebung_t::tree_info );
	READ_BOOL_VALUE( umgebung_t::ground_info );
	READ_BOOL_VALUE( umgebung_t::townhall_info );
	READ_BOOL_VALUE( umgebung_t::single_info );

	READ_BOOL_VALUE( umgebung_t::window_buttons_right );
	READ_BOOL_VALUE( umgebung_t::window_frame_active );
	READ_NUM_VALUE( umgebung_t::front_window_bar_color );
	READ_NUM_VALUE( umgebung_t::front_window_text_color );
	READ_NUM_VALUE( umgebung_t::bottom_window_bar_color );
	READ_NUM_VALUE( umgebung_t::bottom_window_text_color );

	READ_BOOL_VALUE( umgebung_t::show_tooltips );
	READ_NUM_VALUE( umgebung_t::tooltip_color );
	READ_NUM_VALUE( umgebung_t::tooltip_textcolor );
	READ_NUM_VALUE( umgebung_t::tooltip_delay );
	READ_NUM_VALUE( umgebung_t::tooltip_duration );

	READ_NUM_VALUE( umgebung_t::cursor_overlay_color );
	READ_BOOL_VALUE( umgebung_t::left_to_right_graphs );
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
	set_groesse( koord(width, ypos) );
}

void settings_routing_stats_t::read(einstellungen_t *sets)
{
	READ_INIT
	// routing of goods
	READ_BOOL_VALUE( sets->seperate_halt_capacities );
	READ_BOOL_VALUE( sets->avoid_overcrowding );
	READ_NUM_VALUE( sets->station_coverage_size );
	READ_NUM_VALUE( sets->max_route_steps );
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
	INIT_NUM( "industry_increase_every", sets->get_industry_increase_every(), 0, 100000, 100, false );
	INIT_NUM( "factory_spacing", sets->get_factory_spacing(), 1, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "electric_promille", sets->get_electric_promille(), 0, 1000, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_NUM( "passenger_factor",  sets->get_passenger_factor(), 0, 16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "city_isolation_factor", sets->get_city_isolation_factor(), 1, 20000, 1, false );
	INIT_NUM( "factory_worker_radius", sets->get_factory_worker_radius(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "factory_worker_minimum_towns", sets->get_factory_worker_minimum_towns(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "factory_worker_maximum_towns", sets->get_factory_worker_maximum_towns(), 0, 32767, gui_numberinput_t::AUTOLINEAR, false );
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
	set_groesse( koord(width, ypos) );
}

void settings_economy_stats_t::read( einstellungen_t *sets )
{
	READ_INIT
	sint64 start_money_temp;
	READ_COST_VALUE( start_money_temp );
	if(  sets->get_starting_money(sets->get_starting_year())!=start_money_temp  ) {
		// because this will render the table based values invalid, we do this only when needed
		sets->starting_money = start_money_temp;
	}

	READ_BOOL_VALUE( sets->beginner_mode );
	READ_NUM_VALUE( sets->beginner_price_factor );
	READ_BOOL_VALUE( sets->allow_buying_obsolete_vehicles );

	READ_BOOL_VALUE( sets->just_in_time );
	READ_BOOL_VALUE( sets->crossconnect_factories );
	READ_NUM_VALUE( sets->crossconnect_factor );
	READ_NUM_VALUE( sets->industry_increase );
	READ_NUM_VALUE( sets->factory_spacing );
	READ_NUM_VALUE( sets->electric_promille );
	READ_NUM_VALUE( sets->city_isolation_factor );	
	READ_NUM_VALUE( sets->passenger_factor );
	READ_NUM_VALUE( sets->factory_worker_radius );
	READ_NUM_VALUE( sets->factory_worker_minimum_towns );
	READ_NUM_VALUE( sets->factory_worker_maximum_towns );
	READ_NUM_VALUE( sets->factory_worker_percentage );
	READ_NUM_VALUE( sets->tourist_percentage );
	READ_NUM_VALUE( sets->passenger_multiplier );
	READ_NUM_VALUE( sets->mail_multiplier );
	READ_NUM_VALUE( sets->goods_multiplier );
//	READ_NUM_VALUE( sets->set_electricity_multiplier );
	READ_NUM_VALUE( sets->growthfactor_small );
	READ_NUM_VALUE( sets->growthfactor_medium );
	READ_NUM_VALUE( sets->growthfactor_large );
	READ_BOOL( sets->set_random_pedestrians );
	READ_BOOL( sets->set_show_pax );
	READ_NUM( sets->set_verkehr_level );
	READ_NUM_VALUE( sets->stadtauto_duration );
}



void settings_costs_stats_t::init(einstellungen_t *sets)
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
	INIT_COST( "cost_found_city", -sets->cst_found_city, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_found_industry", -sets->cst_multiply_found_industry, 1, 100000000, 10, false );
	INIT_COST( "cost_remove_tree", -sets->cst_remove_tree, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_remove_haus", -sets->cst_multiply_remove_haus, 1, 100000000, 10, false );
	INIT_COST( "cost_multiply_remove_field", -sets->cst_multiply_remove_field, 1, 100000000, 10, false );
	INIT_COST( "cost_transformer", -sets->cst_transformer, 1, 100000000, 10, false );
	INIT_COST( "cost_maintain_transformer", -sets->cst_maintain_transformer, 1, 100000000, 10, false );
	clear_dirty();
	set_groesse( koord(width, ypos) );
}

void settings_costs_stats_t::read(einstellungen_t *sets)
{
	READ_INIT
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
	READ_COST_VALUE( sets->cst_found_city )*(-1);
	READ_COST_VALUE( sets->cst_multiply_found_industry )*(-1);
	READ_COST_VALUE( sets->cst_remove_tree )*(-1);
	READ_COST_VALUE( sets->cst_multiply_remove_haus )*(-1);
	READ_COST_VALUE( sets->cst_multiply_remove_field )*(-1);
	READ_COST_VALUE( sets->cst_transformer )*(-1);
	READ_COST_VALUE( sets->cst_maintain_transformer )*(-1);

	clear_dirty();
	set_groesse( settings_stats_t::get_groesse() );
}


#include "../besch/grund_besch.h"


void settings_climates_stats_t::init(einstellungen_t *sets)
{
	local_sets = sets;
	INIT_INIT
	INIT_NUM( "Water level", sets->get_grundwasser(), -10, 0, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "Mountain height", sets->get_max_mountain_height(), 0, 320, 10, false );
	INIT_NUM( "Map roughness", (sets->get_map_roughness()*20.0 + 0.5)-8, 0, 7, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	INIT_LB( "Summer snowline" );
	INIT_NUM( "Winter snowline", sets->get_winter_snowline(), sets->get_grundwasser(), 24, gui_numberinput_t::AUTOLINEAR, false );
	SEPERATOR
	// other climate borders ...
	sint16 arctic = 0;
	for(  int i=desert_climate;  i!=arctic_climate;  i++  ) {
		INIT_NUM( grund_besch_t::get_climate_name_from_bit((climate)i), sets->get_climate_borders()[i], sets->get_grundwasser(), 24, gui_numberinput_t::AUTOLINEAR, false );
		if(sets->get_climate_borders()[i]>arctic) {
			arctic = sets->get_climate_borders()[i];
		}
	}
	numinp.at(3)->set_limits( 0, arctic );
	buf.clear();
	buf.printf( "%s %i", translator::translate( "Summer snowline" ), arctic );
	label.at(3)->set_text( buf );
	SEPERATOR
	INIT_NUM( "Number of rivers", sets->get_river_number(), 0, 1024, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "minimum length of rivers", sets->get_min_river_length(), 0, max(16,sets->get_max_river_length())-16, gui_numberinput_t::AUTOLINEAR, false );
	INIT_NUM( "maximum length of rivers", sets->get_max_river_length(), sets->get_min_river_length()+16, 8196, gui_numberinput_t::AUTOLINEAR, false );
	// add listener to all of them
	slist_iterator_tpl<gui_numberinput_t *>iter(numinp);
	while(  iter.next()  ) {
		iter.get_current()->add_listener( this );
	}
	// the following are independent and thus need no listener
	SEPERATOR
	INIT_BOOL( "no tree", sets->get_no_trees() );
	INIT_NUM( "forest_base_size", sets->get_forest_base_size(), 10, 255, 1, false );
	INIT_NUM( "forest_map_size_divisor", sets->get_forest_map_size_divisor(), 2, 255, 1, false );
	INIT_NUM( "forest_count_divisor", sets->get_forest_count_divisor(), 2, 255, 1, false );
	INIT_NUM( "forest_inverse_spare_tree_density", sets->get_forest_inverse_spare_tree_density(), 0, 100, 1, false );
	INIT_NUM( "max_no_of_trees_on_square", sets->get_max_no_of_trees_on_square(), 1, 6, 1, true );
	INIT_NUM( "tree_climates", sets->get_tree_climates(), 0, 255, 1, false );
	INIT_NUM( "no_tree_climates", sets->get_no_tree_climates(), 0, 255, 1, false );

	clear_dirty();
	set_groesse( settings_stats_t::get_groesse() );
}


void settings_climates_stats_t::read(einstellungen_t *sets)
{
	READ_INIT
	READ_NUM_VALUE( sets->grundwasser );
	READ_NUM_VALUE( sets->max_mountain_height );
	double n;
	READ_NUM_VALUE( n );
	sets->map_roughness = (n+8.0)/20.0;
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
	READ_NUM_VALUE( sets->river_number );
	READ_NUM_VALUE( sets->min_river_length );
	READ_NUM_VALUE( sets->max_river_length );
	READ_BOOL_VALUE( sets->no_trees );
	READ_NUM_VALUE( sets->forest_base_size );
	READ_NUM_VALUE( sets->forest_map_size_divisor );
	READ_NUM_VALUE( sets->forest_count_divisor );
	READ_NUM_VALUE( sets->forest_inverse_spare_tree_density );
	READ_NUM_VALUE( sets->max_no_of_trees_on_square );
	READ_NUM_VALUE( sets->tree_climates );
	READ_NUM_VALUE( sets->no_tree_climates );
}


bool settings_climates_stats_t::action_triggered(gui_action_creator_t *komp, value_t)
{
	welt_gui_t *welt_gui = dynamic_cast<welt_gui_t *>(win_get_magic( magic_welt_gui_t ));
	read( local_sets );
	slist_iterator_tpl<gui_numberinput_t *>iter(numinp);
	for(  uint i=0;  iter.next();  i++  ) {
		if(  iter.get_current()==komp  &&  i<3  &&  welt_gui  ) {
			// update world preview
			welt_gui->update_preview();
		}
	}
	return true;
}
