/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <stdio.h>
#include <math.h>

#include "../simworld.h"
#include "../vehicle/simvehikel.h"
#include "../simconvoi.h"
#include "../simdepot.h"
#include "../simwin.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../simgraph.h"
#include "../simline.h"
#include "../simlinemgmt.h"

#include "../tpl/slist_tpl.h"

#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "line_item.h"
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
#include "../utils/cbuffer_t.h"

#include "../bauer/warenbauer.h"

#include "../boden/wege/weg.h"


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
	gui_frame_t(txt_title, depot->get_besitzer()),
	depot(depot),
	icnv(depot->convoi_count()-1),
	lb_convois(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_count(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_speed(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_value(NULL, COL_BLACK, gui_label_t::right),
	lb_convoi_line(NULL, COL_BLACK, gui_label_t::left),
	lb_veh_action("Fahrzeuge:", COL_BLACK, gui_label_t::left),
	convoi_pics(depot->get_max_convoi_length()),
	convoi(&convoi_pics),
	pas(&pas_vec),
	electrics(&electrics_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&cont_pas),
	scrolly_electrics(&cont_electrics),
	scrolly_loks(&cont_loks),
	scrolly_waggons(&cont_waggons)
{
DBG_DEBUG("depot_frame_t::depot_frame_t()","get_max_convoi_length()=%i",depot->get_max_convoi_length());
	selected_line = depot->get_selected_line();
	strcpy(no_line_text, translator::translate("<no line>"));

	sprintf(txt_title, "(%d,%d) %s", depot->get_pos().x, depot->get_pos().y, translator::translate(depot->get_name()));

	/*
	* [SELECT]:
	*/
	add_komponente(&lb_convois);

	bt_prev.set_typ(button_t::arrowleft);
	bt_prev.add_listener(this);
	add_komponente(&bt_prev);

	add_komponente(&inp_name);

	bt_next.set_typ(button_t::arrowright);
	bt_next.add_listener(this);
	add_komponente(&bt_next);

	/*
	* [SELECT ROUTE]:
	*/
	line_selector.set_highlight_color(PLAYER_FLAG | (depot->get_besitzer()->get_player_color1()+1));
	line_selector.add_listener(this);
	add_komponente(&line_selector);
	depot->get_besitzer()->simlinemgmt.sort_lines();

	/*
	* [CONVOI]
	*/
	convoi.set_player_nr(depot->get_player_nr());
	convoi.add_listener(this);

	add_komponente(&convoi);
	add_komponente(&lb_convoi_count);
	add_komponente(&lb_convoi_speed);
	add_komponente(&lb_convoi_value);
	add_komponente(&lb_convoi_line);

	/*
	* [ACTIONS]
	*/
	bt_start.set_typ(button_t::roundbox);
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_komponente(&bt_start);

	bt_schedule.set_typ(button_t::roundbox);
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule");
	add_komponente(&bt_schedule);

	bt_destroy.set_typ(button_t::roundbox);
	bt_destroy.add_listener(this);
	bt_destroy.set_tooltip("Move the selected vehicle(s) back to the depot");
	add_komponente(&bt_destroy);

	bt_sell.set_typ(button_t::roundbox);
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_komponente(&bt_sell);

	/*
	* new route management buttons
	*/
	bt_new_line.set_typ(button_t::roundbox);
	bt_new_line.add_listener(this);
	bt_new_line.set_tooltip("Lines are used to manage groups of vehicles");
	add_komponente(&bt_new_line);

	bt_apply_line.set_typ(button_t::roundbox);
	bt_apply_line.add_listener(this);
	bt_apply_line.set_tooltip("Add the selected vehicle(s) to the selected line");
	add_komponente(&bt_apply_line);

	bt_change_line.set_typ(button_t::roundbox);
	bt_change_line.add_listener(this);
	bt_change_line.set_tooltip("Modify the selected line");
	add_komponente(&bt_change_line);

	bt_copy_convoi.set_typ(button_t::roundbox);
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line");
	add_komponente(&bt_copy_convoi);

	/*
	* [PANEL]
	*/
	pas.set_player_nr(depot->get_player_nr());
	pas.add_listener(this);

	electrics.set_player_nr(depot->get_player_nr());
	electrics.add_listener(this);

	loks.set_player_nr(depot->get_player_nr());
	loks.add_listener(this);

	waggons.set_player_nr(depot->get_player_nr());
	waggons.add_listener(this);

	add_komponente(&tabs);
	add_komponente(&div_tabbottom);
	add_komponente(&lb_veh_action);

	veh_action = va_append;
	bt_veh_action.set_typ(button_t::roundbox);
	bt_veh_action.add_listener(this);
	bt_veh_action.set_tooltip("Choose operation executed on clicking stored/new vehicles");
	add_komponente(&bt_veh_action);

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

	koord gr = koord(0,0);
	layout(&gr);
	build_vehicle_lists();
	gui_frame_t::set_fenstergroesse(gr);

	// text will be translated by ourselves (after update data)!
	lb_convois.set_text_pointer(txt_convois);
	lb_convoi_count.set_text_pointer(txt_convoi_count);
	lb_convoi_speed.set_text_pointer(txt_convoi_speed);
	lb_convoi_value.set_text_pointer(txt_convoi_value);
	lb_convoi_line.set_text_pointer(txt_convoi_line);

	// Bolt image for electrified depots:
	add_komponente(&img_bolt);

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);
}



void depot_frame_t::layout(koord *gr)
{
	koord placement;
	koord grid;
	int grid_dx;
	int placement_dx;

	koord fgr = (gr!=NULL)? *gr : get_fenstergroesse();

	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	*/
	grid.x = depot->get_x_grid() * get_base_tile_raster_width() / 64 + 4;
	grid.y = depot->get_y_grid() * get_base_tile_raster_width() / 64 + 6;
	placement.x = depot->get_x_placement() * get_base_tile_raster_width() / 64 + 2;
	placement.y = depot->get_y_placement() * get_base_tile_raster_width() / 64 + 2;
	if(depot->get_wegtyp()==road_wt  &&  umgebung_t::drive_on_left) {
		// correct for dive on left
		placement.x -= (12*get_base_tile_raster_width())/64;
		placement.y -= (6*get_base_tile_raster_width())/64;
	}
	grid_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 2;
	placement_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 4;

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
	int ACTIONS_VSTART = CINFO_VSTART + CINFO_HEIGHT + 2 + LINESPACE * 2;
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
		gui_frame_t::set_fenstergroesse(koord(MIN_TOTAL_WIDTH, max(fgr.y,MIN_TOTAL_HEIGHT) ));
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
	lb_convois.set_pos(koord(4, SELECT_VSTART - 10));

	bt_prev.set_pos(koord(5, SELECT_VSTART + 2));

	inp_name.set_pos(koord(5+12, SELECT_VSTART));
	inp_name.set_groesse(koord(TOTAL_WIDTH - 26-8, 14));

	bt_next.set_pos(koord(TOTAL_WIDTH - 15, SELECT_VSTART + 2));

	/*
	 * [SELECT ROUTE]:
	 * @author hsiegeln
	 */
	line_selector.set_pos(koord(5, SELECT_VSTART + 14));
	line_selector.set_groesse(koord(TOTAL_WIDTH - 8, 14));
	line_selector.set_max_size(koord(TOTAL_WIDTH - 8, LINESPACE*13+2+16));
	line_selector.set_highlight_color(1);

	/*
	 * [CONVOI]
	 */
	convoi.set_grid(koord(grid.x - grid_dx, grid.y));
	convoi.set_placement(koord(placement.x - placement_dx, placement.y));
	convoi.set_pos(koord((TOTAL_WIDTH-CLIST_WIDTH)/2, CONVOI_VSTART));
	convoi.set_groesse(koord(CLIST_WIDTH, CLIST_HEIGHT));

	lb_convoi_count.set_pos(koord(4, CINFO_VSTART));
	lb_convoi_speed.set_pos(koord(4, CINFO_VSTART + LINESPACE));
	lb_convoi_value.set_pos(koord(TOTAL_WIDTH-10, CINFO_VSTART));
	lb_convoi_line.set_pos(koord(4, CINFO_VSTART + LINESPACE * 2));

	/*
	 * [ACTIONS]
	 */
	bt_start.set_pos(koord(0, ACTIONS_VSTART));
	bt_start.set_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_start.set_text("Start");

	bt_schedule.set_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART));
	bt_schedule.set_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_schedule.set_text("Fahrplan");

	bt_destroy.set_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART));
	bt_destroy.set_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
	bt_destroy.set_text("Aufloesen");

	bt_sell.set_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART));
	bt_sell.set_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
	bt_sell.set_text("Verkauf");

	/*
	 * ACTIONS for new route management buttons
	 * @author hsiegeln
	 */
	bt_new_line.set_pos(koord(0, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_new_line.set_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_new_line.set_text("New Line");

	bt_apply_line.set_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_apply_line.set_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
	bt_apply_line.set_text("Apply Line");

	bt_change_line.set_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_change_line.set_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
	bt_change_line.set_text("Update Line");

	bt_copy_convoi.set_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
	bt_copy_convoi.set_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
	bt_copy_convoi.set_text("Copy Convoi");

	/*
	* [PANEL]
	*/
	tabs.set_pos(koord(0, PANEL_VSTART));
	tabs.set_groesse(koord(TOTAL_WIDTH, PANEL_HEIGHT));

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

	div_tabbottom.set_pos(koord(0,PANEL_VSTART+PANEL_HEIGHT));
	div_tabbottom.set_groesse(koord(TOTAL_WIDTH,0));

	lb_veh_action.set_pos(koord(TOTAL_WIDTH-ABUTTON_WIDTH, PANEL_VSTART + PANEL_HEIGHT+4));

	bt_veh_action.set_pos(koord(TOTAL_WIDTH-ABUTTON_WIDTH, PANEL_VSTART + PANEL_HEIGHT + 14));
	bt_veh_action.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));

	bt_show_all.set_pos(koord(TOTAL_WIDTH-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + PANEL_HEIGHT + 4 ));
	bt_show_all.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	bt_show_all.pressed = show_all;

	bt_obsolete.set_pos(koord(TOTAL_WIDTH-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + PANEL_HEIGHT + 16));
	bt_obsolete.set_groesse(koord(ABUTTON_WIDTH, ABUTTON_HEIGHT));
	bt_obsolete.pressed = show_retired_vehicles;

	const uint8 margin = 4;
	img_bolt.set_pos(koord(get_fenstergroesse().x-skinverwaltung_t::electricity->get_bild(0)->get_pic()->w-margin,margin));
}




void depot_frame_t::set_fenstergroesse( koord gr )
{
	koord g=gr;
	layout(&g);
	update_data();
	gui_frame_t::set_fenstergroesse(gr);
}



void depot_frame_t::activate_convoi( convoihandle_t c )
{
	// deselect ...
	icnv = -1;
	for(  uint i=0;  i<depot->convoi_count();  i++  ) {
		if(  c==depot->get_convoi(i)  ) {
			icnv = i;
			break;
		}
	}
	build_vehicle_lists();
}




// true if already stored here
bool depot_frame_t::is_contained(const vehikel_besch_t *info)
{
	if(depot->vehicle_count()>0) {
		slist_iterator_tpl<vehikel_t *> iter(depot->get_vehicle_list());
		while(iter.next()) {
			if(iter.get_current()->get_besch()==info) {
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
	else if(info->get_leistung() > 0  ||  info->get_zuladung()==0) {
		loks_vec.append(img_data);
		vehicle_map.set(info, &loks_vec.back());
	}
	else {
		waggons_vec.append(img_data);
		vehicle_map.set(info, &waggons_vec.back());
	}
}



// add all current vehicles
void depot_frame_t::build_vehicle_lists()
{
	if(depot->get_vehicle_type()==NULL) {
		// there are tracks etc. but now vehicles => do nothing
		return;
	}

	const int month_now = get_welt()->get_timeline_year_month();

	/*
	 * The next block calculates upper bounds for the sizes of the vectors.
	 * If the vectors get resized, the vehicle_map becomes invalid, therefore
	 * we need to resize them before filling them.
	 */
	if(electrics_vec.empty()  &&  pas_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty()) {
		int loks = 0, waggons = 0, pax=0, electrics = 0;
		slist_iterator_tpl<const vehikel_besch_t*> vehinfo(depot->get_vehicle_type());
		while (vehinfo.next()) {
			const vehikel_besch_t* info = vehinfo.get_current();
			if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)) {
				electrics++;
			}
			else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) {
				pax++;
			}
			else if(info->get_leistung() > 0  ||  info->get_zuladung()==0) {
				loks++;
			}
			else {
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

	// we do not allow to built electric vehicle in a depot without electrification
	const waytype_t wt = depot->get_wegtyp();
	const weg_t *w = get_welt()->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
	const bool weg_electrified = w ? w->is_electrified() : false;

	img_bolt.set_image( weg_electrified ? skinverwaltung_t::electricity->get_bild_nr(0) : IMG_LEER );

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell) {
		// just list the one to sell
		slist_iterator_tpl<vehikel_t *> iter2(depot->get_vehicle_list());
		while(iter2.next()) {
			if(vehicle_map.get(iter2.get_current()->get_besch())) {
				continue;
			}
			add_to_vehicle_list( iter2.get_current()->get_besch() );
		}
	}
	else {
		// list only matching ones
		slist_iterator_tpl<const vehikel_besch_t*> vehinfo(depot->get_vehicle_type());
		while (vehinfo.next()) {
			const vehikel_besch_t* info = vehinfo.get_current();
			const vehikel_besch_t *veh = NULL;
			convoihandle_t cnv = depot->get_convoi(icnv);
			if(cnv.is_bound() && cnv->get_vehikel_anzahl()>0) {
				veh = (veh_action == va_insert) ? cnv->get_vehikel(0)->get_besch() : cnv->get_vehikel(cnv->get_vehikel_anzahl() - 1)->get_besch();
			}

			// current vehicle
			if( is_contained(info)  ||
				((weg_electrified  ||  info->get_engine_type()!=vehikel_besch_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) {
				// check, if allowed
				bool append = true;
				if(!show_all) {
					if(veh_action == va_insert) {
						append = !(!convoi_t::pruefe_nachfolger(info, veh) || (veh && !convoi_t::pruefe_vorgaenger(info, veh)));
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
	update_data();
	update_tabs();
}


static void get_line_list(const depot_t* depot, vector_tpl<linehandle_t>* lines)
{
	depot->get_besitzer()->simlinemgmt.get_lines(depot->get_line_type(), lines);
}


void depot_frame_t::update_data()
{
	static const char *txt_veh_action[3] = { "anhaengen", "voranstellen", "verkaufen" };

	// change green into blue for retired vehicles
	const int month_now = get_welt()->get_timeline_year_month();

	bt_veh_action.set_text(txt_veh_action[veh_action]);

	switch(depot->convoi_count()) {
		case 0:
			tstrncpy(txt_convois, translator::translate("no convois"), lengthof(txt_convois));
			break;
		case 1:
			if(icnv == -1) {
				sprintf(txt_convois, translator::translate("1 convoi"));
			}
			else {
				sprintf(txt_convois, translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count());
			}
			break;
		default:
			if(icnv == -1) {
				sprintf(txt_convois, translator::translate("%d convois"), depot->convoi_count());
			}
			else {
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
	if(cnv.is_bound() && cnv->get_vehikel_anzahl() > 0) {

		tstrncpy(txt_cnv_name, cnv->get_internal_name(), lengthof(txt_cnv_name));
		inp_name.set_text(txt_cnv_name, lengthof(txt_cnv_name));

		unsigned i;
		for(i=0; i<cnv->get_vehikel_anzahl(); i++) {

			// just make sure, there is this vehicle also here!
			const vehikel_besch_t *info=cnv->get_vehikel(i)->get_besch();
			if(vehicle_map.get(info)==NULL) {
				add_to_vehicle_list( info );
			}

			gui_image_list_t::image_data_t img_data;
			img_data.image = cnv->get_vehikel(i)->get_besch()->get_basis_bild();
			img_data.count = 0;
			img_data.lcolor = img_data.rcolor= EMPTY_IMAGE_BAR;
			img_data.text = cnv->get_vehikel(i)->get_besch()->get_name();
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0].lcolor = convoi_t::pruefe_vorgaenger(NULL, cnv->get_vehikel(0)->get_besch()) ? COL_GREEN : COL_YELLOW;
		for(  i=1;  i<cnv->get_vehikel_anzahl(); i++) {
			convoi_pics[i - 1].rcolor = convoi_t::pruefe_nachfolger(cnv->get_vehikel(i - 1)->get_besch(), cnv->get_vehikel(i)->get_besch()) ? COL_GREEN : COL_RED;
			convoi_pics[i].lcolor     = convoi_t::pruefe_vorgaenger(cnv->get_vehikel(i - 1)->get_besch(), cnv->get_vehikel(i)->get_besch()) ? COL_GREEN : COL_RED;
		}
		convoi_pics[i - 1].rcolor = convoi_t::pruefe_nachfolger(cnv->get_vehikel(i - 1)->get_besch(), NULL) ? COL_GREEN : COL_YELLOW;

		// change grren into blue for retired vehicles
		for(i=0;  i<cnv->get_vehikel_anzahl(); i++) {
			if(cnv->get_vehikel(i)->get_besch()->is_future(month_now) || cnv->get_vehikel(i)->get_besch()->is_retired(month_now)) {
				if (convoi_pics[i].lcolor == COL_GREEN) {
					convoi_pics[i].lcolor = COL_BLUE;
				}
				if (convoi_pics[i].rcolor == COL_GREEN) {
					convoi_pics[i].rcolor = COL_BLUE;
				}
			}
		}

		if(veh_action == va_insert) {
			veh = cnv->get_vehikel(0)->get_besch();
		} else if(veh_action == va_append) {
			veh = cnv->get_vehikel(cnv->get_vehikel_anzahl() - 1)->get_besch();
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
		} else if( veh_action == va_sell ) {
			iter1.get_current_value()->lcolor = COL_RED;
			iter1.get_current_value()->rcolor = COL_RED;
		}

//DBG_DEBUG("depot_frame_t::update_data()","current %i = %s with color %i",info->get_name(),iter1.get_current_value()->lcolor);
	}

	slist_iterator_tpl<vehikel_t *> iter2(depot->get_vehicle_list());
	while(iter2.next()) {
		gui_image_list_t::image_data_t *imgdat=vehicle_map.get(iter2.get_current()->get_besch());
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
	selected_line = depot->get_selected_line();
	line_selector.clear_elements();
	line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( no_line_text, COL_BLACK ) );
	if(!selected_line.is_bound()) {
		line_selector.set_selection( 0 );
	}

	// check all matching lines
	vector_tpl<linehandle_t> lines;
	get_line_list(depot, &lines);
	for (vector_tpl<linehandle_t>::const_iterator i = lines.begin(), end = lines.end(); i != end; i++) {
		linehandle_t line = *i;
		line_selector.append_element( new line_scrollitem_t(line) );
		if(line==selected_line) {
			line_selector.set_selection( line_selector.count_elements()-1 );
		}
	}
}


sint32 depot_frame_t::calc_restwert(const vehikel_besch_t *veh_type)
{
	sint32 wert = 0;

	slist_iterator_tpl<vehikel_t *> iter(depot->get_vehicle_list());
	while(iter.next()) {
		if(iter.get_current()->get_besch() == veh_type) {
			wert += iter.get_current()->calc_restwert();
		}
	}
	return wert;
}


void depot_frame_t::image_from_storage_list(gui_image_list_t::image_data_t *bild_data)
{
	if(  bild_data->lcolor != COL_RED  &&  bild_data->rcolor != COL_RED  ) {
		if(  veh_action == va_sell  ) {
			depot->call_depot_tool( 's', convoihandle_t(), bild_data->text );
		}
		else {
			depot->call_depot_tool( veh_action == va_insert ? 'i' : 'a', depot->get_convoi(icnv), bild_data->text );
		}
	}
}



void depot_frame_t::image_from_convoi_list(uint nr)
{
	const convoihandle_t cnv = depot->get_convoi(icnv);
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

		cbuffer_t start(16);
		start.append( start_nr );

		depot->call_depot_tool( 'r', cnv, start );
	}
}



bool depot_frame_t::action_triggered( gui_action_creator_t *komp,value_t p)
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()) {
		cnv->set_name(txt_cnv_name);
	}

	if(komp != NULL) {	// message from outside!
		if(komp == &bt_start) {
			depot->call_depot_tool( 'b', cnv, NULL );
		} else if(komp == &bt_schedule) {
			fahrplaneingabe();
			return true;
		} else if(komp == &bt_destroy) {
			depot->call_depot_tool( 'd', cnv, NULL );
		} else if(komp == &bt_sell) {
			depot->call_depot_tool( 'v', cnv, NULL );
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
		} else if (komp == &electrics) {
			image_from_storage_list(&electrics_vec[p.i]);
		} else if(komp == &loks) {
			image_from_storage_list(&loks_vec[p.i]);
		} else if(komp == &waggons) {
			image_from_storage_list(&waggons_vec[p.i]);
		} else if(komp == &bt_obsolete) {
			show_retired_vehicles = (show_retired_vehicles==0);
			depot_t::update_all_win();
		} else if(komp == &bt_show_all) {
			show_all = (show_all==0);
			depot_t::update_all_win();
		} else if(komp == &bt_veh_action) {
			if(veh_action== va_sell) {
				veh_action = va_append;
			}
			else {
				veh_action = veh_action+1;
			}
		} else if(komp == &bt_new_line) {
			depot->call_depot_tool( 'l', convoihandle_t(), NULL );
			return true;
		} else if(komp == &bt_change_line) {
			if(selected_line.is_bound()) {
				create_win(new line_management_gui_t(selected_line, depot->get_besitzer()), w_info, (long)selected_line.get_rep() );
			}
			return true;
		} else if(komp == &bt_copy_convoi) {
			if(  convoihandle_t::is_exhausted()  ) {
				create_win( new news_img("Convoi handles exhausted!"), w_time_delete, magic_none);
			}
			else {
				depot->call_depot_tool( 'c', cnv, NULL);
			}
			return true;
		} else if(komp == &bt_apply_line) {
			apply_line();
			return true;
		} else if(komp == &line_selector) {
			int selection = p.i;
//DBG_MESSAGE("depot_frame_t::action_triggered()","line selection=%i",selection);
			if(  (uint32)(selection-1)<(uint32)line_selector.count_elements()  ) {
				vector_tpl<linehandle_t> lines;
				get_line_list(depot, &lines);
				selected_line = lines[selection - 1];
				depot->set_selected_line(selected_line);
			}
			else {
				// remove line
				selected_line = linehandle_t();
				depot->set_selected_line(selected_line);
				line_selector.set_selection( 0 );
			}
		}
		else {
			return false;
		}
		build_vehicle_lists();
	}
	else {
		update_data();
		update_tabs();
	}
	layout(NULL);
	return true;
}


void depot_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_code!=WIN_CLOSE  &&  get_welt()->get_active_player() != depot->get_besitzer()) {
		destroy_win(this);
		return;
	}

	gui_frame_t::infowin_event(ev);

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {

		bool dir = (ev->ev_code==NEXT_WINDOW);
		depot_t *next_dep = depot_t::find_depot( depot->get_pos(), depot->get_typ(), depot->get_besitzer(), dir == NEXT_WINDOW );
		if(next_dep == NULL) {
			if(dir == NEXT_WINDOW) {
				// check the next from start of map
				next_dep = depot_t::find_depot( koord3d(-1,-1,0), depot->get_typ(), depot->get_besitzer(), true );
			}
			else {
				// respecive end of map
				next_dep = depot_t::find_depot( koord3d(8192,8192,127), depot->get_typ(), depot->get_besitzer(), false );
			}
		}

		if(next_dep  &&  next_dep!=this->depot) {
			/**
			 * Replace our depot_frame_t with a new at the same position.
			 * Volker Meyer
			 */
			int x = win_get_posx(this);
			int y = win_get_posy(this);
			destroy_win( this );

			next_dep->zeige_info();
			win_set_pos( win_get_magic((long)next_dep), x, y );
			get_welt()->change_world_position(next_dep->get_pos());
		}
	} else if(IS_WINDOW_REZOOM(ev)) {
		koord gr = get_fenstergroesse();
		set_fenstergroesse(gr);
	}
	else {
		if(IS_LEFTCLICK(ev) &&  !line_selector.getroffen(ev->cx, ev->cy-16)) {
			// close combo box; we must do it ourselves, since the box does not recieve outside events ...
			line_selector.close_box();
		}
	}
}



void depot_frame_t::zeichnen(koord pos, koord groesse)
{
	if (get_welt()->get_active_player() != depot->get_besitzer()) {
		destroy_win(this);
		return;
	}

	convoihandle_t cnv = depot->get_convoi(icnv);
	// check for data inconsistencies (can happen with withdraw-all and vehicle in depot)
	if(!cnv.is_bound() && convoi_pics.get_count()>0){
		icnv=0;
		update_data();
		cnv = depot->get_convoi(icnv);
	}

	if(cnv.is_bound()) {
		if(cnv->get_vehikel_anzahl() > 0) {
			uint32 total_power=0;
			uint32 total_max_weight=0;
			uint32 total_min_weight=0;
			uint16 max_speed=0;
			uint16 min_speed=0;
			for( unsigned i=0;  i<cnv->get_vehikel_anzahl();  i++) {
				const vehikel_besch_t *besch = cnv->get_vehikel(i)->get_besch();

				total_power += besch->get_leistung()*besch->get_gear()/64;
				total_min_weight += besch->get_gewicht();
				total_max_weight += besch->get_gewicht();

				uint32 max_weight =0;
				uint32 min_weight =100000;
				for(uint32 j=0; j<warenbauer_t::get_waren_anzahl(); j++) {
					const ware_besch_t *ware = warenbauer_t::get_info(j);

					if(besch->get_ware()->get_catg_index()==ware->get_catg_index()) {
						max_weight = max(max_weight, ware->get_weight_per_unit());
						min_weight = min(min_weight, ware->get_weight_per_unit());
					}
				}
				total_max_weight += (max_weight*besch->get_zuladung()+499)/1000;
				total_min_weight += (min_weight*besch->get_zuladung()+499)/1000;
			}
			max_speed = min(speed_to_kmh(cnv->get_min_top_speed()), (uint32) sqrt((((double)total_power/total_min_weight)-1)*2500));
			min_speed = min(speed_to_kmh(cnv->get_min_top_speed()), (uint32) sqrt((((double)total_power/total_max_weight)-1)*2500));
			sprintf(txt_convoi_count, "%s %d (%s %i)",
				translator::translate("Fahrzeuge:"), cnv->get_vehikel_anzahl(),
				translator::translate("Station tiles:"), cnv->get_tile_length() );
			sprintf(txt_convoi_speed,  "%s %d(%d)km/h", translator::translate("Max. speed:"), min_speed, max_speed );
			sprintf(txt_convoi_value, "%s %ld$", translator::translate("Restwert:"), (long)(cnv->calc_restwert()/100) );
			if(  cnv->get_line().is_bound()  &&  cnv->get_line()->get_schedule()->matches( get_welt(),cnv->get_schedule() )  ) {
				sprintf(txt_convoi_line, "%s %s", translator::translate("Serves Line:"), cnv->get_line()->get_name());

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
		static char empty[2] = "\0";
		inp_name.set_text( empty, 0);
		*txt_convoi_count = '\0';
		*txt_convoi_speed = '\0';
		*txt_convoi_value = '\0';
		*txt_convoi_line = '\0';
	}

	bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
	bt_show_all.pressed = show_all;	// otherwise the button would not show depressed

	gui_frame_t::zeichnen(pos, groesse);

	if(!cnv.is_bound()) {
		display_proportional_clip(pos.x+inp_name.get_pos().x+2, pos.y+inp_name.get_pos().y+18, translator::translate("new convoi"), ALIGN_LEFT, COL_GREY1, true);
	}
	draw_vehicle_info_text(pos);
}


void depot_frame_t::apply_line()
{
	if(icnv > -1) {
		convoihandle_t cnv = depot->get_convoi(icnv);
		// if no convoi is selected, do nothing
		if (!cnv.is_bound()) {
			return;
		}

		if(selected_line.is_bound()) {
			// set new route only, a valid route is selected:
			char id[16];
			sprintf( id, "%i", selected_line.get_id() );
			cnv->call_convoi_tool( 'l', id );
		}
		else {
			// sometimes the user might wish to remove convoy from line
			// => we clear the schedule completely
			cnv->call_convoi_tool( 'g', "0|" );
		}
	}
}



void depot_frame_t::fahrplaneingabe()
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	if(cnv.is_bound()  &&  cnv->get_vehikel_anzahl() > 0) {
		// this can happen locally, since any update of the schedule is done during closing window
		schedule_t *fpl = cnv->create_schedule();
		assert(fpl!=NULL);
		gui_fenster_t *fplwin = win_get_magic((long)fpl);
		if(   fplwin==NULL  ) {
			cnv->open_schedule_window();
		}
		else {
			top_win( fplwin );
		}
	}
	else {
		create_win( new news_img("Please choose vehicles first\n"), w_time_delete, magic_none);
	}
}


void depot_frame_t::draw_vehicle_info_text(koord pos)
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
	int value = -1;
	const vehikel_besch_t *veh_type = NULL;
	koord relpos = koord( 0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y() );
	int sel_index = lst->index_at(pos + tabs.get_pos() - relpos, x, y - 16 - gui_tab_panel_t::HEADER_VSIZE);

	if ((sel_index != -1) && (tabs.getroffen(x-pos.x,y-pos.y))) {
		const vector_tpl<gui_image_list_t::image_data_t>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec)));
		veh_type = vehikelbauer_t::get_info(vec[sel_index].text);
		if (vec[sel_index].count > 0) {
			value = calc_restwert(veh_type) / 100;
		}
	}
	else {
		sel_index = convoi.index_at(pos , x, y - 16);
		if(sel_index != -1) {
			convoihandle_t cnv = depot->get_convoi(icnv);
			veh_type = cnv->get_vehikel(sel_index)->get_besch();
			value = cnv->get_vehikel(sel_index)->calc_restwert()/100;
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
	display_proportional( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 16 + 4, c, ALIGN_LEFT, COL_BLACK, true );

	if(veh_type) {
		// lok oder waggon ?
		if(veh_type->get_leistung() > 0) {
			//lok
			const int zuladung = veh_type->get_zuladung();

			char name[128];

			sprintf(name,
			"%s (%s)",
			translator::translate(veh_type->get_name()),
			translator::translate(engine_type_names[veh_type->get_engine_type()+1]));

			int n = sprintf(buf,
				translator::translate("LOCO_INFO"),
				name,
				veh_type->get_preis()/100,
				veh_type->get_betriebskosten()/100.0,
				veh_type->get_leistung(),
				veh_type->get_geschw(),
				veh_type->get_gewicht()
				);

			if(zuladung>0) {
				sprintf(buf + n,
					translator::translate("LOCO_CAP"),
					zuladung,
					translator::translate(veh_type->get_ware()->get_mass()),
					veh_type->get_ware()->get_catg() == 0 ? translator::translate(veh_type->get_ware()->get_name()) : translator::translate(veh_type->get_ware()->get_catg_name())
					);
			}

		}
		else {
			// waggon
			sprintf(buf,
				translator::translate("WAGGON_INFO"),
				translator::translate(veh_type->get_name()),
				veh_type->get_preis()/100,
				veh_type->get_betriebskosten()/100.0,
				veh_type->get_zuladung(),
				translator::translate(veh_type->get_ware()->get_mass()),
				veh_type->get_ware()->get_catg() == 0 ?
				translator::translate(veh_type->get_ware()->get_name()) :
				translator::translate(veh_type->get_ware()->get_catg_name()),
				veh_type->get_gewicht(),
				veh_type->get_geschw()
				);
		}
		display_multiline_text( pos.x + 4, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE*1 + 4, buf,  COL_BLACK);

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

		if(veh_type->get_leistung() > 0  &&  veh_type->get_gear()!=64) {
			n+= sprintf(buf+n, "%s %0.2f : 1\n", translator::translate("Gear:"), 	veh_type->get_gear()/64.0);
		}

		if(veh_type->get_copyright()!=NULL  &&  veh_type->get_copyright()[0]!=0) {
			n += sprintf(buf + n, translator::translate("Constructed by %s"), veh_type->get_copyright());
		}

		if(value != -1) {
			if (buf[n-1]!='\n') {
				n += sprintf(buf + n, "\n");
			}
			sprintf(buf + strlen(buf), "%s %d Cr", translator::translate("Restwert:"), 	value);
		}

		display_multiline_text( pos.x + 200, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE*2 + 4, buf, COL_BLACK);
	}
}



void depot_frame_t::update_tabs()
{
	gui_komponente_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;

	cont_pas.add_komponente(&pas);
	scrolly_pas.set_show_scroll_x(false);
	scrolly_pas.set_size_corner(false);
	scrolly_pas.set_read_only(false);
	// add only if there are any
	if(!pas_vec.empty()) {
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
		one = true;
	}

	cont_electrics.add_komponente(&electrics);
	scrolly_electrics.set_show_scroll_x(false);
	scrolly_electrics.set_size_corner(false);
	scrolly_electrics.set_read_only(false);
	// add only if there are any trolleybuses
	if(!electrics_vec.empty()) {
		tabs.add_tab(&scrolly_electrics, translator::translate( depot->get_electrics_name() ) );
		one = true;
	}

	cont_loks.add_komponente(&loks);
	scrolly_loks.set_show_scroll_x(false);
	scrolly_loks.set_size_corner(false);
	scrolly_loks.set_read_only(false);
	// add, if waggons are there ...
	if (!loks_vec.empty() || !waggons_vec.empty()) {
		tabs.add_tab(&scrolly_loks, translator::translate( depot->get_zieher_name() ) );
		one = true;
	}

	cont_waggons.add_komponente(&waggons);
	scrolly_waggons.set_show_scroll_x(false);
	scrolly_waggons.set_size_corner(false);
	scrolly_waggons.set_read_only(false);
	// only add, if there are waggons
	if (!waggons_vec.empty()) {
		tabs.add_tab(&scrolly_waggons, translator::translate( depot->get_haenger_name() ) );
		one = true;
	}

	if(!one) {
		// add passenger as default
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
	}

	// Look, if there is our old tab present again (otherwise it will be 0 by tabs.clear()).
	for( uint8 i = 0; i < tabs.get_count(); i++ ) {
		if(  old_tab == tabs.get_tab(i)  ) {
			// Found it!
			tabs.set_active_tab_index(i);
			break;
		}
	}
}
