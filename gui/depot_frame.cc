/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdio.h>

#include "../simworld.h"
#include "../simvehikel.h"
#include "../simconvoi.h"
#include "../simdepot.h"
#include "../simwin.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../simgraph.h"
#include "../simline.h"
#include "../simplay.h"
#include "../simlinemgmt.h"

#include "../tpl/slist_tpl.h"

#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "components/gui_image_list.h"
#include "messagebox.h"
#include "depot_frame.h"

#include "../besch/ware_besch.h"
#include "../besch/intro_dates.h"
#include "../bauer/vehikelbauer.h"
#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/simstring.h"

#include "../bauer/warenbauer.h"

#include "../boden/wege/weg.h"
#include "../boden/wege/schiene.h"

char depot_frame_t::no_line_text[128];	// contains the current translation of "<no line>"

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


bool  depot_frame_t::show_retired_vehicles = false;

bool  depot_frame_t::show_all = true;


depot_frame_t::depot_frame_t(depot_t* depot) :
	gui_frame_t(txt_title, depot->gib_besitzer()),
	depot(depot),
	icnv(depot->convoi_count()-1),
	lb_convois(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_count(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_value(NULL, COL_BLACK, gui_label_t::right),
	lb_convoi_line(NULL, COL_BLACK, gui_label_t::left),
	lb_veh_action(NULL, COL_BLACK, gui_label_t::left),
	convoi_pics(depot->get_max_convoi_length()),
	convoi(&convoi_pics),
	pas_vec(0),
	loks_vec(0),
	waggons_vec(0),
	pas(&pas_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&cont_pas),
	scrolly_loks(&cont_loks),
	scrolly_waggons(&cont_waggons)
{
DBG_DEBUG("depot_frame_t::depot_frame_t()","get_max_convoi_length()=%i",depot->get_max_convoi_length());
	selected_line = linehandle_t();
	strcpy(no_line_text, translator::translate("<no line>"));
	setze_opaque( true );

	sprintf(txt_title, "(%d,%d) %s", depot->gib_pos().x, depot->gib_pos().y, translator::translate(depot->gib_name()));

	/*
	* [SELECT]:
	*/
	add_komponente(&lb_convois);

	bt_prev.setze_typ(button_t::arrowleft);
	bt_prev.add_listener(this);
	add_komponente(&bt_prev);

	add_komponente(&inp_name);

	bt_next.setze_typ(button_t::arrowright);
	bt_next.add_listener(this);
	add_komponente(&bt_next);

	/*
	* [SELECT ROUTE]:
	*/
	line_selector.set_highlight_color(PLAYER_FLAG|depot->gib_besitzer()->get_player_color1()+1);
	line_selector.add_listener(this);
	add_komponente(&line_selector);
	depot->gib_besitzer()->simlinemgmt.sort_lines();

	/*
	* [CONVOI]
	*/
	convoi.set_player_nr(depot->get_player_nr());
	convoi.add_listener(this);

	add_komponente(&convoi);
	add_komponente(&lb_convoi_count);
	add_komponente(&lb_convoi_value);
	add_komponente(&lb_convoi_line);

	/*
	* [ACTIONS]
	*/
	bt_start.setze_typ(button_t::roundbox);
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_komponente(&bt_start);

	bt_schedule.setze_typ(button_t::roundbox);
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule");
	add_komponente(&bt_schedule);

	bt_destroy.setze_typ(button_t::roundbox);
	bt_destroy.add_listener(this);
	bt_destroy.set_tooltip("Move the selected vehicle(s) back to the depot");
	add_komponente(&bt_destroy);

	bt_sell.setze_typ(button_t::roundbox);
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_komponente(&bt_sell);

	/*
	* new route management buttons
	*/
	bt_new_line.setze_typ(button_t::roundbox);
	bt_new_line.add_listener(this);
	bt_new_line.set_tooltip("Lines are used to manage groups of vehicles");
	add_komponente(&bt_new_line);

	bt_apply_line.setze_typ(button_t::roundbox);
	bt_apply_line.add_listener(this);
	bt_apply_line.set_tooltip("Add the selected vehicle(s) to the selected line");
	add_komponente(&bt_apply_line);

	bt_change_line.setze_typ(button_t::roundbox);
	bt_change_line.add_listener(this);
	bt_change_line.set_tooltip("Modify the selected line");
	add_komponente(&bt_change_line);

	bt_copy_convoi.setze_typ(button_t::roundbox);
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line");
	add_komponente(&bt_copy_convoi);

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
	einstellungen_t* e = World()->gib_einstellungen();
	char timeline = e->gib_use_timeline();
	e->setze_use_timeline(0);
	build_vehicle_lists();
	e->setze_use_timeline(timeline);
	show_retired_vehicles = old_retired;
	show_all = old_show_all;

	cont_pas.add_komponente(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);
	scrolly_pas.set_read_only(false);

	// always add
	tabs.add_tab(&scrolly_pas, depot->gib_passenger_name());

	cont_loks.add_komponente(&loks);
	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	scrolly_loks.set_read_only(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, depot->gib_zieher_name());
	}

	cont_waggons.add_komponente(&waggons);
	scrolly_waggons.set_show_scroll_x(false);
	scrolly_waggons.set_size_corner(false);
	scrolly_waggons.set_read_only(false);
	// only add, if there are waggons
	if (!waggons_vec.empty()) {
		tabs.add_tab(&scrolly_waggons, depot->gib_haenger_name());
	}


	pas.set_player_nr(depot->get_player_nr());
	pas.add_listener(this);

	loks.set_player_nr(depot->get_player_nr());
	loks.add_listener(this);

	waggons.set_player_nr(depot->get_player_nr());
	waggons.add_listener(this);

	add_komponente(&tabs);
	add_komponente(&div_tabbottom);
	add_komponente(&lb_veh_action);

	veh_action = va_append;
	bt_veh_action.setze_typ(button_t::roundbox);
	bt_veh_action.add_listener(this);
	bt_veh_action.set_tooltip("Choose operation executed on clicking stored/new vehicles");
	add_komponente(&bt_veh_action);

	bt_obsolete.setze_typ(button_t::square);
	bt_obsolete.setze_text("Show obsolete");
	bt_obsolete.add_listener(this);
	bt_obsolete.set_tooltip("Show also vehicles no longer in production.");
	add_komponente(&bt_obsolete);

	bt_show_all.setze_typ(button_t::square);
	bt_show_all.setze_text("Show all");
	bt_show_all.add_listener(this);
	bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
	add_komponente(&bt_show_all);

	koord gr = koord(0,0);
	// text is only known now!
	lb_convois.setze_text(txt_convois);
	lb_convoi_count.setze_text(txt_convoi_count);
	lb_convoi_value.setze_text(txt_convoi_value);
	lb_convoi_line.setze_text(txt_convoi_line);
	layout(&gr);

	gui_frame_t::setze_fenstergroesse(gr);

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);
}




void depot_frame_t::layout(koord *gr)
{
	koord placement;
	koord grid;
	int grid_dx;
	int placement_dx;

	koord fgr = (gr!=NULL)? *gr : gib_fenstergroesse();

	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	*/
	grid.x = depot->get_x_grid() * get_tile_raster_width() / 64 + 4;
	grid.y = depot->get_y_grid() * get_tile_raster_width() / 64 + 6;
	placement.x = depot->get_x_placement() * get_tile_raster_width() / 64 + 2;
	placement.y = depot->get_y_placement() * get_tile_raster_width() / 64 + 2;
	if(depot->get_wegtyp()==road_wt  &&  umgebung_t::drive_on_left) {
		// correct for dive on left
		placement.x -= (12*get_tile_raster_width())/64;
		placement.y -= (6*get_tile_raster_width())/64;
	}
	grid_dx = depot->get_x_grid() * get_tile_raster_width() / 64 / 2;
	placement_dx = depot->get_x_grid() * get_tile_raster_width() / 64 / 4;

	/*
	*	Dialog format:
	*
	*	Main structure are these parts from top to bottom:
	*
	*	    [SELECT]		convoi-selector
	*	    [CONVOI]		current convoi
	*	    [ACTIONS]		convoi action buttons
	*	    [PANEL]		vehicle panel
	*	    [VINFO]		vehicle infotext
	*
	*
	*	Structure of [SELECT] is:
	*
	*	    [Info]
	*	    [PREV][LABEL][NEXT]
	*
	*  PREV and NEXT are small buttons - Label is adjusted to total width.
	*/
	int SELECT_HEIGHT = 14;

	/*
	*	Structure of [CONVOI] is a image_list and an infos:
	*
	*	    [List]
	*	    [Info]
	*
	* The image list is horizontally "condensed".
	*/
	int CINFO_HEIGHT = 14;
	int CLIST_WIDTH = depot->get_max_convoi_length() * (grid.x - grid_dx) + 2 * gui_image_list_t::BORDER;
	int CLIST_HEIGHT = grid.y + 2 * gui_image_list_t::BORDER;
	int CONVOI_WIDTH = CLIST_WIDTH + placement_dx;

	/*
	*	Structure of [ACTIONS] is a row of buttons:
	*
	*	    [Start][Schedule][Destroy][Sell]
	*      [new Route][change Route][delete Route]
	*/
	int ABUTTON_WIDTH = 96;
	int ABUTTON_HEIGHT = 14;
	int ACTIONS_WIDTH = 4 * ABUTTON_WIDTH;
	int ACTIONS_HEIGHT = ABUTTON_HEIGHT + ABUTTON_HEIGHT; // @author hsiegeln: added "+ ABUTTON_HEIGHT"

	/*
	*	Structure of [VINFO] is one multiline text.
	*/
	int VINFO_HEIGHT = 86;

	/*
	* Total width is the max from [CONVOI] and [ACTIONS] width.
	*/
	int MIN_TOTAL_WIDTH = max(CONVOI_WIDTH, ACTIONS_WIDTH);
	int TOTAL_WIDTH = max(fgr.x,max(CONVOI_WIDTH, ACTIONS_WIDTH));

	/*
	*	Now we can do the first vertical adjustement:
	*/
	int SELECT_VSTART = 16;
	int CONVOI_VSTART = SELECT_VSTART + SELECT_HEIGHT + LINESPACE;
	int CINFO_VSTART = CONVOI_VSTART + CLIST_HEIGHT;
	int ACTIONS_VSTART = CINFO_VSTART + CINFO_HEIGHT + 2 + LINESPACE;
	int PANEL_VSTART = ACTIONS_VSTART + ACTIONS_HEIGHT + 8;

	/*
	* Now we determine the row/col layout for the panel and the total panel
	* size.
	* build_vehicle_lists() fills loks_vec and waggon_vec.
	* Total width will be expanded to match completo columns in panel.
	*/
	int total_h = PANEL_VSTART+VINFO_HEIGHT + 17 + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;
	int PANEL_ROWS = max(1, ((fgr.y-total_h)/grid.y) );
	if(gr  &&  gr->y==0) {
		PANEL_ROWS = 3;
	}
	int PANEL_HEIGHT = PANEL_ROWS * grid.y + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;
	int MIN_PANEL_HEIGHT = grid.y + gui_tab_panel_t::HEADER_VSIZE + 2 * gui_image_list_t::BORDER;

    /*
     *	Now we can do the complete vertical adjustement:
     */
    int TOTAL_HEIGHT = PANEL_VSTART + PANEL_HEIGHT + VINFO_HEIGHT + 17;
    int MIN_TOTAL_HEIGHT = PANEL_VSTART + MIN_PANEL_HEIGHT + VINFO_HEIGHT + 17;

	/*
	* DONE with layout planning - now build everything.
	*/
	set_min_windowsize(koord(MIN_TOTAL_WIDTH, MIN_TOTAL_HEIGHT));
	if(fgr.x<TOTAL_WIDTH) {
		gui_frame_t::setze_fenstergroesse(koord(MIN_TOTAL_WIDTH, max(fgr.y,MIN_TOTAL_HEIGHT) ));
	}
	if(gr  &&  gr->x==0) {
		gr->x = TOTAL_WIDTH;
	}
	if(gr  &&  gr->y==0) {
		gr->y = TOTAL_HEIGHT;
	}

    /*
     * [SELECT]:
     */
    lb_convois.setze_pos(koord(4, SELECT_VSTART - 10));

    bt_prev.setze_pos(koord(5, SELECT_VSTART + 2));

    inp_name.setze_pos(koord(5+12, SELECT_VSTART));
    inp_name.setze_groesse(koord(TOTAL_WIDTH - 26-8, 14));

    bt_next.setze_pos(koord(TOTAL_WIDTH - 15, SELECT_VSTART + 2));

    /*
     * [SELECT ROUTE]:
     * @author hsiegeln
     */
    line_selector.setze_pos(koord(5, SELECT_VSTART + 14));
    line_selector.setze_groesse(koord(TOTAL_WIDTH - 8, 14));
    line_selector.set_max_size(koord(TOTAL_WIDTH - 8, LINESPACE*13+2+16));
    line_selector.set_highlight_color(1);

    /*
     * [CONVOI]
     */
    convoi.set_grid(koord(grid.x - grid_dx, grid.y));
    convoi.set_placement(koord(placement.x - placement_dx, placement.y));
    convoi.setze_pos(koord((TOTAL_WIDTH-CLIST_WIDTH)/2, CONVOI_VSTART));
    convoi.setze_groesse(koord(CLIST_WIDTH, CLIST_HEIGHT));

    lb_convoi_count.setze_pos(koord(4, CINFO_VSTART));
    lb_convoi_value.setze_pos(koord(TOTAL_WIDTH-10, CINFO_VSTART));
    lb_convoi_line.setze_pos(koord(4, CINFO_VSTART + LINESPACE));

    /*
     * [ACTIONS]
     */
    bt_start.setze_pos(koord(0, ACTIONS_VSTART));
    bt_start.setze_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_start.setze_text("Start");

    bt_schedule.setze_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART));
    bt_schedule.setze_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_schedule.setze_text("Fahrplan");

    bt_destroy.setze_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART));
    bt_destroy.setze_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
    bt_destroy.setze_text("Aufloesen");

    bt_sell.setze_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART));
    bt_sell.setze_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
    bt_sell.setze_text("Verkauf");

    /*
     * ACTIONS for new route management buttons
     * @author hsiegeln
     */
    bt_new_line.setze_pos(koord(0, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_new_line.setze_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_new_line.setze_text("New Line");

    bt_apply_line.setze_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_apply_line.setze_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_apply_line.setze_text("Apply Line");

    bt_change_line.setze_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_change_line.setze_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
    bt_change_line.setze_text("Update Line");

    bt_copy_convoi.setze_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_copy_convoi.setze_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
    bt_copy_convoi.setze_text("Copy Convoi");

	/*
	* [PANEL]
	*/
	tabs.setze_pos(koord(0, PANEL_VSTART));
	tabs.setze_groesse(koord(TOTAL_WIDTH, PANEL_HEIGHT));

	pas.set_grid(grid);
	pas.set_placement(placement);
	pas.setze_groesse(tabs.gib_groesse());
	pas.recalc_size();
	pas.setze_pos(koord(1,1));
	cont_pas.setze_groesse(pas.gib_groesse());
	scrolly_pas.setze_groesse(scrolly_pas.gib_groesse());

	loks.set_grid(grid);
	loks.set_placement(placement);
	loks.setze_groesse(tabs.gib_groesse());
	loks.recalc_size();
	loks.setze_pos(koord(1,1));
	cont_loks.setze_pos(koord(0,0));
	cont_loks.setze_groesse(loks.gib_groesse());
	scrolly_loks.setze_groesse(scrolly_loks.gib_groesse());

	waggons.set_grid(grid);
	waggons.set_placement(placement);
	waggons.setze_groesse(tabs.gib_groesse());
	waggons.recalc_size();
	waggons.setze_pos(koord(1,1));
	cont_waggons.setze_groesse(waggons.gib_groesse());
	scrolly_waggons.setze_groesse(scrolly_waggons.gib_groesse());

	div_tabbottom.setze_pos(koord(0,PANEL_VSTART+PANEL_HEIGHT));
	div_tabbottom.setze_groesse(koord(TOTAL_WIDTH,0));

	lb_veh_action.setze_pos(koord(TOTAL_WIDTH-ABUTTON_WIDTH, PANEL_VSTART + PANEL_HEIGHT+4));
	lb_veh_action.setze_text("Fahrzeuge:");

	bt_veh_action.setze_pos(koord(TOTAL_WIDTH-ABUTTON_WIDTH, PANEL_VSTART + PANEL_HEIGHT + 14));
	bt_veh_action.setze_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));

	bt_show_all.setze_pos(koord(TOTAL_WIDTH-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + PANEL_HEIGHT + 4 ));
	bt_show_all.setze_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	bt_show_all.pressed = show_all;

	bt_obsolete.setze_pos(koord(TOTAL_WIDTH-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + PANEL_HEIGHT + 16));
	bt_obsolete.setze_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	bt_obsolete.pressed = show_retired_vehicles;
}




void depot_frame_t::setze_fenstergroesse( koord gr )
{
	koord g=gr;
	layout(&g);
	update_data();
	gui_frame_t::setze_fenstergroesse(gr);
}


// true if already stored here
bool depot_frame_t::is_contained(const vehikel_besch_t *info)
{
	if(depot->vehicle_count()>0) {
		slist_iterator_tpl<vehikel_t *> iter(depot->get_vehicle_list());
		while(iter.next()) {
			if(iter.get_current()->gib_besch()==info) {
				return true;
			}
		}
	}
	return false;
}


// add a single vehicle (helper function)
void depot_frame_t::add_to_vehicle_list(const vehikel_besch_t *info)
{
	// prissi: ist a non-electric track?
	// Hajo: check for timeline
	// prissi: and retirement date
	gui_image_list_t::image_data_t img_data;

	img_data.image = info->gib_basis_bild();
	img_data.count = 0;
	img_data.lcolor = img_data.rcolor= EMPTY_IMAGE_BAR;

	// since they come "pre-sorted" for the vehikelbauer, we have to do nothing to keep them sorted
	if(info->gib_ware()==warenbauer_t::passagiere  ||  info->gib_ware()==warenbauer_t::post) {
		pas_vec.append(img_data,4);
		vehicle_map.set(info, &pas_vec.back());
	}
	else if(info->gib_leistung() > 0  ||  info->gib_zuladung()==0) {
		loks_vec.append(img_data,4);
		vehicle_map.set(info, &loks_vec.back());
	}
	else {
		waggons_vec.append(img_data,4);
		vehicle_map.set(info, &waggons_vec.back());
	}
}



// add all current vehicles
void depot_frame_t::build_vehicle_lists()
{
	const int month_now = World()->get_timeline_year_month();

	if (pas_vec.empty() && loks_vec.empty() && waggons_vec.empty()) {
		int loks = 0, waggons = 0, pax=0;
		slist_iterator_tpl<const vehikel_besch_t*> vehinfo(depot->get_vehicle_type());
		while (vehinfo.next()) {
			const vehikel_besch_t* info = vehinfo.get_current();
			if(info->gib_ware()==warenbauer_t::passagiere  ||  info->gib_ware()==warenbauer_t::post) {
				pax++;
			}
			else if(info->gib_leistung() > 0  ||  info->gib_zuladung()==0) {
				loks++;
			}
			else {
				waggons++;
			}
		}
		pas_vec.resize(pax);
		loks_vec.resize(loks);
		waggons_vec.resize(waggons);
	}
	pas_vec.clear();
	loks_vec.clear();
	waggons_vec.clear();

	vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification
	const schiene_t* sch = dynamic_cast<const schiene_t*>(World()->lookup(depot->gib_pos())->gib_weg(track_wt));
	const bool schiene_electric = (sch==NULL)  ||  sch->is_electrified();

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell) {
		// just list the one to sell
		slist_iterator_tpl<vehikel_t *> iter2(depot->get_vehicle_list());
		while(iter2.next()) {
			if(vehicle_map.get(iter2.get_current()->gib_besch())) {
				continue;
			}
			add_to_vehicle_list( iter2.get_current()->gib_besch() );
		}
	}
	else {
		// list only matching ones
		slist_iterator_tpl<const vehikel_besch_t*> vehinfo(depot->get_vehicle_type());
		while (vehinfo.next()) {
			const vehikel_besch_t* info = vehinfo.get_current();
			const vehikel_besch_t *veh = NULL;
			convoihandle_t cnv = depot->get_convoi(icnv);
			if(cnv.is_bound() && cnv->gib_vehikel_anzahl()>0) {
				veh = (veh_action == va_insert) ? cnv->gib_vehikel(0)->gib_besch() : cnv->gib_vehikel(cnv->gib_vehikel_anzahl() - 1)->gib_besch();
			}

			// current vehicle
			if( is_contained(info)  ||
				(schiene_electric  ||  info->get_engine_type()!=vehikel_besch_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) ) {
				// check, if allowed
				bool append = true;
				if(!show_all) {
					if(veh_action == va_insert) {
						append = !(!convoi_t::pruefe_nachfolger(info, veh) ||  veh && !convoi_t::pruefe_vorgaenger(info, veh));
					} else if(veh_action == va_append) {
						append = convoi_t::pruefe_vorgaenger(veh, info);
					}
				}
				if(append) {
					add_to_vehicle_list( info );
				}
			}
		}
	}
DBG_DEBUG("depot_frame_t::build_vehicle_lists()","finally %i passenger vehicle, %i  engines, %i good wagons",pas_vec.get_count(),loks_vec.get_count(),waggons_vec.get_count());
}


void depot_frame_t::update_data()
{
	static const char *txt_veh_action[3] = { "anhaengen", "voranstellen", "verkaufen" };

	// change green into blue for retired vehicles
	const int month_now = World()->get_timeline_year_month();

	bt_veh_action.setze_text(txt_veh_action[veh_action]);

	switch(depot->convoi_count()) {
		case 0:
			tstrncpy(txt_convois, translator::translate("no convois"), lengthof(txt_convois));
			break;
		case 1:
			if(icnv == -1) {
				sprintf(txt_convois, translator::translate("1 convoi"));
			} else {
				sprintf(txt_convois, translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count());
			}
			break;
		default:
			if(icnv == -1) {
				sprintf(txt_convois, translator::translate("%d convois"), depot->convoi_count());
			} else {
				sprintf(txt_convois, translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count());
			}
			break;
	}

	/*
	* Reset counts and check for valid vehicles
	*/
	convoihandle_t cnv = depot->get_convoi(icnv);
	const vehikel_besch_t *veh = NULL;

	convoi_pics.clear();
	if(cnv.is_bound() && cnv->gib_vehikel_anzahl() > 0) {

		tstrncpy(txt_cnv_name, cnv->gib_internal_name(), lengthof(txt_cnv_name));
		inp_name.setze_text(txt_cnv_name, lengthof(txt_cnv_name));

		unsigned i;
		for(i=0; i<cnv->gib_vehikel_anzahl(); i++) {

			// just make sure, there is this vehicle also here!
			const vehikel_besch_t *info=cnv->gib_vehikel(i)->gib_besch();
			if(vehicle_map.get(info)==NULL) {
				add_to_vehicle_list( info );
			}

			gui_image_list_t::image_data_t img_data;
			img_data.image = cnv->gib_vehikel(i)->gib_besch()->gib_basis_bild();
			img_data.count = 0;
			img_data.lcolor = img_data.rcolor= EMPTY_IMAGE_BAR;
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0].lcolor = convoi_t::pruefe_vorgaenger(NULL, cnv->gib_vehikel(0)->gib_besch()) ? COL_GREEN : COL_YELLOW;
		for(  i=1;  i<cnv->gib_vehikel_anzahl(); i++) {
			convoi_pics[i - 1].rcolor = convoi_t::pruefe_nachfolger(cnv->gib_vehikel(i - 1)->gib_besch(), cnv->gib_vehikel(i)->gib_besch()) ? COL_GREEN : COL_RED;
			convoi_pics[i].lcolor     = convoi_t::pruefe_vorgaenger(cnv->gib_vehikel(i - 1)->gib_besch(), cnv->gib_vehikel(i)->gib_besch()) ? COL_GREEN : COL_RED;
		}
		convoi_pics[i - 1].rcolor = convoi_t::pruefe_nachfolger(cnv->gib_vehikel(i - 1)->gib_besch(), NULL) ? COL_GREEN : COL_YELLOW;

		// change grren into blue for retired vehicles
		for(i=0;  i<cnv->gib_vehikel_anzahl(); i++) {
			if(cnv->gib_vehikel(i)->gib_besch()->is_future(month_now) || cnv->gib_vehikel(i)->gib_besch()->is_retired(month_now)) {
				if (convoi_pics[i].lcolor == COL_GREEN) {
					convoi_pics[i].lcolor = COL_BLUE;
				}
				if (convoi_pics[i].rcolor == COL_GREEN) {
					convoi_pics[i].rcolor = COL_BLUE;
				}
			}
		}

		if(veh_action == va_insert) {
			veh = cnv->gib_vehikel(0)->gib_besch();
		} else if(veh_action == va_append) {
			veh = cnv->gib_vehikel(cnv->gib_vehikel_anzahl() - 1)->gib_besch();
		}
	}

	ptrhashtable_iterator_tpl<const vehikel_besch_t *, gui_image_list_t::image_data_t *> iter1(vehicle_map);
	while(iter1.next()) {
		const vehikel_besch_t *info = iter1.get_current_key();
		const uint8 ok_color = info->is_future(month_now) || info->is_retired(month_now) ? COL_BLUE: COL_GREEN;

		iter1.get_current_value()->count = 0;
		iter1.get_current_value()->lcolor = ok_color;
		iter1.get_current_value()->rcolor = ok_color;

		/*
		* color bars for current convoi:
		*  green/green	okay to append/insert
		*  red/red		cannot be appended/inserted
		*  green/yellow	append okay, cannot be end of train
		*  yellow/green	insert okay, cannot be start of train
		*/

		if(veh_action == va_insert) {
			if (!convoi_t::pruefe_nachfolger(info, veh) ||  veh && !convoi_t::pruefe_vorgaenger(info, veh)) {
				iter1.get_current_value()->lcolor = COL_RED;
				iter1.get_current_value()->rcolor = COL_RED;
			} else if(!convoi_t::pruefe_vorgaenger(NULL, info)) {
				iter1.get_current_value()->lcolor = COL_YELLOW;
			}
		} else if(veh_action == va_append) {
			if(!convoi_t::pruefe_vorgaenger(veh, info) ||  veh && !convoi_t::pruefe_nachfolger(veh, info)) {
				iter1.get_current_value()->lcolor = COL_RED;
				iter1.get_current_value()->rcolor = COL_RED;
			} else if(!convoi_t::pruefe_nachfolger(info, NULL)) {
				iter1.get_current_value()->rcolor = COL_YELLOW;
			}
		}

//DBG_DEBUG("depot_frame_t::update_data()","current %i = %s with color %i",info->gib_name(),iter1.get_current_value()->lcolor);
	}

	slist_iterator_tpl<vehikel_t *> iter2(depot->get_vehicle_list());
	while(iter2.next()) {
		gui_image_list_t::image_data_t *imgdat=vehicle_map.get(iter2.get_current()->gib_besch());
		// can fail, if currently not visible
		if(imgdat) {
			imgdat->count++;
			if(veh_action == va_sell) {
				imgdat->lcolor = COL_GREEN;
				imgdat->rcolor = COL_GREEN;
			}
		}
	}

	// update the line selector
	line_selector.clear_elements();
	line_selector.append_element(no_line_text);
	if(!selected_line.is_bound()) {
		line_selector.setze_text(no_line_text, 128);
		line_selector.set_selection( 0 );
	}

	// check all matching lines
	slist_iterator_tpl<linehandle_t> iter( depot->get_line_list() );
	while( iter.next() ) {
		linehandle_t line = iter.get_current();
		line_selector.append_element( line->get_name(), line->get_state_color() );
		if(line==selected_line) {
			line_selector.setze_text( line->get_name(), 128);
			line_selector.set_selection( line_selector.count_elements()-1 );
		}
	}
}




sint32
depot_frame_t::calc_restwert(const vehikel_besch_t *veh_type)
{
	sint32 wert = 0;

	slist_iterator_tpl<vehikel_t *> iter(depot->get_vehicle_list());
	while(iter.next()) {
		if(iter.get_current()->gib_besch() == veh_type) {
			wert += iter.get_current()->calc_restwert();
		}
	}
	return wert;
}


// returns the indest of the old/newest vehicle in a list
vehikel_t* depot_frame_t::find_oldest_newest(const vehikel_besch_t* besch, bool old)
{
	vehikel_t* found_veh = NULL;
	slist_iterator_tpl<vehikel_t*> iter(depot->get_vehicle_list());
	if (iter.next()) {
		found_veh = iter.get_current();
		while (iter.next()) {
			vehikel_t* veh = iter.get_current();
			if (veh->gib_besch() == besch) {
				// joy of XOR, finally a line where I could use it!
				if (old ^ (found_veh->gib_insta_zeit() > veh->gib_insta_zeit())) {
					found_veh = veh;
				}
			}
		}
	}
	return found_veh;
}



void
depot_frame_t::image_from_storage_list(gui_image_list_t::image_data_t *bild_data)
{
	if(bild_data->lcolor != COL_RED && bild_data->rcolor != COL_RED) {
		// we buy/sell all vehicles together!
		slist_tpl<const vehikel_besch_t *>new_vehicle_info;
		const vehikel_besch_t *info = vehikelbauer_t::gib_info(bild_data->image);
		const vehikel_besch_t *start_info = info;

		if(veh_action==va_insert  ||  veh_action==va_sell) {
			// start of composition
			while(1) {
				if(info->gib_vorgaenger_count()!=1  ||  info->gib_vorgaenger(0)==NULL) {
					break;
				}
				info = info->gib_vorgaenger(0);
				new_vehicle_info.insert(info);
			}
			info = start_info;
		}
		// not get the end ...
		while(info) {
			new_vehicle_info.append( info );
DBG_MESSAGE("depot_frame_t::image_from_storage_list()","appended %s",info->gib_name() );
			if(info->gib_nachfolger_count()!=1  ||  (veh_action==va_insert  &&  info==start_info)) {
				break;
			}
			info = info->gib_nachfolger(0);
		}

		if(veh_action == va_sell) {
			while(new_vehicle_info.count()) {
				/*
				*	We sell the newest vehicle - gives most money back.
				*/
				vehikel_t* veh = find_oldest_newest(new_vehicle_info.remove_first(), false);
				depot->sell_vehicle(veh);
			}
		}
		else {

			// append/insert into convoi
			convoihandle_t cnv = depot->get_convoi(icnv);
			if(!cnv.is_bound()) {
				// create a new convoi
				cnv = depot->add_convoi();
				icnv = depot->convoi_count() - 1;
				cnv->setze_name(new_vehicle_info.front()->gib_name());
			}

			if(cnv->gib_vehikel_anzahl()+new_vehicle_info.count() <= depot->get_max_convoi_length()) {

				for( unsigned i=0;  i<new_vehicle_info.count();  i++ ) {
					// insert/append needs reverse order
					unsigned nr = (veh_action == va_insert) ? new_vehicle_info.count()-i-1 : i;
					// We add the oldest vehicle - newer stay for selling
					const vehikel_besch_t* vb = new_vehicle_info.at(nr);
					vehikel_t* veh = find_oldest_newest(vb, true);
DBG_MESSAGE("depot_frame_t::image_from_storage_list()","built nr %i", nr);
					if (veh == NULL) {
						// nothing there => we buy it
						veh = depot->buy_vehicle(vb);
					}
					depot->append_vehicle(cnv, veh, veh_action == va_insert);
				}
			}
		}

		build_vehicle_lists();
		update_data();
		layout(NULL);
	}
}



void depot_frame_t::image_from_convoi_list(uint nr)
{
	const convoihandle_t cnv = depot->get_convoi(icnv);

//DBG_DEBUG("depot_frame_t::bild_gewaehlt()","convoi index %i",nr);
	if(cnv.is_bound() &&  nr<cnv->gib_vehikel_anzahl() ) {

		// we remove all connected vehicles together!
		// find start
		unsigned start_nr = nr;
		while(start_nr>0) {
			start_nr --;
			const vehikel_besch_t *info = cnv->gib_vehikel(start_nr)->gib_besch();
			if(info->gib_nachfolger_count()!=1) {
				start_nr ++;
				break;
			}
		}
		// find end
		while(nr<cnv->gib_vehikel_anzahl()) {
			const vehikel_besch_t *info = cnv->gib_vehikel(nr)->gib_besch();
			nr ++;
			if(info->gib_nachfolger_count()!=1) {
				break;
			}
		}
		// now remove the vehicles
		if(cnv->gib_vehikel_anzahl()==nr-start_nr) {
			depot->disassemble_convoi(icnv, false);
			icnv--;
		}
		else {
			for( unsigned i=start_nr;  i<nr;  i++  ) {
				depot->remove_vehicle(cnv, start_nr);
			}
		}
	}
}



bool
depot_frame_t::action_triggered(gui_komponente_t *komp,value_t p)
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()) {
		cnv->setze_name(txt_cnv_name);
	}

	if(komp != NULL) {	// message from outside!
		if(komp == &bt_start) {
			if(depot->start_convoi(icnv)) {
				icnv--;
			}
		} else if(komp == &bt_schedule) {
			fahrplaneingabe();
			return true;
		} else if(komp == &bt_destroy) {
			if(depot->disassemble_convoi(icnv, false)) {
				icnv--;
			}
		} else if(komp == &bt_sell) {
			if(depot->disassemble_convoi(icnv, true)) {
				icnv--;
			}
		} else if(komp == &bt_next) {
			if(++icnv == (int)depot->convoi_count()) {
				icnv = -1;
			}
		} else if(komp == &bt_prev) {
			if(icnv-- == -1) {
				icnv = depot->convoi_count() - 1;
			}
			// image lsit selction here ...
		} else if(komp == &convoi) {
			image_from_convoi_list( p.i );
		} else if(komp == &pas) {
			image_from_storage_list(&pas_vec[p.i]);
		} else if(komp == &loks) {
			image_from_storage_list(&loks_vec[p.i]);
		} else if(komp == &waggons) {
			image_from_storage_list(&waggons_vec[p.i]);
		} else if(komp == &bt_obsolete) {
			show_retired_vehicles = (show_retired_vehicles==0);
		} else if(komp == &bt_show_all) {
			show_all = (show_all==0);
		} else if(komp == &bt_veh_action) {
			if(veh_action== va_sell) {
				veh_action = va_append;
			}
			else {
				veh_action = veh_action+1;
			}
		} else if(komp == &bt_new_line) {
			new_line();
			return true;
		} else if(komp == &bt_change_line) {
			change_line();
			return true;
		} else if(komp == &bt_copy_convoi) {
			depot->copy_convoi(icnv);
			// automatically select newly created convoi
			icnv = depot->convoi_count()-1;
		} else if(komp == &bt_apply_line) {
			apply_line();
		} else if(komp == &line_selector) {
			int sel=line_selector.get_selection();
//DBG_MESSAGE("depot_frame_t::action_triggered()","selected %i",sel);
			if(sel>0) {
				selected_line = depot->get_line_list()->at(sel-1);
			}
			else {
				selected_line = linehandle_t();
			}
		}
		else {
			return false;
		}
		build_vehicle_lists();
	}
	update_data();
	layout(NULL);
	return true;
}


void depot_frame_t::infowin_event(const event_t *ev)
{
	gui_frame_t::infowin_event(ev);

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {

		bool dir = (ev->ev_code==NEXT_WINDOW);
		depot_t *next_dep = depot_t::find_depot( depot->gib_pos(), depot->gib_typ(), dir == NEXT_WINDOW );
		if(next_dep == NULL) {
			if(dir == NEXT_WINDOW) {
				// check the next from start of map
				next_dep = depot_t::find_depot( koord3d(-1,-1,0), depot->gib_typ(), true );
			}
			else {
				// respecive end of map
				next_dep = depot_t::find_depot( koord3d(8192,8192,127), depot->gib_typ(), false );
			}
		}

		if(next_dep) {
			/**
			 * Replace our depot_frame_t with a new at the same position.
			 * Volker Meyer
			 */
			int x = win_get_posx(this);
			int y = win_get_posy(this);
			destroy_win( this );

			next_dep->zeige_info();
			win_set_pos(next_dep->get_info_win(), x, y);
			World()->setze_ij_off(next_dep->gib_pos());
		}
	} else if(IS_WINDOW_REZOOM(ev)) {
		koord gr = gib_fenstergroesse();
		setze_fenstergroesse(gr);
	} else if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN) {
		build_vehicle_lists();
		update_data();
		layout(NULL);
	}
	else {
		if(IS_LEFTCLICK(ev) &&  !line_selector.getroffen(ev->cx, ev->cy-16)) {
			// close combo box; we must do it ourselves, since the box does not recieve outside events ...
			line_selector.close_box();
		}
	}
}



void
depot_frame_t::zeichnen(koord pos, koord groesse)
{
	if (World()->get_active_player() != depot->gib_besitzer()) {
		destroy_win(this);
	}

	const convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()) {
		if(cnv->gib_vehikel_anzahl() > 0) {
			int length=0;
			for( unsigned i=0;  i<cnv->gib_vehikel_anzahl();  i++) {
				length += cnv->gib_vehikel(i)->gib_besch()->get_length();
			}
			sprintf(txt_convoi_count, "%s %d (%s %i)",
				translator::translate("Fahrzeuge:"), cnv->gib_vehikel_anzahl(),
				translator::translate("Station tiles:"), (length+15)/16 );
			sprintf(txt_convoi_value, "%s %d$", translator::translate("Restwert:"), cnv->calc_restwert()/100);
			if (cnv->get_line()!=NULL) {
				sprintf(txt_convoi_line, "%s %s", translator::translate("Serves Line:"), 	cnv->get_line()->get_name());
			}
			else {
				sprintf(txt_convoi_line, "%s %s", translator::translate("Serves Line:"), no_line_text);
			}
		}
		else {
			tstrncpy(txt_convoi_count, "keine Fahrzeuge", lengthof(txt_convoi_count));
			*txt_convoi_value = '\0';
		}
	}
	else {
		inp_name.setze_text("\0", 0);
		*txt_convoi_count = '\0';
		*txt_convoi_value = '\0';
		*txt_convoi_line = '\0';
	}

	bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
	bt_show_all.pressed = show_all;	// otherwise the button would not show depressed

	gui_frame_t::zeichnen(pos, groesse);

	if(!cnv.is_bound()) {
		display_proportional_clip(pos.x+inp_name.gib_pos().x+2, pos.y+inp_name.gib_pos().y+18, translator::translate("new convoi"), ALIGN_LEFT, COL_GREY1, true);
	}
	draw_vehicle_info_text(pos);
}


/*
 * scheduling system for simutrans
 * @author hsiegeln
 */
void depot_frame_t::new_line()
{
	selected_line = depot->create_line();
DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
	layout(NULL);
	update_data();
	create_win(-1, -1, new line_management_gui_t(selected_line, depot->gib_besitzer()), w_info);
DBG_MESSAGE("depot_frame_t::new_line()","id=%d",selected_line.get_id() );
}

void depot_frame_t::apply_line()
{
	if(icnv > -1) {
		convoihandle_t cnv = depot->get_convoi(icnv);
		// if no convoi is selected, do nothing
		if(cnv==NULL) {
			return;
		}

		if(selected_line.is_bound()) {
			// set new route only, a valid route is selected:
			cnv->set_line(selected_line);
		}
		else {
			// sometimes the user might wish to remove convoy from line
			// this happens here
			cnv->unset_line();
		}
	}
}

void depot_frame_t::change_line()
{
	if(selected_line.is_bound()) {
		create_win(-1, -1, new line_management_gui_t(selected_line, depot->gib_besitzer()), w_info);
	}
}

/*
 * end of scheduling system for simutrans
 * @author hsiegeln
 */

void depot_frame_t::fahrplaneingabe()
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound() && cnv->gib_vehikel_anzahl() > 0) {

		fahrplan_t *fpl = cnv->erzeuge_fahrplan();
		if(fpl != NULL && fpl->ist_abgeschlossen()) {

			// Fahrplandialog oeffnen
			create_win(-1, -1, new fahrplan_gui_t(fpl, cnv->gib_besitzer()), w_info);

			// evtl. hat ein callback cnv gelöscht, so erneut testen
			if(cnv.is_bound() && fpl != NULL) {
				cnv->setze_fahrplan(fpl);
			}
		}
		else {
			create_win(100, 64, new nachrichtenfenster_t(World(), "Es wird bereits\nein Fahrplan\neingegeben\n"), w_autodelete);
		}
	}
	else {
		create_win(100, 64, new nachrichtenfenster_t(World(), "Please choose vehicles first\n"), w_autodelete);
	}
}



void
depot_frame_t::draw_vehicle_info_text(koord pos)
{
	static char buf[1024];
	const char *c;

	gui_image_list_t *lst = dynamic_cast<gui_image_list_t *>(tabs.get_active_tab_index() == 0 ? &pas :(tabs.get_active_tab_index() == 1 ? &loks : &waggons));
	int x = gib_maus_x();
	int y = gib_maus_y();
	int value = -1;
	const vehikel_besch_t *veh_type = NULL;
	koord relpos = koord( 0, ((gui_scrollpane_t *)tabs.gib_aktives_tab())->get_scroll_y() );
	int sel_index = lst->index_at(pos + tabs.gib_pos() - relpos, x, y - 16 - gui_tab_panel_t::HEADER_VSIZE);

	if ((sel_index != -1) && (tabs.getroffen(x-pos.x,y-pos.y))) {
		const vector_tpl<gui_image_list_t::image_data_t>& vec = (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec));
		veh_type = vehikelbauer_t::gib_info(vec[sel_index].image);
		if (vec[sel_index].count > 0) {
			value = calc_restwert(veh_type) / 100;
		}
	}
	else {
		sel_index = convoi.index_at(pos , x, y - 16);
		if(sel_index != -1) {
			convoihandle_t cnv = depot->get_convoi(icnv);
			veh_type = cnv->gib_vehikel(sel_index)->gib_besch();
			value = cnv->gib_vehikel(sel_index)->calc_restwert()/100;
		}
	}

	switch(depot->vehicle_count()) {
		case 0:
			c = translator::translate("Keine Einzelfahrzeuge im Depot");
			break;
		case 1:
			c = translator::translate("1 Einzelfahrzeug im Depot");
			break;
		default:
			sprintf(buf, translator::translate("%d Einzelfahrzeuge im Depot"), depot->vehicle_count());
			c = buf;
			break;
	}
	display_proportional( pos.x + 4, pos.y + tabs.gib_pos().y + tabs.gib_groesse().y + 16 + 4, c, ALIGN_LEFT, COL_BLACK, true );

	if(veh_type) {
		// lok oder waggon ?
		if(veh_type->gib_leistung() > 0) {
			//lok
			const int zuladung = veh_type->gib_zuladung();

			char name[128];

			sprintf(name,
			"%s (%s)",
			translator::translate(veh_type->gib_name()),
			translator::translate(engine_type_names[veh_type->get_engine_type()+1]));

			int n = sprintf(buf,
				translator::translate("LOCO_INFO"),
				name,
				veh_type->gib_preis()/100,
				veh_type->gib_betriebskosten()/100.0,
				veh_type->gib_leistung(),
				veh_type->gib_geschw(),
				veh_type->gib_gewicht()
				);

			if(zuladung>0) {
				sprintf(buf + n,
					translator::translate("LOCO_CAP"),
					zuladung,
					translator::translate(veh_type->gib_ware()->gib_mass()),
					veh_type->gib_ware()->gib_catg() == 0 ? translator::translate(veh_type->gib_ware()->gib_name()) : translator::translate(veh_type->gib_ware()->gib_catg_name())
					);
			}

		}
		else {
			// waggon
			sprintf(buf,
				translator::translate("WAGGON_INFO"),
				translator::translate(veh_type->gib_name()),
				veh_type->gib_preis()/100,
				veh_type->gib_betriebskosten()/100.0,
				veh_type->gib_zuladung(),
				translator::translate(veh_type->gib_ware()->gib_mass()),
				veh_type->gib_ware()->gib_catg() == 0 ?
				translator::translate(veh_type->gib_ware()->gib_name()) :
				translator::translate(veh_type->gib_ware()->gib_catg_name()),
				veh_type->gib_gewicht(),
				veh_type->gib_geschw()
				);
		}
		display_multiline_text( pos.x + 4, pos.y + tabs.gib_pos().y + tabs.gib_groesse().y + 31 + LINESPACE*1 + 4, buf,  COL_BLACK);

		// column 2
		int n = sprintf(buf, "%s %s %04d\n",
			translator::translate("Intro. date:"),
			translator::get_month_name(veh_type->get_intro_year_month()%12),
			veh_type->get_intro_year_month()/12 );

		if(veh_type->get_retire_year_month() !=DEFAULT_RETIRE_DATE*12) {
			n += sprintf(buf+n, "%s %s %04d\n",
				translator::translate("Retire. date:"),
				translator::get_month_name(veh_type->get_retire_year_month()%12),
				veh_type->get_retire_year_month()/12 );
		}

		if(veh_type->gib_leistung() > 0  &&  veh_type->get_gear()!=64) {
			n+= sprintf(buf+n, "%s %0.2f : 1\n", translator::translate("Gear:"), 	veh_type->get_gear()/64.0);
		}

		if(veh_type->gib_copyright()!=NULL  &&  veh_type->gib_copyright()[0]!=0) {
			n += sprintf(buf+n, "%s%s\n",translator::translate("Constructed by"),veh_type->gib_copyright());
		}

		if(value != -1) {
			sprintf(buf + strlen(buf), "%s %d Cr", translator::translate("Restwert:"), 	value);
		}

		display_multiline_text( pos.x + 200, pos.y + tabs.gib_pos().y + tabs.gib_groesse().y + 31 + LINESPACE*2 + 4, buf, COL_BLACK);
	}
}
