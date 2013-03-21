/*
 * Copyright (c) 1997 - 2001 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */ 

#include "depot_frame.h"

#include <stdio.h>

#include "../simunits.h"
#include "../simconvoi.h"
#include "../simdepot.h"

#include "../simcolor.h"
#include "../simdebug.h"
#include "../simgraph.h"
#include "../simline.h"
#include "../simlinemgmt.h"
#include "../vehicle/simvehikel.h"
#include "../simmenu.h"
#include "../simskin.h"
#include "../simtools.h"

#include "../besch/haus_besch.h"

#include "../simworld.h"
#include "../simwin.h"

#include "fahrplan_gui.h"
#include "line_management_gui.h"
#include "line_item.h"
#include "convoy_item.h"
#include "messagebox.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "../boden/wege/weg.h"

#define CREDIT_MESSAGE "That would exceed\nyour credit limit."

//static const char* engine_type_names[9] =
//{
//	"unknown",
//	"steam",
//	"diesel",
//	"electric",
//	"bio",
//	"sail",
//	"fuel_cell",
//	"hydrogene",
//	"battery"
//};

//bool depot_frame_t::show_retired_vehicles = false;
//bool depot_frame_t::show_all = true;
//>>>>>>> aburch/master


depot_frame_t::depot_frame_t(depot_t* depot) :
	gui_frame_t( translator::translate(depot->get_name()), depot->get_besitzer()),
	depot(depot),
	icnv(depot->convoi_count()-1),
	lb_convois(NULL, COL_BLACK, gui_label_t::left),
	lb_convoi_line("Serves Line:", COL_BLACK, gui_label_t::left),
	convoy_assembler(get_welt(), depot->get_wegtyp(), depot->get_player_nr(), check_way_electrified(true) )
{
DBG_DEBUG("depot_frame_t::depot_frame_t()","get_max_convoi_length()=%i",depot->get_max_convoi_length());
	last_selected_line = depot->get_last_selected_line();
	no_schedule_text     = translator::translate("<no schedule set>");
	clear_schedule_text  = translator::translate("<clear schedule>");
	unique_schedule_text = translator::translate("<individual schedule>");
	new_line_text        = translator::translate("<create new line>");
	line_seperator       = translator::translate("--------------------------------");
	new_convoy_text      = translator::translate("new convoi");
	promote_to_line_text = translator::translate("<promote to line>");

	/*
	 * [CONVOY ASSEMBLER]
	 */
	convoy_assembler.set_depot_frame(this);
	convoy_assembler.add_listener(this);
	update_convoy();

	/*
	 * [SELECT]:
	 */
	add_komponente(&lb_convois);

	convoy_selector.add_listener(this);
	convoy_selector.set_highlight_color( depot->get_besitzer()->get_player_color1() + 1);
	add_komponente(&convoy_selector);

	/*
	* [SELECT ROUTE]:
	*/
	line_selector.add_listener(this);
	line_selector.set_highlight_color( depot->get_besitzer()->get_player_color1() + 1);
	line_selector.set_wrapping(false);
	line_selector.set_focusable(true);
	add_komponente(&line_selector);
	depot->get_besitzer()->simlinemgmt.sort_lines();

	// goto line button
	line_button.set_typ(button_t::posbutton);
	line_button.set_targetpos(koord(0,0));
	line_button.add_listener(this);
	add_komponente(&line_button);

	/*
	* [CONVOI]
	*/
	add_komponente(&lb_convoi_line);

	//sb_convoi_length.set_base( depot->get_max_convoi_length() * CARUNITS_PER_TILE / 2 - 1);
	//sb_convoi_length.set_vertical(false);
	//convoi_length_ok_sb = 0;
	//convoi_length_slower_sb = 0;
	//convoi_length_too_slow_sb = 0;
	//convoi_tile_length_sb = 0;
	//new_vehicle_length_sb = 0;
	//if(  depot->get_typ() != depot_t::schiffdepot  &&  depot->get_typ() != depot_t::airdepot  ) { // no convoy length bar for ships or aircraft
	//	sb_convoi_length.add_color_value(&convoi_tile_length_sb, COL_BROWN);
	//	sb_convoi_length.add_color_value(&new_vehicle_length_sb, COL_DARK_GREEN);
	//	sb_convoi_length.add_color_value(&convoi_length_ok_sb, COL_GREEN);
	//	sb_convoi_length.add_color_value(&convoi_length_slower_sb, COL_ORANGE);
	//	sb_convoi_length.add_color_value(&convoi_length_too_slow_sb, COL_RED);
	//	add_komponente(&sb_convoi_length);
	//}

	/*
	* [ACTIONS]
	*/
	bt_start.set_typ(button_t::roundbox);
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_komponente(&bt_start);

	bt_schedule.set_typ(button_t::roundbox);
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule"); // translated to "Edit the selected vehicle(s) individual schedule or assigned line"
	add_komponente(&bt_schedule);

	bt_copy_convoi.set_typ(button_t::roundbox);
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line");
	add_komponente(&bt_copy_convoi);

	bt_sell.set_typ(button_t::roundbox);
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_komponente(&bt_sell);

	koord gr = koord(0,0);
	layout(&gr);
	update_data();
	gui_frame_t::set_fenstergroesse(gr);

	// text will be translated by ourselves (after update data)!
	lb_convois.set_text_pointer(txt_convois);

	check_way_electrified();
	add_komponente(&img_bolt);

	add_komponente(&convoy_assembler);

	cbuffer_t txt_traction_types;
	if(depot->get_tile()->get_besch()->get_enabled() == 0)
	{
		txt_traction_types.printf("%s", translator::translate("Unpowered vehicles only"));
	}
	else if(depot->get_tile()->get_besch()->get_enabled() == 255)
	{
		txt_traction_types.printf("%s", translator::translate("All traction types"));
	}
	else
	{
		uint8 shifter;
		bool first = true;
		for(uint8 i = 0; i < 8; i ++)
		{
			shifter = 1 << i;
			if((shifter & depot->get_tile()->get_besch()->get_enabled()))
			{
				if(first)
				{
					first = false;
					txt_traction_types.clear();
				}
				else
				{
					txt_traction_types.printf(", ");
				}
				txt_traction_types.printf("%s", translator::translate(vehikel_besch_t::get_engine_type((vehikel_besch_t::engine_t)i)));
			}
		}
	}
	convoy_assembler.set_traction_types(txt_traction_types.get_str());

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);

	depot->clear_command_pending();
}

//depot_frame_t::~depot_frame_t()
//{
//	// change convoy name if necessary
//	rename_convoy( depot->get_convoi(icnv) );
//}


// returns position of depot on the map
koord3d depot_frame_t::get_weltpos(bool)
{
	return depot->get_pos();
}


void depot_frame_t::layout(koord *gr)
{
	koord fgr = (gr!=NULL)? *gr : get_fenstergroesse();

	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	grid.x = depot->get_x_grid() * get_base_tile_raster_width() / 64 + 4;
	grid.y = depot->get_y_grid() * get_base_tile_raster_width() / 64 + 6;
	placement.x = depot->get_x_placement() * get_base_tile_raster_width() / 64 + 2;
	placement.y = depot->get_y_placement() * get_base_tile_raster_width() / 64 + 2;
	grid_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 2;
	placement_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 4;

	/*
	*	Dialog format:
	*
	*	Main structure are these parts from top to bottom:
	*
	*	    [SELECT]		convoi-selector
	*	    [CONVOI]		current convoi (*)
	*	    [ACTIONS]		convoi action buttons
	*	    [PANEL]		vehicle panel (*)
	*	    [VINFO]		vehicle infotext (*)
	*
	*	(*) In CONVOI ASSEMBLER
	*
	*
	*	Structure of [SELECT] is:
	*
	*	    [Info]
	*	    [PREV][LABEL][NEXT]
	*
	*  PREV and NEXT are small buttons - Label is adjusted to total width.
	*/
	const int SELECT_HEIGHT = 14;
//<<<<<<< HEAD
//=======
	const int selector_x = max(max(max(max(max(102, proportional_string_width(translator::translate("no convois")) + 4),
		proportional_string_width(translator::translate("1 convoi")) + 4),
		proportional_string_width(translator::translate("%d convois")) + 4),
		proportional_string_width(translator::translate("convoi %d of %d")) + 4),
		line_button.get_groesse().x + 2 + proportional_string_width(translator::translate(lb_convoi_line.get_text_pointer())) + 4
		);

	/*
	*	Structure of [CONVOI] is a image_list and an infos:
	*
	*	    [List]
	*	    [Info]
	*
	* The image list is horizontally "condensed".
	*/
//	const int CLIST_WIDTH = depot->get_max_convoi_length() * (grid.x - grid_dx) + 2 * gui_image_list_t::BORDER;
//	const int CLIST_HEIGHT = grid.y + 2 * gui_image_list_t::BORDER + 5;
//	const int CINFO_HEIGHT = LINESPACE * 4 + 1;
//	const int CONVOI_WIDTH = CLIST_WIDTH + placement_dx;
//>>>>>>> aburch/master

	/*
	*	Structure of [ACTIONS] is a row of buttons:
	*
	*	    [Start][Schedule][Destroy][Sell]
	*	    [new Route][change Route][delete Route]
	*/
//<<<<<<< HEAD
//	const int ABUTTON_WIDTH = 128;
//	const int ABUTTON_HEIGHT = 14;
//	const int ACTIONS_WIDTH = 2+4*(ABUTTON_WIDTH+2);
//	const int ACTIONS_HEIGHT = ABUTTON_HEIGHT + ABUTTON_HEIGHT + 2; // @author hsiegeln: added "+ ABUTTON_HEIGHT"
	const int ACTIONS_WIDTH = D_DEFAULT_WIDTH;
	const int ACTIONS_HEIGHT = D_BUTTON_HEIGHT;
	convoy_assembler.set_convoy_tabs_skip(ACTIONS_HEIGHT);

	/*
	*	Structure of [VINFO] is one multiline text.
	*/
//	const int VINFO_HEIGHT = 9 * LINESPACE - 1;

	/*
	* Total width is the max from [CONVOI] and [ACTIONS] width.
	*/
	const int MIN_DEPOT_FRAME_WIDTH = min(display_get_width(),           max(convoy_assembler.get_convoy_image_width(), ACTIONS_WIDTH));
	const int     DEPOT_FRAME_WIDTH = min(display_get_width(), max(fgr.x,max(convoy_assembler.get_convoy_image_width(), ACTIONS_WIDTH)));

	/*
	*	Now we can do the first vertical adjustement:
	*/
	const int SELECT_VSTART = D_MARGIN_TOP;
	const int ASSEMBLER_VSTART = SELECT_VSTART + SELECT_HEIGHT + LINESPACE;
	const int ACTIONS_VSTART = ASSEMBLER_VSTART + convoy_assembler.get_convoy_height();

	/*
	* Now we determine the row/col layout for the panel and the total panel
	* size.
	* build_vehicle_lists() fills loks_vec and waggon_vec.
	* Total width will be expanded to match completo columns in panel.
	*/
	convoy_assembler.set_panel_rows(gr  &&  gr->y==0?-1:fgr.y-ASSEMBLER_VSTART);

	/*
	 *	Now we can do the complete vertical adjustement:
	 */
	const int TOTAL_HEIGHT     = min(display_get_height(), ASSEMBLER_VSTART + convoy_assembler.get_height());
	const int MIN_TOTAL_HEIGHT = min(display_get_height(), ASSEMBLER_VSTART + convoy_assembler.get_min_height());

	/*
	* DONE with layout planning - now build everything.
	*/
	set_min_windowsize(koord(MIN_DEPOT_FRAME_WIDTH, MIN_TOTAL_HEIGHT));
	if(  fgr.x < DEPOT_FRAME_WIDTH  ) {
		gui_frame_t::set_fenstergroesse(koord(MIN_DEPOT_FRAME_WIDTH, max(fgr.y,MIN_TOTAL_HEIGHT) ));
	}
	if(  gr  &&  gr->x == 0  ) {
		gr->x = DEPOT_FRAME_WIDTH;
	}
	if(  gr  &&  gr->y == 0  ) {
		gr->y = TOTAL_HEIGHT;
	}

//	second_column_x = D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 2 / 4;

	/*
	 * [SELECT]:
	 */
	lb_convois.set_pos(koord(D_MARGIN_LEFT, SELECT_VSTART + 3));

	convoy_selector.set_pos(koord(D_MARGIN_LEFT + selector_x, SELECT_VSTART));
	convoy_selector.set_groesse(koord(DEPOT_FRAME_WIDTH - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, D_BUTTON_HEIGHT));
	convoy_selector.set_max_size(koord(DEPOT_FRAME_WIDTH - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, LINESPACE * 13 + 2 + 16));

	/*
	 * [SELECT ROUTE]:
	 * @author hsiegeln
	 */
	line_button.set_pos(koord(D_MARGIN_LEFT, SELECT_VSTART + D_BUTTON_HEIGHT + 3));
	lb_convoi_line.set_pos(koord(D_MARGIN_LEFT + line_button.get_groesse().x + 2, SELECT_VSTART + D_BUTTON_HEIGHT + 3));

	line_selector.set_pos(koord(D_MARGIN_LEFT + selector_x, SELECT_VSTART + D_BUTTON_HEIGHT));
	line_selector.set_groesse(koord(DEPOT_FRAME_WIDTH - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, D_BUTTON_HEIGHT));
	line_selector.set_max_size(koord(DEPOT_FRAME_WIDTH - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, LINESPACE * 13 + 2 + 16));

	/*
	 * [CONVOI]
	 */
	convoy_assembler.set_pos(koord(0,ASSEMBLER_VSTART));
	convoy_assembler.set_groesse(koord(DEPOT_FRAME_WIDTH,convoy_assembler.get_height()));
	convoy_assembler.layout();

//	lb_convoi_value.set_pos(koord(DEPOT_FRAME_WIDTH-10, ASSEMBLER_VSTART + convoy_assembler.get_convoy_image_height()));
//	lb_convoi_line.set_pos(koord(4, ASSEMBLER_VSTART + convoy_assembler.get_convoy_image_height() + LINESPACE * 2));
 
//=======
//	convoi.set_grid(koord(grid.x - grid_dx, grid.y));
//	convoi.set_placement(koord(placement.x - placement_dx, placement.y));
//	convoi.set_pos(koord((DEPOT_FRAME_WIDTH-CLIST_WIDTH) / 2, CONVOI_VSTART));
//	convoi.set_groesse(koord(CLIST_WIDTH, CLIST_HEIGHT));
//
//	sb_convoi_length.set_pos(koord((DEPOT_FRAME_WIDTH-CLIST_WIDTH) / 2 + 5,CONVOI_VSTART + grid.y + 5));
//	sb_convoi_length.set_groesse(koord(CLIST_WIDTH - 10, 4));
//
//	lb_convoi_count.set_pos(koord(D_MARGIN_LEFT, CINFO_VSTART));
//	cont_convoi_capacity.set_pos(koord(second_column_x, CINFO_VSTART));
//	lb_convoi_cost.set_pos(koord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 1));
//	lb_convoi_value.set_pos(koord(second_column_x, CINFO_VSTART + LINESPACE * 1));
//	lb_convoi_power.set_pos(koord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 2));
//	lb_convoi_weight.set_pos(koord(second_column_x, CINFO_VSTART + LINESPACE * 2));
//	lb_convoi_speed.set_pos(koord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 3));
//>>>>>>> aburch/master

	/*
	 * [ACTIONS]
	 */
	bt_start.set_pos(koord(D_MARGIN_LEFT, ACTIONS_VSTART));
	bt_start.set_groesse(koord((DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) / 4 - 3, D_BUTTON_HEIGHT));
	bt_start.set_text("Start");

	bt_schedule.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) / 4 + 1, ACTIONS_VSTART));
	bt_schedule.set_groesse(koord((DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 2 / 4 - (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) / 4 - 3, D_BUTTON_HEIGHT));
	bt_schedule.set_text("Fahrplan");

	bt_copy_convoi.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 2 / 4 + 2, ACTIONS_VSTART));
	bt_copy_convoi.set_groesse(koord((DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 - (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 2 / 4 - 3, D_BUTTON_HEIGHT));
	bt_copy_convoi.set_text("Copy Convoi");

	bt_sell.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 + 3, ACTIONS_VSTART));
	bt_sell.set_groesse(koord((DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) - (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 - 3, D_BUTTON_HEIGHT));
	bt_sell.set_text("verkaufen");

	/*
//<<<<<<< HEAD
//	 * ACTIONS for new route management buttons
//	 * @author hsiegeln
//	 */
//	bt_new_line.set_pos(koord(2, ACTIONS_VSTART+ABUTTON_HEIGHT));
//	bt_new_line.set_groesse(koord(DEPOT_FRAME_WIDTH/4-2, ABUTTON_HEIGHT));
//	bt_new_line.set_text("New Line");
//
//	bt_apply_line.set_pos(koord(DEPOT_FRAME_WIDTH/4+2, ACTIONS_VSTART+ABUTTON_HEIGHT));
//	bt_apply_line.set_groesse(koord(DEPOT_FRAME_WIDTH*2/4-3-DEPOT_FRAME_WIDTH/4, ABUTTON_HEIGHT));
//	bt_apply_line.set_text("Apply Line");
//
//	bt_change_line.set_pos(koord(DEPOT_FRAME_WIDTH*2/4+1, ACTIONS_VSTART+ABUTTON_HEIGHT));
//	bt_change_line.set_groesse(koord(DEPOT_FRAME_WIDTH*3/4-2-DEPOT_FRAME_WIDTH*2/4, ABUTTON_HEIGHT));
//	bt_change_line.set_text("Update Line");
//
//	bt_copy_convoi.set_pos(koord(DEPOT_FRAME_WIDTH*3/4+1, ACTIONS_VSTART+ABUTTON_HEIGHT));
//	bt_copy_convoi.set_groesse(koord(DEPOT_FRAME_WIDTH-DEPOT_FRAME_WIDTH*3/4-3, ABUTTON_HEIGHT));
//	bt_copy_convoi.set_text("Copy Convoi");
//=======
//	* [PANEL]
//	*/
//	tabs.set_pos(koord(0, PANEL_VSTART));
//	tabs.set_groesse(koord(DEPOT_FRAME_WIDTH, PANEL_HEIGHT));
//
//	pas.set_grid(grid);
//	pas.set_placement(placement);
//	pas.set_groesse(tabs.get_groesse() - koord(scrollbar_t::BAR_SIZE,0));
//	pas.recalc_size();
//	pas.set_pos(koord(0,0));
//	cont_pas.set_groesse(pas.get_groesse());
//	scrolly_pas.set_groesse(scrolly_pas.get_groesse());
//	scrolly_pas.set_scroll_amount_y(grid.y);
//	scrolly_pas.set_scroll_discrete_y(false);
//	scrolly_pas.set_size_corner(false);
//
//	electrics.set_grid(grid);
//	electrics.set_placement(placement);
//	electrics.set_groesse(tabs.get_groesse() - koord(scrollbar_t::BAR_SIZE,0));
//	electrics.recalc_size();
//	electrics.set_pos(koord(0,0));
//	cont_electrics.set_groesse(electrics.get_groesse());
//	scrolly_electrics.set_groesse(scrolly_electrics.get_groesse());
//	scrolly_electrics.set_scroll_amount_y(grid.y);
//	scrolly_electrics.set_scroll_discrete_y(false);
//	scrolly_electrics.set_size_corner(false);
//
//	loks.set_grid(grid);
//	loks.set_placement(placement);
//	loks.set_groesse(tabs.get_groesse() - koord(scrollbar_t::BAR_SIZE,0));
//	loks.recalc_size();
//	loks.set_pos(koord(0,0));
//	cont_loks.set_groesse(loks.get_groesse());
//	scrolly_loks.set_groesse(scrolly_loks.get_groesse());
//	scrolly_loks.set_scroll_amount_y(grid.y);
//	scrolly_loks.set_scroll_discrete_y(false);
//	scrolly_loks.set_size_corner(false);
//
//	waggons.set_grid(grid);
//	waggons.set_placement(placement);
//	waggons.set_groesse(tabs.get_groesse() - koord(scrollbar_t::BAR_SIZE,0));
//	waggons.recalc_size();
//	waggons.set_pos(koord(0,0));
//	cont_waggons.set_groesse(waggons.get_groesse());
//	scrolly_waggons.set_groesse(scrolly_waggons.get_groesse());
//	scrolly_waggons.set_scroll_amount_y(grid.y);
//	scrolly_waggons.set_scroll_discrete_y(false);
//	scrolly_waggons.set_size_corner(false);
//
//	div_tabbottom.set_pos(koord(0,PANEL_VSTART + PANEL_HEIGHT));
//	div_tabbottom.set_groesse(koord(DEPOT_FRAME_WIDTH,0));
//
//	lb_veh_action.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 + 3 - 4, PANEL_VSTART + PANEL_HEIGHT + 2 + 2));
//
//	bt_veh_action.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 + 3, PANEL_VSTART + PANEL_HEIGHT + 2));
//	bt_veh_action.set_groesse(koord((DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) - (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 - 3, D_BUTTON_HEIGHT));
//
//	bt_show_all.set_pos(koord(D_MARGIN_LEFT, PANEL_VSTART + PANEL_HEIGHT + 2 + D_BUTTON_HEIGHT + 1));
//	bt_show_all.pressed = show_all;
//
//	int w = max(72, bt_show_all.get_groesse().x);
//	bt_obsolete.set_pos(koord(D_MARGIN_LEFT + w + 4 + 6, PANEL_VSTART + PANEL_HEIGHT + 2 + D_BUTTON_HEIGHT + 1));
//	bt_obsolete.pressed = show_retired_vehicles;
//
//	lb_vehicle_filter.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 + 3 - 4, PANEL_VSTART + PANEL_HEIGHT + 2 + D_BUTTON_HEIGHT + 3));
//
//	vehicle_filter.set_pos(koord(D_MARGIN_LEFT + (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 + 3, PANEL_VSTART + PANEL_HEIGHT + 2 + D_BUTTON_HEIGHT));
//	vehicle_filter.set_groesse(koord((DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) - (DEPOT_FRAME_WIDTH - D_MARGIN_LEFT - D_MARGIN_RIGHT) * 3 / 4 - 3, D_BUTTON_HEIGHT));
//	vehicle_filter.set_max_size(koord(D_BUTTON_WIDTH + 60, LINESPACE * 7));
//>>>>>>> aburch/master

	const uint8 margin = 4;
	img_bolt.set_pos(koord(get_fenstergroesse().x - skinverwaltung_t::electricity->get_bild(0)->get_pic()->w - margin, margin));
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
	icnv = -1;	// deselect
	for(  uint i = 0;  i < depot->convoi_count();  i++  ) {
		if(  c == depot->get_convoi(i)  ) {
			icnv = i;
			break;
		}
	}
	build_vehicle_lists();
}


//<<<<<<< HEAD
//=======
//// true if already stored here
//bool depot_frame_t::is_contained(const vehikel_besch_t *info)
//{
//	FOR(slist_tpl<vehikel_t*>, const v, depot->get_vehicle_list()) {
//		if(  v->get_besch() == info  ) {
//			return true;
//		}
//	}
//	return false;
//}
//
//
//// add a single vehicle (helper function)
//void depot_frame_t::add_to_vehicle_list(const vehikel_besch_t *info)
//{
//	// prissi: ist a non-electric track?
//	// Hajo: check for timeline
//	// prissi: and retirement date
//
//	// Check if vehicle should be filtered
//	const ware_besch_t *freight = info->get_ware();
//	// Only filter when required and never filter engines
//	if (depot->selected_filter > 0 && info->get_zuladung() > 0) {
//		if (depot->selected_filter == VEHICLE_FILTER_RELEVANT) {
//			if(freight->get_catg_index() >= 3) {
//				bool found = false;
//				FOR(vector_tpl<ware_besch_t const*>, const i, get_welt()->get_goods_list()) {
//					if (freight->get_catg_index() == i->get_catg_index()) {
//						found = true;
//						break;
//					}
//				}
//
//				// If no current goods can be transported by this vehicle, don't display it
//				if (!found) return;
//			}
//		} else if (depot->selected_filter > VEHICLE_FILTER_RELEVANT) {
//			// Filter on specific selected good
//			uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
//			if (goods_index < get_welt()->get_goods_list().get_count()) {
//				const ware_besch_t *selected_good = get_welt()->get_goods_list()[goods_index];
//				if (freight->get_catg_index() != selected_good->get_catg_index()) {
//					return; // This vehicle can't transport the selected good
//				}
//			}
//		}
//	}
//
//	gui_image_list_t::image_data_t img_data;
//
//	img_data.image = info->get_basis_bild();
//	img_data.count = 0;
//	img_data.lcolor = img_data.rcolor = EMPTY_IMAGE_BAR;
//	img_data.text = info->get_name();
//
//	if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)  ) {
//		electrics_vec.append(img_data);
//		vehicle_map.set(info, &electrics_vec.back());
//	}
//	// since they come "pre-sorted" for the vehikelbauer, we have to do nothing to keep them sorted
//	else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) {
//		pas_vec.append(img_data);
//		vehicle_map.set(info, &pas_vec.back());
//	}
//	else if(info->get_leistung() > 0  ||  info->get_zuladung()==0) {
//		loks_vec.append(img_data);
//		vehicle_map.set(info, &loks_vec.back());
//	}
//	else {
//		waggons_vec.append(img_data);
//		vehicle_map.set(info, &waggons_vec.back());
//	}
//}
//
//
//// add all current vehicles
//void depot_frame_t::build_vehicle_lists()
//{
//	if (depot->get_vehicle_type().empty()) {
//		// there are tracks etc. but no vehicles => do nothing
//		// at least initialize some data
//		update_data();
//		update_tabs();
//		return;
//	}
//
//	const int month_now = get_welt()->get_timeline_year_month();
//
//	/*
//	 * The next block calculates upper bounds for the sizes of the vectors.
//	 * If the vectors get resized, the vehicle_map becomes invalid, therefore
//	 * we need to resize them before filling them.
//	 */
//	if(electrics_vec.empty()  &&  pas_vec.empty()  &&  loks_vec.empty()  &&  waggons_vec.empty()) {
//		int loks = 0, waggons = 0, pax=0, electrics = 0;
//		FOR(slist_tpl<vehikel_besch_t const*>, const info, depot->get_vehicle_type()) {
//			if(  info->get_engine_type() == vehikel_besch_t::electric  &&  (info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post)) {
//				electrics++;
//			}
//			else if(info->get_ware()==warenbauer_t::passagiere  ||  info->get_ware()==warenbauer_t::post) {
//				pax++;
//			}
//			else if(info->get_leistung() > 0  ||  info->get_zuladung()==0) {
//				loks++;
//			}
//			else {
//				waggons++;
//			}
//		}
//		pas_vec.resize(pax);
//		electrics_vec.resize(electrics);
//		loks_vec.resize(loks);
//		waggons_vec.resize(waggons);
//	}
//	pas_vec.clear();
//	electrics_vec.clear();
//	loks_vec.clear();
//	waggons_vec.clear();
//
//	vehicle_map.clear();
//
//	// we do not allow to built electric vehicle in a depot without electrification
//	const waytype_t wt = depot->get_waytype();
//	const weg_t *w = get_welt()->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
//	const bool weg_electrified = w ? w->is_electrified() : false;
//
//	img_bolt.set_image( weg_electrified ? skinverwaltung_t::electricity->get_bild_nr(0) : IMG_LEER );
//
//	// use this to show only sellable vehicles
//	if(!show_all  &&  veh_action==va_sell) {
//		// just list the one to sell
//		FOR(slist_tpl<vehikel_t*>, const v, depot->get_vehicle_list()) {
//			vehikel_besch_t const* const d = v->get_besch();
//			if (vehicle_map.get(d)) continue;
//			add_to_vehicle_list(d);
//		}
//	}
//	else {
//		// list only matching ones
//		FOR(slist_tpl<vehikel_besch_t const*>, const info, depot->get_vehicle_type()) {
//			const vehikel_besch_t *veh = NULL;
//			convoihandle_t cnv = depot->get_convoi(icnv);
//			if(cnv.is_bound() && cnv->get_vehikel_anzahl()>0) {
//				veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_besch();
//			}
//
//			// current vehicle
//			if( is_contained(info)  ||
//				((weg_electrified  ||  info->get_engine_type()!=vehikel_besch_t::electric)  &&
//					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) {
//				// check, if allowed
//				bool append = true;
//				if(!show_all) {
//					if(veh_action == va_insert) {
//						append = info->can_lead(veh)  &&  (veh==NULL  ||  veh->can_follow(info));
//					} else if(veh_action == va_append) {
//						append = info->can_follow(veh)  &&  (veh==NULL  ||  veh->can_lead(info));
//					}
//				}
//				if(append) {
//					add_to_vehicle_list( info );
//				}
//			}
//		}
//	}
//DBG_DEBUG("depot_frame_t::build_vehicle_lists()","finally %i passenger vehicle, %i  engines, %i good wagons",pas_vec.get_count(),loks_vec.get_count(),waggons_vec.get_count());
//	update_data();
//	update_tabs();
//}
//
//
//>>>>>>> aburch/master
static void get_line_list(const depot_t* depot, vector_tpl<linehandle_t>* lines)
{
	depot->get_besitzer()->simlinemgmt.get_lines(depot->get_line_type(), lines);
}


/*
* Reset counts and check for valid vehicles
*/
void depot_frame_t::update_data()
{
	txt_convois.clear();
	switch(depot->convoi_count()) {
		case 0: {
			txt_convois.printf( translator::translate("no convois") );
			break;
		}
		
		case 1: {
			if(  icnv == -1  ) {
				txt_convois.append( translator::translate("1 convoi") );
			}
			else {
				txt_convois.printf( translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count() );
			}
			break;
		}

		default: {
			if(  icnv == -1  ) {
				txt_convois.printf( translator::translate("%d convois"), depot->convoi_count() );
			}
			else {
				txt_convois.printf( translator::translate("convoi %d of %d"), icnv + 1, depot->convoi_count() );
			}
			break;
		}
	}

	/*
	* Reset counts and check for valid vehicles
	*/
	convoihandle_t cnv = depot->get_convoi( icnv );

	// update convoy selector
	convoy_selector.clear_elements();
	convoy_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( new_convoy_text, COL_BLACK ) );
	convoy_selector.set_selection(0);

	// check all matching convoys
	FOR(slist_tpl<convoihandle_t>, const c, depot->get_convoy_list()) {
		convoy_selector.append_element( new convoy_scrollitem_t(c) );
		if(  cnv.is_bound()  &&  c == cnv  ) {
			convoy_selector.set_selection( convoy_selector.count_elements() - 1 );
		}
	}

//<<<<<<< HEAD
//=======
//	const vehikel_besch_t *veh = NULL;
//
//	convoi_pics.clear();
//	if(  cnv.is_bound()  &&  cnv->get_vehikel_anzahl() > 0  ) {
//		for(  unsigned i=0;  i < cnv->get_vehikel_anzahl();  i++  ) {
//			// just make sure, there is this vehicle also here!
//			const vehikel_besch_t *info=cnv->get_vehikel(i)->get_besch();
//			if(  vehicle_map.get( info ) == NULL  ) {
//				add_to_vehicle_list( info );
//			}
//
//			gui_image_list_t::image_data_t img_data;
//			img_data.image = cnv->get_vehikel(i)->get_besch()->get_basis_bild();
//			img_data.count = 0;
//			img_data.lcolor = img_data.rcolor= EMPTY_IMAGE_BAR;
//			img_data.text = cnv->get_vehikel(i)->get_besch()->get_name();
//			convoi_pics.append(img_data);
//		}
//
//		/* color bars for current convoi: */
//		convoi_pics[0].lcolor = cnv->front()->get_besch()->can_follow(NULL) ? COL_GREEN : COL_YELLOW;
//		{
//			unsigned i;
//			for(  i = 1;  i < cnv->get_vehikel_anzahl();  i++  ) {
//				convoi_pics[i - 1].rcolor = cnv->get_vehikel(i-1)->get_besch()->can_lead(cnv->get_vehikel(i)->get_besch()) ? COL_GREEN : COL_RED;
//				convoi_pics[i].lcolor     = cnv->get_vehikel(i)->get_besch()->can_follow(cnv->get_vehikel(i-1)->get_besch()) ? COL_GREEN : COL_RED;
//			}
//			convoi_pics[i - 1].rcolor = cnv->get_vehikel(i-1)->get_besch()->can_lead(NULL) ? COL_GREEN : COL_YELLOW;
//		}
//
//		// change green into blue for retired vehicles
//		for(  unsigned i = 0;  i < cnv->get_vehikel_anzahl();  i++  ) {
//			if(  cnv->get_vehikel(i)->get_besch()->is_future(month_now)  ||  cnv->get_vehikel(i)->get_besch()->is_retired(month_now)  ) {
//				if(  convoi_pics[i].lcolor == COL_GREEN  ) {
//					convoi_pics[i].lcolor = COL_BLUE;
//				}
//				if(  convoi_pics[i].rcolor == COL_GREEN  ) {
//					convoi_pics[i].rcolor = COL_BLUE;
//				}
//			}
//		}
//
//		veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_besch();
//	}
//
//	FOR(vehicle_image_map, const& i, vehicle_map) {
//		vehikel_besch_t const* const    info = i.key;
//		gui_image_list_t::image_data_t& img  = *i.value;
//		const uint8 ok_color = info->is_future(month_now) || info->is_retired(month_now) ? COL_BLUE: COL_GREEN;
//
//		img.count = 0;
//		img.lcolor = ok_color;
//		img.rcolor = ok_color;
//
//		/*
//		* color bars for current convoi:
//		*  green/green	okay to append/insert
//		*  red/red		cannot be appended/inserted
//		*  green/yellow	append okay, cannot be end of train
//		*  yellow/green	insert okay, cannot be start of train
//		*/
//
//		if(veh_action == va_insert) {
//			if(!info->can_lead(veh)  ||  (veh  &&  !veh->can_follow(info))) {
//				img.lcolor = COL_RED;
//				img.rcolor = COL_RED;
//			}
//			else if(!info->can_follow(NULL)) {
//				img.lcolor = COL_YELLOW;
//			}
//		}
//		else if(veh_action == va_append) {
//			if(!info->can_follow(veh)  ||  (veh  &&  !veh->can_lead(info))) {
//				img.lcolor = COL_RED;
//				img.rcolor = COL_RED;
//			}
//			else if(!info->can_lead(NULL)) {
//				img.rcolor = COL_YELLOW;
//			}
//		}
//		else if( veh_action == va_sell ) {
//			img.lcolor = COL_RED;
//			img.rcolor = COL_RED;
//		}
//	}
//
//	FOR(slist_tpl<vehikel_t*>, const v, depot->get_vehicle_list()) {
//		// can fail, if currently not visible
//		if (gui_image_list_t::image_data_t* const imgdat = vehicle_map.get(v->get_besch())) {
//			imgdat->count++;
//			if(veh_action == va_sell) {
//				imgdat->lcolor = COL_GREEN;
//				imgdat->rcolor = COL_GREEN;
//			}
//		}
//	}
//
//>>>>>>> aburch/master
	// update the line selector
	line_selector.clear_elements();
	if(  cnv.is_bound()  &&  cnv->get_schedule()  &&  !cnv->get_schedule()->empty()  ) {
		if(  cnv->get_line().is_bound()  ) {
			line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( clear_schedule_text, COL_BLACK ) );
			line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( new_line_text, COL_BLACK ) );
		}
		else {
			line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( unique_schedule_text, COL_BLACK ) );
			line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( promote_to_line_text, COL_BLACK ) );
		}
	}
	else {
		line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( no_schedule_text, COL_BLACK ) );
		line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( new_line_text, COL_BLACK ) );
	}

	if(  last_selected_line.is_bound()  ) {
		line_selector.append_element( new line_scrollitem_t( last_selected_line ) );
	}
	line_selector.append_element( new gui_scrolled_list_t::const_text_scrollitem_t( line_seperator, COL_BLACK ) );
	line_selector.set_selection(0);
	selected_line = linehandle_t();

	// check all matching lines
	vector_tpl<linehandle_t> lines;
	get_line_list(depot, &lines);
	FOR(vector_tpl<linehandle_t>, const line, lines) {
		line_selector.append_element( new line_scrollitem_t(line) );
		if(  cnv.is_bound()  &&  line == cnv->get_line()  ) {
			line_selector.set_selection( line_selector.count_elements() - 1 );
			selected_line = line;
		}
	}

	convoy_assembler.update_data();
}

//void depot_frame_t::reset_convoy_name(convoihandle_t cnv)
//{
//	// reset convoy name only if the convoy is currently selected
//	if(  cnv.is_bound()  &&  cnv==depot->get_convoi(icnv)  ) {
//		tstrncpy(txt_old_cnv_name, cnv->get_name(), lengthof(txt_old_cnv_name));
//		tstrncpy(txt_cnv_name, cnv->get_name(), lengthof(txt_cnv_name));
//		inp_name.set_text(txt_cnv_name, lengthof(txt_cnv_name));
//=======
//
//	// Update vehicle filter
//	vehicle_filter.clear_elements();
//	vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("All"), COL_BLACK));
//	vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("Relevant"), COL_BLACK));
//
//	FOR(vector_tpl<ware_besch_t const*>, const i, get_welt()->get_goods_list()) {
//		vehicle_filter.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(i->get_name()), COL_BLACK));
//	}
//
//	if(  depot->selected_filter > vehicle_filter.count_elements()  ) {
//		depot->selected_filter = VEHICLE_FILTER_RELEVANT;
//	}
//	vehicle_filter.set_selection(depot->selected_filter);
//}


sint64 depot_frame_t::calc_restwert(const vehikel_besch_t *veh_type)
{
	sint64 wert = 0;
	FOR(slist_tpl<vehikel_t*>, const v, depot->get_vehicle_list()) {
		if(  v->get_besch() == veh_type  ) {
			wert += v->calc_restwert();
		}
	}
	return wert;
}


//void depot_frame_t::image_from_storage_list(gui_image_list_t::image_data_t *bild_data)
//{
//	if(  bild_data->lcolor != COL_RED  &&  bild_data->rcolor != COL_RED  ) {
//		if(  veh_action == va_sell  ) {
//			depot->call_depot_tool('s', convoihandle_t(), bild_data->text );
//		}
//		else {
//			convoihandle_t cnv = depot->get_convoi( icnv );
//			if(  !cnv.is_bound()  &&   !depot->get_besitzer()->is_locked()  ) {
//				// adding new convoi, block depot actions until command executed
//				// otherwise in multiplayer it's likely multiple convois get created
//				// rather than one new convoi with multiple vehicles
//				depot->set_command_pending();
//			}
//			depot->call_depot_tool( veh_action == va_insert ? 'i' : 'a', cnv, bild_data->text );
//		}
//>>>>>>> aburch/master
//	}
//}


//<<<<<<< HEAD
//void depot_frame_t::rename_convoy(convoihandle_t cnv)
//{
//	if(  cnv.is_bound()  ) {
//		const char *t = inp_name.get_text();
//		// only change if old name and current name are the same
//		// otherwise some unintended undo if renaming would occur
//		if(  t  &&  t[0]  &&  strcmp(t, cnv->get_name())  &&  strcmp(txt_old_cnv_name, cnv->get_name())==0  ) {
//			// text changed => call tool
//			cbuffer_t buf;
//			buf.printf( "c%u,%s", cnv.get_id(), t );
//			werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
//			w->set_default_param( buf );
//			cnv->get_welt()->set_werkzeug( w, cnv->get_besitzer() );
//			// since init always returns false, it is safe to delete immediately
//			delete w;
//			// do not trigger this command again
//			tstrncpy(txt_old_cnv_name, t, lengthof(txt_old_cnv_name));
//		}
//	}
//}

//void depot_frame_t::image_from_convoi_list(uint nr, bool to_end)
//{
//	const convoihandle_t cnv = depot->get_convoi( icnv );
//	if(  cnv.is_bound()  &&  nr < cnv->get_vehikel_anzahl()  ) {
//		// we remove all connected vehicles together!
//		// find start
//		unsigned start_nr = nr;
//		while(  start_nr > 0  ) {
//			start_nr--;
//			const vehikel_besch_t *info = cnv->get_vehikel(start_nr)->get_besch();
//			if(  info->get_nachfolger_count() != 1  ) {
//				start_nr++;
//				break;
//			}
//		}
//
//		cbuffer_t start;
//		start.printf("%u", start_nr);
//
//		const char tool = to_end ? 'R' : 'r';
//		depot->call_depot_tool( tool, cnv, start );
//	}
//}


bool depot_frame_t::action_triggered( gui_action_creator_t *komp, value_t p)
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  depot->is_command_pending()  ) {
		// block new commands until last command is executed
		return true;
	}

	if(  komp != NULL  ) {	// message from outside!
		if(  komp == &bt_start  ) {
			if(  cnv.is_bound()  ) {
				//first: close schedule (will update schedule on clients)
				destroy_win( (ptrdiff_t)cnv->get_schedule() );
				// only then call the tool to start
				char tool = event_get_last_control_shift() == 2 ? 'B' : 'b'; // start all with CTRL-click
				depot->call_depot_tool( tool, cnv, NULL);
				update_convoy();
			}
		} else if(komp == &bt_schedule) {
			if(  line_selector.get_selection() == 1  &&  !line_selector.is_dropped()  ) { // create new line
				// promote existing individual schedule to line
				cbuffer_t buf;
				if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
					schedule_t* fpl = cnv->get_schedule();
					if(  fpl  ) {
						fpl->sprintf_schedule( buf );
					}
				}
				depot->call_depot_tool('l', convoihandle_t(), buf);
				return true;
			}
			else {
				fahrplaneingabe();
				return true;
			}
		} else if(komp == &bt_destroy) {
			depot->call_depot_tool( 'd', cnv, NULL );
			update_convoy();
		} else if(komp == &bt_sell) {
			depot->call_depot_tool( 'v', cnv, NULL );
			update_convoy();
		//} else if(komp == &inp_name) {
		//	return true;	// already call rename_convoy() above
		//} else if(komp == &bt_next) {
		//	if(++icnv == (int)depot->convoi_count()) {
		//		icnv = -1;
		//	}
		//	update_convoy();
		//} else if(komp == &bt_prev) {
		//	if(icnv-- == -1) {
		//		icnv = depot->convoi_count() - 1;
		//	}
		//	update_convoy();
		//} else if(komp == &bt_new_line) {
		//	depot->call_depot_tool( 'l', convoihandle_t(), NULL );
		//	return true;
		//} else if(komp == &bt_change_line) {
		//	if(selected_line.is_bound()) {
		//		create_win(new line_management_gui_t(selected_line, depot->get_besitzer()), w_info, (ptrdiff_t)selected_line.get_rep() );
		//	}
		//	return true;
		}
		else if(  komp == &line_button  ) {
			if(  cnv.is_bound()  ) {
				cnv->get_besitzer()->simlinemgmt.show_lineinfo( cnv->get_besitzer(), cnv->get_line() );
				cnv->get_welt()->set_dirty();
			}
		}
		else if(  komp == &bt_sell  ) {
			depot->call_depot_tool('v', cnv, NULL);
		}
		else if(  komp == &bt_copy_convoi  ) {
			if(  cnv.is_bound()  ) {
				if(  !get_welt()->use_timeline()  ||  get_welt()->get_settings().get_allow_buying_obsolete_vehicles()  ||  depot->check_obsolete_inventory( cnv )  ) {
					depot->call_depot_tool('c', cnv, NULL);
				}
				else {
					create_win( new news_img("Can't buy obsolete vehicles!"), w_time_delete, magic_none );
				}
			}
			return true;
		}
		else if(  komp == &convoy_selector  ) {
			icnv = p.i - 1;
			if(  !depot->get_convoi(icnv).is_bound()  ) {
				set_focus( NULL );
			}
			else {
				set_focus( (gui_komponente_t *)&convoy_selector );
			}
		}
		else if(  komp == &line_selector  ) {
			int selection = p.i;
			if(  selection == 0  ) { // unique
				if(  selected_line.is_bound()  ) {
					selected_line = linehandle_t();
					apply_line();
				}
				// HACK mark line_selector temporarily unfocusable.
				// We call set_focus(NULL) later if we can.
				// Calling set_focus(NULL) now would have no effect due to logic in gui_container_t::infowin_event.
				line_selector.set_focusable( false );
				return true;
			}
//<<<<<<< HEAD
//				depot->call_depot_tool( 'c', cnv, NULL);
//				update_convoy();
//			}
//			return true;
//		}
//		else if(komp == &bt_apply_line) {
//			apply_line();
//		} else if(komp == &line_selector) {
//			int selection = p.i;
////DBG_MESSAGE("depot_frame_t::action_triggered()","line selection=%i",selection);
//			if(  (unsigned)(selection-1)<(unsigned)line_selector.count_elements()  ) {
//				vector_tpl<linehandle_t> lines;
//				get_line_list(depot, &lines);
//				selected_line = lines[selection - 1];
//				depot->set_selected_line(selected_line);
//=======
			else if(  selection == 1  ) { // create new line
				if(  line_selector.is_dropped()  ) { // but not from next/prev buttons
					// promote existing individual schedule to line
					cbuffer_t buf;
					if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
						schedule_t* fpl = cnv->get_schedule();
						if(  fpl  ) {
							fpl->sprintf_schedule( buf );
						}
					}
					line_selector.set_focusable( false );
					depot->call_depot_tool('l', convoihandle_t(), buf);
				}
				return true;
			}
			if(  last_selected_line.is_bound()  ) {
				if(  selection == 2  ) { // last selected line
					selected_line = last_selected_line;
					apply_line();
					return true;
				}
				selection -= 4;
			}
			else { // skip seperator
				selection -= 3;
			}
			if(  selection >= 0  &&  (uint32)selection < (uint32)line_selector.count_elements()  ) {
				vector_tpl<linehandle_t> lines;
				get_line_list( depot, &lines );
				selected_line = lines[selection];
				depot->set_last_selected_line( selected_line );
				last_selected_line = selected_line;
				apply_line();
				return true;
			}
		}
		else {
			return false;
		}
		convoy_assembler.build_vehicle_lists();
	}
	update_data();
	layout(NULL);
	return true;
}


bool depot_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_code!=WIN_CLOSE  &&  get_welt()->get_active_player() != depot->get_besitzer()) {
		destroy_win(this);
		return true;
	}

	const bool swallowed = gui_frame_t::infowin_event(ev);

	// HACK make line_selector focusable again
	// now we can release focus
	if (!line_selector.is_focusable( ) ) {
		line_selector.set_focusable( true );
		set_focus(NULL);
	}

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
			koord const pos = win_get_pos(this);
			destroy_win( this );

			next_dep->zeige_info();
			win_set_pos(win_get_magic((ptrdiff_t)next_dep), pos.x, pos.y);
			get_welt()->change_world_position(next_dep->get_pos());
		}
		else {
			// recenter on current depot
			get_welt()->change_world_position(depot->get_pos());
		}

		return true;
	} else if(ev->ev_class == INFOWIN && ev->ev_code == WIN_OPEN) {
		convoy_assembler.build_vehicle_lists();
		update_data();
		layout(NULL);
	}
	else {
		if(IS_LEFTCLICK(ev)  ) {
			if(  !convoy_selector.getroffen(ev->cx, ev->cy-16)  &&  convoy_selector.is_dropped()  ) {
				// close combo box; we must do it ourselves, since the box does not receive outside events ...
				convoy_selector.close_box();
			}
			if(  line_selector.is_dropped()  &&  !line_selector.getroffen(ev->cx, ev->cy-16)  ) {
				// close combo box; we must do it ourselves, since the box does not receive outside events ...
				line_selector.close_box();
				if(  line_selector.get_selection() < last_selected_line.is_bound()+3  &&  get_focus() == &line_selector  ) {
					set_focus( NULL );
				}
			}
			//if(  !vehicle_filter.getroffen(ev->cx, ev->cy-16)  &&  vehicle_filter.is_dropped()  ) {
			//	// close combo box; we must do it ourselves, since the box does not receive outside events ...
			//	vehicle_filter.close_box();
			//}
		}
	}

	//if(  get_focus() == &vehicle_filter  &&  !vehicle_filter.is_dropped()  ) {
	//	set_focus( NULL );
	//}

	return swallowed;
}


void depot_frame_t::zeichnen(koord pos, koord groesse)
{
	if(  get_welt()->get_active_player() != depot->get_besitzer()  ) {
		destroy_win(this);
		return;
	}

	convoihandle_t cnv = depot->get_convoi(icnv);
	// check for data inconsistencies (can happen with withdraw-all and vehicle in depot)
	const vector_tpl<gui_image_list_t::image_data_t*>* convoi_pics = convoy_assembler.get_convoi_pics();
	if(  !cnv.is_bound()  &&  !convoi_pics->empty()  ) {
		icnv=0;
		update_data();
		cnv = depot->get_convoi(icnv);
	}

	gui_frame_t::zeichnen(pos, groesse);
}

void depot_frame_t::apply_line()
{
	if(  icnv > -1  ) {
		convoihandle_t cnv = depot->get_convoi( icnv );
		// if no convoi is selected, do nothing
		if(  !cnv.is_bound()  ) {
			return;
		}

		if(  selected_line.is_bound()  ) {
			// set new route only, a valid route is selected:
			char id[16];
			sprintf( id, "%i", selected_line.get_id() );
			cnv->call_convoi_tool('l', id );
		}
		else {
			// sometimes the user might wish to remove convoy from line
			// => we clear the schedule completely
			schedule_t *dummy = cnv->create_schedule()->copy();
			dummy->eintrag.clear();

			cbuffer_t buf;
			dummy->sprintf_schedule(buf);
			cnv->call_convoi_tool('g', (const char*)buf );

			delete dummy;
		}
	}
}


void depot_frame_t::fahrplaneingabe()
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  cnv.is_bound()  &&  cnv->get_vehikel_anzahl() > 0  ) {
		if(  selected_line.is_bound()  &&  event_get_last_control_shift() == 2  ) { // update line with CTRL-click
			create_win( new line_management_gui_t( selected_line, depot->get_besitzer() ), w_info, (ptrdiff_t)selected_line.get_rep() );
		}
		else { // edit individual schedule
			// this can happen locally, since any update of the schedule is done during closing window
			schedule_t *fpl = cnv->create_schedule();
			assert(fpl!=NULL);
			gui_frame_t *fplwin = win_get_magic( (ptrdiff_t)fpl );
			if(  fplwin == NULL  ) {
				cnv->open_schedule_window( get_welt()->get_active_player() == cnv->get_besitzer() );
			}
			else {
				top_win( fplwin );
			}
		}
	}
	else {
		create_win( new news_img("Please choose vehicles first\n"), w_time_delete, magic_none );
	}
}

bool depot_frame_t::check_way_electrified(bool init)
{
	const waytype_t wt = depot->get_wegtyp();
	const weg_t *w = get_welt()->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
	const bool way_electrified = w ? w->is_electrified() : false;
	if(!init)
	{
		convoy_assembler.set_electrified( way_electrified );
	}
	if( way_electrified ) 
	{
		//img_bolt.set_image(skinverwaltung_t::electricity->get_bild_nr(0));
	}

	else
	{
		//img_bolt.set_image(IMG_LEER);
 	}

	return way_electrified;
}
//=======
//	char buf[1024];
//	const koord size = get_fenstergroesse();
//	PUSH_CLIP(pos.x, pos.y, size.x-1, size.y-1);
//
//	gui_komponente_t const* const tab = tabs.get_aktives_tab();
//	gui_image_list_t const* const lst =
//		tab == &scrolly_pas       ? &pas       :
//		tab == &scrolly_electrics ? &electrics :
//		tab == &scrolly_loks      ? &loks      :
//		&waggons;
//	int x = get_maus_x();
//	int y = get_maus_y();
//	double resale_value = -1.0;
//	const vehikel_besch_t *veh_type = NULL;
//	bool new_vehicle_length_sb_force_zero = false;
//	koord relpos = koord( 0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y() );
//	int sel_index = lst->index_at( pos + tabs.get_pos() - relpos, x, y - 16 - gui_tab_panel_t::HEADER_VSIZE);
//
//	if(  (sel_index != -1)  &&  (tabs.getroffen(x - pos.x, y - pos.y - 16))  ) {
//		// cursor over a vehicle in the selection list
//		const vector_tpl<gui_image_list_t::image_data_t>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec)));
//		veh_type = vehikelbauer_t::get_info( vec[sel_index].text );
//		if(  vec[sel_index].lcolor == COL_RED  ||  veh_action == va_sell  ) {
//			// don't show new_vehicle_length_sb when can't actually add the highlighted vehicle, or selling from inventory
//			new_vehicle_length_sb_force_zero = true;
//		}
//		if(  vec[sel_index].count > 0  ) {
//			resale_value = calc_restwert( veh_type );
//		}
//	}
//	else {
//		// cursor over a vehicle in the convoi
//		sel_index = convoi.index_at( pos , x, y - 16);
//		if(  sel_index != -1  ) {
//			convoihandle_t cnv = depot->get_convoi( icnv );
//			veh_type = cnv->get_vehikel( sel_index )->get_besch();
//			resale_value = cnv->get_vehikel( sel_index )->calc_restwert();
//			new_vehicle_length_sb_force_zero = true;
//		}
//	}
//
//	{
//		const char *c;
//		switch(  const uint32 count = depot->get_vehicle_list().get_count()  ) {
//			case 0: {
//				c = translator::translate("Keine Einzelfahrzeuge im Depot");
//				break;
//			}
//			case 1: {
//				c = translator::translate("1 Einzelfahrzeug im Depot");
//				break;
//			}
//			default: {
//				sprintf( buf, translator::translate("%d Einzelfahrzeuge im Depot"), count );
//				c = buf;
//				break;
//			}
//		}
//		display_proportional( pos.x + D_MARGIN_LEFT, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 16 + 4, c, ALIGN_LEFT, COL_BLACK, true );
//	}
//
//	if(  veh_type  ) {
//		// column 1
//		int n = sprintf( buf, "%s", translator::translate( veh_type->get_name(), depot->get_welt()->get_settings().get_name_language_id() ) );
//
//		if(  veh_type->get_leistung() > 0  ) { // LOCO
//			n += sprintf( buf + n, " (%s)\n", translator::translate( engine_type_names[veh_type->get_engine_type()+1] ) );
//		}
//		else {
//			n += sprintf( buf + n, "\n");
//		}
//
//		{
//			char tmp[128];
//			money_to_string( tmp, veh_type->get_preis() / 100.0, false );
//			n += sprintf( buf + n, translator::translate("Cost: %8s (%.2f$/km)\n"), tmp, veh_type->get_betriebskosten() / 100.0 );
//		}
//
//		if(  veh_type->get_zuladung() > 0  ) { // must translate as "Capacity: %3d%s %s\n"
//			n += sprintf( buf + n, translator::translate("Capacity: %d%s %s\n"),
//				veh_type->get_zuladung(),
//				translator::translate( veh_type->get_ware()->get_mass() ),
//				veh_type->get_ware()->get_catg()==0 ? translator::translate( veh_type->get_ware()->get_name() ) : translator::translate( veh_type->get_ware()->get_catg_name() )
//				);
//		}
//		else {
//			n += sprintf( buf + n, "\n");
//		}
//
//		if(  veh_type->get_leistung() > 0  ) { // LOCO
//			n += sprintf( buf + n, translator::translate("Power: %4d kW\n"), veh_type->get_leistung() );
//		}
//		else {
//			n += sprintf( buf + n, "\n");
//		}
//
//		n += sprintf( buf + n, "%s %4.1ft\n", translator::translate("Weight:"), veh_type->get_gewicht() / 1000.0 );
//		sprintf( buf + n, "%s %3d km/h", translator::translate("Max. speed:"), veh_type->get_geschw() );
//
//		display_multiline_text( pos.x + D_MARGIN_LEFT, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE + 4, buf,  COL_BLACK);
//
//		// column 2
//		n = sprintf( buf, "%s %s %04d\n",
//			translator::translate("Intro. date:"),
//			translator::get_month_name( veh_type->get_intro_year_month() % 12 ),
//			veh_type->get_intro_year_month() / 12
//			);
//
//		if(  veh_type->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12  ) {
//			n += sprintf( buf + n, "%s %s %04d\n",
//				translator::translate("Retire. date:"),
//				translator::get_month_name( veh_type->get_retire_year_month() % 12 ),
//				veh_type->get_retire_year_month() / 12
//				);
//		}
//		else {
//			n += sprintf( buf + n, "\n");
//		}
//
//		if(  veh_type->get_leistung() > 0  &&  veh_type->get_gear() != 64  ) {
//			n += sprintf( buf + n, "%s %0.2f : 1\n", translator::translate("Gear:"), veh_type->get_gear() / 64.0 );
//		}
//		else {
//			n += sprintf( buf + n, "\n");
//		}
//
//		if(  char const* const copyright = veh_type->get_copyright()  ) {
//			n += sprintf( buf + n, translator::translate("Constructed by %s"), copyright );
//		}
//		n += sprintf( buf +  n, "\n");
//
//		if(  resale_value != -1.0  ) {
//			char tmp[128];
//			money_to_string(  tmp, resale_value / 100.0, false );
//			sprintf( buf + n, "%s %8s", translator::translate("Restwert:"), tmp );
//		}
//
//		display_multiline_text( pos.x + second_column_x, pos.y + tabs.get_pos().y + tabs.get_groesse().y + 31 + LINESPACE * 2 + 4, buf, COL_BLACK);
//
//		// update speedbar
//		new_vehicle_length_sb = new_vehicle_length_sb_force_zero ? 0 : convoi_length_ok_sb + convoi_length_slower_sb + convoi_length_too_slow_sb + veh_type->get_length();
//	}
//	else {
//		new_vehicle_length_sb = 0;
//	}
//
//	POP_CLIP();
//}
//
//
//void depot_frame_t::update_tabs()
//{
//	gui_komponente_t *old_tab = tabs.get_aktives_tab();
//	tabs.clear();
//
//	bool one = false;
//
//	// add only if there are any
//	if(  !pas_vec.empty()  ) {
//		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
//		one = true;
//	}
//
//	// add only if there are any trolleybuses
//	if(  !electrics_vec.empty()  ) {
//		tabs.add_tab(&scrolly_electrics, translator::translate( depot->get_electrics_name() ) );
//		one = true;
//	}
//
//	// add, if waggons are there ...
//	if(  !loks_vec.empty()  ||  !waggons_vec.empty()  ) {
//		tabs.add_tab(&scrolly_loks, translator::translate( depot->get_zieher_name() ) );
//		one = true;
//	}
//
//	// only add, if there are waggons
//	if(  !waggons_vec.empty()  ) {
//		tabs.add_tab(&scrolly_waggons, translator::translate( depot->get_haenger_name() ) );
//		one = true;
//	}
//
//	if(  !one  ) {
//		// add passenger as default
//		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
//	}
//
//	// Look, if there is our old tab present again (otherwise it will be 0 by tabs.clear()).
//	for(  uint8 i = 0;  i < tabs.get_count();  i++  ) {
//		if(  old_tab == tabs.get_tab(i)  ) {
//			// Found it!
//			tabs.set_active_tab_index(i);
//			break;
//		}
//	}
//>>>>>>> aburch/master
//}
//
//
//depot_convoi_capacity_t::depot_convoi_capacity_t()
//{
//	total_pax = 0;
//	total_mail = 0;
//	total_goods = 0;
//}
//
//
//void depot_convoi_capacity_t::set_totals(uint32 pax, uint32 mail, uint32 goods)
//{
//	total_pax = pax;
//	total_mail = mail;
//	total_goods = goods;
//}
//
//
//void depot_convoi_capacity_t::zeichnen(koord off)
//{
//	cbuffer_t cbuf;
//
//	int w = 0;
//	cbuf.clear();
//	cbuf.printf("%s %d", translator::translate("Capacity:"), total_pax );
//	w += display_proportional_clip( pos.x+off.x + w, pos.y+off.y , cbuf, ALIGN_LEFT, COL_BLACK, true);
//	display_color_img( skinverwaltung_t::passagiere->get_bild_nr(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);
//
//	w += 16;
//	cbuf.clear();
//	cbuf.printf("%d", total_mail );
//	w += display_proportional_clip( pos.x+off.x + w, pos.y+off.y, cbuf, ALIGN_LEFT, COL_BLACK, true);
//	display_color_img( skinverwaltung_t::post->get_bild_nr(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);
//
//	w += 16;
//	cbuf.clear();
//	cbuf.printf("%d", total_goods );
//	w += display_proportional_clip( pos.x+off.x + w, pos.y+off.y, cbuf, ALIGN_LEFT, COL_BLACK, true);
//	display_color_img( skinverwaltung_t::waren->get_bild_nr(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);
//}
