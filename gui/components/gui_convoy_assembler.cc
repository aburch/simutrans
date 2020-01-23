/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <math.h>
#include <algorithm>

#include "gui_convoy_assembler.h"

#include "../depot_frame.h"
#include "../replace_frame.h"

#include "../../simskin.h"
#include "../../simworld.h"
#include "../../simconvoi.h"
#include "../simwin.h"
#include "../../convoy.h"
#include "../vehicle_class_manager.h"


#include "../../bauer/goods_manager.h"
#include "../../bauer/vehikelbauer.h"
#include "../../descriptor/intro_dates.h"
#include "../../dataobj/replace_data.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/environment.h"
#include "../../dataobj/livery_scheme.h"
#include "../../utils/simstring.h"
#include "../../vehicle/simvehicle.h"
#include "../../descriptor/building_desc.h"
#include "../../descriptor/vehicle_desc.h"
#include "../../player/simplay.h"

#include "../../utils/cbuffer_t.h"
#include "../../utils/for.h"

#include "../../dataobj/settings.h"

static const char * engine_type_names [11] =
{
  "unknown",
  "steam",
  "diesel",
  "electric",
  "bio",
  "sail",
  "fuel_cell",
  "hydrogene",
  "battery",
  "petrol",
  "turbine"
};

gui_convoy_assembler_t::gui_convoy_assembler_t(waytype_t wt, signed char player_nr, bool electrified) :
	way_type(wt), way_electrified(electrified), last_changed_vehicle(NULL),
	depot_frame(NULL), replace_frame(NULL), placement(get_placement(wt)),
	placement_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 4),
	grid(get_grid(wt)),
	grid_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 2),
	max_convoy_length(depot_t::get_max_convoy_length(wt)), panel_rows(3), convoy_tabs_skip(0),
	lb_convoi_count(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_tiles(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_speed(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_cost(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_value(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_power(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_weight(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_brake_force(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_rolling_resistance(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_convoi_way_wear_factor(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_traction_types(NULL, SYSCOL_TEXT, gui_label_t::left),
	lb_vehicle_count(NULL, SYSCOL_TEXT, gui_label_t::right),
	lb_veh_action("Fahrzeuge:", SYSCOL_TEXT, gui_label_t::left),
	lb_livery_selector("Livery scheme:", SYSCOL_TEXT, gui_label_t::left),
	lb_livery_counter(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::left),
	lb_too_heavy_notice("too heavy", COL_RED, gui_label_t::left),
	convoi_pics(depot_t::get_max_convoy_length(wt)),
	convoi(&convoi_pics),
	pas(&pas_vec),
	pas2(&pas2_vec),
	electrics(&electrics_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&cont_pas),
	scrolly_pas2(&cont_pas2),
	scrolly_electrics(&cont_electrics),
	scrolly_loks(&cont_loks),
	scrolly_waggons(&cont_waggons),
	lb_vehicle_filter("Filter:", SYSCOL_TEXT, gui_label_t::left)
{

	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	*/
	placement.x=placement.x* get_base_tile_raster_width() / 64 + 2;
	placement.y=placement.y* get_base_tile_raster_width() / 64 + 2;
	grid.x=grid.x* get_base_tile_raster_width() / 64 + 4;
	grid.y=grid.y* get_base_tile_raster_width() / 64 + 6;
	//if(wt==road_wt  &&  welt->get_settings().is_drive_left()) {
	//	// correct for dive on left
	//	placement.x -= (12*get_base_tile_raster_width())/64;
	//	placement.y -= (6*get_base_tile_raster_width())/64;
	//}

	vehicles.clear();

	/*
	* [CONVOI]
	*/
	convoi.set_player_nr(player_nr);
	convoi.add_listener(this);

	add_component(&convoi);
	add_component(&lb_convoi_count);
	add_component(&lb_convoi_speed);
	add_component(&lb_convoi_cost);
	add_component(&lb_convoi_value);
	add_component(&lb_convoi_power);
	add_component(&lb_convoi_weight);
	add_component(&lb_convoi_brake_force);
	add_component(&lb_convoi_rolling_resistance);
	add_component(&lb_convoi_way_wear_factor);
	add_component(&cont_convoi_capacity);

	add_component(&lb_traction_types);
	add_component(&lb_vehicle_count);

	new_vehicle_length = 0;
	if (wt != water_wt && wt != air_wt) {
		add_component(&lb_convoi_tiles);
		add_component(&tile_occupancy);
	}

	/*
	* [PANEL]
	* to test for presence, we must fist switch on all vehicles,
	* and switch off the timeline ...
	* otherwise the tabs will not be present at all
	*/
	bool old_retired=show_retired_vehicles;
	bool show_outdated_vehicles = true;
	bool old_show_all=show_all;
	show_retired_vehicles = true;
	show_all = true;
	settings_t s = welt->get_settings();
	char timeline = s.get_use_timeline();
	s.set_use_timeline(0);
	build_vehicle_lists();
	s.set_use_timeline(timeline);
	show_retired_vehicles = old_retired;
	show_all = old_show_all;

	bool one = false;

	cont_pas.add_component(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);

	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
		one = true;
	}

	cont_pas2.add_component(&pas2);
	scrolly_pas2.set_show_scroll_x(false);
	scrolly_pas2.set_size_corner(false);
	// only add, if there are DMUs
	if (!pas2_vec.empty()) {
		tabs.add_tab(&scrolly_pas2, translator::translate(get_passenger2_name(wt)));
		one = true;
	}

	cont_electrics.add_component(&electrics);
	scrolly_electrics.set_show_scroll_x(false);
	scrolly_electrics.set_size_corner(false);
	// add only if there are any trolleybuses
	const uint16 shifter = 1 << vehicle_desc_t::electric;
	const bool correct_traction_type = !depot_frame || (shifter & depot_frame->get_depot()->get_tile()->get_desc()->get_enabled());
	if(!electrics_vec.empty() && correct_traction_type) 
	{
		tabs.add_tab(&scrolly_electrics, translator::translate( get_electrics_name(wt) ) );
		one = true;
	}

	cont_loks.add_component(&loks);
	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, translator::translate( get_zieher_name(wt) ) );
		one = true;
	}

	cont_waggons.add_component(&waggons);
	scrolly_waggons.set_show_scroll_x(false);
	scrolly_waggons.set_size_corner(false);
	// only add, if there are waggons
	if (!waggons_vec.empty()) {
		tabs.add_tab(&scrolly_waggons, translator::translate( get_haenger_name(wt) ) );
		one = true;
	}

	if(!one) {
		// add passenger as default
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
 	}

	pas.set_player_nr(player_nr);
	pas.add_listener(this);

	pas2.set_player_nr(player_nr);
	pas2.add_listener(this);

	electrics.set_player_nr(player_nr);
	electrics.add_listener(this);

	loks.set_player_nr(player_nr);
	loks.add_listener(this);

	waggons.set_player_nr(player_nr);
	waggons.add_listener(this);

	lb_too_heavy_notice.set_visible(false);

	add_component(&tabs);
	add_component(&div_tabbottom);
	add_component(&lb_veh_action);
	add_component(&lb_too_heavy_notice);
	add_component(&lb_livery_selector);
	add_component(&lb_livery_counter);
	add_component(&lb_vehicle_filter);

	veh_action = va_append;
	action_selector.add_listener(this);
	add_component(&action_selector);
	action_selector.clear_elements();
	static const char *txt_veh_action[4] = { "anhaengen", "voranstellen", "verkaufen", "Upgrade" };
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[0]), SYSCOL_TEXT ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[1]), SYSCOL_TEXT ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[2]), SYSCOL_TEXT ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[3]), SYSCOL_TEXT ) );
	action_selector.set_selection( 0 );

	bt_outdated.set_typ(button_t::square);
	bt_outdated.set_text("Show outdated");
	if (welt->use_timeline() && welt->get_settings().get_allow_buying_obsolete_vehicles() ) {
		bt_outdated.add_listener(this);
		bt_outdated.set_tooltip("Show also vehicles no longer in production.");
		add_component(&bt_outdated);
	}

	bt_obsolete.set_typ(button_t::square);
	bt_obsolete.set_text("Show obsolete");
	if(welt->use_timeline() && welt->get_settings().get_allow_buying_obsolete_vehicles() == 1 ) {
		bt_obsolete.add_listener(this);
		bt_obsolete.set_tooltip("Show also vehicles whose maintenance costs have increased due to obsolescence.");
		add_component(&bt_obsolete);
	}

	bt_show_all.set_typ(button_t::square);
	bt_show_all.set_text("Show all");
	bt_show_all.add_listener(this);
	bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
	add_component(&bt_show_all);

	vehicle_filter.set_highlight_color(depot_frame ? depot_frame->get_depot()->get_owner()->get_player_color1() + 1 : replace_frame ? replace_frame->get_convoy()->get_owner()->get_player_color1() + 1 : COL_BLACK);
	vehicle_filter.add_listener(this);
	add_component(&vehicle_filter);

	livery_selector.add_listener(this);
	add_component(&livery_selector);
	livery_selector.clear_elements();
	livery_scheme_indices.clear();

	bt_class_management.set_typ(button_t::roundbox);
	bt_class_management.set_text("class_manager");
	bt_class_management.add_listener(this);
	bt_class_management.set_tooltip("see_and_change_the_class_assignments");
	add_component(&bt_class_management);

	lb_convoi_count.set_text_pointer(txt_convoi_count);
	lb_convoi_tiles.set_text_pointer(txt_convoi_tiles);
	lb_convoi_speed.set_text_pointer(txt_convoi_speed);
	lb_convoi_speed.set_tooltip(tooltip_convoi_speed);
	lb_convoi_cost.set_text_pointer(txt_convoi_cost);
	lb_convoi_value.set_text_pointer(txt_convoi_value);
	lb_convoi_power.set_text_pointer(txt_convoi_power);
	lb_convoi_power.set_tooltip(tooltip_convoi_acceleration);
	lb_convoi_weight.set_text_pointer(txt_convoi_weight);
	lb_convoi_weight.set_tooltip(tooltip_convoi_axle_load);
	lb_convoi_brake_force.set_text_pointer(txt_convoi_brake_force);
	lb_convoi_brake_force.set_tooltip(tooltip_convoi_brake_distance);
	lb_convoi_rolling_resistance.set_text_pointer(txt_convoi_rolling_resistance);
	lb_convoi_way_wear_factor.set_text_pointer(txt_convoi_way_wear_factor);

	lb_traction_types.set_text_pointer(txt_traction_types);
	lb_vehicle_count.set_text_pointer(txt_vehicle_count);

	selected_filter = VEHICLE_FILTER_RELEVANT;

	replace_frame = NULL;
}

// free memory: all the image_data_t
gui_convoy_assembler_t::~gui_convoy_assembler_t()
{
	clear_vectors();
}

void gui_convoy_assembler_t::clear_vectors()
{
	vehicle_map.clear();
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(pas2_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
}

scr_coord gui_convoy_assembler_t::get_placement(waytype_t wt)
{
	if (wt==road_wt) {
		return scr_coord(-20,-25);
	}
	if (wt==water_wt) {
		return scr_coord(-1,-11);
	}
	if (wt==air_wt) {
		return scr_coord(-10,-23);
	}
	return scr_coord(-25,-28);
}


scr_coord gui_convoy_assembler_t::get_grid(waytype_t wt)
{
	if (wt==water_wt || wt==air_wt) {
		return scr_coord(36,36);
	}
	return scr_coord(24,24);
}


// Names for the tabs for different depot types, defaults to railway depots
const char * gui_convoy_assembler_t::get_passenger_name(waytype_t wt)
{
	if (wt==road_wt) {
		return "Bus_tab";
	}
	if (wt==water_wt) {
		return "Ferry_tab";
	}
	if (wt==air_wt) {
		return "Flug_tab";
	}
	return "Pas_tab";
}

const char * gui_convoy_assembler_t::get_passenger2_name(waytype_t wt)
{
	if (wt==track_wt || wt==tram_wt || wt==narrowgauge_wt) {
		return "Railcar_tab";
	}
	//if (wt==road_wt) {
	//	return "Bus_tab";
	//}
	return "Pas_tab"; // dummy
}

const char * gui_convoy_assembler_t::get_electrics_name(waytype_t wt)
{
	if (wt==road_wt) {
		return "TrolleyBus_tab";
	}
	return "Electrics_tab";
}

const char * gui_convoy_assembler_t::get_zieher_name(waytype_t wt)
{
	if (wt==road_wt) {
		return "LKW_tab";
	}
	if (wt==water_wt) {
		return "Schiff_tab";
	}
	if (wt==air_wt) {
		return "aircraft_tab";
	}
	return "Lokomotive_tab";
}

const char * gui_convoy_assembler_t::get_haenger_name(waytype_t wt)
{
	if (wt==road_wt) {
		return "Anhaenger_tab";
	}
	if (wt==water_wt) {
		return "Schleppkahn_tab";
	}
	return "Waggon_tab";
}

bool  gui_convoy_assembler_t::show_outdated_vehicles = false;

bool  gui_convoy_assembler_t::show_retired_vehicles = false;

bool  gui_convoy_assembler_t::show_all = false;

class karte_ptr_t gui_convoy_assembler_t::welt;

uint16 gui_convoy_assembler_t::livery_scheme_index = 0;

int gui_convoy_assembler_t::selected_filter = VEHICLE_FILTER_RELEVANT;

void gui_convoy_assembler_t::layout()
{
	/*
	*	The component has two parts:
	*		CONVOY IMAGE
	*		[[ optional blank space ]]
	*		VEHICLE SELECTION
	*		VINFO
	*/

	/*
	*	Structure of [VINFO] is one multiline text.
	*/

	/*
	*	Structure of [CONVOY IMAGE] is a image_list and an infos:
	*
	*	    [List]
	*	    [Info]
	*
	* The image list is horizontally "condensed".
	*/

	sint16 y = 0;
	const scr_coord_val c1_x = D_MARGIN_LEFT;
	const scr_size sp_size(size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT, LINESPACE);
	const scr_size lb_size((sp_size.w - D_V_SPACE) / 2, LINESPACE);
	const scr_coord_val c2_x = c1_x + ((lb_size.w / 5) * 4) + D_V_SPACE;
	const scr_coord_val c3_x = c2_x + ((lb_size.w / 4) * 3) + D_V_SPACE;

	/*
	 * [CONVOI]
	 */
	lb_convoi_count.set_pos(scr_coord(c1_x, y - LINESPACE + 2));
	lb_convoi_count.set_size(D_BUTTON_SIZE); //
	convoi.set_grid(scr_coord(grid.x - grid_dx, grid.y));
	convoi.set_placement(scr_coord(placement.x - placement_dx, placement.y));
	convoi.set_pos(scr_coord((max(c1_x, size.w-get_convoy_image_width())/2), y));
	convoi.set_size(scr_size(get_convoy_image_width(), get_convoy_image_height()));
	y += get_convoy_image_height();

	lb_convoi_tiles.set_pos(scr_coord(c1_x, y));
	lb_convoi_tiles.set_size(scr_size(proportional_string_width(translator::translate("Station tiles:")) + proportional_string_width(" 000"), LINESPACE));
	tile_occupancy.set_pos(scr_coord(c1_x + lb_convoi_tiles.get_size().w, y+3));
	tile_occupancy.set_size(scr_size(size.w - lb_convoi_tiles.get_size().w - bt_class_management.get_size().w, 5));
	bt_class_management.set_pos(scr_coord(c3_x, y));
	bt_class_management.set_size(scr_size(size.w - c3_x-5, LINESPACE));
	bt_class_management.pressed = win_get_magic(magic_class_manager);
	//bt_class_management.pressed = show_class_management;
	y += LINESPACE + 1;
	lb_convoi_cost.set_pos(scr_coord(c1_x, y));
	lb_convoi_cost.set_size(scr_size(c2_x - c1_x, LINESPACE));
	lb_convoi_value.set_pos(scr_coord(c2_x, y));
	lb_convoi_value.set_size(scr_size(c3_x - c2_x, LINESPACE));
	cont_convoi_capacity.set_pos(scr_coord(c3_x, y));
	cont_convoi_capacity.set_size(lb_size);
	y += LINESPACE + 1;
	lb_convoi_power.set_pos(scr_coord(c1_x, y));
	lb_convoi_power.set_size(scr_size(c2_x - c1_x, LINESPACE));
	lb_convoi_weight.set_pos(scr_coord(c2_x, y));
	lb_convoi_weight.set_size(scr_size(c3_x - c2_x, LINESPACE));
	y += LINESPACE + 1;
	lb_convoi_speed.set_pos(scr_coord(c1_x, y));
	lb_convoi_speed.set_size(scr_size(c2_x - c1_x, LINESPACE));
	lb_convoi_rolling_resistance.set_pos(scr_coord(c2_x, y));
	lb_convoi_rolling_resistance.set_size(scr_size(c3_x - c2_x, LINESPACE));
	y += LINESPACE + 1;
	lb_convoi_brake_force.set_pos(scr_coord(c1_x, y));
	lb_convoi_brake_force.set_size(scr_size(c2_x - c1_x, LINESPACE));
	lb_convoi_way_wear_factor.set_pos(scr_coord(c2_x, y));
	lb_convoi_way_wear_factor.set_size(scr_size(c3_x - c2_x, LINESPACE));
	y += LINESPACE + 2;

	/*
	* [PANEL]
	*/

	y += convoy_tabs_skip + 2;

	lb_traction_types.set_pos(scr_coord(c1_x, y));
	lb_traction_types.set_size(lb_size);
	lb_vehicle_count.set_pos(scr_coord(c2_x, y));
	lb_vehicle_count.set_size(lb_size);
	
	y += 7;

	tabs.set_pos(scr_coord(0, y));
	tabs.set_size(scr_size(size.w, get_panel_height()));
	y += get_panel_height();

	pas.set_grid(grid);
	pas.set_placement(placement);
	pas.set_size(tabs.get_size());
	pas.recalc_size();
	pas.set_pos(scr_coord(1,1));
	cont_pas.set_size(pas.get_size());
	scrolly_pas.set_size(scrolly_pas.get_size());

	pas2.set_grid(grid);
	pas2.set_placement(placement);
	pas2.set_size(tabs.get_size());
	pas2.recalc_size();
	pas2.set_pos(scr_coord(1, 1));
	cont_pas2.set_pos(scr_coord(0, 0));
	cont_pas2.set_size(pas2.get_size());
	scrolly_pas2.set_size(scrolly_pas2.get_size());

	electrics.set_grid(grid);
	electrics.set_placement(placement);
	electrics.set_size(tabs.get_size());
	electrics.recalc_size();
	electrics.set_pos(scr_coord(1,1));
	cont_electrics.set_size(electrics.get_size());
	scrolly_electrics.set_size(scrolly_electrics.get_size());

	loks.set_grid(grid);
	loks.set_placement(placement);
	loks.set_size(tabs.get_size());
	loks.recalc_size();
	loks.set_pos(scr_coord(1,1));
	cont_loks.set_pos(scr_coord(0,0));
	cont_loks.set_size(loks.get_size());
	scrolly_loks.set_size(scrolly_loks.get_size());

	waggons.set_grid(grid);
	waggons.set_placement(placement);
	waggons.set_size(tabs.get_size());
	waggons.recalc_size();
	waggons.set_pos(scr_coord(1,1));
	cont_waggons.set_size(waggons.get_size());
	scrolly_waggons.set_size(scrolly_waggons.get_size());

	div_tabbottom.set_pos(scr_coord(0, y));
	div_tabbottom.set_size(scr_size(size.w,0));

	/*
	 * below the tabs
	 */

	// left aligned column 1
	// right aligned columns 2..4 

	y += LINESPACE;

	const scr_size column2_size(size.w / 5, D_BUTTON_HEIGHT);
	const scr_size column3_size(126, D_BUTTON_HEIGHT);
	const scr_size column4_size( 96, D_BUTTON_HEIGHT);
	const scr_coord_val column4_x = size.w - column4_size.w - D_MARGIN_RIGHT;
	const scr_coord_val column3_x = column4_x - column3_size.w - D_MARGIN_RIGHT;
	const scr_coord_val column2_x = column3_x - column2_size.w - D_MARGIN_RIGHT;
	const scr_coord_val livery_counter_x = column2_x + proportional_string_width(translator::translate("Livery scheme:")) + 5;

	// header row

	lb_too_heavy_notice.set_pos(scr_coord(c1_x, y));
	lb_livery_selector.set_pos(scr_coord(column2_x, y));
	lb_livery_counter.set_pos(scr_coord(livery_counter_x, y));
	lb_vehicle_filter.set_pos(scr_coord(column3_x, y));
	lb_veh_action.set_pos(scr_coord(column4_x, y));
	y += 4 + D_BUTTON_HEIGHT;

	// 1st row

	bt_show_all.set_pos(scr_coord(c1_x, y));
	bt_show_all.pressed = show_all;
	livery_selector.set_pos(scr_coord(column2_x, y));
	livery_selector.set_size(column2_size);
	livery_selector.set_max_size(scr_size(column2_size.w + 30, LINESPACE*8+2+16));
	livery_selector.set_highlight_color(1);
	vehicle_filter.set_pos(scr_coord(column3_x, y));
	vehicle_filter.set_size(column3_size);
	vehicle_filter.set_max_size(scr_size(column3_size.w + 30, LINESPACE*8+2+16));
	action_selector.set_pos(scr_coord(column4_x, y));
	action_selector.set_size(column4_size);
	action_selector.set_max_size(scr_size(column4_size.w - 8, LINESPACE*3+2+16));
	action_selector.set_highlight_color(1);
	y += 4 + D_BUTTON_HEIGHT;

	// 2nd row 

	bt_outdated.set_pos(scr_coord(c1_x, y));
	bt_outdated.pressed = show_outdated_vehicles;

	bt_obsolete.set_pos(scr_coord(c1_x + D_CHECKBOX_WIDTH + D_H_SPACE*3 + proportional_string_width(translator::translate("Show outdated")), y));
	bt_obsolete.pressed = show_retired_vehicles;
	y += 4 + D_BUTTON_HEIGHT;

	// Class entries location on the window is specified together with their object specification // Ves
	



	const livery_scheme_t* const liv = welt->get_settings().get_livery_scheme(livery_scheme_index);
	if(liv && liv->is_available((welt->get_timeline_year_month())))
	{
		livery_selector.set_selection(livery_scheme_indices.index_of(livery_scheme_index));
	}
	else
	{
		vector_tpl<livery_scheme_t*>* schemes = welt->get_settings().get_livery_schemes();
		ITERATE_PTR(schemes, n)
		{
			if(schemes->get_element(n)->is_available(welt->get_timeline_year_month()))
			{
				livery_selector.set_selection(livery_scheme_indices.index_of(n));
			}
		}
	}
}


bool gui_convoy_assembler_t::action_triggered( gui_action_creator_t *comp,value_t p)
{
	if(comp != NULL) {	// message from outside!
			// image lsit selction here ...
		if(comp == &convoi) {
			image_from_convoi_list( p.i );
			update_data();
		} else if(comp == &pas) {
			image_from_storage_list(pas_vec[p.i]);
		} else if (comp == &pas2) {
			image_from_storage_list(pas2_vec[p.i]);
		} else if (comp == &electrics) {
			image_from_storage_list(electrics_vec[p.i]);
		} else if(comp == &loks) {
			image_from_storage_list(loks_vec[p.i]);
		} else if(comp == &waggons) {
			image_from_storage_list(waggons_vec[p.i]);
		} else if (comp == &bt_outdated) {
			show_outdated_vehicles = (show_outdated_vehicles == false);
			build_vehicle_lists();
			update_data();
		} else if(comp == &bt_obsolete) {
			show_retired_vehicles = (show_retired_vehicles == false);
			build_vehicle_lists();
			update_data();
		} else if(comp == &bt_show_all) {
			show_all = (show_all == false);
			build_vehicle_lists();
			update_data();
		} else if(comp == &action_selector) {
			sint32 selection = p.i;
			if ( selection < 0 ) {
				action_selector.set_selection(0);
				selection=0;
			}
				if(  (unsigned)(selection)<= va_upgrade) {
				veh_action=(unsigned)(selection);
				build_vehicle_lists();
				update_data();
				}
		} 
		else if (comp == &bt_class_management) 
		{
			convoihandle_t cnv;
			if (depot_frame)
			{
				cnv = depot_frame->get_convoy();
			}
			else if (replace_frame)
			{
				cnv = replace_frame->get_convoy();
			}
			create_win(20, 20, new vehicle_class_manager_t(cnv), w_info, magic_class_manager+ cnv.get_id());
			return true;
		}
		else if(comp == &vehicle_filter) 
		{
			selected_filter = vehicle_filter.get_selection();
		} 
		
		else if (comp == &livery_selector)
		{
			sint32 livery_selection = p.i;
			if (livery_selection < 0)
			{
				livery_selector.set_selection(0);
				livery_selection = 0;
			}
			livery_scheme_index = livery_scheme_indices.empty() ? 0 : livery_scheme_indices[livery_selection];
		}

		else 
		{
			return false;
		}
		
		build_vehicle_lists();
	}

	else
	{
		update_data();
		update_tabs();

	}
	layout();
	return true;
}

void gui_convoy_assembler_t::draw(scr_coord parent_pos)
{
	txt_convoi_cost.clear();
	txt_convoi_count.clear();
	txt_convoi_tiles.clear();
	txt_convoi_power.clear();
	txt_convoi_speed.clear();
	txt_convoi_value.clear();
	txt_convoi_weight.clear();
	txt_convoi_brake_force.clear();
	txt_convoi_rolling_resistance.clear();
	txt_convoi_way_wear_factor.clear();
	tooltip_convoi_acceleration.clear();
	tooltip_convoi_brake_distance.clear();
	tooltip_convoi_speed.clear();
	tooltip_convoi_axle_load.clear();
	cont_convoi_capacity.set_visible(!vehicles.empty());
	bt_class_management.set_visible(false);
	if (!vehicles.empty()) {
		potential_convoy_t convoy(vehicles);
		const vehicle_summary_t &vsum = convoy.get_vehicle_summary();
		const sint32 friction = convoy.get_current_friction();
		const double rolling_resistance = convoy.get_resistance_summary().to_double();
		const uint32 number_of_vehicles = vehicles.get_count();
		sint32 allowed_speed = vsum.max_speed;
		sint32 min_weight = vsum.weight;
		sint32 max_weight = vsum.weight + convoy.get_freight_summary().max_freight_weight;
		sint32 min_speed = convoy.calc_max_speed(weight_summary_t(max_weight, friction));
		sint32 max_speed = min_speed;

		uint32 total_pax = 0;
		uint32 total_standing_pax = 0;
		uint32 total_mail = 0;
		uint32 total_goods = 0;

		uint8 total_pax_classes = 0;
		uint8 total_mail_classes = 0;

		int pass_class_capacity[255] = { 0 };
		int mail_class_capacity[255] = { 0 };

		int good_type_0 = -1;
		int good_type_1 = -1;
		int good_type_2 = -1;
		int good_type_3 = -1;
		int good_type_4 = -1;

		uint32 good_type_0_amount = 0;
		uint32 good_type_1_amount = 0;
		uint32 good_type_2_amount = 0;
		uint32 good_type_3_amount = 0;
		uint32 good_type_4_amount = 0;
		uint32 rest_good_amount = 0;

		uint8 highest_catering = 0;
		bool is_tpo = false;

		uint32 total_power = 0;
		uint32 total_force = 0;
		uint32 total_empty_weight = min_weight;
		uint32 total_max_weight = max_weight;
		uint32 total_min_weight = vsum.weight + convoy.get_freight_summary().max_freight_weight;
		uint16 highest_axle_load = 0;
		uint32 total_cost = 0;
		uint32 maint_per_km = 0;
		uint32 maint_per_month = 0;
		double way_wear_factor = 0.0;

		tile_occupancy.set_base_convoy_length(vsum.length, vehicles.get_element(number_of_vehicles - 1)->get_length());
		tile_occupancy.set_visible(true);

		for (unsigned i = 0; i < number_of_vehicles; i++) {
			const vehicle_desc_t *desc = vehicles.get_element(i);
			//const vehicle_t *act_veh;
			const goods_desc_t* const ware = desc->get_freight_type();

			total_cost += desc->get_value();
			total_power += desc->get_power();
			total_force += desc->get_tractive_effort();
			maint_per_km += desc->get_running_cost();
			maint_per_month += desc->get_adjusted_monthly_fixed_cost(welt);
			way_wear_factor += (double)desc->get_way_wear_factor();
			highest_axle_load = max(highest_axle_load, desc->get_axle_load());
			convoihandle_t cnv;

			if (depot_frame)
			{
				cnv = depot_frame->get_convoy();
			}
			else if (replace_frame)
			{
				cnv = replace_frame->get_convoy();
			}

			if (cnv.is_bound() && cnv->get_vehicle_count() > i)
			{
				vehicle_t* v = cnv->get_vehicle(i);

				switch (ware->get_catg_index())
				{
				case goods_manager_t::INDEX_PAS:
				{
					total_pax += desc->get_total_capacity();
					total_standing_pax += desc->get_overcrowded_capacity();
					for (uint8 j = 0; j < goods_manager_t::passengers->get_number_of_classes(); j++)
					{
						pass_class_capacity[j] += v->get_fare_capacity(j);
						if (v && v->get_desc()->get_catering_level() > highest_catering)
						{
							highest_catering = v->get_desc()->get_catering_level();
						}
					}
					break;
				}
				case goods_manager_t::INDEX_MAIL:
				{
					total_mail += desc->get_total_capacity();
					for (uint8 j = 0; j < goods_manager_t::mail->get_number_of_classes(); j++)
					{
						mail_class_capacity[j] += v->get_fare_capacity(j);
						if (v && v->get_desc()->get_catering_level() > 0)
						{
							is_tpo = true;
						}
					}
					break;
				}
				default:
				{
					total_goods += desc->get_capacity();
					if (desc->get_capacity() > 0)
					{
						if (good_type_0 < 0 || good_type_0 == desc->get_freight_type()->get_catg_index())
						{
							good_type_0 = ware->get_catg_index();
							good_type_0_amount += desc->get_capacity();
						}
						else if (good_type_1 < 0 || good_type_1 == desc->get_freight_type()->get_catg_index())
						{
							good_type_1 = ware->get_catg_index();
							good_type_1_amount += desc->get_capacity();
						}
						else if (good_type_2 < 0 || good_type_2 == desc->get_freight_type()->get_catg_index())
						{
							good_type_2 = ware->get_catg_index();
							good_type_2_amount += desc->get_capacity();
						}
						else if (good_type_3 < 0 || good_type_3 == desc->get_freight_type()->get_catg_index())
						{
							good_type_3 = ware->get_catg_index();
							good_type_3_amount += desc->get_capacity();
						}
						else if (good_type_4 < 0 || good_type_4 == desc->get_freight_type()->get_catg_index())
						{
							good_type_4 = ware->get_catg_index();
							good_type_4_amount += desc->get_capacity();
						}
						else
						{
							rest_good_amount += desc->get_capacity();
						}
					}
				}
				break;
				}
			}
		}

		for (uint8 j = 0; j < goods_manager_t::passengers->get_number_of_classes(); j++)
		{
			if (pass_class_capacity[j] > 0)
			{
				total_pax_classes++;
			}
		}
		for (uint8 j = 0; j < goods_manager_t::mail->get_number_of_classes(); j++)
		{
			if (mail_class_capacity[j] > 0)
			{
				total_mail_classes++;
			}
		}



		// (total_pax, total_standing_pax, pax_classes, total_mail, mail_classes, total_goods)
		cont_convoi_capacity.set_totals(total_pax, total_standing_pax, total_mail, total_goods, total_pax_classes, total_mail_classes, good_type_0, good_type_1, good_type_2, good_type_3, good_type_4, good_type_0_amount, good_type_1_amount, good_type_2_amount, good_type_3_amount, good_type_4_amount, rest_good_amount, highest_catering, is_tpo);

		way_wear_factor /= 10000.0;
		txt_convoi_count.printf("%s %d", translator::translate("Fahrzeuge:"), vehicles.get_count());
		txt_convoi_tiles.printf("%s %i",
			translator::translate("Station tiles:"), vsum.tiles);

		COLOR_VAL col_convoi_speed = min_speed == 0 ? SYSCOL_TEXT_STRONG : SYSCOL_TEXT;
		if (min_speed < allowed_speed && min_weight < max_weight && min_speed != 0) {
			if (skinverwaltung_t::alerts) {
				txt_convoi_speed.append("   ");
				display_color_img(skinverwaltung_t::alerts->get_image_id(2), parent_pos.x + D_MARGIN_LEFT, parent_pos.y + pos.y + lb_convoi_speed.get_pos().y, 0, false, false);
			}
			else {
				lb_convoi_speed.set_color(COL_DARK_ORANGE);
			}
		}
		txt_convoi_speed.append(translator::translate("Max. speed:"));
		if (min_speed == 0)
		{
			if (convoy.get_starting_force() > 0)
			{
				txt_convoi_speed.printf(" %d km/h", min_speed);
				tooltip_convoi_speed.printf(" %d km/h @ %g t: ", min_speed, max_weight * 0.001f);
				//
				max_weight = convoy.calc_max_starting_weight(friction);
				min_speed = convoy.calc_max_speed(weight_summary_t(max_weight, friction));
				// 
				max_speed = allowed_speed;
				min_weight = convoy.calc_max_weight(friction);
			}
		}
		else if (min_speed < allowed_speed && min_weight < max_weight)
		{
			max_speed = convoy.calc_max_speed(weight_summary_t(min_weight, friction));
			if (min_speed < max_speed)
			{
				if (max_speed == allowed_speed)
				{
					// Show max. weight that can be pulled with max. allowed speed
					min_weight = convoy.calc_max_weight(friction);
				}
			}
		}
		txt_convoi_speed.printf(" %d km/h", min_speed);
		tooltip_convoi_speed.printf(
			min_speed == max_speed ? " %d km/h @ %g t" : " %d km/h @ %g t %s %d km/h @ %g t",
			min_speed, max_weight * 0.001f, translator::translate("..."), max_speed, min_weight * 0.001f);

		const sint32 brake_distance_min = convoy.calc_min_braking_distance(weight_summary_t(min_weight, friction), kmh2ms * max_speed);
		const sint32 brake_distance_max = convoy.calc_min_braking_distance(weight_summary_t(max_weight, friction), kmh2ms * max_speed);
		tooltip_convoi_brake_distance.printf(
			brake_distance_min == brake_distance_max ? translator::translate("brakes from max. speed in %i m") : translator::translate("brakes from max. speed in %i - %i m"),
			brake_distance_min, brake_distance_max);
		lb_convoi_speed.set_color(col_convoi_speed);

		// starting acceleration
		const uint32 starting_acceleration_max = (convoy.get_starting_force().to_sint32() - (rolling_resistance * (double)total_empty_weight) / number_of_vehicles) * 1000 / 9.80665 / 30.9 / min_weight * 100;
		const uint32 starting_acceleration_min = (convoy.get_starting_force().to_sint32() - (rolling_resistance * (double)total_max_weight) / number_of_vehicles) * 1000 / 9.80665 / 30.9 / max_weight * 100;
		tooltip_convoi_acceleration.append(translator::translate("Starting acceleration:"));
		tooltip_convoi_acceleration.printf(
			starting_acceleration_min == starting_acceleration_max ? " %.2f km/h/s" : " %.2f - %.2f km/h/s",
			(double)starting_acceleration_min / 100.0, (double)starting_acceleration_max / 100.0);
		// acceleration time
		const double min_acceleration_time = convoy.calc_acceleration_time(weight_summary_t(min_weight, friction), max_speed);
		const double max_acceleration_time = convoy.calc_acceleration_time(weight_summary_t(max_weight, friction), min_speed);
		if (min_speed == max_speed) {
			tooltip_convoi_acceleration.printf(min_weight == max_weight ? translator::translate("; %i km/h @ %.2f sec") : translator::translate("; %i km/h @ %.2f - %.2f sec"),
				max_speed, min_acceleration_time, max_acceleration_time);
		}
		else {
			tooltip_convoi_acceleration.printf(translator::translate("; %i km/h @ %.2f sec - %i km/h @ %.2f sec"),
				max_speed, min_acceleration_time, min_speed, max_acceleration_time);
		}
		// acceleration distance
		const uint32 min_acceleration_distance = convoy.calc_acceleration_distance(weight_summary_t(min_weight, friction), max_speed);
		const uint32 max_acceleration_distance = convoy.calc_acceleration_distance(weight_summary_t(max_weight, friction), min_speed);
		tooltip_convoi_acceleration.printf(min_weight == max_weight ? translator::translate("; %i m") : translator::translate("; %i m - %i m"),
			min_acceleration_distance, max_acceleration_distance);

		{
			char buf[128];
			if (depot_frame) {
				convoihandle_t cnv = depot_frame->get_convoy();
				if(cnv.is_bound())
				{
					money_to_string(buf, cnv->calc_sale_value() / 100.0 );
					txt_convoi_value.printf("%s %s", translator::translate("Restwert:"), buf);

				}
			}

			money_to_string(  buf, total_cost / 100.0 );
			txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km, %.2f$/month)\n"), buf, (double)maint_per_km / 100.0, (double)maint_per_month / 100.0 );
		}

		txt_convoi_power.clear();
		if (!total_power) {
			if (skinverwaltung_t::alerts) {
				txt_convoi_power.append("   ");
				display_color_img(skinverwaltung_t::alerts->get_image_id(4), parent_pos.x+D_MARGIN_LEFT, parent_pos.y + pos.y + lb_convoi_power.get_pos().y, 0, false, false);
			}
			else {
				lb_convoi_power.set_color(col_convoi_speed);
			}
		}
		else {
					lb_convoi_power.set_color(SYSCOL_TEXT);
		}
		txt_convoi_power.printf("%s %4d kW, %d kN\n", translator::translate("Power:"), total_power, total_force);

		txt_convoi_brake_force.clear();
		txt_convoi_brake_force.printf("%s %4.2f kN\n", translator::translate("Max. brake force:"), convoy.get_braking_force().to_double() / 1000.0);

		txt_convoi_weight.clear();
		txt_convoi_rolling_resistance.clear();
		if(  total_empty_weight != total_max_weight  ) {
			if(  total_min_weight != total_max_weight  ) {
				txt_convoi_weight.printf("%s %.3ft, %.3f-%.3ft\n", translator::translate("Weight:"), total_empty_weight / 1000.0, total_min_weight / 1000.0, total_max_weight / 1000.0 ); 
				txt_convoi_rolling_resistance.printf("%s %.3fkN, %.3fkN, %.3fkN", translator::translate("Rolling resistance:"), (rolling_resistance * (double)total_empty_weight / 1000.0) / number_of_vehicles, (rolling_resistance * (double)total_min_weight / 1000.0) / number_of_vehicles, (rolling_resistance * (double)total_max_weight / 1000.0) / number_of_vehicles);
			}
			else {
				txt_convoi_weight.printf("%s %.3ft, %.3ft\n", translator::translate("Weight:"), total_empty_weight / 1000.0, total_max_weight / 1000.0 );
				txt_convoi_rolling_resistance.printf("%s %.3fkN, %.3fkN", translator::translate("Rolling resistance:"), (rolling_resistance * (double)total_empty_weight / 1000.0) / number_of_vehicles, (rolling_resistance * (double)total_max_weight / 1000.0) / number_of_vehicles);
			}
		}
		else {
				txt_convoi_weight.printf("%s %.3ft\n", translator::translate("Weight:"), total_empty_weight / 1000.0 );
				txt_convoi_rolling_resistance.printf("%s %.3fkN\n", translator::translate("Rolling resistance:"), (rolling_resistance * (double)total_empty_weight / 1000.0) / number_of_vehicles);
		}
		tooltip_convoi_axle_load.printf("%s %d t", translator::translate("Max. axle load:"), highest_axle_load);

		txt_convoi_way_wear_factor.clear();
		txt_convoi_way_wear_factor.printf("%s: %.4f", translator::translate("Way wear factor"), way_wear_factor);

		if (total_pax > 0 || total_standing_pax > 0 || total_mail > 0)
		{
			bt_class_management.set_visible(true);
		}
	}
	else {
		tile_occupancy.set_visible(false);
	}
	bt_outdated.pressed = show_outdated_vehicles;	// otherwise the button would not show depressed
	bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
	bt_show_all.pressed = show_all;					// otherwise the button would not show depressed
	draw_vehicle_info_text(parent_pos+pos);
	gui_container_t::draw(parent_pos);
}



// add all current vehicles
void gui_convoy_assembler_t::build_vehicle_lists()
{
	if(vehicle_builder_t::get_info(way_type).empty()) 
	{
		// there are tracks etc. but now vehicles => do nothing
		// at least initialize some data
		if(depot_frame)
		{
			depot_frame->update_data();
		}
		else if(replace_frame)
		{
			replace_frame->update_data();
		}
		update_tabs();
		return;
	}

	const uint16 month_now = welt->get_timeline_year_month();

	vector_tpl<livery_scheme_t*>* schemes = welt->get_settings().get_livery_schemes();

	if(electrics_vec.empty()  &&  pas_vec.empty()  &&  pas2_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty())
	{
		/*
		 * The next block calculates upper bounds for the sizes of the vectors.
		 * If the vectors get resized, the vehicle_map becomes invalid, therefore
		 * we need to resize them before filling them.
		 */
		if(electrics_vec.empty()  &&  pas_vec.empty()  &&  pas2_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty())
		{
			int loks = 0, waggons = 0, pax=0, electrics = 0, pax2=0;
			FOR(slist_tpl<vehicle_desc_t *>, const info, vehicle_builder_t::get_info(way_type)) 
			{
				if(info->get_engine_type() == vehicle_desc_t::electric  &&  (info->get_freight_type()==goods_manager_t::passengers  ||  info->get_freight_type()==goods_manager_t::mail))
				{
					electrics++;
				}
				else if(info->get_freight_type()==goods_manager_t::passengers  ||  info->get_freight_type()==goods_manager_t::mail)
				{
					if (info->get_engine_type() == vehicle_desc_t::diesel || info->get_engine_type() == vehicle_desc_t::petrol) {
						pax2++;
					}
					else {
						pax++;
					}
				}
				else if(info->get_power() > 0  ||  info->get_total_capacity()==0)
				{
					loks++;
				}
				else 
				{
					waggons++;
				}
			}
			pas_vec.resize(pax);
			pas2_vec.resize(pax2);
			electrics_vec.resize(electrics);
			loks_vec.resize(loks);
			waggons_vec.resize(waggons);
		}
	}
	clear_vectors();

	// we do not allow to built electric vehicle in a depot without electrification (way_electrified)

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell && depot_frame) {
		// just list the one to sell
		FOR(slist_tpl<vehicle_t*>, const v, depot_frame->get_depot()->get_vehicle_list()) {
			vehicle_desc_t const* const d = v->get_desc();
			if (vehicle_map.get(d)) continue;
			add_to_vehicle_list(d);
		}
	}
	else {
		// list only matching ones

		if(depot_frame && env_t::networkmode)
		{
		  depot_frame->get_icnv() < 0 ? clear_convoy() : set_vehicles(depot_frame->get_convoy());
		  depot_frame->update_data();
		}

		FOR(slist_tpl<vehicle_desc_t *>, const info, vehicle_builder_t::get_info(way_type)) 
		{
			const vehicle_desc_t *veh = NULL;
			if(vehicles.get_count()>0) {
				veh = (veh_action == va_insert) ? vehicles[0] : vehicles[vehicles.get_count() - 1];
			}

			// current vehicle
			if ((depot_frame && depot_frame->get_depot()->is_contained(info)) ||
				((way_electrified || info->get_engine_type() != vehicle_desc_t::electric) &&
				(((!info->is_future(month_now)) && (!info->is_retired(month_now))) ||
					(info->is_retired(month_now) &&	((show_retired_vehicles && info->is_obsolete(month_now, welt) ||
					(show_outdated_vehicles && (!info->is_obsolete(month_now, welt)))))))))
			{
				// check if allowed
				bool append = true;
				bool upgradeable = true;
				if(!show_all) 
				{
					if(veh_action == va_insert) 
					{
						append = info->can_lead(veh)  &&  (veh==NULL  ||  veh->can_follow(info));
					} 
					else if(veh_action == va_append) 
					{
						append = info->can_follow(veh)  &&  (veh==NULL  ||  veh->can_lead(info));
					}
					if(veh_action == va_upgrade)
					{
						vector_tpl<const vehicle_desc_t*> vehicle_list;
						upgradeable = false;

						if(replace_frame == NULL)
						{
							FOR(vector_tpl<const vehicle_desc_t*>, vehicle, vehicles)
							{
								vehicle_list.append(vehicle);
							}
						}
						else
						{
							const convoihandle_t cnv = replace_frame->get_convoy();
						
							for(uint8 i = 0; i < cnv->get_vehicle_count(); i ++)
							{
								vehicle_list.append(cnv->get_vehicle(i)->get_desc());
							}
						}
						
						if(vehicle_list.get_count() < 1)
						{
							break;
						}

						FOR(vector_tpl<const vehicle_desc_t*>, vehicle, vehicle_list)
						{
							for(uint16 c = 0; c < vehicle->get_upgrades_count(); c++)
							{
								if(vehicle->get_upgrades(c) && (info == vehicle->get_upgrades(c)))
								{
									upgradeable = true;
								}
							}
						}
					}
					else
					{
						if(info->is_available_only_as_upgrade())
						{
							if(depot_frame && !depot_frame->get_depot()->find_oldest_newest(info, false))
							{
								append = false;
							}
							else if(replace_frame)
							{
								append = false;
								convoihandle_t cnv = replace_frame->get_convoy();
								const uint8 count = cnv->get_vehicle_count();
								for(uint8 i = 0; i < count; i++)
								{
									if(cnv->get_vehicle(i)->get_desc() == info)
									{
										append = true;
										break;
									}
								}
							}
						}
					}
					const uint16 shifter = 1 << info->get_engine_type();
					const bool correct_traction_type = !depot_frame || (shifter & depot_frame->get_depot()->get_tile()->get_desc()->get_enabled());
					const weg_t* way = depot_frame ? welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype()) : NULL;
					const bool correct_way_constraint = !way || missing_way_constraints_t(info->get_way_constraints(), way->get_way_constraints()).check_next_tile();
					if(!correct_way_constraint || (!correct_traction_type && (info->get_power() > 0 || (veh_action == va_insert && info->get_leader_count() == 1 && info->get_leader(0) && info->get_leader(0)->get_power() > 0))))
					{
						append = false;
					}
				}
				if((append && (veh_action == va_append || veh_action == va_insert)) || (upgradeable &&  veh_action == va_upgrade))
				{
					add_to_vehicle_list( info );
				}
			}

			// check livery scheme and build the abailable livery scheme list
			if (info->get_livery_count()>0)
			{
				ITERATE_PTR(schemes, i)
				{
					livery_scheme_t* scheme = schemes->get_element(i);
					if (scheme->is_available(welt->get_timeline_year_month()))
					{
						if(livery_scheme_indices.is_contained(i)){
							continue;
						}
						if (scheme->get_latest_available_livery(welt->get_timeline_year_month(), info)) {
							livery_scheme_indices.append(i);
							livery_scheme_index = i;
							continue;
						}
					}
				}
			}
		}
		livery_selector.clear_elements();
		std::sort(livery_scheme_indices.begin(), livery_scheme_indices.end());
		FOR(vector_tpl<uint16>, const& i, livery_scheme_indices) {
			livery_scheme_t* scheme = schemes->get_element(i);
			livery_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(scheme->get_name()), SYSCOL_TEXT));
			livery_selector.set_selection(i);
		}

	}
DBG_DEBUG("gui_convoy_assembler_t::build_vehicle_lists()","finally %i passenger vehicle, %i  engines, %i good wagons",pas_vec.get_count(),loks_vec.get_count(),waggons_vec.get_count());



	if(depot_frame)
	{
		depot_frame->get_icnv() < 0 ? clear_convoy() : set_vehicles(depot_frame->get_convoy());
		depot_frame->update_data();
	}
	else if(replace_frame)
	{
		//replace_frame->get_convoy().is_bound() ? clear_convoy() : set_vehicles(replace_frame->get_convoy());
		// We do not need to set the convoy here, as this will be done when the replacing takes place.
		replace_frame->update_data();
	}
	update_tabs();
}



// add a single vehicle (helper function)
void gui_convoy_assembler_t::add_to_vehicle_list(const vehicle_desc_t *info)
{
	// Prevent multiple instances of the same vehicle
	if(vehicle_map.is_contained(info))
	{
		return;
	}

	// Check if vehicle should be filtered
	const goods_desc_t *freight = info->get_freight_type();
	// Only filter when required and never filter engines
	if (selected_filter > 0 && info->get_total_capacity() > 0)
	{
		if (selected_filter == VEHICLE_FILTER_RELEVANT) 
		{
			if(freight->get_catg_index() >= 3) 
			{
				bool found = false;
				FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) 
				{
					if (freight->get_catg_index() == i->get_catg_index())
					{
						found = true;
						break;
					}
				}

				// If no current goods can be transported by this vehicle, don't display it
				if (!found) return;
			}
		} 
		
		else if (selected_filter > VEHICLE_FILTER_RELEVANT) 
		{
			// Filter on specific selected good
			uint32 goods_index = selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
			if (goods_index < welt->get_goods_list().get_count()) 
			{
				const goods_desc_t *selected_good = welt->get_goods_list()[goods_index];
				if (freight->get_catg_index() != selected_good->get_catg_index()) 
				{
					return; // This vehicle can't transport the selected good
				}
			}
		}
	}
	
	// prissi: ist a non-electric track?
	// Hajo: check for timeline
	// prissi: and retirement date

	if(livery_scheme_index >= welt->get_settings().get_livery_schemes()->get_count())
	{
		// To prevent errors when loading a game with fewer livery schemes than that just played.
		livery_scheme_index = 0;
	}

	const livery_scheme_t* const scheme = welt->get_settings().get_livery_scheme(livery_scheme_index);
	uint16 date = welt->get_timeline_year_month();
	image_id image;
	if(scheme)
	{
		const char* livery = scheme->get_latest_available_livery(date, info);
		if(livery)
		{
			image = info->get_base_image(livery);
		}
		else
		{
			bool found = false;
			for(uint32 j = 0; j < welt->get_settings().get_livery_schemes()->get_count(); j ++)
			{
				const livery_scheme_t* const new_scheme = welt->get_settings().get_livery_scheme(j);
				const char* new_livery = new_scheme->get_latest_available_livery(date, info);
				if(new_livery)
				{
					image = info->get_base_image(new_livery);
					found = true;
					break;
				}
			}
			if(!found)
			{
				image =info->get_base_image();
			}
		}
	}
	else
	{
		image = info->get_base_image();
	}
	gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), image);

	// since they come "pre-sorted" for the vehiclebauer, we have to do nothing to keep them sorted
	if (info->get_freight_type() == goods_manager_t::passengers || info->get_freight_type() == goods_manager_t::mail) {
		// Distributing passenger and mail cars to three types of tabs
		if(info->get_engine_type() == vehicle_desc_t::electric) {
			electrics_vec.append(img_data);
			vehicle_map.set(info, electrics_vec.back());
		}
		else if ((info->get_waytype() == track_wt || info->get_waytype() == tram_wt || info->get_waytype() == narrowgauge_wt)
				&& (info->get_engine_type() != vehicle_desc_t::unknown && info->get_engine_type() != 255))
		{
			pas2_vec.append(img_data);
			vehicle_map.set(info, pas2_vec.back());
		}
		else {
			pas_vec.append(img_data);
			vehicle_map.set(info, pas_vec.back());
		}
	}
	else if(info->get_power() > 0  || (info->get_total_capacity()==0  && (info->get_leader_count() > 0 || info->get_trailer_count() > 0)))
	{
		loks_vec.append(img_data);
		vehicle_map.set(info, loks_vec.back());
	}
	else {
		waggons_vec.append(img_data);
		vehicle_map.set(info, waggons_vec.back());
	}
}



void gui_convoy_assembler_t::image_from_convoi_list(uint nr)
{
	if(depot_frame)
	{
		// We're in an actual depot.
		depot_t* depot;
		depot = depot_frame->get_depot();

		const convoihandle_t cnv = depot->get_convoi(depot_frame->get_icnv());
		if(cnv.is_bound() &&  nr<cnv->get_vehicle_count() ) {

			// we remove all connected vehicles together!
			// find start
			unsigned start_nr = nr;
			while(start_nr>0) {
				start_nr --;
				const vehicle_desc_t *info = cnv->get_vehicle(start_nr)->get_desc();
				if(info->get_trailer_count()!=1) {
					start_nr ++;
					break;
				}
			}

			cbuffer_t start;
			start.append( start_nr );

			depot->call_depot_tool( 'r', cnv, start, livery_scheme_index );
		}
	}
	else
	{
		// We're in a replacer.  Less work.
		vehicles.remove_at(nr);
	}
}



void gui_convoy_assembler_t::image_from_storage_list(gui_image_list_t::image_data_t* image_data)
{
	
	const vehicle_desc_t *info = vehicle_builder_t::get_info(image_data->text);

	const convoihandle_t cnv = depot_frame ? depot_frame->get_depot()->get_convoi(depot_frame->get_icnv()) : replace_frame->get_convoy();
	
	const uint32 length = vehicles.get_count() + 1;
	if(cnv.is_bound() && (length > max_convoy_length))
	{
		// Do not add vehicles over maximum length.
		return;
	}

	if(depot_frame)
	{
		depot_t *depot = depot_frame->get_depot();
		if(image_data->lcolor != COL_RED &&
			image_data->rcolor != COL_RED &&
			image_data->rcolor != COL_GREY3 &&
			image_data->lcolor != COL_GREY3 &&
			image_data->rcolor != COL_DARK_PURPLE &&
			image_data->lcolor != COL_DARK_PURPLE &&
			image_data->rcolor != COL_PURPLE &&
			image_data->lcolor != COL_PURPLE &&
			!((image_data->lcolor == COL_DARK_ORANGE || image_data->rcolor == COL_DARK_ORANGE)
			&& veh_action != va_sell
			&& depot_frame != NULL && !depot_frame->get_depot()->find_oldest_newest(info, true))) 
		{
			// Dark orange = too expensive
			// Purple = available only as upgrade
			// Grey = too heavy

			if(veh_action == va_sell)
			{
				depot->call_depot_tool( 's', convoihandle_t(), image_data->text, livery_scheme_index );
			}
			else if(veh_action != va_upgrade)
			{
				depot->call_depot_tool( veh_action == va_insert ? 'i' : 'a', cnv, image_data->text, livery_scheme_index );
			}
			else
			{
				depot->call_depot_tool( 'u', cnv, image_data->text, livery_scheme_index );
			}
		}	
	}
	else
	{
		if(image_data->lcolor != COL_RED &&
			image_data->rcolor != COL_RED &&
			image_data->rcolor != COL_GREY3 &&
			image_data->lcolor != COL_GREY3 &&
			image_data->rcolor != COL_DARK_PURPLE &&
			image_data->lcolor != COL_DARK_PURPLE &&
			image_data->rcolor != COL_PURPLE &&
			image_data->lcolor != COL_PURPLE &&
			!((image_data->lcolor == COL_DARK_ORANGE || image_data->rcolor == COL_DARK_ORANGE)
			&& veh_action != va_sell
			/*&& depot_frame != NULL && !depot_frame->get_depot()->find_oldest_newest(info, true)*/)) 
		{
			//replace_frame->replace.add_vehicle(info);
			if(veh_action == va_upgrade)
			{
				uint8 count;
				ITERATE(vehicles,n)
				{
					count = vehicles[n]->get_upgrades_count();
					for(int i = 0; i < count; i++)
					{
						if(vehicles[n]->get_upgrades(i) == info)
						{
							vehicles.insert_at(n, info);
							vehicles.remove_at(n+1);
							return;
						}
					}
				}
			}
			else
			{
				if(veh_action == va_insert)
				{
					vehicles.insert_at(0, info);
				}
				else if(veh_action == va_append)
				{
					vehicles.append(info);
				}
			}
			// No action for sell - not available in the replacer window.
		}
	}
}


void gui_convoy_assembler_t::update_data()
{
	// change green into blue for retired vehicles
	const uint16 month_now = welt->get_timeline_year_month();

	const vehicle_desc_t *veh = NULL;

	convoi_pics.clear();
	if(vehicles.get_count() > 0) {

		unsigned i;
		for(i=0; i<vehicles.get_count(); i++) {

			// just make sure, there is this vehicle also here!
			const vehicle_desc_t *info=vehicles[i];
			if(vehicle_map.get(info)==NULL) {
				add_to_vehicle_list( info );
			}

			bool vehicle_available = false;
			image_id image;
			if(depot_frame)
			{
				FOR(slist_tpl<vehicle_t *>, const& iter, depot_frame->get_depot()->get_vehicle_list())
				{
					if(iter->get_desc() == info)
					{
						vehicle_available = true;
						image = info->get_base_image(iter->get_current_livery());
						break;
					}
				}

				for(unsigned int i = 0; i < depot_frame->get_depot()->convoi_count(); i ++)
				{
					convoihandle_t cnv = depot_frame->get_depot()->get_convoi(i);
					for(int n = 0; n < cnv->get_vehicle_count(); n ++)
					{
						if(cnv->get_vehicle(n)->get_desc() == info)
						{
							vehicle_available = true;
							image = info->get_base_image(cnv->get_vehicle(n)->get_current_livery());
							// Necessary to break out of double loop.
							goto end;
						}
					}
				}
			}
			else if(replace_frame)
			{
				convoihandle_t cnv = replace_frame->get_convoy();
				for(int n = 0; n < cnv->get_vehicle_count(); n ++)
				{
					if(cnv->get_vehicle(n)->get_desc() == info)
					{
						vehicle_available = true;
						image = info->get_base_image(cnv->get_vehicle(n)->get_current_livery());
						break;
					}
				}
			}

			end:

			if(!vehicle_available)
			{
				const livery_scheme_t* const scheme = welt->get_settings().get_livery_scheme(livery_scheme_index);
				uint16 date = welt->get_timeline_year_month();
				if(scheme)
				{
					const char* livery = scheme->get_latest_available_livery(date, vehicles[i]);
					if(livery)
					{
						image = vehicles[i]->get_base_image(livery);
					}
					else
					{
						bool found = false;
						for(uint32 j = 0; j < welt->get_settings().get_livery_schemes()->get_count(); j ++)
						{
							const livery_scheme_t* const new_scheme = welt->get_settings().get_livery_scheme(j);
							const char* new_livery = new_scheme->get_latest_available_livery(date, vehicles[i]);
							if(new_livery)
							{
								image = vehicles[i]->get_base_image(new_livery);
								found = true;
								break;
							}
						}
						if(!found)
						{
							image = vehicles[i]->get_base_image();
						}
					}
				}
				else
				{
					image = vehicles[i]->get_base_image();
				}
			}
			gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(vehicles[i]->get_name(), image);
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0]->lcolor = vehicles[0]->can_follow(NULL) ? COL_DARK_GREEN : COL_YELLOW;
		for(  i=1;  i<vehicles.get_count(); i++) {
			convoi_pics[i - 1]->rcolor = vehicles[i - 1]->can_lead(vehicles[i]) ? COL_DARK_GREEN : COL_RED;
			convoi_pics[i]->lcolor     = vehicles[i]->can_follow(vehicles[i - 1]) ? COL_DARK_GREEN : COL_RED;
		}
		convoi_pics[i - 1]->rcolor = vehicles[i - 1]->can_lead(NULL) ? COL_DARK_GREEN : COL_YELLOW;

		// change green into blue for retired vehicles
		for(i=0;  i<vehicles.get_count(); i++) {
			if(vehicles[i]->is_future(month_now) || vehicles[i]->is_retired(month_now)) {
				if (convoi_pics[i]->lcolor == COL_DARK_GREEN) {
					if (vehicles[i]->is_obsolete(month_now, welt)) {
						convoi_pics[i]->lcolor = COL_DARK_BLUE;
					}
					else {
						convoi_pics[i]->lcolor = COL_ROYAL_BLUE;
					}
				}
				if (convoi_pics[i]->rcolor == COL_DARK_GREEN) {
					if (vehicles[i]->is_obsolete(month_now, welt)) {
						convoi_pics[i]->rcolor = COL_DARK_BLUE;
					}
					else {
						convoi_pics[i]->rcolor = COL_ROYAL_BLUE;
					}
				}
			}
		}

		if(veh_action == va_insert) {
			veh = vehicles[0];
		} else if(veh_action == va_append) {
			veh = vehicles[vehicles.get_count() - 1];
		}
	}
	
	const player_t *player = welt->get_active_player();
	
	FOR(vehicle_image_map, const& i, vehicle_map) 
	{
		vehicle_desc_t const* const    info = i.key;
		gui_image_list_t::image_data_t& img  = *i.value;

		uint8 ok_color = info->is_future(month_now) || info->is_retired(month_now) ? COL_ROYAL_BLUE : COL_DARK_GREEN;
		if (info->is_obsolete(month_now, welt)) {
			ok_color = COL_DARK_BLUE;
		}

		img.count = 0;
		img.lcolor = ok_color;
		img.rcolor = ok_color;

		/*
		* color bars for current convoi:
		*  green/green	okay to append/insert
		*  red/red		cannot be appended/inserted
		*  green/yellow	append okay, cannot be end of train
		*  yellow/green	insert okay, cannot be start of train
		*  orange/orange - too expensive
		*  purple/purple available only as upgrade
		*  dark purple/dark purple cannot upgrade to this vehicle
		*/
		if(veh_action == va_insert) {
			if(!info->can_lead(veh)  ||  (veh  &&  !veh->can_follow(info))) {
				img.lcolor = COL_RED;
				img.rcolor = COL_RED;
			} else if(!info->can_follow(NULL)) {
				img.lcolor = COL_YELLOW;
			}
		} else if(veh_action == va_append) {
			if(!info->can_follow(veh)  ||  (veh  &&  !veh->can_lead(info))) {
				img.lcolor = COL_RED;
				img.rcolor = COL_RED;
			} else if(!info->can_lead(NULL)) {
				img.rcolor = COL_YELLOW;
			}
		}
		if (veh_action != va_sell)
		{
			// Check whether too expensive
			// @author: jamespetts
			if(img.lcolor == ok_color || img.lcolor == COL_YELLOW)
			{
				// Only flag as too expensive that which could be purchased but for its price.
				if(veh_action != va_upgrade)
				{
					if(!player->can_afford(info->get_value()))
					{
						img.lcolor = COL_DARK_ORANGE;
						img.rcolor = COL_DARK_ORANGE;
					}
				}
				else
				{
					if(!player->can_afford(info->get_upgrade_price()))
					{
						img.lcolor = COL_DARK_ORANGE;
						img.rcolor = COL_DARK_ORANGE;
					}
				}
				if(depot_frame && (i.key->get_power() > 0 || veh_action == va_insert && i.key->get_leader_count() == 1 && i.key->get_leader(0) && i.key->get_leader(0)->get_power() > 0))
				{
					const uint16 traction_type = i.key->get_engine_type();
					const uint16 shifter = 1 << traction_type;
					if(!(shifter & depot_frame->get_depot()->get_tile()->get_desc()->get_enabled()))
					{
						// Do not allow purchasing of vehicle if depot is of the wrong type.
						img.lcolor = COL_RED;
						img.rcolor = COL_RED;
					}
				}
				const weg_t* way = depot_frame ? welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype()) : NULL;
				if (!way && depot_frame)
				{
					// Might be tram depot, using way no. 2.
					const grund_t* gr_depot = welt->lookup(depot_frame->get_depot()->get_pos()); 
					if (gr_depot)
					{
						way = gr_depot->get_weg_nr(0);
					}
				}
				if(way && !missing_way_constraints_t(i.key->get_way_constraints(), way->get_way_constraints()).check_next_tile())
				{
					// Do not allow purchasing of vehicle if depot is on an incompatible way.
					img.lcolor = COL_RED;
					img.rcolor = COL_RED;
				} //(highest_axle_load * 100) / weight_limit > 110)

				if(depot_frame &&
					(way &&
					((welt->get_settings().get_enforce_weight_limits() == 2
						&& welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype())
						&& i.key->get_axle_load() > welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype())->get_max_axle_load())
					|| (welt->get_settings().get_enforce_weight_limits() == 3
						&& (way->get_max_axle_load() > 0 && (i.key->get_axle_load() * 100) / way->get_max_axle_load() > 110)))))
					
				{
					// Indicate if vehicles are too heavy
					img.lcolor = COL_GREY3;
					img.rcolor = COL_GREY3;
				}
			}
		}
		else
		{
			// If selling, one cannot buy - mark all purchasable vehicles red.
			img.lcolor = COL_RED;
			img.rcolor = COL_RED;
		}

		if(veh_action == va_upgrade)
		{
			//Check whether there are any vehicles to upgrade
			img.lcolor = COL_DARK_PURPLE;
			img.rcolor = COL_DARK_PURPLE;
			vector_tpl<const vehicle_desc_t*> vehicle_list;

			//if(replace_frame == NULL)
			//{
				ITERATE(vehicles,i)
				{
					vehicle_list.append(vehicles[i]);
				}
			//}
			//else
			//{
			//	const convoihandle_t cnv = replace_frame->get_convoy();
			//
			//	for(uint8 i = 0; i < cnv->get_vehicle_count(); i ++)
			//	{
			//		vehicle_list.append(cnv->get_vehicle(i)->get_desc());
			//	}
			//}

			ITERATE(vehicle_list, i)
			{
				for(uint16 c = 0; c < vehicle_list[i]->get_upgrades_count(); c++)
				{
					if(vehicle_list[i]->get_upgrades(c) && (info == vehicle_list[i]->get_upgrades(c)))
					{
						if(!player->can_afford(info->get_upgrade_price()))
						{
							img.lcolor = COL_DARK_ORANGE;
							img.rcolor = COL_DARK_ORANGE;
						}
						else
						{
							img.lcolor = COL_DARK_GREEN;
							img.rcolor = COL_DARK_GREEN;
						}
						if(replace_frame != NULL)
						{
							// If we are using the replacing window,
							// vehicle_list is the list of all vehicles in the current convoy.
							// vehicles is the list of the vehicles to replace them with.
							int upgradable_count = 0;
							
							ITERATE(vehicle_list,j)
							{
								for(uint16 k = 0; k < vehicle_list[j]->get_upgrades_count(); k++)
								{
									if(vehicle_list[j]->get_upgrades(k) && (vehicle_list[j]->get_upgrades(k) == info))
									{
										// Counts the number of vehicles in the current convoy that can
										// upgrade to the currently selected vehicle.
										upgradable_count ++;
									}
								}
							}

							if(upgradable_count == 0)
							{
								//There are not enough vehicles left to upgrade.
								img.lcolor = COL_DARK_PURPLE;
								img.rcolor = COL_DARK_PURPLE;
							}

						}
						/*if(veh_action == va_insert) 
						{
							if (!veh->can_lead(info) || (veh && !info->can_follow(veh)))
							{
								img.lcolor = COL_RED;
								img.rcolor = COL_RED;
							} 
							else if(!info->can_follow(NULL)) 
							{
								img.lcolor = COL_YELLOW;
							}
						} 
						else if(veh_action == va_append) 
						{
							if(!veh->can_lead(info) || (veh  &&  !veh->can_lead(info))) {
								img.lcolor = COL_RED;
								img.rcolor = COL_RED;
							} 
							else if(!info->can_lead(NULL))
							{
								img.rcolor = COL_YELLOW;
							}
						}*/
					}
				}
			}
		}
		else
		{	
			if(info->is_available_only_as_upgrade())
			{
				bool purple = false;
				if(depot_frame && !depot_frame->get_depot()->find_oldest_newest(info, false))
				{
						purple = true;
				}
				else if(replace_frame)
				{
					purple = true;
					convoihandle_t cnv = replace_frame->get_convoy();
					const uint8 count = cnv->get_vehicle_count();
					for(uint8 i = 0; i < count; i++)
					{
						if(cnv->get_vehicle(i)->get_desc() == info)
						{
							purple = false;
							break;
						}
					}
				}
				if(purple)
				{
					img.lcolor = COL_PURPLE;
					img.rcolor = COL_PURPLE;
				}
			}
		}

DBG_DEBUG("gui_convoy_assembler_t::update_data()","current %s with colors %i,%i",info->get_name(),img.lcolor,img.rcolor);
	}

	if (depot_frame) 
	{
		FOR(slist_tpl<vehicle_t *>, const& iter2, depot_frame->get_depot()->get_vehicle_list())
		{
			gui_image_list_t::image_data_t *imgdat=vehicle_map.get(iter2->get_desc());
			// can fail, if currently not visible
			if(imgdat) 
			{
				imgdat->count++;
				if(veh_action == va_sell) 
				{
					imgdat->lcolor = COL_DARK_GREEN;
					imgdat->rcolor = COL_DARK_GREEN;
				}
			}
		}
		depot_frame->layout(NULL);
	}
	else if(replace_frame)
	{
		replace_frame->layout(NULL);
	}
}

void gui_convoy_assembler_t::update_tabs()
{
	waytype_t wt;
	if(depot_frame)
	{
		wt = depot_frame->get_depot()->get_wegtyp();
	}
	else if(replace_frame)
	{
		wt = replace_frame->get_convoy()->get_vehicle(0)->get_waytype();
	}
	else
	{
		wt = road_wt;
	}

	gui_component_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;

	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);
	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
		one = true;
	}

	scrolly_pas2.set_show_scroll_x(false);
	scrolly_pas2.set_size_corner(false);
	// add only if there are any
	if (!pas2_vec.empty()) {
		tabs.add_tab(&scrolly_pas2, translator::translate(get_passenger2_name(wt)));
		one = true;
	}

	scrolly_electrics.set_show_scroll_x(false);
	scrolly_electrics.set_size_corner(false);
	// add only if there are any trolleybuses
	const uint16 shifter = 1 << vehicle_desc_t::electric;
	const bool correct_traction_type = !depot_frame || (shifter & depot_frame->get_depot()->get_tile()->get_desc()->get_enabled());
	if(!electrics_vec.empty() && correct_traction_type) 
	{
		tabs.add_tab(&scrolly_electrics, translator::translate( get_electrics_name(wt) ) );
		one = true;
	}

	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, translator::translate( get_zieher_name(wt) ) );
		one = true;
	}

	scrolly_waggons.set_show_scroll_x(false);
	scrolly_waggons.set_size_corner(false);
	// only add, if there are waggons
	if (!waggons_vec.empty()) {
		tabs.add_tab(&scrolly_waggons, translator::translate( get_haenger_name(wt) ) );
		one = true;
	}

	if(!one) {
		// add passenger as default
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
	}

	// Look, if there is our old tab present again (otherwise it will be 0 by tabs.clear()).
	for( uint8 i = 0; i < tabs.get_count(); i++ ) {
		if(  old_tab == tabs.get_tab(i)  ) {
			// Found it!
			tabs.set_active_tab_index(i);
			break;
		}
	}

	// Update vehicle filter
	vehicle_filter.clear_elements();
	vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("All"), SYSCOL_TEXT));
	vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("Relevant"), SYSCOL_TEXT));

	FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
		vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(i->get_name()), SYSCOL_TEXT));
	}

	if (selected_filter > vehicle_filter.count_elements()) {
		selected_filter = VEHICLE_FILTER_RELEVANT;
	}
	vehicle_filter.set_selection(selected_filter);
	vehicle_filter.set_highlight_color(depot_frame ? depot_frame->get_depot()->get_owner()->get_player_color1() + 1 : replace_frame ? replace_frame->get_convoy()->get_owner()->get_player_color1() + 1 : COL_BLACK);
}



void gui_convoy_assembler_t::draw_vehicle_info_text(const scr_coord& pos)
{
	cbuffer_t buf;
	const scr_size size = depot_frame ? depot_frame->get_windowsize() : replace_frame->get_windowsize();
	PUSH_CLIP(pos.x, pos.y, size.w-1, size.h-1);

	gui_component_t const* const tab = tabs.get_aktives_tab();
	gui_image_list_t const* const lst =
		tab == &scrolly_pas       ? &pas       :
		tab == &scrolly_pas2      ? &pas2      :
		tab == &scrolly_electrics ? &electrics :
		tab == &scrolly_loks      ? &loks      :
		&waggons;
	int x = get_mouse_x();
	int y = get_mouse_y();
	double resale_value = -1.0;
	const vehicle_desc_t *veh_type = NULL;
	bool new_vehicle_length_sb_force_zero = false;
	scr_coord relpos = scr_coord( 0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y() );
	int sel_index = lst->index_at(pos + tabs.get_pos() - relpos, x, y - D_TAB_HEADER_HEIGHT);

	if ((sel_index != -1) && (tabs.getroffen(x-pos.x,y-pos.y))) {
		// cursor over a vehicle in the selection list
		const vector_tpl<gui_image_list_t::image_data_t*>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &pas2 ? pas2_vec : (lst == &loks ? loks_vec : waggons_vec))));
		veh_type = vehicle_builder_t::get_info(vec[sel_index]->text);
		if(  vec[sel_index]->lcolor == COL_RED  ||  veh_action == va_sell  ) {
			// don't show new_vehicle_length_sb when can't actually add the highlighted vehicle, or selling from inventory
			new_vehicle_length_sb_force_zero = true;
		}
		if (depot_frame && vec[sel_index]->count > 0) {
			resale_value = depot_frame->calc_sale_value(veh_type);
		}
		if(vec[sel_index]->lcolor == COL_GREY3)
		{
			lb_too_heavy_notice.set_visible(true);
		}
		else
		{
			lb_too_heavy_notice.set_visible(false);
		}
		// update tile occupancy bar
		new_vehicle_length = new_vehicle_length_sb_force_zero ? 0 : veh_type->get_length();
		tile_occupancy.set_new_veh_length(new_vehicle_length, veh_action == va_insert ? true : false);
	}
	else {
		// cursor over a vehicle in the convoi
		sel_index = convoi.index_at(pos , x, y );
		if(sel_index != -1) {
			if (depot_frame) 
			{
				convoihandle_t cnv = depot_frame->get_convoy();
				if(cnv.is_bound())
				{
					veh_type = cnv->get_vehicle(sel_index)->get_desc();
					resale_value = cnv->get_vehicle(sel_index)->calc_sale_value();
					// update tile occupancy bar
					const uint8 new_last_veh_length = (sel_index == cnv->get_vehicle_count()-1 && cnv->get_vehicle_count() > 1) ?
						cnv->get_vehicle(cnv->get_vehicle_count() - 2)->get_desc()->get_length() :
						cnv->get_vehicle(cnv->get_vehicle_count() - 1)->get_desc()->get_length();
					tile_occupancy.set_new_veh_length(0 - veh_type->get_length(), false, new_last_veh_length);
				}
				else
				{
					resale_value = 0;
					veh_type = NULL;
				}
			}
		}
	}

	txt_vehicle_count.clear();
	if (depot_frame) 
	{
		switch(depot_frame->get_depot()->vehicle_count()) 
		{
			case 0:
				txt_vehicle_count.append(translator::translate("Keine Einzelfahrzeuge im Depot"));
				break;
			case 1:
				txt_vehicle_count.append(translator::translate("1 Einzelfahrzeug im Depot"));
				break;
			default:
				txt_vehicle_count.printf(translator::translate("%d Einzelfahrzeuge im Depot"), depot_frame->get_depot()->vehicle_count());
				break;
		}
	}

	if (veh_type) {
		// column 1
		vehicle_as_potential_convoy_t convoy(*veh_type);
		int linespace_skips = 0;

		// Name and traction type
		buf.printf("%s", translator::translate(veh_type->get_name(), welt->get_settings().get_name_language_id()));
		if (veh_type->get_power() > 0)
		{
			buf.printf(" (%s)", translator::translate(engine_type_names[veh_type->get_engine_type() + 1]));
		}
		buf.append("\n");

		// Cost information:
		char tmp[128];
		money_to_string(tmp, veh_type->get_value() / 100.0, false);
		char resale_entry[32] = "\0";
		if (resale_value != -1.0) {
			char tmp[128];
			money_to_string(tmp, resale_value / 100.0, false);
			sprintf(resale_entry, "(%s %8s)", translator::translate("Restwert:"), tmp);
		}
		else if (depot_frame && (veh_action == va_upgrade || show_all && veh_type->is_available_only_as_upgrade())) {
			char tmp[128];
			double upgrade_price = veh_type->get_upgrade_price();
			if (veh_type->is_available_only_as_upgrade() && !upgrade_price) {
				upgrade_price = veh_type->get_value();
			}
			money_to_string(tmp, upgrade_price / 100.0, false);
			sprintf(resale_entry, "(%s %8s)", translator::translate("Upgrade price:"), tmp);
		}
		buf.printf(translator::translate("Cost: %8s %s"), tmp, resale_entry);
		buf.append("\n");
		buf.printf(translator::translate("Maintenance: %1.2f$/km, %1.2f$/month\n"), veh_type->get_running_cost() / 100.0, veh_type->get_adjusted_monthly_fixed_cost(welt) / 100.0);
		buf.append("\n");

		// Physics information:
		buf.printf("%s %3d km/h\n", translator::translate("Max. speed:"), veh_type->get_topspeed());
		buf.printf("%s %4.1ft\n", translator::translate("Weight:"), veh_type->get_weight() / 1000.0); // Convert kg to tonnes
		if (veh_type->get_waytype() != water_wt)
		{
			buf.printf("%s %it\n", translator::translate("Axle load:"), veh_type->get_axle_load());
			char tmpbuf[16];
			const double reduced_way_wear_factor = veh_type->get_way_wear_factor() / 10000.0;
			number_to_string(tmpbuf, reduced_way_wear_factor, 4);
			buf.printf("%s: %s\n", translator::translate("Way wear factor"), tmpbuf);
		}
		buf.printf("%s %4.1fkN\n", translator::translate("Max. brake force:"), convoy.get_braking_force().to_double() / 1000.0); // Extended only
		buf.printf("%s %4.3fkN\n", translator::translate("Rolling resistance:"), veh_type->get_rolling_resistance().to_double() * (double)veh_type->get_weight() / 1000.0); // Extended only

		buf.printf("%s: ", translator::translate("Range"));
		if (veh_type->get_range() == 0)
		{
			buf.printf(translator::translate("unlimited"));
		}
		else
		{
			buf.printf("%i km", veh_type->get_range());
		}
		buf.append("\n");

		if (veh_type->get_waytype() == air_wt)
		{
			buf.printf("%s: %i m \n", translator::translate("Minimum runway length"), veh_type->get_minimum_runway_length());
		}
		buf.append("\n");

		linespace_skips = 0;

		// Engine information:
		linespace_skips = 0;
		if (veh_type->get_power() > 0)
		{ // LOCO
			sint32 friction = convoy.get_current_friction();
			sint32 max_weight = convoy.calc_max_starting_weight(friction);
			sint32 min_speed = convoy.calc_max_speed(weight_summary_t(max_weight, friction));
			sint32 min_weight = convoy.calc_max_weight(friction);
			sint32 max_speed = convoy.get_vehicle_summary().max_speed;
			if (min_weight < convoy.get_vehicle_summary().weight)
			{
				min_weight = convoy.get_vehicle_summary().weight;
				max_speed = convoy.calc_max_speed(weight_summary_t(min_weight, friction));
			}
			buf.printf("%s:", translator::translate("Pulls"));
			buf.printf(
				min_speed == max_speed ? translator::translate(" %gt @ %d km/h ") : translator::translate(" %gt @ %dkm/h%s%gt @ %dkm/h")  /*" %g t @ %d km/h " : " %g t @ %d km/h %s %g t @ %d km/h"*/,
				min_weight * 0.001f, max_speed, translator::translate("..."), max_weight * 0.001f, min_speed);
			buf.append("\n");
			buf.printf(translator::translate("Power/tractive force (%s): %4d kW / %d kN\n"), translator::translate(engine_type_names[veh_type->get_engine_type() + 1]), veh_type->get_power(), veh_type->get_tractive_effort());
			if (veh_type->get_gear() != 64) // Do this entry really have to be here...??? If not, it should be skipped. Space is precious..
			{
				buf.printf("%s %0.2f : 1", translator::translate("Gear:"), veh_type->get_gear() / 64.0);
			}
			else
			{
				//linespace_skips++;
			}
			buf.append("\n");
		}
		else
		{
			buf.printf("%s ", translator::translate("unpowered"));
			linespace_skips =+ 2;
		}
		if (linespace_skips > 0)
		{
			for (int i = 0; i < linespace_skips; i++)
			{
				buf.append("\n");
			}		
		}
		linespace_skips = 0;
			
		// Copyright information:
		if (char const* const copyright = veh_type->get_copyright())
		{
			buf.printf(translator::translate("Constructed by %s"), copyright);
		}
		buf.append("\n");

		display_multiline_text( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_size().h + 31 + LINESPACE*1 + 4 + 16, buf, SYSCOL_TEXT);

		buf.clear();
		// column 2
		// Vehicle intro and retire information:
		linespace_skips = 0;
		uint8 lines = 0;
		int top = pos.y + tabs.get_pos().y + tabs.get_size().h + 31 + LINESPACE * 2 + 4 + 16;
		
		buf.printf( "%s %s\n",
			translator::translate("Intro. date:"),
			translator::get_year_month( veh_type->get_intro_year_month())
			);
		lines++;

		if(  veh_type->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12  )
		{
			buf.printf( "%s %s\n",
				translator::translate("Retire. date:"),
				translator::get_year_month( veh_type->get_retire_year_month())
			);
		}
		else 
		{
			buf.append("\n");
		}
		buf.append("\n");
		lines += 2;

		display_multiline_text(pos.x + 335/*370*/, top, buf, SYSCOL_TEXT);

		buf.clear();
		top += lines * LINESPACE;
		lines = 0;

		// Capacity information:
		linespace_skips = 0;
		if (veh_type->get_total_capacity() > 0)
		{
			bool pass_veh = veh_type->get_freight_type()->get_catg_index() == goods_manager_t::INDEX_PAS;
			bool mail_veh = veh_type->get_freight_type()->get_catg_index() == goods_manager_t::INDEX_MAIL;

			int left = display_proportional(pos.x + 335/*370*/, top, translator::translate("Capacity:"), ALIGN_LEFT, SYSCOL_TEXT, true);
			buf.clear();
			// Category symbol:
			display_color_img(veh_type->get_freight_type()->get_catg_symbol(), pos.x + 335 + left + 5, top, 0, false, false);
			left += 14; // icon width + margin

			if (pass_veh || mail_veh)
			{
				uint8 classes_amount = veh_type->get_number_of_classes() < 1 ? 1 : veh_type->get_number_of_classes();
				char extra_pass[8];
				if (veh_type->get_overcrowded_capacity() > 0)
				{
					sprintf(extra_pass, "(%i)", veh_type->get_overcrowded_capacity());
				}
				else
				{
					extra_pass[0] = '\0';
				}

				buf.printf("  %3d %s%s %s\n",
					veh_type->get_total_capacity(), extra_pass,
					translator::translate(veh_type->get_freight_type()->get_mass()),
					translator::translate(veh_type->get_freight_type()->get_catg_name()));
				display_proportional(pos.x + 335+left, top, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				buf.clear();
				top += LINESPACE;
				lines++;

				for (uint8 i = 0; i < classes_amount; i++)
				{
					if (veh_type->get_capacity(i) > 0)
					{
						char class_name_untranslated[32];
						if (mail_veh)
						{
							sprintf(class_name_untranslated, "m_class[%u]", i);
						}
						else
						{
							sprintf(class_name_untranslated, "p_class[%u]", i);
						}
						const char* class_name = translator::translate(class_name_untranslated);

						buf.printf("%s: %3d %s %s", class_name, veh_type->get_capacity(i), translator::translate(veh_type->get_freight_type()->get_mass()), translator::translate(veh_type->get_freight_type()->get_name()));
						buf.append("\n");
						lines++;

						if (pass_veh)
						{
							char timebuf[32];
							uint8 base_comfort = veh_type->get_comfort(i);
							uint8 modified_comfort = 0;
							if (i >= veh_type->get_catering_level())
							{ 
								modified_comfort = veh_type->get_catering_level() > 0 ? veh_type->get_adjusted_comfort(veh_type->get_catering_level(), i) - base_comfort : 0;
							}
							char extra_comfort[8];
							if (modified_comfort > 0)
							{
								sprintf(extra_comfort, "+%i", modified_comfort);
							}
							else
							{
								extra_comfort[0] = '\0';
							}

							buf.printf(" - %s %i", translator::translate("Comfort:"), base_comfort);
							welt->sprintf_time_secs(timebuf, sizeof(timebuf), welt->get_settings().max_tolerable_journey(base_comfort + modified_comfort));
							buf.printf("%s %s %s%s", extra_comfort, translator::translate("(Max. comfortable journey time: "), timebuf, ")\n");
						}
						else
						{
							linespace_skips++;
						}
						lines++;
					}

				}
			}
			else
			{
				buf.printf("  %3d %s%s %s\n",
					veh_type->get_total_capacity(),
					"\0",
					translator::translate(veh_type->get_freight_type()->get_mass()),
					translator::translate(veh_type->get_freight_type()->get_catg_name()));
				display_proportional(pos.x + 335 + left, top, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				buf.clear();
				top += LINESPACE;
				linespace_skips += 2;
				lines += 2;
			}


			char min_loading_time_as_clock[32];
			char max_loading_time_as_clock[32];
			//Loading time is only relevant if there is something to load.
			welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), veh_type->get_min_loading_time());
			welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), veh_type->get_max_loading_time());
			buf.printf("%s %s - %s \n", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
			lines++;

			if (veh_type->get_catering_level() > 0)
			{
				if (mail_veh)
				{
					//Catering vehicles that carry mail are treated as TPOs.
					buf.printf(translator::translate("This is a travelling post office"));
					buf.append("\n");
				}
				else
				{
					buf.printf(translator::translate("Catering level: %i"), veh_type->get_catering_level());
					buf.append("\n");
				}
			}
			else
			{
				linespace_skips++;
			}
			lines++;

		}
		else
		{
			buf.printf("%s ", translator::translate("this_vehicle_carries_no_good"));
			linespace_skips += 3;
			lines += 3;
		}
		if (linespace_skips > 0)
		{
			for (int i = 0; i < linespace_skips; i++)
			{
				buf.append("\n");
			}
			linespace_skips = 0;
		}

		// Permissive way constraints
		// (If vehicle has, way must have)
		// @author: jamespetts
		const way_constraints_t &way_constraints = veh_type->get_way_constraints();
		for (uint8 i = 0; i < way_constraints.get_count(); i++)
		{
			if (way_constraints.get_permissive(i))
			{
				buf.printf("%s", translator::translate("\nMUST USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Permissive %i-%i", veh_type->get_waytype(), i);
				buf.printf("%s", translator::translate(tmpbuf));
				lines++;
			}
		}
		if (veh_type->get_is_tall())
		{
			buf.printf("%s", translator::translate("\nMUST USE: "));
			buf.printf("%s", translator::translate("high_clearance_under_bridges_(no_low_bridges)"));
			lines++;
		}


		// Prohibitibve way constraints
		// (If way has, vehicle must have)
		// @author: jamespetts
		for (uint8 i = 0; i < way_constraints.get_count(); i++)
		{
			if (way_constraints.get_prohibitive(i))
			{
				buf.printf("%s", translator::translate("\nMAY USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Prohibitive %i-%i", veh_type->get_waytype(), i);
				buf.printf("%s", translator::translate(tmpbuf));
				lines++;
			}
		}
		if (veh_type->get_tilting())
		{
			buf.append("\n");
			buf.printf("%s: ", translator::translate("equipped_with"));
			buf.printf("%s", translator::translate("tilting_vehicle_equipment"));
			lines++;
		}

		//// TODO: (Upgrade information)
		//if (convoi.index_at(pos, x, y) != -1 && veh_type->get_upgrades_count() > 0)
		//{
		//	const uint16 month_now = welt->get_timeline_year_month();
		//	int amount_of_upgrades = 0;
		//	int max_display_of_upgrades = 3;
		//	for (int i = 0; i < veh_type->get_upgrades_count(); i++)
		//	{
		//		//if (veh_type->get_upgrades(i) && !veh_type->get_upgrades(i)->is_future(month_now) && (!veh_type->get_upgrades(i)->is_retired(month_now)))
		//		{
		//			amount_of_upgrades++;
		//		}
		//	}
		//	if (amount_of_upgrades > 0)
		//	{
		//		n += sprintf(buf + n, "%s:\n", translator::translate("this_vehicle_can_upgrade_to"));
		//		for (uint8 i = 0; i < min(veh_type->get_upgrades_count(), max_display_of_upgrades); i++)
		//		{
		//			//if (veh_type->get_upgrades(i) && !veh_type->get_upgrades(i)->is_future(month_now) && (!veh_type->get_upgrades(i)->is_retired(month_now)))
		//			{
		//				//money_to_string(tmp, veh_type->get_upgrades(i)->get_upgrade_price() / 100);
		//				//n += sprintf(buf + n, " - %s (%8s)\n", translator::translate(veh_type->get_upgrades(i)->get_name()), tmp);
		//				
		//			}
		//		}
		//		if (amount_of_upgrades > max_display_of_upgrades)
		//		{
		//			
		//			n += sprintf(buf + n, "+ %i %s\n", amount_of_upgrades - max_display_of_upgrades, translator::translate("additional_upgrades"));
		//			}
		//	}
		//}
		//else
		//{
		//	linespace_skips += 2;
		//}

		//
		//if (linespace_skips > 0)
		//{
		//	for (int i = 0; i < linespace_skips; i++)
		//	{
		//		n += sprintf(buf + n, "\n");
		//	}
		//}

		// livery counter and avairavle livery scheme list
		if (veh_type->get_livery_count() > 0) {
			// display the available number of liveries
			txt_livery_count.clear();
			txt_livery_count.printf("(%i)", veh_type->get_available_livery_count(welt));
			lb_livery_counter.set_text(txt_livery_count);

			// which livery scheme this vehicle has? - this list is not displayed for convoy's vehicles being assembled
			if (veh_type->get_available_livery_count(welt) > 1 && convoi.index_at(pos, x, y) == -1) {
				const char* current_livery_text = NULL;
				if (livery_scheme_index) {
					livery_scheme_t* selected_scheme = welt->get_settings().get_livery_schemes()->get_element(livery_scheme_index);
					current_livery_text = selected_scheme->get_latest_available_livery(welt->get_timeline_year_month(), veh_type);
				}
				buf.append("\n\n");
				buf.printf("%s: \n", translator::translate("Selectable livery schemes"));
				lines++;

				FOR(vector_tpl<uint16>, const& i, livery_scheme_indices) {
					livery_scheme_t* scheme = welt->get_settings().get_livery_schemes()->get_element(i);
					if (scheme->get_latest_available_livery(welt->get_timeline_year_month(), veh_type))
					{
						//check the default livery
						if (!current_livery_text) {
							current_livery_text = strcmp(scheme->get_latest_available_livery(welt->get_timeline_year_month(), veh_type), "default") ?
								scheme->get_latest_available_livery(welt->get_timeline_year_month(), veh_type) : NULL;
						}
						if (livery_scheme_index == i || (current_livery_text && scheme->is_contained(current_livery_text, welt->get_timeline_year_month()))) {
							buf.append("*");
						}
						else {
							buf.append("-");
						}
						buf.printf(" %s\n", translator::translate(scheme->get_name()));
						lines++;
					}
					if (lines > 13) {
						buf.append(translator::translate("..."));
						break;
					}
				}
			}
		}
		else {
			lb_livery_counter.set_text(NULL);
		}

		display_multiline_text(pos.x + 335/*370*/, top, buf, SYSCOL_TEXT);
	}
	else {
		new_vehicle_length = 0;
		tile_occupancy.set_new_veh_length(new_vehicle_length);
		lb_livery_counter.set_text(NULL);
	}

	POP_CLIP();
}



void gui_convoy_assembler_t::set_vehicles(convoihandle_t cnv)
{
	clear_convoy();
	if (cnv.is_bound()) {
		for (uint8 i=0; i<cnv->get_vehicle_count(); i++) {
			vehicles.append(cnv->get_vehicle(i)->get_desc());
		}
	}
}


void gui_convoy_assembler_t::set_vehicles(const vector_tpl<const vehicle_desc_t *>* vv)
{
	vehicles.clear();
	vehicles.resize(vv->get_count());  // To save some memory
	for (uint16 i=0; i<vv->get_count(); ++i) {
		vehicles.append((*vv)[i]);
	}
}


bool gui_convoy_assembler_t::infowin_event(const event_t *ev)
{
	gui_container_t::infowin_event(ev);

	if(IS_LEFTCLICK(ev) &&  !action_selector.getroffen(ev->cx, ev->cy-16)) {
		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		action_selector.close_box();
		return true;
	}

	return false;
}

void gui_convoy_assembler_t::set_panel_rows(int dy)
{
	if (dy==-1) {
		uint16 default_grid_y = 24 + 6; // grid.y of ships in pak64.
		panel_rows=(7 * default_grid_y) / grid.y; // 7 rows of cars in pak64 fit at the screen (initial screen size).
		panel_rows = clamp( panel_rows, 2, 3 ); // Not more then 3 rows and at least 2.
		return;
	}
	dy -= get_convoy_height() + convoy_tabs_skip + 8 + get_vinfo_height() + 17 + D_TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER;
	panel_rows = max(1, (dy/grid.y) );
}

void gui_convoy_assembler_t::set_electrified( bool ele )
{
	if( way_electrified != ele ) 
	{
		way_electrified = ele;
		build_vehicle_lists();
	}
};


depot_convoi_capacity_t::depot_convoi_capacity_t()
{
	total_pax = 0;
	total_standing_pax = 0;
	total_mail = 0;
	total_goods = 0;
}

//(total_pax, total_standing_pax, total_mail, total_goods, pax_classes, mail_classes)
void depot_convoi_capacity_t::set_totals(uint32 pax, uint32 standing_pax, uint32 mail, uint32 goods, uint8 pax_classes, uint8 mail_classes, int good_0, int good_1, int good_2, int good_3, int good_4, uint32 good_0_amount, uint32 good_1_amount, uint32 good_2_amount, uint32 good_3_amount, uint32 good_4_amount, uint32 rest_good, uint8 catering, bool tpo)
{
	total_pax = pax;
	total_standing_pax = standing_pax;
	total_mail = mail;
	total_goods = goods;
	total_pax_classes =  pax_classes ;
	total_mail_classes =  mail_classes ;

	good_type_0 = good_0;
	good_type_1 = good_1;
	good_type_2 = good_2;
	good_type_3 = good_3;
	good_type_4 = good_4;

	good_type_0_amount = good_0_amount;
	good_type_1_amount = good_1_amount;
	good_type_2_amount = good_2_amount;
	good_type_3_amount = good_3_amount;
	good_type_4_amount = good_4_amount;
	rest_good_amount = rest_good;

	highest_catering = catering;
	is_tpo = tpo;
}


void depot_convoi_capacity_t::draw(scr_coord offset)
{
	cbuffer_t cbuf;

	int w_icon = 0;
	int w_text = 16;
	sint16 y = 0;
	char classes[32];

	if (total_pax > 0)
	{
		if (total_pax_classes >= 2)
		{
			sprintf(classes, "classes");
		}
		else
		{
			sprintf(classes, "class");
		}
		cbuf.clear();
		cbuf.printf("%d %s: %d (%d)", total_pax_classes, translator::translate(classes), total_pax, total_standing_pax);
		display_color_img(skinverwaltung_t::passengers->get_image_id(0), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
		display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
		y += LINESPACE + 1;
	}
	if (total_mail > 0)
	{
		if (total_mail_classes >= 2)
		{
			sprintf(classes, "classes");
		}
		else
		{
			sprintf(classes, "class");
		}
		cbuf.clear();
		cbuf.printf("%d %s: %d", total_mail_classes, translator::translate(classes), total_mail);
		display_color_img(skinverwaltung_t::mail->get_image_id(0), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
		display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
		y += LINESPACE + 1;
	}
	if (total_pax > 0 && highest_catering > 0 && total_goods == 0)
	{
		cbuf.clear();
		cbuf.printf(translator::translate("Catering level: %i"), highest_catering);
		display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
		y += LINESPACE + 1;
	}
	if (total_mail > 0 && is_tpo && total_goods == 0)
	{
		cbuf.clear();
		cbuf.printf("%s", translator::translate("traveling_post_office"));
		display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
		y += LINESPACE + 1;
	}
	if (total_goods > 0)
	{
		if (total_pax == 0 && total_mail == 0)
		{
			y = -LINESPACE - 1; // To make the text appear in line with the other text //Ves
		}
		if ((total_pax > 0 && total_mail > 0) && good_type_2 > 0)
		{
			cbuf.clear();
			cbuf.printf("%s: %d", translator::translate("good_units"), total_goods);
			display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
			display_color_img(skinverwaltung_t::goods->get_image_id(0), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
			y += LINESPACE + 1;
		}
		else
		{
			if (good_type_0 > 0)
			{
				cbuf.clear();
				cbuf.printf("%d%s %s", good_type_0_amount, translator::translate(goods_manager_t::get_info_catg_index(good_type_0)->get_mass()), translator::translate(goods_manager_t::get_info_catg_index(good_type_0)->get_catg_name()));
				display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
				display_color_img(goods_manager_t::get_info_catg_index(good_type_0)->get_catg_symbol(), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
				y += LINESPACE + 1;
			}
			if (good_type_1 > 0)
			{
				cbuf.clear();
				cbuf.printf("%d%s %s", good_type_1_amount, translator::translate(goods_manager_t::get_info_catg_index(good_type_1)->get_mass()), translator::translate(goods_manager_t::get_info_catg_index(good_type_1)->get_catg_name()));
				display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
				display_color_img(goods_manager_t::get_info_catg_index(good_type_1)->get_catg_symbol(), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
				y += LINESPACE + 1;
			}
			if (total_pax > 0 || total_mail > 0)
			{
				if (good_type_3 > 0)
				{
					cbuf.clear();
					cbuf.printf("%d %s", rest_good_amount + good_type_4_amount + good_type_3_amount + good_type_2_amount, translator::translate("more_units"));
					display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
					y += LINESPACE + 1;
				}
				else if (good_type_2 > 0)
				{
					cbuf.clear();
					cbuf.printf("%d%s %s", good_type_2_amount, translator::translate(goods_manager_t::get_info_catg_index(good_type_2)->get_mass()), translator::translate(goods_manager_t::get_info_catg_index(good_type_2)->get_catg_name()));
					display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
					display_color_img(goods_manager_t::get_info_catg_index(good_type_2)->get_catg_symbol(), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
					y += LINESPACE + 1;
				}
			}
			if (total_pax == 0 && total_mail == 0)
			{
				if (good_type_2 > 0)
				{
					cbuf.clear();
					cbuf.printf("%d%s %s", good_type_2_amount, translator::translate(goods_manager_t::get_info_catg_index(good_type_2)->get_mass()), translator::translate(goods_manager_t::get_info_catg_index(good_type_2)->get_catg_name()));
					display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
					display_color_img(goods_manager_t::get_info_catg_index(good_type_2)->get_catg_symbol(), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
					y += LINESPACE + 1;
				}
				if (good_type_3 > 0)
				{
					cbuf.clear();
					cbuf.printf("%d%s %s", good_type_3_amount, translator::translate(goods_manager_t::get_info_catg_index(good_type_3)->get_mass()), translator::translate(goods_manager_t::get_info_catg_index(good_type_3)->get_catg_name()));
					display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
					display_color_img(goods_manager_t::get_info_catg_index(good_type_3)->get_catg_symbol(), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
					y += LINESPACE + 1;
				}
				if (rest_good_amount > 0)
				{
					cbuf.clear();
					cbuf.printf("%d %s", rest_good_amount + good_type_4_amount, translator::translate("more_units"));
					display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
					y += LINESPACE + 1;
				}
				else if (good_type_4 > 0)
				{
					cbuf.clear();
					cbuf.printf("%d%s %s", good_type_4_amount, translator::translate(goods_manager_t::get_info_catg_index(good_type_4)->get_mass()), translator::translate(goods_manager_t::get_info_catg_index(good_type_4)->get_catg_name()));
					display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
					display_color_img(goods_manager_t::get_info_catg_index(good_type_4)->get_catg_symbol(), pos.x + offset.x + w_icon, pos.y + offset.y + y, 0, false, false);
					y += LINESPACE + 1;
				}
			}
		}
	}	
	if (total_pax == 0 && total_mail == 0 && total_goods == 0)
	{
		y = -LINESPACE - 1; // To make the text appear in line with the other text //Ves
		cbuf.clear();
		cbuf.printf(translator::translate("no_cargo_capacity"));
		display_proportional_clip(pos.x + offset.x + w_text, pos.y + offset.y + y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
		y += LINESPACE + 1;
	}
}
