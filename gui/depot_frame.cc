/*
 * depot_frame.cc
 *
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
#include "../simtime.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../simgraph.h"
#include "../simline.h"
#include "../simlinemgmt.h"

#include "../tpl/ptrhashtable_tpl.h"
#include "../tpl/slist_tpl.h"

#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "image_list.h"
#include "messagebox.h"
#include "depot_frame.h"

#include "../besch/ware_besch.h"
#include "../bauer/vehikelbauer.h"
#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../utils/simstring.h"

#include "../bauer/warenbauer.h"

#include "../boden/wege/weg.h"
#include "../boden/wege/schiene.h"

static const char * engine_type_names [6] =
{
  "steam",
  "diesel",
  "electric",
  "bio",
  "fuel_cell",
  "hydrogene"
};

bool  depot_frame_t::show_retired_vehicles = false;


depot_frame_t::depot_frame_t(karte_t *welt, depot_t *depot, int &farbe) :
    gui_frame_t(txt_title, farbe),
    welt(welt),
    kennfarbe(farbe),
    depot(depot),
    icnv(depot->convoi_count() > 0 ? 0 : -1),
    iroute(-1),
    lb_convois(NULL, SCHWARZ, gui_label_t::left),
    lb_convoi_count(NULL, SCHWARZ, gui_label_t::left),
    lb_convoi_value(NULL, SCHWARZ, gui_label_t::left),
    lb_convoi_line(NULL, SCHWARZ, gui_label_t::left),
    lb_veh_action(NULL, SCHWARZ, gui_label_t::left),
    convoi_pics(depot->get_max_convoi_length())
{
DBG_DEBUG("depot_frame_t::depot_frame_t()","get_max_convoi_length()=%i",depot->get_max_convoi_length());
  waggons_vec = 0;
  loks_vec = 0;
  pas_vec = 0;

    tstrncpy(no_line_text, translator::translate("<no line>"), 124);

    setze_opaque( true );

    sprintf(txt_title, "(%d,%d) %s",
	depot->gib_pos().x, depot->gib_pos().y,
	translator::translate(depot->gib_name()));

    build_vehicle_lists();

    /*
     * [SELECT]:
     */
    lb_convois.setze_text(txt_convois);
    add_komponente(&lb_convois);

    // bt_prev.setze_groesse(koord(13, 13));
    // bt_prev.setze_text("«");
    bt_prev.setze_typ(button_t::arrowleft);
    bt_prev.add_listener(this);
    add_komponente(&bt_prev);

    inp_name.add_listener(this);
    add_komponente(&inp_name);

    // bt_next.setze_groesse(koord(13, 13));
    // bt_next.setze_text("»");
    bt_next.setze_typ(button_t::arrowright);
    bt_next.add_listener(this);
    add_komponente(&bt_next);

    /*
     * [SELECT ROUTE]:
     */

    // bt_prev_line.setze_groesse(koord(13, 13));
    // bt_prev_line.setze_text("«");
    bt_prev_line.setze_typ(button_t::arrowleft);
    bt_prev_line.add_listener(this);
    add_komponente(&bt_prev_line);

    inp_name_line.add_listener(this);
    //add_komponente(&inp_name_line);

    box.add_listener(this);
    add_komponente(&box);

    // bt_next_line.setze_groesse(koord(13, 13));
    // bt_next_line.setze_text("»");
    bt_next_line.setze_typ(button_t::arrowright);
    bt_next_line.add_listener(this);
    add_komponente(&bt_next_line);

    /*
     * [CONVOI]
     */
    convoi = new image_list_t(&convoi_pics);
    convoi->set_player_color(kennfarbe);
    convoi->add_listener(this);
    add_komponente(convoi);

    lb_convoi_count.setze_text(txt_convoi_count);
    add_komponente(&lb_convoi_count);

    lb_convoi_value.setze_text(txt_convoi_value);
    add_komponente(&lb_convoi_value);

    lb_convoi_line.setze_text(txt_convoi_line);
    add_komponente(&lb_convoi_line);

    /*
     * [ACTIONS]
     */
    bt_start.setze_typ(button_t::roundbox);
    bt_start.add_listener(this);
    bt_start.set_tooltip(translator::translate("Start the selected vehicle(s)"));
    add_komponente(&bt_start);

    bt_schedule.setze_typ(button_t::roundbox);
    bt_schedule.add_listener(this);
    bt_schedule.set_tooltip(translator::translate("Give the selected vehicle(s) an individual schedule"));
    add_komponente(&bt_schedule);

    bt_destroy.setze_typ(button_t::roundbox);
    bt_destroy.add_listener(this);
    bt_destroy.set_tooltip(translator::translate("Move the selected vehicle(s) back to the depot"));
    add_komponente(&bt_destroy);

    bt_sell.setze_typ(button_t::roundbox);
    bt_sell.add_listener(this);
    bt_sell.set_tooltip(translator::translate("Sell the selected vehicle(s)"));
    add_komponente(&bt_sell);

    /*
     * new route management buttons
     */

    bt_new_line.setze_typ(button_t::roundbox);
    bt_new_line.add_listener(this);
    bt_new_line.set_tooltip(translator::translate("Lines are used to manage groups of vehicles"));
    add_komponente(&bt_new_line);

    bt_apply_line.setze_typ(button_t::roundbox);
    bt_apply_line.add_listener(this);
    bt_apply_line.set_tooltip(translator::translate("Add the selected vehicle(s) to the selected line"));
    add_komponente(&bt_apply_line);

    bt_change_line.setze_typ(button_t::roundbox);
    bt_change_line.add_listener(this);
    bt_change_line.set_tooltip(translator::translate("Modify the selected line"));
    add_komponente(&bt_change_line);

    bt_copy_convoi.setze_typ(button_t::roundbox);
    bt_copy_convoi.add_listener(this);
    bt_copy_convoi.set_tooltip(translator::translate("Copy the selected convoi and its schedule or line"));
    add_komponente(&bt_copy_convoi);

    /*
     * [PANEL]
     */
    pas = new image_list_t(pas_vec);
    pas->set_player_color(kennfarbe);
    pas->add_listener(this);

    loks = new image_list_t(loks_vec);
    loks->set_player_color(kennfarbe);
    loks->add_listener(this);

    waggons = new image_list_t(waggons_vec);
    waggons->set_player_color(kennfarbe);
    waggons->add_listener(this);

    cont_pas = new gui_container_t();
    cont_pas->add_komponente(pas);
    scrolly_pas = new gui_scrollpane_t(cont_pas);
    scrolly_pas->set_show_scroll_x(false);
    scrolly_pas->set_read_only(false);

    if(pas_vec->get_count() > 0  ||  (loks_vec->get_count()==0  && waggons_vec->get_count()==0)  ) {
	tabs.add_tab(scrolly_pas, depot->gib_passenger_name());
    }

    cont_loks = new gui_container_t();
    cont_loks->add_komponente(loks);
    scrolly_loks = new gui_scrollpane_t(cont_loks);
    scrolly_loks->set_show_scroll_x(false);
    scrolly_loks->set_read_only(false);

    if(loks_vec->get_count() > 0) {
	tabs.add_tab(scrolly_loks, depot->gib_zieher_name());
    }

    cont_waggons = new gui_container_t();
    cont_waggons->add_komponente(waggons);
    scrolly_waggons = new gui_scrollpane_t(cont_waggons);
    scrolly_waggons->set_show_scroll_x(false);
    scrolly_waggons->set_read_only(false);

    if(waggons_vec->get_count() > 0) {
	tabs.add_tab(scrolly_waggons, depot->gib_haenger_name());
    }


    add_komponente(&tabs);

    add_komponente(&div_tabbottom);

    add_komponente(&lb_veh_action);

    veh_action = va_append;
    bt_veh_action.setze_typ(button_t::roundbox);
    bt_veh_action.add_listener(this);
    bt_veh_action.set_tooltip(translator::translate("Choose operation executed on clicking stored/new vehicles"));
    add_komponente(&bt_veh_action);

    bt_obsolete.setze_typ(button_t::square);
    bt_obsolete.setze_text(translator::translate("Show obsolete"));
    bt_obsolete.add_listener(this);
    bt_obsolete.set_tooltip(translator::translate("Show also vehicles no longer in production."));
    add_komponente(&bt_obsolete);

    welt->simlinemgmt->sort_lines();
    koord gr = koord(0,0);
    layout(&gr);
    update_data();
//    gr = koord(0,0);
//    layout(gr);
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
    int CLIST_WIDTH = depot->get_max_convoi_length() * (grid.x - grid_dx) + 2 * image_list_t::BORDER;
    int CLIST_HEIGHT = grid.y + 2 * image_list_t::BORDER;
    int CONVOI_WIDTH = CLIST_WIDTH + placement_dx;
    // int CONVOI_HEIGHT = CLIST_HEIGHT + CINFO_HEIGHT;

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
    int VINFO_HEIGHT = 82;

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
//    int PANEL_COLS = (TOTAL_WIDTH - 2 * image_list_t::BORDER - 1) / grid.x;
//    int PANEL_WIDTH = PANEL_COLS * grid.x + 2 * image_list_t::BORDER + 12;
    int total_h = PANEL_VSTART+VINFO_HEIGHT + 17 + tab_panel_t::HEADER_VSIZE + 2 * image_list_t::BORDER;
    int PANEL_ROWS = max(1, ((fgr.y-total_h)/grid.y) );
	if(gr  &&  gr->y==0) {
		PANEL_ROWS = 3;
	}
    int PANEL_HEIGHT = PANEL_ROWS * grid.y + tab_panel_t::HEADER_VSIZE + 2 * image_list_t::BORDER;
    int MIN_PANEL_HEIGHT = grid.y + tab_panel_t::HEADER_VSIZE + 2 * image_list_t::BORDER;


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

    inp_name.setze_pos(koord(20, SELECT_VSTART));
    inp_name.setze_groesse(koord(TOTAL_WIDTH - 40, 14));

    bt_next.setze_pos(koord(TOTAL_WIDTH - 16, SELECT_VSTART + 2));

    /*
     * [SELECT ROUTE]:
     * @author hsiegeln
     */

    bt_prev_line.setze_pos(koord(5, SELECT_VSTART + 15));

    inp_name_line.setze_pos(koord(20, SELECT_VSTART + 13));
    inp_name_line.setze_groesse(koord(TOTAL_WIDTH - 40, 14));

    box.setze_pos(koord(20, SELECT_VSTART + 13));
    box.setze_groesse(koord(TOTAL_WIDTH - 40, 14));
    box.set_max_size(koord(TOTAL_WIDTH - 40, 150));
    box.set_highlight_color(1);

    bt_next_line.setze_pos(koord(TOTAL_WIDTH - 16, SELECT_VSTART + 15));

    /*
     * [CONVOI]
     */
    convoi->set_grid(koord(grid.x - grid_dx, grid.y));
    convoi->set_placement(koord(placement.x - placement_dx, placement.y));
    convoi->setze_pos(koord((TOTAL_WIDTH-CLIST_WIDTH)/2, CONVOI_VSTART));
    convoi->setze_groesse(koord(CLIST_WIDTH, CLIST_HEIGHT));

    lb_convoi_count.setze_pos(koord(4, CINFO_VSTART));
    lb_convoi_value.setze_pos(koord(4 + 100, CINFO_VSTART));
    lb_convoi_line.setze_pos(koord(4, CINFO_VSTART + LINESPACE));

    /*
     * [ACTIONS]
     */
    bt_start.setze_pos(koord(0, ACTIONS_VSTART));
    bt_start.setze_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_start.setze_text(translator::translate("Start"));

    bt_schedule.setze_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART));
    bt_schedule.setze_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_schedule.setze_text(translator::translate("Fahrplan"));

    bt_destroy.setze_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART));
    bt_destroy.setze_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
    bt_destroy.setze_text(translator::translate("Auflösen"));

    bt_sell.setze_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART));
    bt_sell.setze_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
    bt_sell.setze_text(translator::translate("Verkauf"));

    /*
     * ACTIONS for new route management buttons
     * @author hsiegeln
     */

    bt_new_line.setze_pos(koord(0, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_new_line.setze_groesse(koord(TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_new_line.setze_text(translator::translate("New Line"));

    bt_apply_line.setze_pos(koord(TOTAL_WIDTH/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_apply_line.setze_groesse(koord(TOTAL_WIDTH*2/4-TOTAL_WIDTH/4, ABUTTON_HEIGHT));
    bt_apply_line.setze_text(translator::translate("Apply Line"));

    bt_change_line.setze_pos(koord(TOTAL_WIDTH*2/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_change_line.setze_groesse(koord(TOTAL_WIDTH*3/4-TOTAL_WIDTH*2/4, ABUTTON_HEIGHT));
    bt_change_line.setze_text(translator::translate("Update Line"));

    bt_copy_convoi.setze_pos(koord(TOTAL_WIDTH*3/4, ACTIONS_VSTART+ABUTTON_HEIGHT));
    bt_copy_convoi.setze_groesse(koord(TOTAL_WIDTH-TOTAL_WIDTH*3/4, ABUTTON_HEIGHT));
    bt_copy_convoi.setze_text(translator::translate("Copy Convoi"));

    /*
     * [PANEL]
     */
    tabs.setze_pos(koord(0, PANEL_VSTART));
    tabs.setze_groesse(koord(TOTAL_WIDTH, PANEL_HEIGHT+8));

    pas->set_grid(grid);
    pas->set_placement(placement);
	pas->setze_groesse(tabs.gib_groesse());
	pas->recalc_size();
	pas->setze_pos(koord(1,1));
	cont_pas->setze_groesse(pas->gib_groesse());
	scrolly_pas->setze_groesse(scrolly_pas->gib_groesse());

    loks->set_grid(grid);
    loks->set_placement(placement);
	loks->setze_groesse(tabs.gib_groesse());
	loks->recalc_size();
	loks->setze_pos(koord(1,1));
	cont_loks->setze_groesse(loks->gib_groesse());
	scrolly_loks->setze_groesse(scrolly_loks->gib_groesse());

    waggons->set_grid(grid);
    waggons->set_placement(placement);
	waggons->setze_groesse(tabs.gib_groesse());
	waggons->recalc_size();
	waggons->setze_pos(koord(1,1));
	cont_waggons->setze_groesse(waggons->gib_groesse());
	scrolly_waggons->setze_groesse(scrolly_waggons->gib_groesse());

    div_tabbottom.setze_pos(koord(0,PANEL_VSTART+PANEL_HEIGHT));
    div_tabbottom.setze_groesse(koord(TOTAL_WIDTH,0));

    lb_veh_action.setze_pos(koord(TOTAL_WIDTH-ABUTTON_WIDTH, PANEL_VSTART + PANEL_HEIGHT+4));
    lb_veh_action.setze_text(translator::translate("Fahrzeuge:"));

    bt_veh_action.setze_pos(koord(TOTAL_WIDTH-ABUTTON_WIDTH, PANEL_VSTART + PANEL_HEIGHT + 14));
    bt_veh_action.setze_groesse(koord(ABUTTON_WIDTH, 13));

    bt_obsolete.setze_pos(koord(TOTAL_WIDTH-(ABUTTON_WIDTH*5)/2, PANEL_VSTART + PANEL_HEIGHT + 14));
    bt_obsolete.setze_groesse(koord(ABUTTON_WIDTH, 13));
    bt_obsolete.pressed = show_retired_vehicles;
}




void depot_frame_t::setze_fenstergroesse( koord gr )
{
	koord g=gr;
	layout(&g);
	gui_frame_t::setze_fenstergroesse(gr);
}


// true if future
bool depot_frame_t::is_future(const vehikel_besch_t *info,const int month_now)
{
	const int intro_month = info->get_intro_year() * 12 + info->get_intro_month();
	return welt->use_timeline()  &&  intro_month > month_now;
}

// true if obsolete
bool depot_frame_t::is_retired(const vehikel_besch_t *info,const int month_now)
{
	const int retire_month = info->get_retire_year() * 12 + info->get_retire_month();
	return welt->use_timeline()  &&  retire_month <= month_now;
}

// true if already stored here
bool depot_frame_t::is_contained(const vehikel_besch_t *info)
{
	if(depot->vehicle_count()>0) {
		const slist_tpl<vehikel_t *> *vehicle_list = depot->get_vehicle_list();
		slist_iterator_tpl<vehikel_t *> iter(vehicle_list);
		while(iter.next()) {
			if(iter.get_current()->gib_besch()==info) {
				return true;
			}
		}
	}
	return false;
}



void depot_frame_t::build_vehicle_lists()
{
	const int month_now = welt->get_current_month();
	int i = 0;

	if(waggons_vec == 0) {

		int loks = 0, waggons = 0, pax=0;
		while(depot->get_vehicle_type(i)) {
			const vehikel_besch_t *info = depot->get_vehicle_type(i);

			if(info->gib_ware()==warenbauer_t::passagiere  ||  info->gib_ware()==warenbauer_t::post) {
				pax++;
			}
			else if(info->gib_leistung() > 0  ||  info->gib_zuladung()==0) {
				loks++;
			}
			else {
				waggons++;
			}
			i++;
		}

		waggons_vec = new vector_tpl<image_list_t::image_data_t>(waggons+1);
		loks_vec = new vector_tpl<image_list_t::image_data_t>(loks+1);
		pas_vec = new vector_tpl<image_list_t::image_data_t>(pax+1);
DBG_DEBUG("depot_frame_t::build_vehicle_lists()","%i passenger vehicle, %i  engines, %i good wagons",pax,loks,waggons);
	}
	else {
		waggons_vec->clear();
		loks_vec->clear();
		pas_vec->clear();
	}

	vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification
	const schiene_t *sch = dynamic_cast<const schiene_t *>(welt->lookup(depot->gib_pos())->gib_weg(weg_t::schiene));
	const bool schiene_electric = (sch==NULL)  ||  sch->ist_elektrisch();


	i = 0;
	while(depot->get_vehicle_type(i)) {
		const vehikel_besch_t *info = depot->get_vehicle_type(i);
		image_list_t::image_data_t img_data;

		// prissi: ist a non-electric track?
		// Hajo: check for timeline
		// prissi: and retirement date
		if(  (schiene_electric  ||  info->get_engine_type()!=vehikel_besch_t::electric)  &&
			(!is_future(info,month_now))  &&  (show_retired_vehicles  ||  (!is_retired(info,month_now)) )   ||  is_contained(info)
		) {

			img_data.image = info->gib_basis_bild();
			img_data.count = 0;
			img_data.lcolor = img_data.rcolor= -1;

			if(info->gib_ware()==warenbauer_t::passagiere  ||  info->gib_ware()==warenbauer_t::post) {
				pas_vec->append(img_data);
				vehicle_map.set(info, &pas_vec->at(pas_vec->get_count() - 1));
			}
			else if(info->gib_leistung() > 0  ||  info->gib_zuladung()==0) {
				loks_vec->append(img_data);
				vehicle_map.set(info, &loks_vec->at(loks_vec->get_count() - 1));
			}
			else {
				waggons_vec->append(img_data);
				vehicle_map.set(info, &waggons_vec->at(waggons_vec->get_count() - 1));
			}
		} else {
//DBG_DEBUG("depot_frame_t::build_vehicle_lists()","now vehicle %s not available.", info->gib_name());
		}

		i++;
	}
}


void depot_frame_t::update_data()
{
    static const char *txt_veh_action[3] = {
	"anhängen",
	"voranstellen",
	"verkaufen"
    };

	// change grren into blue for retired vehicles
	const int month_now = welt->get_current_month();

    bt_veh_action.setze_text(translator::translate(txt_veh_action[veh_action]));

    // set line text: (line name or <no line>
    if ((iroute > -1) && (iroute < (int)depot->get_line_list()->count()))
    {
    	box.setze_text(depot->get_line_list()->at(iroute)->get_name(), 128);
    } else {
    	box.setze_text(no_line_text, 128);
    	iroute = -1;
    }

    switch(depot->convoi_count()) {
    case 0:
	tstrncpy(txt_convois, translator::translate("no convois"), 40);
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
    ptrhashtable_iterator_tpl<const vehikel_besch_t *, image_list_t::image_data_t *> iter1(vehicle_map);
    convoihandle_t cnv = depot->get_convoi(icnv);
    const vehikel_besch_t *veh = NULL;

    convoi_pics.clear();
    if(cnv.is_bound() && cnv->gib_vehikel_anzahl() > 0) {
		inp_name.setze_text(cnv->access_name(), 48);
	image_list_t::image_data_t img_data;

	uint16 i;
	for(i = 0; i < cnv->gib_vehikel_anzahl(); i++) {
	    img_data.image = cnv->gib_vehikel(i)->gib_bild();
	    img_data.count = 0;
	    img_data.lcolor = img_data.rcolor= -1;
	    convoi_pics.append(img_data);
	}

	/* color bars for current convoi: */
	convoi_pics.at(0).lcolor = convoi_t::pruefe_vorgaenger(NULL,cnv->gib_vehikel(0)->gib_besch()) ? GREEN : GELB;
	for(i = 1; i < cnv->gib_vehikel_anzahl(); i++) {
	    convoi_pics.at(i - 1).rcolor =
		convoi_t::pruefe_nachfolger(cnv->gib_vehikel(i - 1)->gib_besch(),cnv->gib_vehikel(i)->gib_besch()) ? GREEN : ROT;
	    convoi_pics.at(i).lcolor =
		convoi_t::pruefe_vorgaenger(cnv->gib_vehikel(i - 1)->gib_besch(),cnv->gib_vehikel(i)->gib_besch()) ? GREEN : ROT;
	}
	convoi_pics.at(i - 1).rcolor =
	    convoi_t::pruefe_nachfolger(cnv->gib_vehikel(i - 1)->gib_besch(),NULL) ? GREEN : GELB;

		// change grren into blue for retired vehicles
		for(i=0;  i<cnv->gib_vehikel_anzahl(); i++) {
			if(is_future(cnv->gib_vehikel(i)->gib_besch(),month_now) || is_retired(cnv->gib_vehikel(i)->gib_besch(),month_now)) {
				if(convoi_pics.at(i).lcolor==GREEN) {
					convoi_pics.at(i).lcolor = BLAU;
				}
				if(convoi_pics.at(i).rcolor==GREEN) {
					convoi_pics.at(i).rcolor = BLAU;
				}
			}
		}

	if(veh_action == va_insert) {
	    veh = cnv->gib_vehikel(0)->gib_besch();
	} else if(veh_action == va_append) {
	    veh = cnv->gib_vehikel(cnv->gib_vehikel_anzahl() - 1)->gib_besch();
	}
    }
    while(iter1.next()) {
	const int ok_color = is_future(iter1.get_current_key(),month_now) || is_retired(iter1.get_current_key(),month_now) ? BLAU: GREEN;

	iter1.get_current_value()->count = 0;
	iter1.get_current_value()->lcolor = -1;
	iter1.get_current_value()->rcolor = -1;

	/*
	 * color bars for current convoi:
	 *  green/green	okay to append/insert
	 *  red/red		cannot be appended/inserted
	 *  green/yellow	append okay, cannot be end of train
	 *  yellow/green	insert okay, cannot be start of train
	 */

	if(veh_action == va_insert) {
	    if (!convoi_t::pruefe_nachfolger(iter1.get_current_key(), veh) ||
			veh && !convoi_t::pruefe_vorgaenger(iter1.get_current_key(), veh)) {
			iter1.get_current_value()->lcolor = ROT;
			iter1.get_current_value()->rcolor = ROT;
	    } else if(!convoi_t::pruefe_vorgaenger(NULL, iter1.get_current_key())) {
			iter1.get_current_value()->lcolor = GELB;
			iter1.get_current_value()->rcolor = ok_color;
	    } else {
			iter1.get_current_value()->lcolor = ok_color;
			iter1.get_current_value()->rcolor = ok_color;
	    }
	} else if(veh_action == va_append) {
	    if(!convoi_t::pruefe_vorgaenger(veh, iter1.get_current_key()) ||
			veh && !convoi_t::pruefe_nachfolger(veh, iter1.get_current_key())) {
			iter1.get_current_value()->lcolor = ROT;
			iter1.get_current_value()->rcolor = ROT;
	    } else if(!convoi_t::pruefe_nachfolger(iter1.get_current_key(), NULL)) {
			iter1.get_current_value()->lcolor = ok_color;
			iter1.get_current_value()->rcolor = GELB;
	    } else {
			iter1.get_current_value()->lcolor = ok_color;
			iter1.get_current_value()->rcolor = ok_color;
	    }
	}
    }

    slist_iterator_tpl<vehikel_t *> iter2(depot->get_vehicle_list());

    while(iter2.next()) {
	vehicle_map.get(iter2.get_current()->gib_besch())->count++;
		if(veh_action == va_sell) {
		    vehicle_map.get(iter2.get_current()->gib_besch())->lcolor = GREEN;
		    vehicle_map.get(iter2.get_current()->gib_besch())->rcolor = GREEN;
		}
    }

    box.clear_elements();
	if (welt->simlinemgmt->count_lines() > 0)
	{
		slist_iterator_tpl<simline_t *> iter( depot->get_line_list() );
		while( iter.next() ) {
			simline_t *line = iter.get_current();
			if (line) {
				box.append_element( line->get_name() );
			}
		}
	}
	box.set_selection(iroute);
}





int depot_frame_t::calc_restwert(const vehikel_besch_t *veh_type)
{
    int wert = 0;

    slist_iterator_tpl<vehikel_t *> iter(depot->get_vehicle_list());
    while(iter.next()) {
	if(iter.get_current()->gib_besch() == veh_type) {
	    wert += iter.get_current()->calc_restwert();
	}
    }
    return wert;
}

void
depot_frame_t::bild_gewaehlt(image_list_t *lst, int bild_index)
{
    if(lst == loks || lst == waggons  ||  lst == pas) {
	image_list_t::image_data_t *bild_data = &(lst == loks ? loks_vec : (lst==pas? pas_vec :waggons_vec))->at(bild_index);

	if(bild_data->lcolor != ROT && bild_data->rcolor != ROT) {
	    int bild = bild_data->image;

	    int oldest_veh = -1;
	    int newest_veh = -1;
	    sint32 insta_time_old = 0;
	    sint32 insta_time_new = 0;

	    int i = 0;
	    slist_iterator_tpl<vehikel_t *> iter(depot->get_vehicle_list());
	    while(iter.next()) {
		if(iter.get_current()->gib_basis_bild() == bild) {
		    if(oldest_veh == -1 || insta_time_old > iter.get_current()->gib_insta_zeit()) {
			oldest_veh = i;
			insta_time_old = iter.get_current()->gib_insta_zeit();
		    }
		    if(newest_veh == -1 || insta_time_new < iter.get_current()->gib_insta_zeit()) {
			newest_veh = i;
			insta_time_new = iter.get_current()->gib_insta_zeit();
		    }
		}
		i++;
	    }

	    if(veh_action == va_sell) {
		/*
		 *	We sell the newest vehicle - gives most money back.
		 */
DBG_DEBUG("depot_frame_t::bild_gewaehlt()","sell %i",newest_veh);
		depot->sell_vehicle(newest_veh);
	    } else {
		const convoihandle_t cnv = depot->get_convoi(icnv);

		if(!cnv.is_bound() || cnv->gib_vehikel_anzahl() < depot->get_max_convoi_length()) {
		    /*
		     *	We add the oldest vehicle - newer stay for selling
		     */
		    if(oldest_veh == -1) {
			oldest_veh = depot->buy_vehicle(bild);
		    }
		    if(oldest_veh != -1) {
			depot->append_vehicle(icnv, oldest_veh, veh_action == va_insert);
DBG_DEBUG("depot_frame_t::bild_gewaehlt()","appended vehicle");
			if(icnv == -1) {
			    icnv = depot->convoi_count() - 1;
			    depot->get_convoi(icnv)->setze_name(
				depot->get_convoi(icnv)->gib_vehikel(0)->gib_besch()->gib_name());

			} // endif (icnv == -1)
DBG_DEBUG("depot_frame_t::bild_gewaehlt()","icnv");
		    } // endif (oldest_veh != -1)
		}  // endif (!cnv.is_bound() || cnv->gib_vehikel_anzahl() < depot->get_max_convoi_length())
	    }
 	    update_data();
 	    layout(NULL);
	}
    } else if(lst == convoi) {
	const convoihandle_t cnv = depot->get_convoi(icnv);

DBG_DEBUG("depot_frame_t::bild_gewaehlt()","convoi index %i",bild_index);
	if(cnv.is_bound() && bild_index < cnv->gib_vehikel_anzahl() ) {
	    if(cnv->gib_vehikel_anzahl() == 1) {
		depot->disassemble_convoi(icnv, false);
		icnv--;
	    } else {
		depot->remove_vehicle(icnv, bild_index);
	    }
	    build_vehicle_lists();
	    update_data();
	    layout(NULL);
	}
    }
}



bool
depot_frame_t::action_triggered(gui_komponente_t *komp)
{
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
	} else if(komp == &bt_obsolete) {
	    show_retired_vehicles = (show_retired_vehicles==0);
         build_vehicle_lists();
    	} else if(komp == &bt_veh_action) {
	    if(veh_action++ == va_sell) {
		veh_action = va_append;
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
	} else if(komp == &bt_next_line) {
	    if(++iroute == (int)depot->get_line_list()->count()) {
			iroute = -1;
	    }
		box.set_selection(iroute);
	} else if(komp == &bt_prev_line) {
	    if(iroute-- == -1) {
			iroute = depot->get_line_list()->count() - 1;
	    }
		box.set_selection(iroute);
	} else if(komp == &box) {
		iroute = box.get_selection();
	} else {
	    return false;
	}
    }
	update_data();
	layout(NULL);
	return true;
}


void depot_frame_t::infowin_event(const event_t *ev)
{
    if(IS_WINDOW_CHOOSE_NEXT(ev)) {
	/**
	 * Since there is no depot list we search the whole map.
	 * Volker Meyer
	 */
	depot_t *next_dep  = NULL;

	koord pos = depot->gib_pos().gib_2d();
	koord end = pos;
	unsigned int dir = ev->ev_code;

	do {
	    if(dir == NEXT_WINDOW) {
		pos.x++;
		if(pos.x == welt->gib_groesse_x()) {
		    pos.x = 0;
		    pos.y++;
		    if(pos.y == welt->gib_groesse_y()) {
			pos.y = 0;
		    }
		}
	    } else {
		if(pos.x == 0) {
		    pos.x = welt->gib_groesse_x();
		    if(pos.y == 0) {
			pos.y = welt->gib_groesse_y();
		    }
		    pos.y--;
		}
		pos.x--;
	    }
	    if(pos == end) {
		next_dep = NULL;
		break;
	    }
	    grund_t *gr = welt->lookup(pos)->gib_kartenboden();

	    next_dep = gr ? gr->gib_depot() : NULL;
	} while(!next_dep || next_dep->gib_typ() != depot->gib_typ());

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
	}
	/**
	 * Always center the map to depot
	 * Volker Meyer
	 */
        welt->zentriere_auf(pos);
	} else if(IS_WINDOW_REZOOM(ev)) {
	    koord gr = gib_fenstergroesse();
	    setze_fenstergroesse(gr);
	} else if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN) {
		// Hajo: this seems to be needed to refresh the list of lines
		update_data();
	    layout(NULL);
    } else {
		gui_frame_t::infowin_event(ev);
	}
}



void
depot_frame_t::zeichnen(koord pos, koord groesse)
{
    const convoihandle_t cnv = depot->get_convoi(icnv);

    if(cnv.is_bound()) {
	//inp_name.setze_text(cnv->access_name(), 48);
	if(cnv->gib_vehikel_anzahl() > 0) {
	    sprintf(txt_convoi_count, "%s %d",
		translator::translate("Fahrzeuge:"),
		cnv->gib_vehikel_anzahl());
	    sprintf(txt_convoi_value, "%s %d Cr",
		translator::translate("Restwert:"),
		cnv->calc_restwert()/100);
	    if (cnv->get_line() != NULL)
	    {
		sprintf(txt_convoi_line, "%s %s",
  			translator::translate("Serves Line:"),
			cnv->get_line()->get_name());
	    } else {
		sprintf(txt_convoi_line, "%s %s",
  			translator::translate("Serves Line:"),
			translator::translate("<no line>"));
	    }
	} else {
	    tstrncpy(txt_convoi_count, "keine Fahrzeuge", 40);
	    *txt_convoi_value = '\0';
	}
    } else {
	inp_name.setze_text("\0", 0);
	*txt_convoi_count = '\0';
	*txt_convoi_value = '\0';
	*txt_convoi_line = '\0';
    }

    bt_obsolete.pressed = show_retired_vehicles;	// otherwise the button would not show depressed
    gui_frame_t::zeichnen(pos, groesse);
    if(!cnv.is_bound()) {
	display_proportional_clip(pos.x+inp_name.gib_pos().x+2,
			     pos.y+inp_name.gib_pos().y+18,
                             translator::translate("new convoi"),
			     ALIGN_LEFT, GRAU1, true);
    }

    draw_vehicle_info_text(pos);
}


/*
 * scheduling system for simutrans
 * @author hsiegeln
 */
void depot_frame_t::new_line()
{
	simline_t * new_line = depot->create_line(welt);
	iroute = depot->get_line_list()->count() -1;

	// newly created lines will be put into a sorted list of all lines
	// to make it easier for the player we must select the newly created
	// line in the dialog. This code block identifies the new line by name
	// and sets the selector accordingly
        for (int i = 0; i<=iroute; i++)
        {
		if ( strcmp(new_line->get_name(), depot->get_line_list()->at(i)->get_name()) == 0)
		{
			iroute = i;
			break;
		}
	}

	line_management_gui_t *line_gui = new line_management_gui_t(welt, new_line, welt->get_active_player());
	line_gui->zeige_info();
	update_data();

	layout(NULL);
}

void depot_frame_t::apply_line()
{
	convoihandle_t cnv = depot->get_convoi(icnv);
	// if no convoi is selected, do nothing
	if (cnv == NULL)
	{
		return;
	}

	if ((iroute > -1) && (icnv > -1))
	{
		// get selected route
		simline_t * line = depot->get_line_list()->at(iroute);
		// set new route only, a valid route is selected:
		if (line != NULL)
		{
			cnv->set_line(line);
		}
	} else if (iroute == -1) {
		// sometimes the user might wish to remove convoy from line
		// this happens here
		cnv->unset_line();
	}

}

void depot_frame_t::change_line()
{
	if (iroute > -1) {
		line_management_gui_t *line_gui = new line_management_gui_t(welt, depot->get_line_list()->at(iroute), welt->get_active_player());
		line_gui->zeige_info();
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
	    fahrplan_gui_t *fpl_gui = new fahrplan_gui_t(welt, fpl, welt->get_active_player());
	    fpl_gui->zeige_info();


	    // evtl. hat ein callback cnv gelöscht, so erneut testen
	    if(cnv.is_bound() && fpl != NULL) {
		cnv->setze_fahrplan(fpl);
	    }
	} else {
	    create_win(100, 64, new nachrichtenfenster_t(welt, "Es wird bereits\nein Fahrplan\neingegeben\n"), w_autodelete);
	}
    } else {
	create_win(100, 64, new nachrichtenfenster_t(welt, "Please choose vehicles first\n"), w_autodelete);
    }
}


void
depot_frame_t::draw_vehicle_info_text(koord pos)
{
    static char buf[1024];

    switch(depot->vehicle_count()) {
    case 0:
	tstrncpy(buf, translator::translate("Keine Einzelfahrzeuge im Depot"), 128);
	break;
    case 1:
	tstrncpy(buf, translator::translate("1 Einzelfahrzeug im Depot"), 128);
        break;
    default:
	sprintf(buf, translator::translate("%d Einzelfahrzeuge im Depot"), depot->vehicle_count());
        break;
    }
    image_list_t *lst = dynamic_cast<image_list_t *>(tabs.get_active_tab_index() == 0 ? pas :(tabs.get_active_tab_index() == 1 ? loks : waggons));
    int x = gib_maus_x();
    int y = gib_maus_y();
    int value = -1;
    const vehikel_besch_t *veh_type = NULL;
    koord relpos = (tabs.get_active_tab_index() == 0 ? koord(0, scrolly_pas->get_scroll_y()) : (tabs.get_active_tab_index() == 1 ? koord(0, scrolly_loks->get_scroll_y()) : koord(0, scrolly_waggons->get_scroll_y()) ));

    int sel_index = lst->index_at(pos + tabs.gib_pos() - relpos, x, y - 16 - tab_panel_t::HEADER_VSIZE);
    if ((sel_index != -1) && (tabs.getroffen(x-pos.x,y-pos.y))) {
			vector_tpl<image_list_t::image_data_t> *vec = (lst == pas ? pas_vec : (lst == loks ? loks_vec : waggons_vec));
			veh_type = vehikelbauer_t::gib_info(vec->at(sel_index).image);
			if(vec->at(sel_index).count > 0) {
	    	value = calc_restwert(veh_type) / 100;
			}
    } else {
			sel_index = convoi->index_at(pos , x, y - 16);
			if(sel_index != -1) {
		    convoihandle_t cnv = depot->get_convoi(icnv);
		    veh_type = cnv->gib_vehikel(sel_index)->gib_besch();
		    value = cnv->gib_vehikel(sel_index)->calc_restwert()/100;

			}
    }
    if(veh_type) {
        strcat(buf, "\n\n");

	// lok oder waggon ?
	if(veh_type->gib_leistung() > 0) {
	    //lok
	    const int zuladung = veh_type->gib_zuladung();

	    char name[128];

	    sprintf(name,
		    "%s (%s)",
		    translator::translate(veh_type->gib_name()),
		    translator::translate(engine_type_names[veh_type->get_engine_type()]));

	    sprintf(buf + strlen(buf),
		    translator::translate("LOCO_INFO"),
		    name,
		    veh_type->gib_preis()/100,
		    veh_type->gib_betriebskosten()/100.0,
		    veh_type->gib_leistung(),
		    veh_type->gib_geschw(),
		    veh_type->gib_gewicht(),
		    zuladung,
		    zuladung ? translator::translate(veh_type->gib_ware()->gib_mass()) : "",
		    zuladung ? (veh_type->gib_ware()->gib_catg() == 0 ? translator::translate(veh_type->gib_ware()->gib_name()) : translator::translate(veh_type->gib_ware()->gib_catg_name())) : ""
		    );
	} else {
	    // waggon
	    sprintf(buf + strlen(buf),
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
	if(value != -1) {
	    sprintf(buf + strlen(buf), "%s %d Cr",
		translator::translate("Restwert:"),
		value);
	}
    }


    display_multiline_text(
	pos.x + 4,
	pos.y + tabs.gib_pos().y + tabs.gib_groesse().y + 12,
	buf,
	SCHWARZ);


    if(veh_type) {
      int n = sprintf(buf, "%s %s %04d\n",
		      translator::translate("Intro. date:"),
		      translator::translate(month_names[veh_type->get_intro_month()]),
		      veh_type->get_intro_year() );

      if(veh_type->get_retire_year() !=2999) {
      n += sprintf(buf+n, "%s %s %04d\n",
		      translator::translate("Retire. date:"),
		      translator::translate(month_names[veh_type->get_retire_month()]),
		      veh_type->get_retire_year() );
      }

      if(veh_type->gib_leistung() > 0) {
	n+= sprintf(buf+n, "%s %0.2f : 1\n",
		    translator::translate("Gear:"),
		    veh_type->get_gear()/64.0);
      }

      display_multiline_text(
			     pos.x + 200,
			     pos.y + tabs.gib_pos().y + tabs.gib_groesse().y + 20 + LINESPACE*3,
			     buf,
			     SCHWARZ);
    }
}
