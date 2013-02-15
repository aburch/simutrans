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
#include "../../besch/vehikel_besch.h"
#include "../../dataobj/replace_data.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/umgebung.h"
#include "../../dataobj/livery_scheme.h"
#include "../../utils/simstring.h"
#include "../../vehicle/simvehikel.h"
#include "../../besch/haus_besch.h"
#include "../../player/simplay.h"

#include "../../utils/cbuffer_t.h"


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

gui_convoy_assembler_t::gui_convoy_assembler_t(karte_t *w, waytype_t wt, signed char player_nr, bool electrified) :
	way_type(wt), way_electrified(electrified), welt(w), last_changed_vehicle(NULL),
	depot_frame(NULL), replace_frame(NULL), placement(get_placement(wt)),
	placement_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 4),
	grid(get_grid(wt)),
	grid_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 2),
	max_convoy_length(depot_t::get_max_convoy_length(wt)), panel_rows(3), convoy_tabs_skip(0),
	lb_convoi_count(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_speed(NULL, COL_BLACK, gui_label_t::left),
	lb_veh_action("Fahrzeuge:", COL_BLACK, gui_label_t::left),
	lb_livery_selector("Livery scheme:", COL_BLACK, gui_label_t::left),
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

	add_komponente(&convoi);
	add_komponente(&lb_convoi_count);
	add_komponente(&lb_convoi_speed);

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

	cont_pas.add_komponente(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);

	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
		one = true;
	}

	cont_electrics.add_komponente(&electrics);
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

	cont_loks.add_komponente(&loks);
	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, translator::translate( get_zieher_name(wt) ) );
		one = true;
	}

	cont_waggons.add_komponente(&waggons);
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

	add_komponente(&tabs);
	add_komponente(&div_tabbottom);
	add_komponente(&lb_veh_action);
	add_komponente(&lb_livery_selector);
	add_komponente(&lb_vehicle_filter);

	veh_action = va_append;
	action_selector.add_listener(this);
	add_komponente(&action_selector);
	action_selector.clear_elements();
	static const char *txt_veh_action[3] = { "anhaengen", "voranstellen", "verkaufen" };
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[0]), COL_BLACK ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[1]), COL_BLACK ) );
	action_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( translator::translate(txt_veh_action[2]), COL_BLACK ) );
	action_selector.set_selection( 0 );

	upgrade = u_buy;
	upgrade_selector.add_listener(this);
	add_komponente(&upgrade_selector);
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
		add_komponente(&bt_obsolete);
	}

	bt_show_all.set_typ(button_t::square);
	bt_show_all.set_text("Show all");
	bt_show_all.add_listener(this);
	bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
	add_komponente(&bt_show_all);

	vehicle_filter.set_highlight_color(depot_frame ? depot_frame->get_depot()->get_besitzer()->get_player_color1() + 1 : replace_frame ? replace_frame->get_convoy()->get_besitzer()->get_player_color1() + 1 : COL_BLACK);
	vehicle_filter.add_listener(this);
	add_komponente(&vehicle_filter);

	livery_selector.add_listener(this);
	add_komponente(&livery_selector);
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

	selected_filter = VEHICLE_FILTER_RELEVANT;

	replace_frame = NULL;
}


koord gui_convoy_assembler_t::get_placement(waytype_t wt)
{
	if (wt==road_wt) {
		return koord(-20,-25);
	}
	if (wt==water_wt) {
		return koord(-1,-11);
	}
	if (wt==air_wt) {
		return koord(-10,-23);
	}
	return koord(-25,-28);
}


koord gui_convoy_assembler_t::get_grid(waytype_t wt)
{
	if (wt==water_wt) {
		return koord(60,46);
	}
	if (wt==air_wt) {
		return koord(36,36);
	}
	return koord(24,24);
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

	/*
	 * [CONVOI]
	 */
	convoi.set_grid(koord(grid.x - grid_dx, grid.y));
	convoi.set_placement(koord(placement.x - placement_dx, placement.y));
	convoi.set_pos(koord((max(get_pos().x + 10, groesse.x-get_convoy_image_width())/2), 0));
	convoi.set_groesse(koord(get_convoy_image_width(), get_convoy_image_height()));
	

	sint16 CINFO_VSTART = get_convoy_image_height();
	lb_convoi_count.set_pos(koord(4, CINFO_VSTART));
	lb_convoi_speed.set_pos(koord(4, CINFO_VSTART + LINESPACE));

	/*
	* [PANEL]
	*/
	sint16 PANEL_VSTART = get_convoy_height() + convoy_tabs_skip + 8;
	tabs.set_pos(koord(0, PANEL_VSTART));
	tabs.set_groesse(koord(groesse.x, get_panel_height()));

	pas.set_grid(grid);
	pas.set_placement(placement);
	pas.set_groesse(tabs.get_groesse());
	pas.recalc_size();
	pas.set_pos(koord(1,1));
	cont_pas.set_groesse(pas.get_groesse());
	scrolly_pas.set_groesse(scrolly_pas.get_groesse());

	electrics.set_grid(grid);
	electrics.set_placement(placement);
	electrics.set_groesse(tabs.get_groesse());
	electrics.recalc_size();
	electrics.set_pos(koord(1,1));
	cont_electrics.set_groesse(electrics.get_groesse());
	scrolly_electrics.set_groesse(scrolly_electrics.get_groesse());

	loks.set_grid(grid);
	loks.set_placement(placement);
	loks.set_groesse(tabs.get_groesse());
	loks.recalc_size();
	loks.set_pos(koord(1,1));
	cont_loks.set_pos(koord(0,0));
	cont_loks.set_groesse(loks.get_groesse());
	scrolly_loks.set_groesse(scrolly_loks.get_groesse());

	waggons.set_grid(grid);
	waggons.set_placement(placement);
	waggons.set_groesse(tabs.get_groesse());
	waggons.recalc_size();
	waggons.set_pos(koord(1,1));
	cont_waggons.set_groesse(waggons.get_groesse());
	scrolly_waggons.set_groesse(scrolly_waggons.get_groesse());

	div_tabbottom.set_pos(koord(0,PANEL_VSTART+ get_panel_height()));
	div_tabbottom.set_groesse(koord(groesse.x,0));

	sint16 ABUTTON_WIDTH=96;
	sint16 ABUTTON_HEIGHT=14;
	lb_veh_action.set_pos(koord(groesse.x-ABUTTON_WIDTH, PANEL_VSTART + get_panel_height() + 4));

	action_selector.set_pos(koord(groesse.x-ABUTTON_WIDTH, PANEL_VSTART + get_panel_height() + 16));
	action_selector.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	action_selector.set_max_size(koord(ABUTTON_WIDTH - 8, LINESPACE*3+2+16));
	action_selector.set_highlight_color(1);

	upgrade_selector.set_pos(koord(groesse.x-ABUTTON_WIDTH, PANEL_VSTART + get_panel_height() + 34));
	upgrade_selector.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	upgrade_selector.set_max_size(koord(ABUTTON_WIDTH - 8, LINESPACE*2+2+16));
	upgrade_selector.set_highlight_color(1);

	bt_show_all.set_pos(koord(4, PANEL_VSTART + get_panel_height() + 34 ));
	bt_show_all.pressed = show_all;

	bt_obsolete.set_pos(koord(groesse.x-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + get_panel_height() + 44));
	bt_obsolete.pressed = show_retired_vehicles;

	lb_vehicle_filter.set_pos(koord(groesse.x - (ABUTTON_WIDTH*5)/2 + 4, PANEL_VSTART + get_panel_height() + 4));

	vehicle_filter.set_pos(koord(groesse.x - (ABUTTON_WIDTH*5)/2 + 4, PANEL_VSTART +  get_panel_height() + 16));
	vehicle_filter.set_groesse(koord(ABUTTON_WIDTH + 30, 14));
	vehicle_filter.set_max_size(koord(ABUTTON_WIDTH + 60, LINESPACE * 8));
	
	lb_livery_selector.set_pos(koord(groesse.x / 4, PANEL_VSTART + get_panel_height() + 4));

	livery_selector.set_pos(koord(groesse.x / 4, PANEL_VSTART + get_panel_height() + 16));
	livery_selector.set_groesse(koord((groesse.x / 5), ABUTTON_HEIGHT));
	livery_selector.set_max_size(koord(ABUTTON_WIDTH + 80, LINESPACE * 8));
	livery_selector.set_highlight_color(1);

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


bool gui_convoy_assembler_t::action_triggered( gui_action_creator_t *komp,value_t p)
{
	if(komp != NULL) {	// message from outside!
			// image lsit selction here ...
		if(komp == &convoi) {
			image_from_convoi_list( p.i );
			update_data();
		} else if(komp == &pas) {
			image_from_storage_list(&pas_vec[p.i]);
		} else if (komp == &electrics) {
			image_from_storage_list(&electrics_vec[p.i]);
		} else if(komp == &loks) {
			image_from_storage_list(&loks_vec[p.i]);
		} else if(komp == &waggons) {
			image_from_storage_list(&waggons_vec[p.i]);
		} else if(komp == &bt_obsolete) {
			show_retired_vehicles = (show_retired_vehicles == false);
			build_vehicle_lists();
			update_data();
		} else if(komp == &bt_show_all) {
			show_all = (show_all == false);
			build_vehicle_lists();
			update_data();
		} else if(komp == &action_selector) {
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
		
		else if(komp == &vehicle_filter) 
		{
			selected_filter = vehicle_filter.get_selection();
		} 
		
		else if(komp == &livery_selector)
		{
			sint32 livery_selection = p.i;
			if(livery_selection < 0) 
			{
				livery_selector.set_selection(0);
				livery_selection = 0;
			}
			livery_scheme_index = livery_scheme_indices.empty() ? 0 : livery_scheme_indices[livery_selection];
		} 

		else if(komp == &upgrade_selector) 
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

void gui_convoy_assembler_t::zeichnen(koord parent_pos)
{
	*txt_convoi_count = '\0';
	*txt_convoi_speed = '\0';
	if (vehicles.get_count()>0) {
		potential_convoy_t convoy(*welt, vehicles);
		const vehicle_summary_t &vsum = convoy.get_vehicle_summary();
		sint32 friction = convoy.get_current_friction();
		sint32 allowed_speed = vsum.max_speed;
		sint32 min_weight = vsum.weight;
		sint32 max_weight = min_weight + convoy.get_freight_summary().max_freight_weight;
		sint32 min_speed = convoy.calc_max_speed(weight_summary_t(max_weight, friction));
		sint32 max_speed = min_speed;
		uint32 txt_convoi_speed_offs = sprintf(txt_convoi_speed, "%s", translator::translate("Max. speed:"));
		int col_convoi_speed = COL_BLACK;
		if (min_speed == 0)
		{
			col_convoi_speed = COL_RED;
			if (convoy.get_starting_force() > 0)
			{
				txt_convoi_speed_offs += sprintf(txt_convoi_speed + txt_convoi_speed_offs, " %d km/h @ %g t: ", min_speed, max_weight * 0.001f);
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
					// show max weight, that can be pulled with max allowed speed
					min_weight = convoy.calc_max_weight(friction);
				}
			}
		}
		sprintf(txt_convoi_count, "%s %d (%s %i)",
			translator::translate("Fahrzeuge:"), vehicles.get_count(),
			translator::translate("Station tiles:"), vsum.tiles);
		txt_convoi_speed_offs += sprintf(txt_convoi_speed + txt_convoi_speed_offs,  
			min_speed == max_speed ? " %d km/h @ %g t" : " %d km/h @ %g t %s %d km/h @ %g t", 
			min_speed, max_weight * 0.001f,	translator::translate("..."), max_speed, min_weight * 0.001f);

		const sint32 brake_distance_min = convoy.calc_min_braking_distance(weight_summary_t(min_weight, friction), kmh2ms * max_speed);
		const sint32 brake_distance_max = convoy.calc_min_braking_distance(weight_summary_t(max_weight, friction), kmh2ms * max_speed);
		txt_convoi_speed_offs += sprintf(txt_convoi_speed + txt_convoi_speed_offs, 
			brake_distance_min == brake_distance_max ? translator::translate("; brakes from max. speed in %i m") : translator::translate("; brakes from max. speed in %i - %i m"), 
			brake_distance_min, brake_distance_max);
		lb_convoi_speed.set_color(col_convoi_speed);
	}

	bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
	bt_show_all.pressed = show_all;					// otherwise the button would not show depressed
	draw_vehicle_info_text(parent_pos+pos);
	gui_container_t::zeichnen(parent_pos);
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

	const uint16 month_now = get_welt()->get_timeline_year_month();

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
		FOR(slist_tpl<vehikel_t*>, const v, depot_frame->get_depot()->get_vehicle_list()) {
			vehikel_besch_t const* const d = v->get_besch();
			if (vehicle_map.get(d)) continue;
			add_to_vehicle_list(d);
		}
	}
	else {
		// list only matching ones

		if(depot_frame && umgebung_t::networkmode)
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
							ITERATE(vehicles,i)
							{
								vehicle_list.append(vehicles[i]);
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

						ITERATE(vehicle_list, i)
						{
							for(uint16 c = 0; c < vehicle_list[i]->get_upgrades_count(); c++)
							{
								if(vehicle_list[i]->get_upgrades(c) && (info == vehicle_list[i]->get_upgrades(c)))
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
					if(!correct_traction_type && info->get_leistung() > 0)
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
				FOR(vector_tpl<ware_besch_t const*>, const i, get_welt()->get_goods_list()) 
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
			if (goods_index < get_welt()->get_goods_list().get_count()) 
			{
				const ware_besch_t *selected_good = get_welt()->get_goods_list()[goods_index];
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
	gui_image_list_t::image_data_t img_data;

	if(livery_scheme_index >= welt->get_settings().get_livery_schemes()->get_count())
	{
		// To prevent errors when loading a game with fewer livery schemes than that just played.
		livery_scheme_index = 0;
	}

	const livery_scheme_t* const scheme = welt->get_settings().get_livery_scheme(livery_scheme_index);
	uint16 date = welt->get_timeline_year_month();
	if(scheme)
	{
		const char* livery = scheme->get_latest_available_livery(date, info);
		if(livery)
		{
			img_data.image = info->get_basis_bild(livery);
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
					img_data.image = info->get_basis_bild(new_livery);
					found = true;
					break;
				}
			}
			if(!found)
			{
				img_data.image =info->get_basis_bild();
			}
		}
	}
	else
	{
		img_data.image = info->get_basis_bild();
	}
	img_data.count = 0;
	img_data.lcolor = img_data.rcolor = EMPTY_IMAGE_BAR;
	img_data.text = info->get_name();

	if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)  ) {
		electrics_vec.append(img_data);
		vehicle_map.set(info, &electrics_vec.back());
	}
	// since they come "pre-sorted" for the vehikelbauer, we have to do nothing to keep them sorted
	else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) {
		pas_vec.append(img_data);
		vehicle_map.set(info, &pas_vec.back());
	}
	else if(info->get_leistung() > 0  || (info->get_zuladung()==0  && (info->get_vorgaenger_count() > 0 || info->get_nachfolger_count() > 0)))
	{
		loks_vec.append(img_data);
		vehicle_map.set(info, &loks_vec.back());
	}
	else {
		waggons_vec.append(img_data);
		vehicle_map.set(info, &waggons_vec.back());
	}
}



void gui_convoy_assembler_t::image_from_convoi_list(uint nr)
{
	depot_t* depot;
	if(depot_frame)
	{
		depot = depot_frame->get_depot();
	}
	else
	{
		const grund_t* gr = welt->lookup(replace_frame->get_convoy()->get_home_depot());
		depot = gr->get_depot();
	}

	if(depot_frame)
	{
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
		vehicles.remove_at(nr);
	}
}



void gui_convoy_assembler_t::image_from_storage_list(gui_image_list_t::image_data_t *bild_data)
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
	const uint16 month_now = get_welt()->get_timeline_year_month();

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

			gui_image_list_t::image_data_t img_data;

			bool vehicle_available = false;
			if(depot_frame)
			{
				FOR(slist_tpl<vehikel_t *>, const& iter, depot_frame->get_depot()->get_vehicle_list())
				{
					if(iter->get_besch() == info)
					{
						vehicle_available = true;
						img_data.image = info->get_basis_bild(iter->get_current_livery());
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
							img_data.image = info->get_basis_bild(cnv->get_vehikel(n)->get_current_livery());
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
						img_data.image = info->get_basis_bild(cnv->get_vehikel(n)->get_current_livery());
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
						img_data.image = vehicles[i]->get_basis_bild(livery);
					}
					else
					{
						bool found = false;
						for(int j = 0; j < welt->get_settings().get_livery_schemes()->get_count(); j ++)
						{
							const livery_scheme_t* const new_scheme = welt->get_settings().get_livery_scheme(j);
							const char* new_livery = new_scheme->get_latest_available_livery(date, vehicles[i]);
							if(new_livery)
							{
								img_data.image = vehicles[i]->get_basis_bild(new_livery);
								found = true;
								break;
							}
						}
						if(!found)
						{
							img_data.image = vehicles[i]->get_basis_bild();
						}
					}
				}
				else
				{
					img_data.image = vehicles[i]->get_basis_bild();
				}
			}
			img_data.count = 0;
			img_data.lcolor = img_data.rcolor= EMPTY_IMAGE_BAR;
			img_data.text = vehicles[i]->get_name();
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0].lcolor = vehicles[0]->can_follow(NULL) ? COL_DARK_GREEN : COL_YELLOW;
		for(  i=1;  i<vehicles.get_count(); i++) {
			convoi_pics[i - 1].rcolor = vehicles[i - 1]->can_lead(vehicles[i]) ? COL_DARK_GREEN : COL_RED;
			convoi_pics[i].lcolor     = vehicles[i]->can_follow(vehicles[i - 1]) ? COL_DARK_GREEN : COL_RED;
		}
		convoi_pics[i - 1].rcolor = vehicles[i - 1]->can_lead(NULL) ? COL_DARK_GREEN : COL_YELLOW;

		// change green into blue for retired vehicles
		for(i=0;  i<vehicles.get_count(); i++) {
			if(vehicles[i]->is_future(month_now) || vehicles[i]->is_retired(month_now)) {
				if (convoi_pics[i].lcolor == COL_DARK_GREEN) {
					convoi_pics[i].lcolor = COL_DARK_BLUE;
				}
				if (convoi_pics[i].rcolor == COL_DARK_GREEN) {
					convoi_pics[i].rcolor = COL_DARK_BLUE;
				}
			}
		}

		if(veh_action == va_insert) {
			veh = vehicles[0];
		} else if(veh_action == va_append) {
			veh = vehicles[vehicles.get_count() - 1];
		}
	}
	
	const spieler_t *sp = welt->get_active_player();
	
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
			//Check whether too expensive
			//@author: jamespetts
			if(img.lcolor == ok_color || img.lcolor == COL_YELLOW)
			{
				//Only flag as too expensive that which could be purchased anyway.
				if(upgrade == u_buy)
				{
					if(!sp->can_afford(info->get_preis()))
					{
						img.lcolor = COL_DARK_ORANGE;
						img.rcolor = COL_DARK_ORANGE;
					}
				}
				else
				{
					if(!sp->can_afford(info->get_upgrade_price()))
					{
						img.lcolor = COL_DARK_ORANGE;
						img.rcolor = COL_DARK_ORANGE;
					}
				}
				if(depot_frame && i.key->get_leistung() > 0)
				{
					const uint8 traction_type =i.key->get_engine_type();
					const uint8 shifter = 1 << traction_type;
					if(!(shifter & depot_frame->get_depot()->get_tile()->get_besch()->get_enabled()))
					{
						// Do not allow purchasing of vehicle if depot is of the wrong type.
						img.lcolor = COL_RED;
						img.rcolor = COL_RED;
					}
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
						if(!sp->can_afford(info->get_upgrade_price()))
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
							sint8 upgradeable_count = 0;
							ITERATE(vehicles,j)
							{
								if(vehicles[j] == info)
								{
									// Counts the number of vehicles in the current convoy that can
									// upgrade to the currently selected vehicle.
									upgradeable_count --;
								}
							}
							ITERATE(vehicle_list,j)
							{
								for(uint16 k = 0; k < vehicle_list[j]->get_upgrades_count(); k++)
								{
									if(vehicle_list[j]->get_upgrades(k) && (vehicle_list[j]->get_upgrades(k) == info))
									{
										// Counts the number of vehicles currently marked to be upgraded
										// to the selected vehicle.
										upgradeable_count ++;
									}
								}
							}

							if(upgradeable_count < 1)
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
		FOR(slist_tpl<vehikel_t *>, const& iter2, depot_frame->get_depot()->get_vehicle_list())
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

	gui_komponente_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;

	cont_pas.add_komponente(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);
	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
		one = true;
	}

	cont_electrics.add_komponente(&electrics);
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

	cont_loks.add_komponente(&loks);
	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, translator::translate( get_zieher_name(wt) ) );
		one = true;
	}

	cont_waggons.add_komponente(&waggons);
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

	FOR(vector_tpl<ware_besch_t const*>, const i, get_welt()->get_goods_list()) {
		vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(i->get_name()), COL_BLACK));
	}

	if (selected_filter > vehicle_filter.count_elements()) {
		selected_filter = VEHICLE_FILTER_RELEVANT;
	}
	vehicle_filter.set_selection(selected_filter);
	vehicle_filter.set_highlight_color(depot_frame ? depot_frame->get_depot()->get_besitzer()->get_player_color1() + 1 : replace_frame ? replace_frame->get_convoy()->get_besitzer()->get_player_color1() + 1 : COL_BLACK);
}



void gui_convoy_assembler_t::draw_vehicle_info_text(koord pos)
{
	char buf[1024];
	const char *c;
	const koord size = depot_frame ? depot_frame->get_fenstergroesse() : replace_frame->get_fenstergroesse();
	PUSH_CLIP(pos.x, pos.y, size.x-1, size.y-1);

	gui_komponente_t const* const tab = tabs.get_aktives_tab();
	gui_image_list_t const* const lst =
		tab == &scrolly_pas       ? &pas       :
		tab == &scrolly_electrics ? &electrics :
		tab == &scrolly_loks      ? &loks      :
		&waggons;
	int x = get_maus_x();
	int y = get_maus_y();
	sint64 value = -1;
	const vehikel_besch_t *veh_type = NULL;
	koord relpos = koord( 0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y() );
	int sel_index = lst->index_at(pos + tabs.get_pos() - relpos, x, y - gui_tab_panel_t::HEADER_VSIZE);

	if ((sel_index != -1) && (tabs.getroffen(x-pos.x,y-pos.y))) {
		const vector_tpl<gui_image_list_t::image_data_t>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec)));
		veh_type = vehikelbauer_t::get_info(vec[sel_index].text);
		if (depot_frame && vec[sel_index].count > 0) {
			value = depot_frame->get_depot()->calc_restwert(veh_type) / 100;
		}
	}
	else {
		sel_index = convoi.index_at(pos , x, y );
		if(sel_index != -1) {
			if (depot_frame) {
				convoihandle_t cnv = depot_frame->get_convoy();
				if(cnv.is_bound())
				{
					veh_type = cnv->get_vehikel(sel_index)->get_besch();
					value = cnv->get_vehikel(sel_index)->calc_restwert()/100;
				}
				else
				{
					value = 0;
					veh_type = NULL;
				}
			}
		}
	}

	buf[0]='\0';
	c=buf;
	if (depot_frame) 
	{
		switch(depot_frame->get_depot()->vehicle_count()) 
		{
			case 0:
				c = translator::translate("Keine Einzelfahrzeuge im Depot");
				break;
			case 1:
				c = translator::translate("1 Einzelfahrzeug im Depot");
				break;
			default:
				sprintf(buf, translator::translate("%d Einzelfahrzeuge im Depot"), depot_frame->get_depot()->vehicle_count());
				c = buf;
				break;
		}
	}

	display_proportional( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 16 + 4, c, ALIGN_LEFT, COL_BLACK, true );

	if(veh_type) {
		sint32 k = 0;

		uint16 brake_force = veh_type->get_brake_force();

		if(brake_force == BRAKE_FORCE_UNKNOWN)
		{
			float32e8_t br;
			switch(veh_type->get_waytype())
			{
				case air_wt:
					br = BR_AIR;
					break;

					case water_wt:
					br = BR_WATER;
					break;
	
				case track_wt:
				case narrowgauge_wt:
				case overheadlines_wt: 
					br = BR_TRACK;
					break;

				case tram_wt:
				case monorail_wt:      
					br = BR_TRAM;
					break;
		
				case maglev_wt:
					br = BR_MAGLEV;
					break;

				case road_wt:
					br = BR_ROAD;
					break;

				default:
					br = BR_DEFAULT;
					break;
			}
			brake_force = br * ((uint32) veh_type->get_gewicht());
		}

		// lok oder waggon ?
		if(veh_type->get_leistung() > 0) { //"Leistung" = performance (Google)
			//lok
			const sint32 zuladung = veh_type->get_zuladung(); //"Zuladung" = payload (Google)

			char name[512];
			sint32 n;

			if(veh_type->get_waytype() != air_wt)
			{
				n = sprintf(name, "%s (%s)",
				translator::translate(veh_type->get_name()),
				translator::translate(engine_type_names[veh_type->get_engine_type()+1]));
			}
			else
			{
				n = sprintf(name, "%s",
				translator::translate(veh_type->get_name()));
			}
			
			vehicle_as_potential_convoy_t convoy(*get_welt(), *veh_type);
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
			n += sprintf(name + n, "\n%s:", translator::translate("Pulls")); 
			n += sprintf(name + n,  
				min_speed == max_speed ? " %g t @ %d km/h " : " %g t @ %d km/h %s %g t @ %d km/h", 
				min_weight * 0.001f, max_speed, translator::translate("..."), max_weight * 0.001f, min_speed);
			
			if(upgrade == u_buy)
			{
				char cost[64];
				money_to_string( cost, veh_type->get_preis()/100.0 );
				n = sprintf(buf,
					translator::translate("LOCO_INFO_EXT"),
					name, cost,
					veh_type->get_running_cost()/100.0F,
					veh_type->get_adjusted_monthly_fixed_cost(get_welt())/100.0F,
					veh_type->get_leistung(),
					veh_type->get_tractive_effort(),
					veh_type->get_geschw(),
					veh_type->get_gewicht(),
					brake_force,
					veh_type->get_rolling_resistance().to_double() * 1000
					);
			}
			else
			{
				char cost[64];
				money_to_string( cost, veh_type->get_upgrade_price()/100.0 );
				n = sprintf(buf,
					translator::translate("LOCO_INFO_EXT"),
					name, cost,
					veh_type->get_running_cost()/100.0F,
					veh_type->get_adjusted_monthly_fixed_cost(get_welt())/100.0F,
					veh_type->get_leistung(),
					veh_type->get_tractive_effort(),
					veh_type->get_geschw(),
					veh_type->get_gewicht(),
					brake_force,
					veh_type->get_rolling_resistance().to_double() * 1000
					);
			}

			//"Payload" (Google)
			if(zuladung > 0 && veh_type->get_overcrowded_capacity() < 1) 
			{
				n += sprintf(buf + n,
					translator::translate("LOCO_CAP"),
					zuladung,
					translator::translate(veh_type->get_ware()->get_mass()),
					veh_type->get_ware()->get_catg() == 0 ? translator::translate(veh_type->get_ware()->get_name()) : translator::translate(veh_type->get_ware()->get_catg_name())
					);
			}
			else if(zuladung > 0 && veh_type->get_overcrowded_capacity() > 0)
			{
				n += sprintf(buf + n,
					translator::translate("Capacity: %d (%d)%s %s\n"),
					zuladung,
					veh_type->get_overcrowded_capacity(),
					translator::translate(veh_type->get_ware()->get_mass()),
					veh_type->get_ware()->get_catg() == 0 ? translator::translate(veh_type->get_ware()->get_name()) : translator::translate(veh_type->get_ware()->get_catg_name())
					);
			}
			
			k = n;

		}
		else {
			// waggon
			sint32 n;
			if(upgrade == u_buy)
			{
				if(veh_type->get_overcrowded_capacity() < 1)
				{
					char cost[64];
					money_to_string( cost, veh_type->get_preis()/100.0 );
					n = sprintf(buf,
						translator::translate("%s\nCost:     %s\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d%s %s\nWeight: %dt\nTop speed: %dkm/h\nMax. brake force: %dkN\nRolling resistance: %fN\n"),
						translator::translate(veh_type->get_name()),
						cost,
						veh_type->get_running_cost()/100.0F,
 						veh_type->get_adjusted_monthly_fixed_cost(get_welt())/100.0F,
						veh_type->get_zuladung(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw(),
						brake_force,
						veh_type->get_rolling_resistance().to_double() * 1000
						);
				}
				else
				{
					char cost[64];
					money_to_string( cost, veh_type->get_preis()/100.0 );
					n = sprintf(buf,
						translator::translate("%s\nCost:     %s\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d (%d)%s %s\nWeight: %dt\nTop speed: %dkm/h\nMax. brake force: %dkN\nRolling resistance: %fN\n"),
						translator::translate(veh_type->get_name()),
						cost,
						veh_type->get_running_cost()/100.0F,
 						veh_type->get_adjusted_monthly_fixed_cost(get_welt())/100.0F,
						veh_type->get_zuladung(),
						veh_type->get_overcrowded_capacity(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw(),
						brake_force,
						veh_type->get_rolling_resistance().to_double() * 1000
						);
				}
			}
			else
			{
				if(veh_type->get_overcrowded_capacity() < 1)
				{
					char cost[64];
					money_to_string( cost, veh_type->get_preis()/100.0 );
					n = sprintf(buf,
						translator::translate("%s\nCost:     %s\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d%s %s\nWeight: %dt\nTop speed: %dkm/h\n\nMax. brake force: %dkN\nRolling resistance: %fN\n"),
						translator::translate(veh_type->get_name()),
						cost,
						veh_type->get_running_cost()/100.0F,
						veh_type->get_adjusted_monthly_fixed_cost(get_welt())/100.0F,
						veh_type->get_zuladung(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw(),
						brake_force,
						veh_type->get_rolling_resistance().to_double() * 1000
						);
				}
				else
				{
					char cost[64];
					money_to_string( cost, veh_type->get_preis()/100.0 );
					n = sprintf(buf,
						translator::translate("%s\nCost:     %s\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d (%d)%s %s\nWeight: %dt\nTop speed: %dkm/h\n\nMax. brake force: %dkN\nRolling resistance: %fN\n"),
						translator::translate(veh_type->get_name()),
						cost,
						veh_type->get_running_cost()/100.0F,
						veh_type->get_adjusted_monthly_fixed_cost(get_welt())/100.0F,
						veh_type->get_zuladung(),
						veh_type->get_overcrowded_capacity(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw(),
						brake_force,
						veh_type->get_rolling_resistance().to_double() * 1000
						);
				}
			}

			k = n;
		}
				
		if(veh_type->get_catering_level() > 0)
		{
			if(veh_type->get_ware()->get_catg_index() == 1) 
			{
				//Catering vehicles that carry mail are treated as TPOs.
				k +=  sprintf(buf + k, "%s", translator::translate("This is a travelling post office"));
			}
			else
			{
				k += sprintf(buf + k, translator::translate("Catering level: %i"), veh_type->get_catering_level());
			}
		}
		else {
			k += sprintf(buf+k, "\n");
		}
		
		const way_constraints_t &way_constraints = veh_type->get_way_constraints();

		// Permissive way constraints
		// (If vehicle has, way must have)
		// @author: jamespetts
		for(uint8 i = 0; i < way_constraints.get_count(); i++)
		{
			if(way_constraints.get_permissive(i))
			{
				k += sprintf(buf + k, "%s", translator::translate("\nMUST USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Permissive %i", i);
				k += sprintf(buf + k, "%s", translator::translate(tmpbuf));
			}
		}

		display_multiline_text( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE*1 + 4 + 16, buf,  COL_BLACK);

		// column 2
	
		sint32 j = 0;

		if(veh_type->get_waytype() == air_wt)
		{
			j += sprintf(buf + j, "%s: %i m \n", translator::translate("Minimum runway length"), veh_type->get_minimum_runway_length());
		}

		if(veh_type->get_zuladung() > 0)
		{
			char min_loading_time_as_clock[32];
			char max_loading_time_as_clock[32];
			//Loading time is only relevant if there is something to load.
			welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), veh_type->get_min_loading_time());
			welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), veh_type->get_max_loading_time());
			j += sprintf(buf + j, "%s %s - %s \n", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
		}
		else
		{
			j += sprintf(buf+j, "\n");
		}

		if(veh_type->get_ware()->get_catg_index() == 0)
		{
			//Comfort only applies to passengers.
			j += sprintf(buf + j, "%s %i ", translator::translate("Comfort:"), veh_type->get_comfort());
			char timebuf[32];
			welt->sprintf_time(timebuf, sizeof(timebuf), (uint32)convoi_t::calc_max_tolerable_journey_time(veh_type->get_comfort(), welt));
			j += sprintf(buf + j, "%s %s%s", translator::translate("(Max. comfortable journey time: "), timebuf, ")\n");
		}
		else {
			j += sprintf(buf+j, "\n");
		}

		j += sprintf(buf + j, "%s %s %04d\n",
			translator::translate("Intro. date:"),	
			translator::get_month_name(veh_type->get_intro_year_month()%12),
			veh_type->get_intro_year_month()/12 );

		if(veh_type->get_retire_year_month() !=DEFAULT_RETIRE_DATE*12) {
			j += sprintf(buf + j, "%s %s %04d\n",
				translator::translate("Retire. date:"),
				translator::get_month_name(veh_type->get_retire_year_month()%12),
				veh_type->get_retire_year_month()/12 );
		}
		else {
			j += sprintf(buf+j, "\n");
		}

		if(veh_type->get_leistung() > 0  &&  veh_type->get_gear()!=64) {
			j+= sprintf(buf + j, "%s %0.2f : 1\n", translator::translate("Gear: "), 	veh_type->get_gear()/64.0);
		}
		else {
			j += sprintf(buf+j, "\n");
		}

		if(veh_type->get_tilting())
		{
			j += sprintf(buf + j, "%s", translator::translate("This is a tilting vehicle\n"));
		}
		else {
			j += sprintf(buf+j, "\n");
		}

		if(veh_type->get_copyright()!=NULL  &&  veh_type->get_copyright()[0]!=0) 
		{
			j += sprintf(buf + j, translator::translate("Constructed by %s\n"), veh_type->get_copyright());
		}
		else {
			j += sprintf(buf+j, "\n");
		}
		
		if(value != -1) 
		{
			if (buf[j-1]!='\n') 
			{
				j += sprintf(buf + j, "\n");
			}
			sprintf(buf + strlen(buf), "%s %lld Cr", translator::translate("Restwert: "), 	value); //"Restwert" = residual (Google)
		}
		else {
			j += sprintf(buf+j, "\n");
		}
		
		// Prohibitibve way constraints
		// (If way has, vehicle must have)
		// @author: jamespetts
		j += sprintf(buf + j, "\n");
		for(uint8 i = 0; i < way_constraints.get_count(); i++)
		{
			if(way_constraints.get_prohibitive(i))
			{
				j += sprintf(buf + j, "%s", translator::translate("\nMAY USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Prohibitive %i", i);
				j += sprintf(buf + j, "%s", translator::translate(tmpbuf));
			}
		}

		display_multiline_text( pos.x + 220, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE*3 + 4 + 16, buf, COL_BLACK);
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

	if(IS_LEFTCLICK(ev) &&  !action_selector.getroffen(ev->cx, ev->cy-16)) {
		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		action_selector.close_box();
		return true;
	}

	if(IS_LEFTCLICK(ev) &&  !upgrade_selector.getroffen(ev->cx, ev->cy-16)) {
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
	dy -= get_convoy_height() + convoy_tabs_skip + 8 + get_vinfo_height() + 17 + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;
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

