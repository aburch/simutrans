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

#include "../../bauer/warenbauer.h"
#include "../../bauer/vehikelbauer.h"
#include "../../besch/intro_dates.h"
#include "../../besch/vehikel_besch.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/umgebung.h"
#include "../../utils/simstring.h"
#include "../../vehicle/simvehikel.h"


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
	way_type(wt), welt(w), last_changed_vehicle(NULL),
	depot_frame(NULL), placement(get_placement(wt)),
	placement_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 4),
	grid(get_grid(wt)),
	grid_dx(get_grid(wt).x * get_base_tile_raster_width() / 64 / 2),
	max_convoy_length(depot_t::get_max_convoy_length(wt)), panel_rows(3), convoy_tabs_skip(0),
	lb_convoi_count(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_speed(NULL, COL_BLACK, gui_label_t::left),
	lb_veh_action("Fahrzeuge:", COL_BLACK, gui_label_t::left),
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
	way_electrified(electrified)

{

	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	*/
	placement.x=placement.x* get_base_tile_raster_width() / 64 + 2;
	placement.y=placement.y* get_base_tile_raster_width() / 64 + 2;
	grid.x=grid.x* get_base_tile_raster_width() / 64 + 4;
	grid.y=grid.y* get_base_tile_raster_width() / 64 + 6;
	if(wt==road_wt  &&  umgebung_t::drive_on_left) {
		// correct for dive on left
		placement.x -= (12*get_base_tile_raster_width())/64;
		placement.y -= (6*get_base_tile_raster_width())/64;
	}

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
	einstellungen_t* e = get_welt()->get_einstellungen();
	char timeline = e->get_use_timeline();
	e->set_use_timeline(0);
	build_vehicle_lists();
	e->set_use_timeline(timeline);
	show_retired_vehicles = old_retired;
	show_all = old_show_all;

	bool one = false;

	cont_pas.add_komponente(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);
	scrolly_pas.set_read_only(false);

	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( get_passenger_name(wt) ) );
		one = true;
	}

	cont_electrics.add_komponente(&electrics);
	scrolly_electrics.set_show_scroll_x(false);
	scrolly_electrics.set_size_corner(false);
	scrolly_electrics.set_read_only(false);
	// add only if there are any trolleybuses
	if(!electrics_vec.empty()) {
		tabs.add_tab(&scrolly_electrics, translator::translate( get_electrics_name(wt) ) );
		one = true;
	}

	cont_loks.add_komponente(&loks);
	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	scrolly_loks.set_read_only(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, translator::translate( get_zieher_name(wt) ) );
		one = true;
	}

	cont_waggons.add_komponente(&waggons);
	scrolly_waggons.set_show_scroll_x(false);
	scrolly_waggons.set_size_corner(false);
	scrolly_waggons.set_read_only(false);
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
	//add_komponente(&lb_upgrade);

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
	bt_obsolete.add_listener(this);
	bt_obsolete.set_tooltip("Show also vehicles no longer in production.");
	add_komponente(&bt_obsolete);

	bt_show_all.set_typ(button_t::square);
	bt_show_all.set_text("Show all");
	bt_show_all.add_listener(this);
	bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
	add_komponente(&bt_show_all);

	lb_convoi_count.set_text_pointer(txt_convoi_count);
	lb_convoi_speed.set_text_pointer(txt_convoi_speed);

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
	convoi.set_pos(koord((groesse.x-get_convoy_image_width())/2, 0));
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
	//lb_upgrade.set_pos(koord(groesse.x-ABUTTON_WIDTH, PANEL_VSTART + get_panel_height() + 34));

	action_selector.set_pos(koord(groesse.x-ABUTTON_WIDTH, PANEL_VSTART + get_panel_height() + 14));
	action_selector.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	action_selector.set_max_size(koord(ABUTTON_WIDTH - 8, LINESPACE*3+2+16));
	action_selector.set_highlight_color(1);

	upgrade_selector.set_pos(koord(groesse.x-ABUTTON_WIDTH, PANEL_VSTART + get_panel_height() + 34));
	upgrade_selector.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	upgrade_selector.set_max_size(koord(ABUTTON_WIDTH - 8, LINESPACE*2+2+16));
	upgrade_selector.set_highlight_color(1);

	bt_show_all.set_pos(koord(groesse.x-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + get_panel_height() + 4 ));
	bt_show_all.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	bt_show_all.pressed = show_all;

	bt_obsolete.set_pos(koord(groesse.x-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + get_panel_height() + 16));
	bt_obsolete.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	bt_obsolete.pressed = show_retired_vehicles;
}


bool gui_convoy_assembler_t::action_triggered( gui_action_creator_t *komp,value_t p)
{
	if(komp != NULL) {	// message from outside!
			// image lsit selction here ...
		if(komp == &convoi) {
			image_from_convoi_list( p.i );
		} else if(komp == &pas) {
			image_from_storage_list(&pas_vec[p.i]);
		} else if (komp == &electrics) {
			image_from_storage_list(&electrics_vec[p.i]);
		} else if(komp == &loks) {
			image_from_storage_list(&loks_vec[p.i]);
		} else if(komp == &waggons) {
			image_from_storage_list(&waggons_vec[p.i]);
		} else if(komp == &bt_obsolete) {
			show_retired_vehicles = (show_retired_vehicles==0);
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
		} else if(komp == &upgrade_selector) {
			sint32 upgrade_selection = p.i;
			if ( upgrade_selection < 0 ) {
				upgrade_selector.set_selection(0);
				upgrade_selection=0;
			}
			if(  (unsigned)(upgrade_selection)<=u_upgrade  ) {
				upgrade=(unsigned)(upgrade_selection);
				build_vehicle_lists();
				update_data();
			}

		} else {
			return false;
		}
	}
	return true;
}

void gui_convoy_assembler_t::zeichnen(koord parent_pos)
{
	*txt_convoi_count = '\0';
	*txt_convoi_speed = '\0';
	if (vehicles.get_count()>0) {
		convoy_metrics_t metrics(vehicles);
		uint32 min_speed = metrics.get_speed(metrics.get_vehicle_weight() + metrics.get_max_freight_weight());
		uint32 max_speed = metrics.get_speed(metrics.get_vehicle_weight() /*+ metrics.get_min_freight_weight()*/);
		sprintf(txt_convoi_count, "%s %d (%s %i)",
			translator::translate("Fahrzeuge:"), vehicles.get_count(),
			translator::translate("Station tiles:"), metrics.get_tile_length() );
		sprintf(txt_convoi_speed,  min_speed == max_speed ? "%s %d km/h" : "%s %d %s %d km/h", 
			translator::translate("Max. speed:"), min_speed, 
			translator::translate("..."), max_speed );
	}

	bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
	bt_show_all.pressed = show_all;	// otherwise the button would not show depressed
	draw_vehicle_info_text(parent_pos+pos);
	gui_container_t::zeichnen(parent_pos);
}



// add all current vehicles
void gui_convoy_assembler_t::build_vehicle_lists()
{
	if(vehikelbauer_t::get_info(way_type)==NULL) {
		// there are tracks etc. but now vehicles => do nothing
		return;
	}

	const uint16 month_now = get_welt()->get_timeline_year_month();

	if(electrics_vec.empty()  &&  pas_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty()) 
	{
		uint16 loks = 0, waggons = 0, pax=0, electrics = 0;
		slist_iterator_tpl<vehikel_besch_t*> vehinfo(vehikelbauer_t::get_info(way_type));
		while (vehinfo.next()) 
		{
			const vehikel_besch_t* info = vehinfo.get_current();
			if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)) 
			{
				electrics++;
			}
			else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) 
			{
				pax++;
			}
			else if(info->get_leistung() > 0  ||  (info->get_zuladung() == 0 && (info->get_vorgaenger_count() > 0 || info->get_nachfolger_count() > 0))) 
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
	pas_vec.clear();
	electrics_vec.clear();
	loks_vec.clear();
	waggons_vec.clear();

	vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification (way_electrified)

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell && depot_frame) {
		// just list the one to sell
		slist_iterator_tpl<vehikel_t *> iter2(depot_frame->get_depot()->get_vehicle_list());
		while(iter2.next()) {
			if(vehicle_map.get(iter2.get_current()->get_besch())) {
				continue;
			}
			add_to_vehicle_list( iter2.get_current()->get_besch() );
		}
	}
	else {
		// list only matching ones
		slist_iterator_tpl<vehikel_besch_t*> vehinfo(vehikelbauer_t::get_info(way_type));
		while (vehinfo.next()) {
			const vehikel_besch_t* info = vehinfo.get_current();
			const vehikel_besch_t *veh = NULL;
			if(vehicles.get_count()>0) {
				veh = (veh_action == va_insert) ? vehicles[0] : vehicles[vehicles.get_count() - 1];
			}

			// current vehicle
			if( (depot_frame && depot_frame->get_depot()->is_contained(info))  ||
				((way_electrified  ||  info->get_engine_type()!=vehikel_besch_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) {
				// check, if allowed
				bool append = true;
				bool upgradeable = true;
				if(!show_all) 
				{
					if(veh_action == va_insert) 
					{
						append = !(!convoi_t::pruefe_nachfolger(info, veh) || (veh && !convoi_t::pruefe_vorgaenger(info, veh)));
					} 
					else if(veh_action == va_append) 
					{
						append = !(!convoi_t::pruefe_vorgaenger(veh, info) || (veh && !convoi_t::pruefe_nachfolger(veh, info)));
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
								if(info->get_name() == vehicle_list[i]->get_upgrades(c)->get_name())
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
							append = false;
						}
					}
				}
				if(append && (upgrade == u_buy || upgradeable)) 
				{
					add_to_vehicle_list( info );
				}
			}
		}
	}
DBG_DEBUG("gui_convoy_assembler_t::build_vehicle_lists()","finally %i passenger vehicle, %i  engines, %i good wagons",pas_vec.get_count(),loks_vec.get_count(),waggons_vec.get_count());
}



// add a single vehicle (helper function)
void gui_convoy_assembler_t::add_to_vehicle_list(const vehikel_besch_t *info)
{
	// prissi: ist a non-electric track?
	// Hajo: check for timeline
	// prissi: and retirement date
	gui_image_list_t::image_data_t img_data;

	img_data.image = info->get_basis_bild();
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
	if(nr < vehicles.get_count()) {

		// we remove all connected vehicles together!
		// find start
		unsigned start_nr = nr;
		while(start_nr>0) {
			start_nr --;
			const vehikel_besch_t *info = vehicles[start_nr];
			if(info->get_nachfolger_count()!=1) {
				start_nr ++;
				break;
			}
		}
		// find end
		while(nr<vehicles.get_count()) {
			const vehikel_besch_t *info = vehicles[nr];
			nr ++;
			if(info->get_nachfolger_count()!=1) {
				break;
			}
		}
		// now remove the vehicles
		if(vehicles.get_count()==nr-start_nr) {
			clear_convoy();
			koord k=koord(clear_convoy_action,0);
			value_t v;
			v.p=&k;
			last_changed_vehicle=NULL;
			call_listeners(v);
		}
		else {
			koord k=koord(remove_vehicle_action,0);
			value_t v;
			v.p=&k;
			for( unsigned i=start_nr;  i<nr;  i++  ) {
				last_changed_vehicle=vehicles[start_nr];
				remove_vehicle_at(start_nr);
				k.y=start_nr;
				call_listeners(v);
			}
		}
	}
}



void gui_convoy_assembler_t::image_from_storage_list(gui_image_list_t::image_data_t *bild_data)
{
	const vehikel_besch_t *info = vehikelbauer_t::get_info(bild_data->text);

	if(bild_data->lcolor != COL_RED &&
		bild_data->rcolor != COL_RED &&
		bild_data->rcolor != COL_DARK_PURPLE &&
		bild_data->lcolor != COL_DARK_PURPLE &&
		bild_data->rcolor != COL_PURPLE &&
		bild_data->lcolor != COL_PURPLE &&
		!((bild_data->lcolor == COL_DARK_ORANGE || bild_data->rcolor == COL_DARK_ORANGE)
		&& veh_action != va_sell
		&& depot_frame != NULL && !depot_frame->get_depot()->find_oldest_newest(info, true))) {
			
			// Dark orange = too expensive
			// Purple = available only as upgrade

		// we buy/sell all vehicles together!
		slist_tpl<const vehikel_besch_t *>new_vehicle_info;
		
		const vehikel_besch_t *start_info = info;
		if(veh_action==va_insert  ||  veh_action==va_sell) {
			// start of composition
			while (info->get_vorgaenger_count() == 1 && info->get_vorgaenger(0) != NULL) {
				info = info->get_vorgaenger(0);
				new_vehicle_info.insert(info);
			}
			info = start_info;
		}
		// not get the end ...
		while(info) 
		{
			new_vehicle_info.append( info );
DBG_MESSAGE("gui_convoy_assembler_t::image_from_storage_list()","appended %s",info->get_name() );
			if(info->get_nachfolger_count()!=1  ||  (veh_action==va_insert  &&  info==start_info)) {
				break;
			}
			info = info->get_nachfolger(0);
		}

		if(veh_action == va_sell) 
		{
			while(new_vehicle_info.get_count() && depot_frame) {
				/*
				*	We sell the newest vehicle - gives most money back.
				*/
				vehikel_t* veh = depot_frame->get_depot()->find_oldest_newest(new_vehicle_info.remove_first(), false);
				if (veh != NULL) {
					depot_frame->get_depot()->sell_vehicle(veh);
					build_vehicle_lists();
					update_data();
				}
			}
		}
		else 
		{
			// append/insert into convoi
			if(vehicles.get_count()+new_vehicle_info.get_count() <= max_convoy_length) {

				koord k=koord(veh_action==va_insert?insert_vehicle_in_front_action:append_vehicle_action,0);
				value_t v;
				v.p=&k;
				for( unsigned i=0;  i<new_vehicle_info.get_count();  i++ ) {
					// insert/append needs reverse order
					unsigned nr = (veh_action == va_insert) ? new_vehicle_info.get_count()-i-1 : i;
					// We add the oldest vehicle - newer stay for selling
					const vehikel_besch_t* vb = new_vehicle_info.at(nr);
					append_vehicle(vb, veh_action == va_insert);
					last_changed_vehicle=vb;
					call_listeners(v);
DBG_MESSAGE("gui_convoy_assembler_t::image_from_storage_list()","built nr %i", nr);
				}
			}
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
			img_data.image = vehicles[i]->get_basis_bild();
			img_data.count = 0;
			img_data.lcolor = img_data.rcolor= EMPTY_IMAGE_BAR;
			img_data.text = vehicles[i]->get_name();
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0].lcolor = convoi_t::pruefe_vorgaenger(NULL, vehicles[0]) ? COL_DARK_GREEN : COL_YELLOW;
		for(  i=1;  i<vehicles.get_count(); i++) {
			convoi_pics[i - 1].rcolor = convoi_t::pruefe_nachfolger(vehicles[i - 1], vehicles[i]) ? COL_DARK_GREEN : COL_RED;
			convoi_pics[i].lcolor     = convoi_t::pruefe_vorgaenger(vehicles[i - 1], vehicles[i]) ? COL_DARK_GREEN : COL_RED;
		}
		convoi_pics[i - 1].rcolor = convoi_t::pruefe_nachfolger(vehicles[i - 1], NULL) ? COL_DARK_GREEN : COL_YELLOW;

		// change grren into blue for retired vehicles
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

	ptrhashtable_iterator_tpl<const vehikel_besch_t *, gui_image_list_t::image_data_t *> iter1(vehicle_map);
	
	const spieler_t *sp = welt->get_active_player();
	
	while(iter1.next()) 
	{
		const vehikel_besch_t *info = iter1.get_current_key();
		const uint8 ok_color = info->is_future(month_now) || info->is_retired(month_now) ? COL_DARK_BLUE: COL_DARK_GREEN;

		iter1.get_current_value()->count = 0;
		iter1.get_current_value()->lcolor = ok_color;
		iter1.get_current_value()->rcolor = ok_color;

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
			if (!convoi_t::pruefe_nachfolger(info, veh) || (veh && !convoi_t::pruefe_vorgaenger(info, veh))) {
				iter1.get_current_value()->lcolor = COL_RED;
				iter1.get_current_value()->rcolor = COL_RED;
			} else if(!convoi_t::pruefe_vorgaenger(NULL, info)) {
				iter1.get_current_value()->lcolor = COL_YELLOW;
			}
		} else if(veh_action == va_append) {
			if(!convoi_t::pruefe_vorgaenger(veh, info) || (veh && !convoi_t::pruefe_nachfolger(veh, info))) {
				iter1.get_current_value()->lcolor = COL_RED;
				iter1.get_current_value()->rcolor = COL_RED;
			} else if(!convoi_t::pruefe_nachfolger(info, NULL)) {
				iter1.get_current_value()->rcolor = COL_YELLOW;
			}
		}
		if (veh_action != va_sell)
		{
			//Check whether too expensive
			//@author: jamespetts
			if(iter1.get_current_value()->lcolor == ok_color || iter1.get_current_value()->lcolor == COL_YELLOW)
			{
				//Only flag as too expensive that which could be purchased anyway.
				if(upgrade == u_buy)
				{
					if(!sp->can_afford(info->get_preis()))
					{
						iter1.get_current_value()->lcolor = COL_DARK_ORANGE;
						iter1.get_current_value()->rcolor = COL_DARK_ORANGE;
					}
				}
				else
				{
					if(!sp->can_afford(info->get_upgrade_price()))
					{
						iter1.get_current_value()->lcolor = COL_DARK_ORANGE;
						iter1.get_current_value()->rcolor = COL_DARK_ORANGE;
					}
				}
			}
		}

		if(upgrade == u_upgrade)
		{
			//Check whether there are any vehicles to upgrade
			iter1.get_current_value()->lcolor = COL_DARK_PURPLE;
			iter1.get_current_value()->rcolor = COL_DARK_PURPLE;
			vector_tpl<const vehikel_besch_t*> vehicle_list;

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

			ITERATE(vehicle_list, i)
			{
				for(uint16 c = 0; c < vehicle_list[i]->get_upgrades_count(); c++)
				{
					if(info->get_name() == vehicle_list[i]->get_upgrades(c)->get_name())
					{
						iter1.get_current_value()->lcolor = COL_DARK_GREEN;
						iter1.get_current_value()->rcolor = COL_DARK_GREEN;
						if(replace_frame != NULL)
						{
							// If we are using the replacing window,
							// vehicle_list is the list of all vehicles in the current convoy.
							// vehicles is the list of the vehicles to replace them with.
							sint8 upgradeable_count = 0;
							ITERATE(vehicles,j)
							{
								if(vehicles[j]->get_name() == info->get_name())
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
									if(vehicle_list[j]->get_upgrades(k)->get_name() == info->get_name())
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
								iter1.get_current_value()->lcolor = COL_DARK_PURPLE;
								iter1.get_current_value()->rcolor = COL_DARK_PURPLE;
							}

						}
						if(veh_action == va_insert) 
						{
							if (!convoi_t::pruefe_nachfolger(info, veh) || (veh && !convoi_t::pruefe_vorgaenger(info, veh))) 
							{
								iter1.get_current_value()->lcolor = COL_RED;
								iter1.get_current_value()->rcolor = COL_RED;
							} 
							else if(!convoi_t::pruefe_vorgaenger(NULL, info)) 
							{
								iter1.get_current_value()->lcolor = COL_YELLOW;
							}
						} 
						else if(veh_action == va_append) 
						{
							if(!convoi_t::pruefe_vorgaenger(veh, info) || (veh && !convoi_t::pruefe_nachfolger(veh, info))) {
								iter1.get_current_value()->lcolor = COL_RED;
								iter1.get_current_value()->rcolor = COL_RED;
							} 
							else if(!convoi_t::pruefe_nachfolger(info, NULL)) 
							{
								iter1.get_current_value()->rcolor = COL_YELLOW;
							}
						}
					}
				}
			}
		}
		else
		{
			if(info->is_available_only_as_upgrade())
			{
				iter1.get_current_value()->lcolor = COL_PURPLE;
				iter1.get_current_value()->rcolor = COL_PURPLE;
			}
		}

DBG_DEBUG("gui_convoy_assembler_t::update_data()","current %s with colors %i,%i",info->get_name(),iter1.get_current_value()->lcolor,iter1.get_current_value()->rcolor);
	}

	if (depot_frame) {
		slist_iterator_tpl<vehikel_t *> iter2(depot_frame->get_depot()->get_vehicle_list());
		while(iter2.next()) {
			gui_image_list_t::image_data_t *imgdat=vehicle_map.get(iter2.get_current()->get_besch());
			// can fail, if currently not visible
			if(imgdat) {
				imgdat->count++;
				if(veh_action == va_sell) {
					imgdat->lcolor = COL_DARK_GREEN;
					imgdat->rcolor = COL_DARK_GREEN;
				}
			}
		}
	}
}



void gui_convoy_assembler_t::draw_vehicle_info_text(koord pos)
{
	char buf[1024];
	const char *c;

	gui_image_list_t *lst;
	if(  tabs.get_aktives_tab()==&scrolly_pas  ) {
		lst = dynamic_cast<gui_image_list_t *>(&pas);
	}
	else if(  tabs.get_aktives_tab()==&scrolly_electrics  ) {
		lst = dynamic_cast<gui_image_list_t *>(&electrics);
	}
	else if(  tabs.get_aktives_tab()==&scrolly_loks  ) {
		lst = dynamic_cast<gui_image_list_t *>(&loks);
	}
	else {
		lst = dynamic_cast<gui_image_list_t *>(&waggons);
	}
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
				veh_type = cnv->get_vehikel(sel_index)->get_besch();
				value = cnv->get_vehikel(sel_index)->calc_restwert()/100;
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
		// lok oder waggon ?
		if(veh_type->get_leistung() > 0) { //"Leistung" = performance (Google)
			//lok
			const sint32 zuladung = veh_type->get_zuladung(); //"Zuladung" = payload (Google)

			char name[128];

			sprintf(name,
			"%s (%s)",
			translator::translate(veh_type->get_name()),
			translator::translate(engine_type_names[veh_type->get_engine_type()+1]));

			sint32 n;

			
			if(upgrade == u_buy)
			{
				n = sprintf(buf,
					translator::translate("LOCO_INFO_EXT"),
					//translator::translate("%s\nCost: %d$\nMaint.: %1.2f$/km, %1.2f$/month\nPower: %dkW, %dkm/h\nWeight: %dt\n"),
					name,
					veh_type->get_preis()/100,
					veh_type->get_betriebskosten(get_welt())/100.0F,
					veh_type->get_adjusted_monthly_fixed_maintenance(get_welt())/100.0F,
					veh_type->get_leistung(),
					veh_type->get_geschw(),
					veh_type->get_gewicht()
					);
			}
			else
			{
				n = sprintf(buf,
					translator::translate("LOCO_INFO_EXT"),
					//translator::translate("%s\nCost: %d$\nMaint.: %1.2f$/km, %1.2f$/month\nPower: %dkW, %dkm/h\nWeight: %dt\n"),
					name,
					veh_type->get_upgrade_price()/100,
					veh_type->get_betriebskosten(get_welt())/100.0F,
					veh_type->get_adjusted_monthly_fixed_maintenance(get_welt())/100.0F,
					veh_type->get_leistung(),
					veh_type->get_geschw(),
					veh_type->get_gewicht()
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
					n = sprintf(buf,
						/*translator::translate("WAGGON_INFO"),*/
						translator::translate("%s\nCost:     %d$\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d%s %s\nWeight: %dt\nTop speed: %dkm/h\n"),
						translator::translate(veh_type->get_name()),
						veh_type->get_preis()/100,
						veh_type->get_betriebskosten(get_welt())/100.0F,
 						veh_type->get_adjusted_monthly_fixed_maintenance(get_welt())/100.0F,
						veh_type->get_zuladung(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw()
						);
				}
				else
				{
					n = sprintf(buf,
						//translator::translate("%s\nCost:     %d$ (%1.2f$/km)\nCapacity: %d (%d)%s %s\nWeight: %dt\nTop speed: %dkm/h\n"),
						/*translator::translate("WAGGON_INFO_OVERCROWD"),*/
						translator::translate("%s\nCost:     %d$\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d (%d)%s %s\nWeight: %dt\nTop speed: %dkm/h\n"),
						translator::translate(veh_type->get_name()),
						veh_type->get_preis()/100,
						veh_type->get_betriebskosten(get_welt())/100.0F,
 						veh_type->get_adjusted_monthly_fixed_maintenance(get_welt())/100.0F,
						veh_type->get_zuladung(),
						veh_type->get_overcrowded_capacity(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw()
						);
				}
			}
			else
			{
				if(veh_type->get_overcrowded_capacity() < 1)
				{

					n = sprintf(buf,
						/*translator::translate("WAGGON_INFO"),*/
						translator::translate("%s\nCost:     %d$\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d%s %s\nWeight: %dt\nTop speed: %dkm/h\n"),
						translator::translate(veh_type->get_name()),
						veh_type->get_upgrade_price()/100,
						veh_type->get_betriebskosten(get_welt())/100.0F,
						veh_type->get_adjusted_monthly_fixed_maintenance(get_welt())/100.0F,
						veh_type->get_zuladung(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw()
						);
				}
				else
				{
					n = sprintf(buf,
						/*translator::translate("WAGGON_INFO"),*/
						translator::translate("%s\nCost:     %d$\nMaint.: %1.2f$/km, %1.2f$/month\nCapacity: %d%s %s\nWeight: %dt\nTop speed: %dkm/h\n"),
						translator::translate(veh_type->get_name()),
						veh_type->get_upgrade_price()/100,
						veh_type->get_betriebskosten(get_welt())/100.0F,
						veh_type->get_adjusted_monthly_fixed_maintenance(get_welt())/100.0F,
						veh_type->get_zuladung(),
						veh_type->get_overcrowded_capacity(),
						translator::translate(veh_type->get_ware()->get_mass()),
						veh_type->get_ware()->get_catg() == 0 ?
						translator::translate(veh_type->get_ware()->get_name()) :
						translator::translate(veh_type->get_ware()->get_catg_name()),
						veh_type->get_gewicht(),
						veh_type->get_geschw()
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
				k +=  sprintf(buf + k, translator::translate("This is a travelling post office"));
			}
			else
			{
				k += sprintf(buf + k, translator::translate("Catering level: %i"), veh_type->get_catering_level());
			}
		}
		
		// Permissive way constraints
		// (If vehicle has, way must have)
		// @author: jamespetts
		for(uint8 i = 0; i < 8; i++)
		{
			if(veh_type->permissive_way_constraint_set(i))
			{
				k += sprintf(buf + k, translator::translate("\nMUST USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Permissive %i", i);
				k += sprintf(buf + k, translator::translate(tmpbuf));
			}
		}

		display_multiline_text( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE*1 + 4, buf,  COL_BLACK);

		// column 2
	
		sint32 j = 0;

		if(veh_type->get_zuladung() > 0)
		{
			//Loading time is only relevant if there is something to load.
			j +=  sprintf(buf, "%s %i \n", translator::translate("Loading time:"), veh_type->get_loading_time());
		}

		if(veh_type->get_ware()->get_catg_index() == 0)
		{
			//Comfort only applies to passengers.
			j += sprintf(buf + j, "%s %i \n", translator::translate("Comfort:"), veh_type->get_comfort());
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

		if(veh_type->get_leistung() > 0  &&  veh_type->get_gear()!=64) {
			j+= sprintf(buf + j, "%s %0.2f : 1\n", translator::translate("Gear: "), 	veh_type->get_gear()/64.0);
		}

		if(veh_type->get_tilting())
		{
			j += sprintf(buf + j, translator::translate("This is a tilting vehicle\n"));
		}

		if(veh_type->get_copyright()!=NULL  &&  veh_type->get_copyright()[0]!=0) 
		{
			j += sprintf(buf + j, translator::translate("Constructed by %s\n"), veh_type->get_copyright());
		}
		
		if(value != -1) 
		{
			sprintf(buf + strlen(buf), "%s %d Cr", translator::translate("Restwert: "), 	value); //"Restwert" = residual (Google)
		}
		
		// Prohibitibve way constraints
		// (If way has, vehicle must have)
		// @author: jamespetts
		j += sprintf(buf + j, "\n");
		for(uint8 i = 0; i < 8; i++)
		{
			if(veh_type->prohibitive_way_constraint_set(i))
			{
				j += sprintf(buf + j, translator::translate("\nMAY USE: "));
				char tmpbuf[30];
				sprintf(tmpbuf, "Prohibitive %i", i);
				j += sprintf(buf + j, translator::translate(tmpbuf));
			}
		}

		display_multiline_text( pos.x + 200, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE*2 + 4, buf, COL_BLACK);
	}
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


void gui_convoy_assembler_t::infowin_event(const event_t *ev)
{
	gui_container_t::infowin_event(ev);

	if(IS_LEFTCLICK(ev) &&  !action_selector.getroffen(ev->cx, ev->cy-16)) {
		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		action_selector.close_box();
	}

	if(IS_LEFTCLICK(ev) &&  !upgrade_selector.getroffen(ev->cx, ev->cy-16)) {
		// close combo box; we must do it ourselves, since the box does not recieve outside events ...
		upgrade_selector.close_box();
	}
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
