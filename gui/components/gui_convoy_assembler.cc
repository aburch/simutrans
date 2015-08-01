/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <math.h>

#include "gui_convoy_assembler.h"

#include "../depot_frame.h"
#include "../replace_frame.h"

#include "../../simskin.h"
#include "../../simworld.h"
#include "../../simconvoi.h"
#include "../../convoy.h"

#include "../../bauer/warenbauer.h"
#include "../../bauer/vehikelbauer.h"
#include "../../besch/intro_dates.h"
#include "../../dataobj/replace_data.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/environment.h"
#include "../../dataobj/livery_scheme.h"
#include "../../utils/simstring.h"
#include "../../vehicle/simvehicle.h"
#include "../../besch/haus_besch.h"
#include "../../besch/vehikel_besch.h"
#include "../../player/simplay.h"

#include "../../utils/cbuffer_t.h"
#include "../../utils/for.h"

#include "../../dataobj/settings.h"

static const char * engine_type_names [9] =
{
  "unknown",
  "steam",
  "diesel",
  "electric",
  "bio",
  "sail",
  "fuel_cell",
  "hydrogene",
  "battery"
};

gui_convoy_assembler_t::gui_convoy_assembler_t(waytype_t wt, signed char player_nr, bool electrified) :
	way_type(wt), way_electrified(electrified), last_changed_vehicle(NULL),
	depot_frame(NULL), replace_frame(NULL), placement(get_placement(wt)),
	placement_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 4),
	grid(get_grid(wt)),
	grid_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 2),
	max_convoy_length(depot_t::get_max_convoy_length(wt)), panel_rows(3), convoy_tabs_skip(0),
	lb_convoi_count(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_speed(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_cost(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_value(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_power(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_weight(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_brake_force(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_rolling_resistance(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_way_wear_factor(NULL, COL_BLACK, gui_label_t::left),
	lb_traction_types(NULL, COL_BLACK, gui_label_t::left),
	lb_vehicle_count(NULL, COL_BLACK, gui_label_t::right),
	lb_veh_action("Fahrzeuge:", COL_BLACK, gui_label_t::left),
	lb_livery_selector("Livery scheme:", COL_BLACK, gui_label_t::left),
	lb_too_heavy_notice("too heavy", COL_RED, gui_label_t::left),
	convoi_pics(depot_t::get_max_convoy_length(wt)),
	convoi(&convoi_pics),
	pas(&pas_vec),
	electrics(&electrics_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&cont_pas),
	scrolly_electrics(&cont_electrics),
	scrolly_loks(&cont_loks),
	scrolly_waggons(&cont_waggons),
	lb_vehicle_filter("Filter:", COL_BLACK, gui_label_t::left)
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

	/*
	* [PANEL]
	* to test for presence, we must fist switch on all vehicles,
	* and switch off the timeline ...
	* otherwise the tabs will not be present at all
	*/
	bool old_retired=show_retired_vehicles;
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

	cont_electrics.add_component(&electrics);
	scrolly_electrics.set_show_scroll_x(false);
	scrolly_electrics.set_size_corner(false);
	// add only if there are any trolleybuses
	const uint8 shifter = 1 << vehikel_besch_t::electric;
	const bool correct_traction_type = !depot_frame || (shifter & depot_frame->get_depot()->get_tile()->get_besch()->get_enabled());
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
	add_component(&lb_vehicle_filter);

	veh_action = va_append;
	action_selector.add_listener(this);
	add_component(&action_selector);
	action_selector.clear_elements();
	static const char *txt_veh_action[3] = { "anhaengen", "voranstellen", "verkaufen" };
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[0]), COL_BLACK ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[1]), COL_BLACK ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[2]), COL_BLACK ) );
	action_selector.set_selection( 0 );

	upgrade = u_buy;
	upgrade_selector.add_listener(this);
	add_component(&upgrade_selector);
	upgrade_selector.clear_elements();
	static const char *txt_upgrade[2] = { "Buy/sell", "Upgrade" };
	upgrade_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(txt_upgrade[0]), COL_BLACK ) );
	upgrade_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(txt_upgrade[1]), COL_BLACK ) );
	upgrade_selector.set_selection(0);

	bt_obsolete.set_typ(button_t::square);
	bt_obsolete.set_text("Show obsolete");
	if(  welt->get_settings().get_allow_buying_obsolete_vehicles()  ) {
		bt_obsolete.add_listener(this);
		bt_obsolete.set_tooltip("Show also vehicles no longer in production.");
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
	vector_tpl<livery_scheme_t*>* schemes = welt->get_settings().get_livery_schemes();
	livery_scheme_indices.clear();
	ITERATE_PTR(schemes, i)
	{
		livery_scheme_t* scheme = schemes->get_element(i);
		if(scheme->is_available(welt->get_timeline_year_month()))
		{
			livery_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(scheme->get_name()), COL_BLACK));
			livery_scheme_indices.append(i);
			livery_selector.set_selection(i);
			livery_scheme_index = i;
		}
	}

	lb_convoi_count.set_text_pointer(txt_convoi_count);
	lb_convoi_speed.set_text_pointer(txt_convoi_speed);
	lb_convoi_cost.set_text_pointer(txt_convoi_cost);
	lb_convoi_value.set_text_pointer(txt_convoi_value);
	lb_convoi_power.set_text_pointer(txt_convoi_power);
	lb_convoi_weight.set_text_pointer(txt_convoi_weight);
	lb_convoi_brake_force.set_text_pointer(txt_convoi_brake_force);
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
	clear_ptr_vector(pas_vec);
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
	if (wt==water_wt) {
		return scr_coord(60,46);
	}
	if (wt==air_wt) {
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
	const scr_coord_val c3_x = c2_x + ((lb_size.w / 5) * 4) + D_V_SPACE;

	/*
	 * [CONVOI]
	 */
	convoi.set_grid(scr_coord(grid.x - grid_dx, grid.y));
	convoi.set_placement(scr_coord(placement.x - placement_dx, placement.y));
	convoi.set_pos(scr_coord((max(c1_x, size.w-get_convoy_image_width())/2), y));
	convoi.set_size(scr_size(get_convoy_image_width(), get_convoy_image_height()));
	y += get_convoy_image_height() + 1;

	lb_convoi_count.set_pos(scr_coord(c1_x, y));
	lb_convoi_count.set_size(lb_size);
	cont_convoi_capacity.set_pos(scr_coord(c2_x, y));
	cont_convoi_capacity.set_size(lb_size);
	lb_convoi_way_wear_factor.set_pos(scr_coord(c3_x, y));
	lb_convoi_way_wear_factor.set_size(lb_size); 
	y += LINESPACE + 1;
	lb_convoi_cost.set_pos(scr_coord(c1_x, y));
	lb_convoi_cost.set_size(lb_size);
	lb_convoi_value.set_pos(scr_coord(c2_x, y));
	lb_convoi_value.set_size(lb_size);
	y += LINESPACE + 1;
	lb_convoi_power.set_pos(scr_coord(c1_x, y));
	lb_convoi_power.set_size(lb_size);
	lb_convoi_weight.set_pos(scr_coord(c2_x, y));
	lb_convoi_weight.set_size(lb_size);
	y += LINESPACE + 1;
	lb_convoi_brake_force.set_pos(scr_coord(c1_x, y));
	lb_convoi_brake_force.set_size(lb_size);
	lb_convoi_rolling_resistance.set_pos(scr_coord(c2_x, y));
	lb_convoi_rolling_resistance.set_size(lb_size);
	y += LINESPACE + 1;
	lb_convoi_speed.set_pos(scr_coord(c1_x, y));
	lb_convoi_speed.set_size(sp_size);
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

	// header row

	lb_too_heavy_notice.set_pos(scr_coord(c1_x, y));
	lb_livery_selector.set_pos(scr_coord(column2_x, y));
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

	bt_obsolete.set_pos(scr_coord(c1_x, y));
	bt_obsolete.pressed = show_retired_vehicles;
	upgrade_selector.set_pos(scr_coord(column4_x, y));
	upgrade_selector.set_size(column4_size);
	upgrade_selector.set_max_size(scr_size(column4_size.w - 8, LINESPACE*2+2+16));
	upgrade_selector.set_highlight_color(1);
	y += 4 + D_BUTTON_HEIGHT;

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
		} else if (comp == &electrics) {
			image_from_storage_list(electrics_vec[p.i]);
		} else if(comp == &loks) {
			image_from_storage_list(loks_vec[p.i]);
		} else if(comp == &waggons) {
			image_from_storage_list(waggons_vec[p.i]);
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
				if(  (unsigned)(selection)<=va_sell  ) {
				veh_action=(unsigned)(selection);
				build_vehicle_lists();
				update_data();
				}
		} 
		
		else if(comp == &vehicle_filter) 
		{
			selected_filter = vehicle_filter.get_selection();
		} 
		
		else if(comp == &livery_selector)
		{
			sint32 livery_selection = p.i;
			if(livery_selection < 0) 
			{
				livery_selector.set_selection(0);
				livery_selection = 0;
			}
			livery_scheme_index = livery_scheme_indices.empty() ? 0 : livery_scheme_indices[livery_selection];
		} 

		else if(comp == &upgrade_selector) 
		{
			sint32 upgrade_selection = p.i;
			if ( upgrade_selection < 0 ) 
			{
				upgrade_selector.set_selection(0);
				upgrade_selection=0;
			}
			if(  (unsigned)(upgrade_selection)<=u_upgrade  ) 
			{
				upgrade=(unsigned)(upgrade_selection);
				build_vehicle_lists();
				update_data();
			}

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
	txt_convoi_power.clear();
	txt_convoi_speed.clear();
	txt_convoi_value.clear();
	txt_convoi_weight.clear();
	txt_convoi_brake_force.clear();
	txt_convoi_rolling_resistance.clear();
	txt_convoi_way_wear_factor.clear();
	cont_convoi_capacity.set_visible(!vehicles.empty());
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

		uint32 total_power = 0;
		uint32 total_force = 0;
		uint32 total_empty_weight = min_weight;
		uint32 total_max_weight = max_weight;
		uint32 total_min_weight = vsum.weight + convoy.get_freight_summary().max_freight_weight;
		uint32 total_cost = 0;
		uint32 maint_per_km = 0;
		uint32 maint_per_month = 0;
		double way_wear_factor = 0.0;

		for(  unsigned i = 0;  i < number_of_vehicles;  i++  ) {
			const vehikel_besch_t *besch = vehicles.get_element(i);
			const ware_besch_t* const ware = besch->get_ware();

			total_cost += besch->get_preis();
			total_power += besch->get_leistung();
			total_force += besch->get_tractive_effort();
 			maint_per_km += besch->get_running_cost();
 			maint_per_month += besch->get_adjusted_monthly_fixed_cost(welt);
			way_wear_factor += (double)besch->get_way_wear_factor();

			switch(  ware->get_catg_index()  ) {
				case warenbauer_t::INDEX_PAS: {
					total_pax += besch->get_zuladung();
					total_standing_pax += besch->get_overcrowded_capacity();
					break;
				}
				case warenbauer_t::INDEX_MAIL: {
					total_mail += besch->get_zuladung();
					break;
				}
				default: {
					total_goods += besch->get_zuladung();
					break;
				}
			}
		}
		way_wear_factor /= 10000.0;
		cont_convoi_capacity.set_totals( total_pax, total_standing_pax, total_mail, total_goods );

		txt_convoi_count.printf("%s %d (%s %i)",
			translator::translate("Fahrzeuge:"), vehicles.get_count(),
			translator::translate("Station tiles:"), vsum.tiles);

		txt_convoi_speed.append(translator::translate("Max. speed:"));
		int col_convoi_speed = COL_BLACK;
		if (min_speed == 0)
		{
			col_convoi_speed = COL_RED;
			if (convoy.get_starting_force() > 0)
			{
				txt_convoi_speed.printf(" %d km/h @ %g t: ", min_speed, max_weight * 0.001f);
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
		txt_convoi_speed.printf(
			min_speed == max_speed ? " %d km/h @ %g t" : " %d km/h @ %g t %s %d km/h @ %g t", 
			min_speed, max_weight * 0.001f,	translator::translate("..."), max_speed, min_weight * 0.001f);

		const sint32 brake_distance_min = convoy.calc_min_braking_distance(weight_summary_t(min_weight, friction), kmh2ms * max_speed);
		const sint32 brake_distance_max = convoy.calc_min_braking_distance(weight_summary_t(max_weight, friction), kmh2ms * max_speed);
		txt_convoi_speed.printf(
			brake_distance_min == brake_distance_max ? translator::translate("; brakes from max. speed in %i m") : translator::translate("; brakes from max. speed in %i - %i m"), 
			brake_distance_min, brake_distance_max);
		lb_convoi_speed.set_color(col_convoi_speed);

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
		txt_convoi_power.printf( translator::translate("Power: %4d kW, %d kN\n"), total_power, total_force);

		txt_convoi_brake_force.clear();
		txt_convoi_brake_force.printf("%s %4.1fkN\n", translator::translate("Max. brake force:"), convoy.get_braking_force().to_double() / 1000.0);

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
		txt_convoi_way_wear_factor.clear();
		txt_convoi_way_wear_factor.printf("%s: %.4f", translator::translate("Way wear factor"), way_wear_factor);
	}

	bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
	bt_show_all.pressed = show_all;					// otherwise the button would not show depressed
	draw_vehicle_info_text(parent_pos+pos);
	gui_container_t::draw(parent_pos);
}



// add all current vehicles
void gui_convoy_assembler_t::build_vehicle_lists()
{
	if(vehikelbauer_t::get_info(way_type).empty()) 
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

	if(electrics_vec.empty()  &&  pas_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty()) 
	{
		/*
		 * The next block calculates upper bounds for the sizes of the vectors.
		 * If the vectors get resized, the vehicle_map becomes invalid, therefore
		 * we need to resize them before filling them.
		 */
		if(electrics_vec.empty()  &&  pas_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty())
		{
			int loks = 0, waggons = 0, pax=0, electrics = 0;
			FOR(slist_tpl<vehikel_besch_t *>, const info, vehikelbauer_t::get_info(way_type)) 
			{
				if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)) 
				{
					electrics++;
				}
				else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) 
				{
					pax++;
				}
				else if(info->get_leistung() > 0  ||  info->get_zuladung()==0) 
				{
					loks++;
				}
				else 
				{
					waggons++;
				}
			}
			pas_vec.resize(pax);
			electrics_vec.resize(electrics);
			loks_vec.resize(loks);
			waggons_vec.resize(waggons);
		}
	}
	pas_vec.clear();
	electrics_vec.clear();
	loks_vec.clear();
	waggons_vec.clear();

	vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification (way_electrified)

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell && depot_frame) {
		// just list the one to sell
		FOR(slist_tpl<vehicle_t*>, const v, depot_frame->get_depot()->get_vehicle_list()) {
			vehikel_besch_t const* const d = v->get_besch();
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

		FOR(slist_tpl<vehikel_besch_t *>, const info, vehikelbauer_t::get_info(way_type)) 
		{
			const vehikel_besch_t *veh = NULL;
			if(vehicles.get_count()>0) {
				veh = (veh_action == va_insert) ? vehicles[0] : vehicles[vehicles.get_count() - 1];
			}

			// current vehicle
			if( (depot_frame && depot_frame->get_depot()->is_contained(info))  ||
				((way_electrified  ||  info->get_engine_type()!=vehikel_besch_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) 
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
					if(upgrade == u_upgrade)
					{
						vector_tpl<const vehikel_besch_t*> vehicle_list;
						upgradeable = false;

						if(replace_frame == NULL)
						{
							FOR(vector_tpl<const vehikel_besch_t*>, vehicle, vehicles)
							{
								vehicle_list.append(vehicle);
							}
						}
						else
						{
							const convoihandle_t cnv = replace_frame->get_convoy();
						
							for(uint8 i = 0; i < cnv->get_vehikel_anzahl(); i ++)
							{
								vehicle_list.append(cnv->get_vehikel(i)->get_besch());
							}
						}
						
						if(vehicle_list.get_count() < 1)
						{
							break;
						}

						FOR(vector_tpl<const vehikel_besch_t*>, vehicle, vehicle_list)
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
								const uint8 count = cnv->get_vehikel_anzahl();
								for(uint8 i = 0; i < count; i++)
								{
									if(cnv->get_vehikel(i)->get_besch() == info)
									{
										append = true;
										break;
									}
								}
							}
						}
					}
					const uint8 shifter = 1 << info->get_engine_type();
					const bool correct_traction_type = !depot_frame || (shifter & depot_frame->get_depot()->get_tile()->get_besch()->get_enabled());
					const weg_t* way = depot_frame ? welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype()) : NULL;
					const bool correct_way_constraint = !way || missing_way_constraints_t(info->get_way_constraints(), way->get_way_constraints()).check_next_tile();
					if(!correct_way_constraint || (!correct_traction_type && (info->get_leistung() > 0 || (veh_action == va_insert && info->get_vorgaenger_count() == 1 && info->get_vorgaenger(0) && info->get_vorgaenger(0)->get_leistung() > 0))))
					{
						append = false;
					}
				}
				if((append && upgrade == u_buy) || (upgradeable &&  upgrade == u_upgrade))
				{
					add_to_vehicle_list( info );
				}
			}
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
void gui_convoy_assembler_t::add_to_vehicle_list(const vehikel_besch_t *info)
{
	// Prevent multiple instances of the same vehicle
	if(vehicle_map.is_contained(info))
	{
		return;
	}

	// Check if vehicle should be filtered
	const ware_besch_t *freight = info->get_ware();
	// Only filter when required and never filter engines
	if (selected_filter > 0 && info->get_zuladung() > 0) 
	{
		if (selected_filter == VEHICLE_FILTER_RELEVANT) 
		{
			if(freight->get_catg_index() >= 3) 
			{
				bool found = false;
				FOR(vector_tpl<ware_besch_t const*>, const i, welt->get_goods_list()) 
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
				const ware_besch_t *selected_good = welt->get_goods_list()[goods_index];
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

	if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)  ) {
		electrics_vec.append(img_data);
		vehicle_map.set(info, electrics_vec.back());
	}
	// since they come "pre-sorted" for the vehikelbauer, we have to do nothing to keep them sorted
	else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) {
		pas_vec.append(img_data);
		vehicle_map.set(info, pas_vec.back());
	}
	else if(info->get_leistung() > 0  || (info->get_zuladung()==0  && (info->get_vorgaenger_count() > 0 || info->get_nachfolger_count() > 0)))
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
		if(cnv.is_bound() &&  nr<cnv->get_vehikel_anzahl() ) {

			// we remove all connected vehicles together!
			// find start
			unsigned start_nr = nr;
			while(start_nr>0) {
				start_nr --;
				const vehikel_besch_t *info = cnv->get_vehikel(start_nr)->get_besch();
				if(info->get_nachfolger_count()!=1) {
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



void gui_convoy_assembler_t::image_from_storage_list(gui_image_list_t::image_data_t* bild_data)
{
	
	const vehikel_besch_t *info = vehikelbauer_t::get_info(bild_data->text);

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
		if(bild_data->lcolor != COL_RED &&
			bild_data->rcolor != COL_RED &&
			bild_data->rcolor != COL_GREY3 &&
			bild_data->lcolor != COL_GREY3 &&
			bild_data->rcolor != COL_DARK_PURPLE &&
			bild_data->lcolor != COL_DARK_PURPLE &&
			bild_data->rcolor != COL_PURPLE &&
			bild_data->lcolor != COL_PURPLE &&
			!((bild_data->lcolor == COL_DARK_ORANGE || bild_data->rcolor == COL_DARK_ORANGE)
			&& veh_action != va_sell
			&& depot_frame != NULL && !depot_frame->get_depot()->find_oldest_newest(info, true))) 
		{
			// Dark orange = too expensive
			// Purple = available only as upgrade
			// Grey = too heavy

			if(veh_action == va_sell)
			{
				depot->call_depot_tool( 's', convoihandle_t(), bild_data->text, livery_scheme_index );
			}
			else if(upgrade != u_upgrade)
			{
				depot->call_depot_tool( veh_action == va_insert ? 'i' : 'a', cnv, bild_data->text, livery_scheme_index );
			}
			else
			{
				depot->call_depot_tool( 'u', cnv, bild_data->text, livery_scheme_index );
			}
		}	
	}
	else
	{
		if(bild_data->lcolor != COL_RED &&
			bild_data->rcolor != COL_RED &&
			bild_data->rcolor != COL_GREY3 &&
			bild_data->lcolor != COL_GREY3 &&
			bild_data->rcolor != COL_DARK_PURPLE &&
			bild_data->lcolor != COL_DARK_PURPLE &&
			bild_data->rcolor != COL_PURPLE &&
			bild_data->lcolor != COL_PURPLE &&
			!((bild_data->lcolor == COL_DARK_ORANGE || bild_data->rcolor == COL_DARK_ORANGE)
			&& veh_action != va_sell
			/*&& depot_frame != NULL && !depot_frame->get_depot()->find_oldest_newest(info, true)*/)) 
		{
			//replace_frame->replace.add_vehicle(info);
			if(upgrade == u_upgrade)
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

	const vehikel_besch_t *veh = NULL;

	convoi_pics.clear();
	if(vehicles.get_count() > 0) {

		unsigned i;
		for(i=0; i<vehicles.get_count(); i++) {

			// just make sure, there is this vehicle also here!
			const vehikel_besch_t *info=vehicles[i];
			if(vehicle_map.get(info)==NULL) {
				add_to_vehicle_list( info );
			}

			bool vehicle_available = false;
			image_id image;
			if(depot_frame)
			{
				FOR(slist_tpl<vehicle_t *>, const& iter, depot_frame->get_depot()->get_vehicle_list())
				{
					if(iter->get_besch() == info)
					{
						vehicle_available = true;
						image = info->get_base_image(iter->get_current_livery());
						break;
					}
				}

				for(unsigned int i = 0; i < depot_frame->get_depot()->convoi_count(); i ++)
				{
					convoihandle_t cnv = depot_frame->get_depot()->get_convoi(i);
					for(int n = 0; n < cnv->get_vehikel_anzahl(); n ++)
					{
						if(cnv->get_vehikel(n)->get_besch() == info)
						{
							vehicle_available = true;
							image = info->get_base_image(cnv->get_vehikel(n)->get_current_livery());
							// Necessary to break out of double loop.
							goto end;
						}
					}
				}
			}
			else if(replace_frame)
			{
				convoihandle_t cnv = replace_frame->get_convoy();
				for(int n = 0; n < cnv->get_vehikel_anzahl(); n ++)
				{
					if(cnv->get_vehikel(n)->get_besch() == info)
					{
						vehicle_available = true;
						image = info->get_base_image(cnv->get_vehikel(n)->get_current_livery());
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
					convoi_pics[i]->lcolor = COL_DARK_BLUE;
				}
				if (convoi_pics[i]->rcolor == COL_DARK_GREEN) {
					convoi_pics[i]->rcolor = COL_DARK_BLUE;
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
		vehikel_besch_t const* const    info = i.key;
		gui_image_list_t::image_data_t& img  = *i.value;

		const uint8 ok_color = info->is_future(month_now) || info->is_retired(month_now) ? COL_DARK_BLUE : COL_DARK_GREEN;

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
				if(upgrade == u_buy)
				{
					if(!player->can_afford(info->get_preis()))
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
				if(depot_frame && (i.key->get_leistung() > 0 || veh_action == va_insert && i.key->get_vorgaenger_count() == 1 && i.key->get_vorgaenger(0) && i.key->get_vorgaenger(0)->get_leistung() > 0))
				{
					const uint8 traction_type = i.key->get_engine_type();
					const uint8 shifter = 1 << traction_type;
					if(!(shifter & depot_frame->get_depot()->get_tile()->get_besch()->get_enabled()))
					{
						// Do not allow purchasing of vehicle if depot is of the wrong type.
						img.lcolor = COL_RED;
						img.rcolor = COL_RED;
					}
				}
				const weg_t* way = depot_frame ? welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype()) : NULL;
				if(way && !missing_way_constraints_t(i.key->get_way_constraints(), way->get_way_constraints()).check_next_tile())
				{
					// Do not allow purchasing of vehicle if depot is on an incompatible way.
					img.lcolor = COL_RED;
					img.rcolor = COL_RED;
				} //(highest_axle_load * 100) / weight_limit > 110)
				if(way && 
					(welt->get_settings().get_enforce_weight_limits() == 2
						&& i.key->get_axle_load() > welt->lookup(depot_frame->get_depot()->get_pos())->get_weg(depot_frame->get_depot()->get_waytype())->get_max_axle_load())
					|| (welt->get_settings().get_enforce_weight_limits() == 3
						&& (i.key->get_axle_load() * 100) / way->get_max_axle_load() < 110))
					
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

		if(upgrade == u_upgrade)
		{
			//Check whether there are any vehicles to upgrade
			img.lcolor = COL_DARK_PURPLE;
			img.rcolor = COL_DARK_PURPLE;
			vector_tpl<const vehikel_besch_t*> vehicle_list;

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
			//	for(uint8 i = 0; i < cnv->get_vehikel_anzahl(); i ++)
			//	{
			//		vehicle_list.append(cnv->get_vehikel(i)->get_besch());
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
					const uint8 count = cnv->get_vehikel_anzahl();
					for(uint8 i = 0; i < count; i++)
					{
						if(cnv->get_vehikel(i)->get_besch() == info)
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
			gui_image_list_t::image_data_t *imgdat=vehicle_map.get(iter2->get_besch());
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
		wt = replace_frame->get_convoy()->get_vehikel(0)->get_waytype();
	}
	else
	{
		wt = road_wt;
	}

	gui_component_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;

	cont_pas.add_component(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);
	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
		one = true;
	}

	cont_electrics.add_component(&electrics);
	scrolly_electrics.set_show_scroll_x(false);
	scrolly_electrics.set_size_corner(false);
	// add only if there are any trolleybuses
	const uint8 shifter = 1 << vehikel_besch_t::electric;
	const bool correct_traction_type = !depot_frame || (shifter & depot_frame->get_depot()->get_tile()->get_besch()->get_enabled());
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
	vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("All"), COL_BLACK));
	vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("Relevant"), COL_BLACK));

	FOR(vector_tpl<ware_besch_t const*>, const i, welt->get_goods_list()) {
		vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(i->get_name()), COL_BLACK));
	}

	if (selected_filter > vehicle_filter.count_elements()) {
		selected_filter = VEHICLE_FILTER_RELEVANT;
	}
	vehicle_filter.set_selection(selected_filter);
	vehicle_filter.set_highlight_color(depot_frame ? depot_frame->get_depot()->get_owner()->get_player_color1() + 1 : replace_frame ? replace_frame->get_convoy()->get_owner()->get_player_color1() + 1 : COL_BLACK);
}



void gui_convoy_assembler_t::draw_vehicle_info_text(const scr_coord& pos)
{
	char buf[1024];
	const scr_size size = depot_frame ? depot_frame->get_windowsize() : replace_frame->get_windowsize();
	PUSH_CLIP(pos.x, pos.y, size.w-1, size.h-1);

	gui_component_t const* const tab = tabs.get_aktives_tab();
	gui_image_list_t const* const lst =
		tab == &scrolly_pas       ? &pas       :
		tab == &scrolly_electrics ? &electrics :
		tab == &scrolly_loks      ? &loks      :
		&waggons;
	int x = get_maus_x();
	int y = get_maus_y();
	double resale_value = -1.0;
	const vehikel_besch_t *veh_type = NULL;
	bool new_vehicle_length_sb_force_zero = false;
	scr_coord relpos = scr_coord( 0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y() );
	int sel_index = lst->index_at(pos + tabs.get_pos() - relpos, x, y - TAB_HEADER_V_SIZE);

	if ((sel_index != -1) && (tabs.is_hit(x-pos.x,y-pos.y))) {
		// cursor over a vehicle in the selection list
		const vector_tpl<gui_image_list_t::image_data_t*>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec)));
		veh_type = vehikelbauer_t::get_info(vec[sel_index]->text);
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
					veh_type = cnv->get_vehikel(sel_index)->get_besch();
					resale_value = cnv->get_vehikel(sel_index)->calc_sale_value();
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

	buf[0]='\0';
	if(veh_type) {
		// column 1
		vehicle_as_potential_convoy_t convoy(*veh_type);

		int n = sprintf(buf, "%s", translator::translate(veh_type->get_name(),welt->get_settings().get_name_language_id()));

		if(  veh_type->get_leistung() > 0 && veh_type->get_waytype() != air_wt ) { // LOCO
			n += sprintf( buf + n, " (%s)\n", translator::translate( engine_type_names[veh_type->get_engine_type()+1] ) );
		}
		else {
			n += sprintf( buf + n, "\n");
		}

		if(veh_type->get_leistung() > 0) 
		{  
			// LOCO
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
			n += sprintf(buf + n, "%s:", translator::translate("Pulls")); 
			n += sprintf(buf + n,  
				min_speed == max_speed ? " %g t @ %d km/h " : " %g t @ %d km/h %s %g t @ %d km/h", 
				min_weight * 0.001f, max_speed, translator::translate("..."), max_weight * 0.001f, min_speed);
			n += sprintf( buf + n, "\n");
		}
		else {
			n += sprintf( buf + n, "\n");
		}
		
		char tmp[128];
		money_to_string( tmp, veh_type->get_preis() / 100.0, false );
		// These two lines differ from the Standard translation texts, as Standard does not have a monthly cost.
		n += sprintf( buf + n, translator::translate("Cost: %8s\n"), tmp);
		n += sprintf( buf + n, translator::translate("Maintenance: %1.2f$/km, %1.2f$/month\n"), veh_type->get_running_cost() / 100.0, veh_type->get_adjusted_monthly_fixed_cost(welt)/100.0);
		
		char cap[8];
		if(veh_type->get_overcrowded_capacity())
		{
			sprintf(cap, "(%d)", veh_type->get_overcrowded_capacity());
		}
		else
		{
			cap[0] = '\0';
		}
		if(  veh_type->get_zuladung() > 0  ) { // Standard translation is "Capacity: %3d%s %s\n", as Standard has no overcrowding
			n += sprintf(buf + n, translator::translate("Capacity: %3d %s%s %s\n"),
				veh_type->get_zuladung(),
				cap,
				translator::translate( veh_type->get_ware()->get_mass() ),
				veh_type->get_ware()->get_catg()==0 ? translator::translate( veh_type->get_ware()->get_name() ) : translator::translate( veh_type->get_ware()->get_catg_name() )
				);
		}
		else {
			n += sprintf( buf + n, "\n");
		}

		if(  veh_type->get_leistung() > 0  ) { // LOCO
			// Standard translation is "Power: %4d kW\n", as Standard has no tractive effort
			n += sprintf( buf + n, translator::translate("Power/tractive force: %4d kW / %d kN\n"), veh_type->get_leistung(), veh_type->get_tractive_effort());
		}
		else {
			n += sprintf( buf + n, "\n");
		}

		n += sprintf( buf + n, "%s %4.1ft\n", translator::translate("Weight:"), veh_type->get_gewicht() / 1000.0 ); // Convert kg to tonnes
		if(veh_type->get_waytype() != water_wt)
		{
			n += sprintf( buf + n, "%s %it\n", translator::translate("Axle load:"), veh_type->get_axle_load()); // Experimental only
		}
		else
		{
			n += sprintf( buf + n, "\n");
		}
		n += sprintf( buf + n, "%s %4.1fkN\n", translator::translate("Max. brake force:"), convoy.get_braking_force().to_double() / 1000.0); // Experimental only
		n += sprintf( buf + n, "%s %4.3fkN\n", translator::translate("Rolling resistance:"), veh_type->get_rolling_resistance().to_double() * (double)veh_type->get_gewicht() / 1000.0); // Experimental only
		
		n += sprintf( buf + n, "%s %3d km/h", translator::translate("Max. speed:"), veh_type->get_geschw() );

		// Permissive way constraints
		// (If vehicle has, way must have)
		// @author: jamespetts
		const way_constraints_t &way_constraints = veh_type->get_way_constraints();
		for(uint8 i = 0; i < way_constraints.get_count(); i++)
		{
			if(way_constraints.get_permissive(i))
			{
				n += sprintf( buf + n, "\n");
				n += sprintf(buf + n, "%s", translator::translate("\nMUST USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Permissive %i", i);
				n += sprintf(buf + n, "%s", translator::translate(tmpbuf));
			}
		}

		display_multiline_text( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_size().h + 31 + LINESPACE*1 + 4 + 16, buf,  COL_BLACK);

		// column 2
		n = sprintf( buf, "%s %s %04d\n",
			translator::translate("Intro. date:"),
			translator::get_month_name( veh_type->get_intro_year_month() % 12 ),
			veh_type->get_intro_year_month() / 12
			);

		if(  veh_type->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12  ) {
			n += sprintf( buf + n, "%s %s %04d\n",
				translator::translate("Retire. date:"),
				translator::get_month_name( veh_type->get_retire_year_month() % 12 ),
				veh_type->get_retire_year_month() / 12
				);
		}
		else {
			n += sprintf( buf + n, "\n");
		}

		if(veh_type->get_zuladung() > 0)
		{
			char min_loading_time_as_clock[32];
			char max_loading_time_as_clock[32];
			//Loading time is only relevant if there is something to load.
			welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), veh_type->get_min_loading_time());
			welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), veh_type->get_max_loading_time());
			n += sprintf(buf + n, "%s %s - %s \n", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
		}
		else
		{
			n += sprintf(buf + n, "\n");
		}

		if(veh_type->get_ware()->get_catg_index() == 0)
		{
			//Comfort only applies to passengers.
			uint8 comfort = veh_type->get_comfort();
			n += sprintf(buf + n, "%s %i ", translator::translate("Comfort:"), comfort);
			char timebuf[32];
			welt->sprintf_time_secs(timebuf, sizeof(timebuf), welt->get_settings().max_tolerable_journey(comfort) );
			n += sprintf(buf + n, "%s %s%s", translator::translate("(Max. comfortable journey time: "), timebuf, ")\n");
		}
		else 
		{
			n += sprintf(buf + n, "\n");
		}

		if(veh_type->get_catering_level() > 0)
		{
			if(veh_type->get_ware()->get_catg_index() == 1)
			{
				//Catering vehicles that carry mail are treated as TPOs.
				n +=  sprintf(buf + n, "%s", translator::translate("This is a travelling post office"));
			}
			else
			{
				n += sprintf(buf + n, translator::translate("Catering level: %i"), veh_type->get_catering_level());
				char timebuf[32];
				uint8 modified_comfort = veh_type->get_adjusted_comfort(veh_type->get_catering_level());
				welt->sprintf_time_secs(timebuf, sizeof(timebuf), welt->get_settings().max_tolerable_journey(modified_comfort) );
				n += sprintf(buf + n, " (%s: %i, %s)", translator::translate("Modified comfort"), modified_comfort, timebuf);
			}
			n += sprintf( buf + n, "\n");
		}

		if(veh_type->get_tilting())
		{
			n += sprintf(buf + n, "%s", translator::translate("This is a tilting vehicle\n"));
		}

		n += sprintf(buf + n, "%s: ", translator::translate("Range"));
		if(veh_type->get_range() == 0)
		{
			n += sprintf(buf + n, translator::translate("unlimited"));
		}
		else
		{
			n += sprintf(buf + n, "%i km", veh_type->get_range());
		}

		n += sprintf(buf + n, "\n");

		char tmpbuf[16];
		const double reduced_way_wear_factor = veh_type->get_way_wear_factor() / 10000.0;
		number_to_string(tmpbuf, reduced_way_wear_factor, 4);
		n += sprintf(buf + n, "%s: %s", translator::translate("Way wear factor"), tmpbuf);

		n += sprintf(buf + n, "\n");

		if(veh_type->get_waytype() == air_wt)
		{
			n += sprintf(buf + n, "%s: %i m \n", translator::translate("Minimum runway length"), veh_type->get_minimum_runway_length());
		}

		if(  veh_type->get_leistung() > 0  &&  veh_type->get_gear() != 64  ) {
			n += sprintf( buf + n, "%s %0.2f : 1\n", translator::translate("Gear:"), veh_type->get_gear() / 64.0 );
		}
		else {
			n += sprintf( buf + n, "\n");
		}

		if(  char const* const copyright = veh_type->get_copyright()  ) {
			n += sprintf( buf + n, translator::translate("Constructed by %s"), copyright );
		}
		n += sprintf( buf +  n, "\n");

		if(  resale_value != -1.0  ) {
			char tmp[128];
			money_to_string(  tmp, resale_value / 100.0, false );
			sprintf( buf + n, "%s %8s", translator::translate("Restwert:"), tmp );
		}

		
		// Prohibitibve way constraints
		// (If way has, vehicle must have)
		// @author: jamespetts
		for(uint8 i = 0; i < way_constraints.get_count(); i++)
		{
			if(way_constraints.get_prohibitive(i))
			{
				n += sprintf(buf + n, "%s", translator::translate("\nMAY USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Prohibitive %i", i);
				n += sprintf(buf + n, "%s", translator::translate(tmpbuf));
			}
		}

		display_multiline_text(pos.x + 370, pos.y + tabs.get_pos().y + tabs.get_size().h + 31 + LINESPACE * 2 + 4 + 16, buf, COL_BLACK);

		// update speedbar
		new_vehicle_length_sb = new_vehicle_length_sb_force_zero ? 0 : convoi_length_ok_sb + convoi_length_slower_sb + convoi_length_too_slow_sb + veh_type->get_length();
	}
	else {
		new_vehicle_length_sb = 0;
	}

	POP_CLIP();
}



void gui_convoy_assembler_t::set_vehicles(convoihandle_t cnv)
{
	clear_convoy();
	if (cnv.is_bound()) {
		for (uint8 i=0; i<cnv->get_vehikel_anzahl(); i++) {
			vehicles.append(cnv->get_vehikel(i)->get_besch());
		}
	}
}


void gui_convoy_assembler_t::set_vehicles(const vector_tpl<const vehikel_besch_t *>* vv)
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

	if(IS_LEFTCLICK(ev) &&  !action_selector.is_hit(ev->cx, ev->cy-16)) {
		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		action_selector.close_box();
		return true;
	}

	if(IS_LEFTCLICK(ev) &&  !upgrade_selector.is_hit(ev->cx, ev->cy-16)) {
		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		upgrade_selector.close_box();
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
	dy -= get_convoy_height() + convoy_tabs_skip + 8 + get_vinfo_height() + 17 + TAB_HEADER_V_SIZE + 2 * gui_image_list_t::BORDER;
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


void depot_convoi_capacity_t::set_totals(uint32 pax, uint32 standing_pax, uint32 mail, uint32 goods)
{
	total_pax = pax;
	total_standing_pax = standing_pax;
	total_mail = mail;
	total_goods = goods;
}


void depot_convoi_capacity_t::draw(scr_coord offset)
{
	cbuffer_t cbuf;

	int w = 0;
	cbuf.clear();
	cbuf.printf("%s %d (%d)", translator::translate("Capacity:"), total_pax, total_standing_pax );
	w += display_proportional_clip( pos.x+offset.x + w, pos.y+offset.y , cbuf, ALIGN_LEFT, COL_BLACK, true);
	display_color_img( skinverwaltung_t::passagiere->get_bild_nr(0), pos.x + offset.x + w, pos.y + offset.y, 0, false, false);

	w += 16;
	cbuf.clear();
	cbuf.printf("%d", total_mail );
	w += display_proportional_clip( pos.x+offset.x + w, pos.y+offset.y, cbuf, ALIGN_LEFT, COL_BLACK, true);
	display_color_img( skinverwaltung_t::post->get_bild_nr(0), pos.x + offset.x + w, pos.y + offset.y, 0, false, false);

	w += 16;
	cbuf.clear();
	cbuf.printf("%d", total_goods );
	w += display_proportional_clip( pos.x+offset.x + w, pos.y+offset.y, cbuf, ALIGN_LEFT, COL_BLACK, true);
	display_color_img( skinverwaltung_t::waren->get_bild_nr(0), pos.x + offset.x + w, pos.y + offset.y, 0, false, false);
}
