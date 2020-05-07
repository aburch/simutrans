/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * The depot window, where to buy convois
 */

#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "../simunits.h"
#include "../simworld.h"
#include "../vehicle/simvehicle.h"
#include "../simconvoi.h"
#include "../simdepot.h"
#include "simwin.h"
#include "../simcolor.h"
#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simline.h"
#include "../simlinemgmt.h"
#include "../simmenu.h"
#include "../simskin.h"

#include "../tpl/slist_tpl.h"

#include "schedule_gui.h"
#include "line_management_gui.h"
#include "line_item.h"
#include "convoy_item.h"
#include "components/gui_image_list.h"
#include "messagebox.h"
#include "depot_frame.h"
#include "schedule_list.h"

#include "../descriptor/goods_desc.h"
#include "../descriptor/intro_dates.h"
#include "../bauer/vehikelbauer.h"
#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simrandom.h"

#include "../bauer/goods_manager.h"

#include "../boden/wege/weg.h"


char depot_frame_t::name_filter_value[64] = "";


static int sort_by_action;

bool depot_frame_t::show_retired_vehicles = false;
bool depot_frame_t::show_all = true;

depot_frame_t::depot_frame_t(depot_t* depot) :
	gui_frame_t("", NULL),
	depot(depot),
	icnv(-1),
	lb_convoi_line("Serves Line:", SYSCOL_TEXT, gui_label_t::left),
	lb_sort_by("Sort by:", SYSCOL_TEXT, gui_label_t::right),
	lb_name_filter_input("Search:", SYSCOL_TEXT, gui_label_t::right),
	lb_veh_action("Fahrzeuge:", SYSCOL_TEXT, gui_label_t::right),
	convoi(&convoi_pics),
	scrolly_convoi(&cont_convoi),
	pas(&pas_vec),
	electrics(&electrics_vec),
	loks(&loks_vec),
	waggons(&waggons_vec),
	scrolly_pas(&pas),
	scrolly_electrics(&electrics),
	scrolly_loks(&loks),
	scrolly_waggons(&waggons),
	line_selector(line_scrollitem_t::compare),
	lb_vehicle_filter("Filter:", SYSCOL_TEXT, gui_label_t::right)
{
	if (depot) {
		init(depot);
	}
}

void depot_frame_t::init(depot_t *dep)
{
	depot = dep;
	set_name(translator::translate(depot->get_name()));
	set_owner(depot->get_owner());
	icnv = depot->convoi_count()-1;

	scr_size size = scr_size(0,0);

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
	* [SELECT]:
	*/
	add_component(&lb_convois);

	convoy_selector.add_listener(this);
	add_component(&convoy_selector);

	/*
	* [SELECT ROUTE]:
	*/
	line_selector.add_listener(this);
	line_selector.set_wrapping(false);
	add_component(&line_selector);

	// goto line button
	line_button.set_typ(button_t::posbutton);
	line_button.set_targetpos3d(koord3d::invalid);
	line_button.add_listener(this);
	add_component(&line_button);

	/*
	* [CONVOI]
	*/
	convoi.set_player_nr(depot->get_player_nr());
	convoi.add_listener(this);

	cont_convoi.add_component(&lb_convoi_number);
	cont_convoi.add_component(&convoi);

	//	add_component(&convoi);

	add_component(&lb_convoi_count);
	add_component(&lb_convoi_speed);
	add_component(&lb_convoi_cost);
	add_component(&lb_convoi_value);
	add_component(&lb_convoi_line);
	add_component(&lb_convoi_power);
	add_component(&lb_convoi_weight);
	add_component(&cont_convoi_capacity);

	sb_convoi_length.set_base( depot->get_max_convoi_length() * CARUNITS_PER_TILE / 2 - 1);
	sb_convoi_length.set_vertical(false);
	convoi_length_ok_sb = 0;
	convoi_length_slower_sb = 0;
	convoi_length_too_slow_sb = 0;
	convoi_tile_length_sb = 0;
	new_vehicle_length_sb = 0;
	if(  depot->get_max_convoi_length() > 1  ) { // no convoy length bar for ships or aircraft
		sb_convoi_length.add_color_value(&convoi_tile_length_sb, color_idx_to_rgb(COL_BROWN));
		sb_convoi_length.add_color_value(&new_vehicle_length_sb, color_idx_to_rgb(COL_DARK_GREEN));
		sb_convoi_length.add_color_value(&convoi_length_ok_sb, color_idx_to_rgb(COL_GREEN));
		sb_convoi_length.add_color_value(&convoi_length_slower_sb, color_idx_to_rgb(COL_ORANGE));
		sb_convoi_length.add_color_value(&convoi_length_too_slow_sb, color_idx_to_rgb(COL_RED));
		cont_convoi.add_component(&sb_convoi_length);
	}

	scrolly_convoi.set_scrollbar_mode(scrollbar_t::show_disabled);
	scrolly_convoi.set_size_corner(false);
	add_component(&scrolly_convoi);

	/*
	* [ACTIONS]
	*/
	bt_start.set_typ(button_t::roundbox);
	bt_start.add_listener(this);
	bt_start.set_tooltip("Start the selected vehicle(s)");
	add_component(&bt_start);

	bt_schedule.set_typ(button_t::roundbox);
	bt_schedule.add_listener(this);
	bt_schedule.set_tooltip("Give the selected vehicle(s) an individual schedule"); // translated to "Edit the selected vehicle(s) individual schedule or assigned line"
	add_component(&bt_schedule);

	bt_copy_convoi.set_typ(button_t::roundbox);
	bt_copy_convoi.add_listener(this);
	bt_copy_convoi.set_tooltip("Copy the selected convoi and its schedule or line");
	add_component(&bt_copy_convoi);

	bt_sell.set_typ(button_t::roundbox);
	bt_sell.add_listener(this);
	bt_sell.set_tooltip("Sell the selected vehicle(s)");
	add_component(&bt_sell);

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

	add_component(&tabs);
	add_component(&div_tabbottom);
	add_component(&lb_veh_action);
	add_component(&lb_sort_by);
	add_component(&lb_name_filter_input);
	add_component(&lb_vehicle_filter);
	add_component(&div_action_bottom);

	veh_action = va_append;
	bt_veh_action.set_typ(button_t::roundbox);
	bt_veh_action.add_listener(this);
	bt_veh_action.set_tooltip("Choose operation executed on clicking stored/new vehicles");
	add_component(&bt_veh_action);

	bt_show_all.set_typ(button_t::square_state);
	bt_show_all.set_text("Show all");
	bt_show_all.add_listener(this);
	bt_show_all.set_tooltip("Show also vehicles that do not match for current action.");
	bt_show_all.pressed = show_all;
	add_component(&bt_show_all);

	bt_obsolete.set_typ(button_t::square_state);
	bt_obsolete.set_text("Show obsolete");
	bt_obsolete.pressed = show_retired_vehicles;
	if(  welt->get_settings().get_allow_buying_obsolete_vehicles()  ) {
		bt_obsolete.add_listener(this);
		bt_obsolete.set_tooltip("Show also vehicles no longer in production.");
		add_component(&bt_obsolete);
	}

	sort_by.add_listener(this);
	add_component(&sort_by);

	vehicle_filter.add_listener(this);
	add_component(&vehicle_filter);

	name_filter_input.set_text(name_filter_value, 60);
	add_component(&name_filter_input);
	name_filter_input.add_listener(this);

	build_vehicle_lists();

	// text will be translated by ourselves (after update data)!
	lb_convois.set_text_pointer( txt_convois );
	lb_convoi_number.set_text_pointer( txt_convoi_number );
	lb_convoi_count.set_text_pointer( txt_convoi_count );
	lb_convoi_speed.set_text_pointer( txt_convoi_speed );
	lb_convoi_cost.set_text_pointer( txt_convoi_cost );
	lb_convoi_value.set_text_pointer( txt_convoi_value );
	lb_convoi_power.set_text_pointer( txt_convoi_power );
	lb_convoi_weight.set_text_pointer( txt_convoi_weight );

	// Bolt image for electrified depots:
	add_component(&img_bolt);

	scrolly_pas.set_scrollbar_mode       ( scrollbar_t::show_disabled );
	scrolly_pas.set_size_corner(false);
	scrolly_electrics.set_scrollbar_mode ( scrollbar_t::show_disabled );
	scrolly_electrics.set_size_corner(false);
	scrolly_loks.set_scrollbar_mode      ( scrollbar_t::show_disabled );
	scrolly_loks.set_size_corner(false);
	scrolly_waggons.set_scrollbar_mode   ( scrollbar_t::show_disabled );
	scrolly_waggons.set_size_corner(false);

	layout(&size);
	gui_frame_t::set_windowsize(size);
	set_resizemode( diagonal_resize );

	depot->clear_command_pending();
}


// free memory: all the image_data_t
depot_frame_t::~depot_frame_t()
{
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
}


// returns position of depot on the map
koord3d depot_frame_t::get_weltpos(bool)
{
	return depot->get_pos();
}


bool depot_frame_t::is_weltpos()
{
	return ( welt->get_viewport()->is_on_center( get_weltpos(false) ) );
}


void depot_frame_t::layout(scr_size *size)
{
	scr_coord placement;
	scr_coord grid;
	scr_coord_val grid_dx;
	scr_coord_val placement_dx;

	/*
	* These parameter are adjusted to resolution.
	* - Some extra space looks nicer.
	*/
	grid.x = depot->get_x_grid() * get_base_tile_raster_width() / 64 + 4;
	grid.y = depot->get_y_grid() * get_base_tile_raster_width() / 64 + 6;
	placement.x = depot->get_x_placement() * get_base_tile_raster_width() / 64 + 2;
	placement.y = depot->get_y_placement() * get_base_tile_raster_width() / 64 + 2;
	grid_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 2;
	placement_dx = depot->get_x_grid() * get_base_tile_raster_width() / 64 / 4;

	// calculate some useful default width first (since everything depends on width)
	scr_size win_size = (size != NULL) ? *size : get_windowsize();
	if (win_size.w <= 0) {
		win_size.w = max(D_DEFAULT_WIDTH,(grid.x - grid_dx)*18);
		win_size.h = 0;
	}
	if (size  &&  size->w == 0) {
		size->w = D_DEFAULT_WIDTH;
	}

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
	const scr_coord_val SELECT_HEIGHT = D_BUTTON_HEIGHT;
	const scr_coord_val selector_x = max(max(max(max(max(102, proportional_string_width(translator::translate("no convois")) + 4),
		proportional_string_width(translator::translate("1 convoi")) + 4),
		proportional_string_width(translator::translate("%d convois")) + 4),
		proportional_string_width(translator::translate("convoi %d of %d")) + 4),
		line_button.get_size().w + 2 + proportional_string_width(translator::translate(lb_convoi_line.get_text_pointer())) + 4
		);

	const scr_coord_val BUTTON_WIDTH_DEPOT = max(D_BUTTON_WIDTH,(win_size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT - 4*D_H_SPACE) / 4);

	/*
	*	Structure of [CONVOI] is a image_list and an infos:
	*
	*	    [List]
	*	    [Info]
	*
	* The image list is horizontally "condensed".
	*/
	const scr_coord_val CLIST_WIDTH = depot->get_max_convoi_length() * (grid.x - grid_dx) + 2 * gui_image_list_t::BORDER;
	const scr_coord_val CLIST_HEIGHT = grid.y + 2 * gui_image_list_t::BORDER + 5;
	const scr_coord_val CINFO_HEIGHT = LINESPACE * 3 + D_BUTTON_HEIGHT + 1;

	/*
	*	Structure of [ACTIONS] is a row of buttons:
	*
	*	    [Start][Schedule][Destroy][Sell]
	*	    [new Route][change Route][delete Route]
	*/
	/*
	*	Structure of [VINFO] is one multiline text.
	*/
	const scr_coord_val VINFO_HEIGHT =  7*LINESPACE + D_BUTTON_HEIGHT + D_EDIT_HEIGHT + 5*D_V_SPACE;

	/*
	*  Now we can do the first vertical adjustment:
	*  Calculate position of each element to tabs.
	*/
	const scr_coord_val SELECT_VSTART = D_MARGIN_TOP;
	const scr_coord_val CONVOI_VSTART = SELECT_VSTART + SELECT_HEIGHT + LINESPACE + D_V_SPACE;
	const scr_coord_val CINFO_VSTART = CONVOI_VSTART + CLIST_HEIGHT +  D_SCROLLBAR_HEIGHT*(CLIST_WIDTH >= win_size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT);
	const scr_coord_val ACTIONS_VSTART = CINFO_VSTART + CINFO_HEIGHT;
	const scr_coord_val PANEL_VSTART = ACTIONS_VSTART + D_BUTTON_HEIGHT;

	/*
	* Now we determine the row/col layout for the panel and the total panel
	* size.
	* build_vehicle_lists() fills loks_vec and waggon_vec.
	* Total width will be expanded to match complete columns in panel.
	*/
	const scr_coord_val TAB_HEADER_HEIGHT = tabs.get_required_size().h;
	const scr_coord_val total_h = PANEL_VSTART + VINFO_HEIGHT + D_TITLEBAR_HEIGHT + TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER + D_MARGIN_BOTTOM + 1;
	scr_coord_val PANEL_ROWS = max(1, ((win_size.h - total_h) / grid.y));
	if (size  &&  size->h == 0) {
		PANEL_ROWS = 3;
	}
	const scr_coord_val PANEL_HEIGHT = PANEL_ROWS * grid.y + TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER;
	const scr_coord_val MIN_PANEL_HEIGHT = grid.y + TAB_HEADER_HEIGHT + 2 * gui_image_list_t::BORDER;
	const scr_coord_val INFO_VSTART = PANEL_VSTART + PANEL_HEIGHT + div_tabbottom.get_size().h;

	/*
	*  Now we can do the complete vertical adjustment:
	*/
	const scr_coord_val TOTAL_HEIGHT = PANEL_VSTART + PANEL_HEIGHT + D_V_SPACE + VINFO_HEIGHT + D_TITLEBAR_HEIGHT + 1 + D_MARGIN_BOTTOM;
	const scr_coord_val MIN_TOTAL_HEIGHT = PANEL_VSTART + MIN_PANEL_HEIGHT + D_V_SPACE + VINFO_HEIGHT + D_TITLEBAR_HEIGHT + 1 + D_MARGIN_BOTTOM;

	// now we know also the height, so we can set the size of the whole dialogue
	if(  win_size.h <= 0  ) {
		// set default size on first call
		win_size.h = TOTAL_HEIGHT;
	}
	win_size.h = max(win_size.h, MIN_TOTAL_HEIGHT);
	if(  size  ) {
		*size = win_size;
	}
	gui_frame_t::set_windowsize(win_size);
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, MIN_TOTAL_HEIGHT));

	/*
	* DONE with layout planning - now build everything.
	*/

	/*
	 * [SELECT]:
	 */
	lb_convois.set_pos(scr_coord(D_MARGIN_LEFT, SELECT_VSTART + 3));
	lb_convois.set_width(selector_x - D_H_SPACE);

	convoy_selector.set_pos(scr_coord(D_MARGIN_LEFT + selector_x, SELECT_VSTART));
	convoy_selector.set_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, D_BUTTON_HEIGHT));
	convoy_selector.set_max_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, LINESPACE * 13 + 2 + 16));

	/*
	 * [SELECT ROUTE]:
	 */
	line_button.set_pos(scr_coord(D_MARGIN_LEFT, SELECT_VSTART + D_BUTTON_HEIGHT));
	lb_convoi_line.set_pos(scr_coord(D_MARGIN_LEFT + line_button.get_size().w + 2, SELECT_VSTART + D_BUTTON_HEIGHT));
	lb_convoi_line.set_width(selector_x - line_button.get_size().w - 2 - D_H_SPACE);

	line_selector.set_pos(scr_coord(D_MARGIN_LEFT + selector_x, SELECT_VSTART + D_BUTTON_HEIGHT));
	line_selector.set_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, D_BUTTON_HEIGHT));
	line_selector.set_max_size(scr_size(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT - selector_x, LINESPACE * 13 + 2 + 16));

	line_button.align_to(&line_selector, ALIGN_CENTER_V);
	lb_convoi_line.align_to(&line_selector, ALIGN_CENTER_V);

	/*
	 * [CONVOI]
	 */
	convoi.set_grid(scr_coord(grid.x - grid_dx, grid.y));
	convoi.set_placement(scr_coord(placement.x - placement_dx, placement.y));
	convoi.set_pos(scr_coord(0, 0));
	convoi.set_size(scr_size(CLIST_WIDTH, CLIST_HEIGHT));

	sb_convoi_length.set_pos(scr_coord(5, grid.y + 5));
	sb_convoi_length.set_size(scr_size(CLIST_WIDTH - 10, 4));

	cont_convoi.set_size(scr_size(CLIST_WIDTH, grid.y + 5 + 4));
	scrolly_convoi.set_size(scr_size(win_size.w - D_MARGIN_LEFT - D_MARGIN_RIGHT, cont_convoi.get_size().h + (3+D_SCROLLBAR_HEIGHT)*(CLIST_WIDTH >= win_size.w-D_MARGIN_LEFT-D_MARGIN_RIGHT) ));
	scrolly_convoi.set_show_scroll_x(true);
	scrolly_convoi.set_show_scroll_y(false);
	scrolly_convoi.set_scroll_discrete_x(false);
	scrolly_convoi.set_size_corner(false);
	scrolly_convoi.set_pos(scr_coord(D_MARGIN_LEFT,CONVOI_VSTART));

	lb_convoi_number.set_width(30);
	lb_convoi_number.set_color(COL_WHITE);

	// place for description text
	second_column_x = D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE*2)*2;
	second_column_w = (BUTTON_WIDTH_DEPOT+D_H_SPACE)*2;

	lb_convoi_count.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART));
	lb_convoi_count.set_width(second_column_w);

	cont_convoi_capacity.set_pos(scr_coord(second_column_x, CINFO_VSTART));
	cont_convoi_capacity.set_width(second_column_w);

	lb_convoi_cost.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 1));
	lb_convoi_cost.set_width(second_column_w);
	lb_convoi_value.set_pos(scr_coord(second_column_x, CINFO_VSTART + LINESPACE * 1));
	lb_convoi_value.set_width(second_column_w);

	lb_convoi_power.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 2));
	lb_convoi_power.set_width(second_column_w);
	lb_convoi_weight.set_pos(scr_coord(second_column_x, CINFO_VSTART + LINESPACE * 2));
	lb_convoi_weight.set_width(second_column_w);

	lb_convoi_speed.set_pos(scr_coord(D_MARGIN_LEFT, CINFO_VSTART + LINESPACE * 3));
	lb_convoi_speed.set_width(win_size.w - D_MARGIN_RIGHT - D_MARGIN_LEFT);

	/*
	 * [ACTIONS]
	 */
	bt_start.set_pos(scr_coord(D_MARGIN_LEFT, ACTIONS_VSTART));
	bt_start.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_start.set_text("Start");

	bt_schedule.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE), ACTIONS_VSTART));
	bt_schedule.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_schedule.set_text("Fahrplan");

	bt_copy_convoi.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*2, ACTIONS_VSTART));
	bt_copy_convoi.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_copy_convoi.set_text("Copy Convoi");

	bt_sell.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT + D_H_SPACE)*3, ACTIONS_VSTART));
	bt_sell.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	bt_sell.set_text("verkaufen");

	/*
	* [PANEL]
	*/
	tabs.set_pos(scr_coord(0, PANEL_VSTART));
	tabs.set_size(scr_size(win_size.w, PANEL_HEIGHT));

	pas.set_grid(grid);
	pas.set_placement(placement);
	pas.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	pas.recalc_size();
	pas.set_pos(scr_coord(0, 0));
	scrolly_pas.set_size(scrolly_pas.get_size());
	scrolly_pas.set_scroll_amount_y(grid.y);
	scrolly_pas.set_scroll_discrete_y(false);
	scrolly_pas.set_size_corner(false);

	electrics.set_grid(grid);
	electrics.set_placement(placement);
	electrics.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	electrics.recalc_size();
	electrics.set_pos(scr_coord(0, 0));
	scrolly_electrics.set_size(scrolly_electrics.get_size());
	scrolly_electrics.set_scroll_amount_y(grid.y);
	scrolly_electrics.set_scroll_discrete_y(false);
	scrolly_electrics.set_size_corner(false);

	loks.set_grid(grid);
	loks.set_placement(placement);
	loks.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	loks.recalc_size();
	loks.set_pos(scr_coord(0, 0));
	scrolly_loks.set_size(scrolly_loks.get_size());
	scrolly_loks.set_scroll_amount_y(grid.y);
	scrolly_loks.set_scroll_discrete_y(false);
	scrolly_loks.set_size_corner(false);

	waggons.set_grid(grid);
	waggons.set_placement(placement);
	waggons.set_size(tabs.get_size() - scr_size(D_SCROLLBAR_WIDTH, 0));
	waggons.recalc_size();
	waggons.set_pos(scr_coord(0, 0));
	scrolly_waggons.set_size(scrolly_waggons.get_size());
	scrolly_waggons.set_scroll_amount_y(grid.y);
	scrolly_waggons.set_scroll_discrete_y(false);
	scrolly_waggons.set_size_corner(false);

	div_tabbottom.set_pos(scr_coord(0, PANEL_VSTART + PANEL_HEIGHT));
	div_tabbottom.set_width(win_size.w);

	/*
	* [BOTTOM]
	*/

	// 1st line
	bt_veh_action.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*3, INFO_VSTART));
	bt_veh_action.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	lb_veh_action.align_to(&bt_veh_action, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_CENTER_V, scr_coord(D_H_SPACE, 0));

	// 2nd line
	vehicle_filter.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*2 - proportional_string_width(translator::translate("Search:")), INFO_VSTART + D_BUTTON_HEIGHT + D_V_SPACE));
	vehicle_filter.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	vehicle_filter.set_max_size(scr_size(D_BUTTON_WIDTH, LINESPACE*7));
	lb_vehicle_filter.align_to(&vehicle_filter, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(2,D_GET_CENTER_ALIGN_OFFSET(LINESPACE,D_BUTTON_HEIGHT)));

	name_filter_input.set_pos( scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*3, INFO_VSTART + D_BUTTON_HEIGHT + D_V_SPACE) );
	name_filter_input.set_size( scr_size( BUTTON_WIDTH_DEPOT, D_EDIT_HEIGHT ) );
	lb_name_filter_input.align_to(&name_filter_input, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(D_H_SPACE,D_GET_CENTER_ALIGN_OFFSET(LINESPACE,D_BUTTON_HEIGHT)));

	bt_obsolete.set_pos(scr_coord(D_MARGIN_LEFT, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	bt_obsolete.align_to(&name_filter_input, ALIGN_CENTER_V);

	//3rd line
	sort_by.set_pos(scr_coord(D_MARGIN_LEFT + (BUTTON_WIDTH_DEPOT+D_H_SPACE)*3, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
	sort_by.set_size(scr_size(BUTTON_WIDTH_DEPOT, D_BUTTON_HEIGHT));
	sort_by.set_max_size(scr_size(D_BUTTON_WIDTH, LINESPACE * 7));
	lb_sort_by.align_to(&sort_by, ALIGN_RIGHT | ALIGN_EXTERIOR_H | ALIGN_TOP, scr_coord(D_H_SPACE,D_GET_CENTER_ALIGN_OFFSET(LINESPACE,D_BUTTON_HEIGHT)));

	bt_show_all.set_pos(scr_coord(D_MARGIN_LEFT, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE)*2));
//	bt_show_all.align_to(&sort_by, ALIGN_CENTER_TOP); Comboboxes change height when openen!!!

	div_action_bottom.set_pos(scr_coord(0, INFO_VSTART + (D_BUTTON_HEIGHT + D_V_SPACE) * 3));
	div_action_bottom.set_width(win_size.w);

	const scr_coord_val margin = 4;
	img_bolt.set_pos(scr_coord(get_windowsize().w - skinverwaltung_t::electricity->get_image(0)->get_pic()->w - margin, margin));

}


void depot_frame_t::set_windowsize( scr_size size )
{
	if (depot) {
		update_data();
		layout(&size);
	}
	gui_frame_t::set_windowsize(size);
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


// true if already stored here
bool depot_frame_t::is_in_vehicle_list(const vehicle_desc_t *info)
{
	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		if(  v->get_desc() == info  ) {
			return true;
		}
	}
	return false;
}


// add a single vehicle (helper function)
void depot_frame_t::add_to_vehicle_list(const vehicle_desc_t *info)
{
	// Check if vehicle should be filtered
	const goods_desc_t *freight = info->get_freight_type();
	// Only filter when required and never filter engines
	if (depot->selected_filter > 0 && info->get_capacity() > 0) {
		if (depot->selected_filter == VEHICLE_FILTER_RELEVANT) {
			if(freight->get_catg_index() >= 3) {
				bool found = false;
				FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
					if (freight->get_catg_index() == i->get_catg_index()) {
						found = true;
						break;
					}
				}

				// If no current goods can be transported by this vehicle, don't display it
				if (!found) return;
			}
		}
		else if (depot->selected_filter > VEHICLE_FILTER_RELEVANT) {
			// Filter on specific selected good
			uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
			if (goods_index < welt->get_goods_list().get_count()) {
				const goods_desc_t *selected_good = welt->get_goods_list()[goods_index];
				if (freight->get_catg_index() != selected_good->get_catg_index()) {
					return; // This vehicle can't transport the selected good
				}
			}
		}
	}

	gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), info->get_base_image());

	if(  info->get_engine_type() == vehicle_desc_t::electric  &&  (info->get_freight_type()==goods_manager_t::passengers  ||  info->get_freight_type()==goods_manager_t::mail)  ) {
		electrics_vec.append(img_data);
	}
	// since they come "pre-sorted" from the vehikelbauer, we have to do nothing to keep them sorted
	else if(info->get_freight_type() == goods_manager_t::passengers  ||  info->get_freight_type() == goods_manager_t::mail) {
		pas_vec.append(img_data);
	}
	else if(info->get_power() > 0  ||  info->get_capacity()==0) {
		loks_vec.append(img_data);
	}
	else {
		waggons_vec.append(img_data);
	}
	// add reference to map
	vehicle_map.set(info, img_data);
}

// add all current vehicles
void depot_frame_t::build_vehicle_lists()
{
	if (depot->get_vehicle_type().empty()) {
		// there are tracks etc. but no vehicles => do nothing
		// at least initialize some data
		update_data();
		update_tabs();
		return;
	}

	const int month_now = welt->get_timeline_year_month();

	// free vectors
	clear_ptr_vector(pas_vec);
	clear_ptr_vector(electrics_vec);
	clear_ptr_vector(loks_vec);
	clear_ptr_vector(waggons_vec);
	// clear map
	vehicle_map.clear();

	// we do not allow to built electric vehicle in a depot without electrification
	const waytype_t wt = depot->get_waytype();
	const weg_t *w = welt->lookup(depot->get_pos())->get_weg(wt!=tram_wt ? wt : track_wt);
	const bool weg_electrified = w ? w->is_electrified() : false;

	img_bolt.set_image( weg_electrified ? skinverwaltung_t::electricity->get_image_id(0) : IMG_EMPTY );

	sort_by_action = depot->selected_sort_by;

	vector_tpl<const vehicle_desc_t*> typ_list;

	if(!show_all  &&  veh_action==va_sell) {
		// show only sellable vehicles
		FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
			vehicle_desc_t const* const d = v->get_desc();
			typ_list.append(d);
		}
	}
	else {
		slist_tpl<const vehicle_desc_t*> const& tmp_list = depot->get_vehicle_type(sort_by_action);
		for(slist_tpl<const vehicle_desc_t*>::const_iterator itr = tmp_list.begin(); itr != tmp_list.end(); ++itr) {
			typ_list.append(*itr);
		}
	}

	// use this to show only sellable vehicles
	if(!show_all  &&  veh_action==va_sell) {
		// just list the one to sell
		FOR(vector_tpl<vehicle_desc_t const*>, const info, typ_list) {
			if (vehicle_map.get(info)) continue;
			add_to_vehicle_list(info);
		}
	}
	else {
		// list only matching ones
		FOR(vector_tpl<vehicle_desc_t const*>, const info, typ_list) {
			const vehicle_desc_t *veh = NULL;
			convoihandle_t cnv = depot->get_convoi(icnv);
			if(cnv.is_bound() && cnv->get_vehicle_count()>0) {
				veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
			}

			// current vehicle
			if( is_in_vehicle_list(info)  ||
				((weg_electrified  ||  info->get_engine_type()!=vehicle_desc_t::electric)  &&
					 ((!info->is_future(month_now))  &&  (show_retired_vehicles  ||  (!info->is_retired(month_now)) )  ) )) {
				// check, if allowed
				bool append = true;
				if(!show_all) {
					if(veh_action == va_insert) {
						append = info->can_lead(veh)  &&  (veh==NULL  ||  veh->can_follow(info));
					} else if(veh_action == va_append) {
						append = info->can_follow(veh)  &&  (veh==NULL  ||  veh->can_lead(info));
					}
				}
				if(append) {
					// name filter. Try to check both object name and translation name (case sensitive though!)
					if(  name_filter_value[0]==0  ||  (strstr(info->get_name(), name_filter_value)  ||  strstr(translator::translate(info->get_name()), name_filter_value))  ) {
						add_to_vehicle_list( info );
					}
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
	depot->get_owner()->simlinemgmt.get_lines(depot->get_line_type(), lines);
}


void depot_frame_t::update_data()
{
	static const char *txt_veh_action[3] = { "anhaengen", "voranstellen", "verkaufen" };

	// change green into blue for retired vehicles
	const int month_now = welt->get_timeline_year_month();

	bt_veh_action.set_text(txt_veh_action[veh_action]);

	txt_convois.clear();
	switch(  depot->convoi_count()  ) {
		case 0: {
			txt_convois.append( translator::translate("no convois") );
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
	convoy_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_convoy_text, SYSCOL_TEXT ) ;
	convoy_selector.set_selection(0);

	// check all matching convoys
	FOR(slist_tpl<convoihandle_t>, const c, depot->get_convoy_list()) {
		convoy_selector.new_component<convoy_scrollitem_t>(c) ;
		if(  cnv.is_bound()  &&  c == cnv  ) {
			convoy_selector.set_selection( convoy_selector.count_elements() - 1 );
		}
	}

	const vehicle_desc_t *veh = NULL;

	clear_ptr_vector( convoi_pics );
	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		for(  unsigned i=0;  i < cnv->get_vehicle_count();  i++  ) {
			// just make sure, there is this vehicle also here!
			const vehicle_desc_t *info=cnv->get_vehikel(i)->get_desc();
			if(  vehicle_map.get( info ) == NULL  ) {
				add_to_vehicle_list( info );
			}

			gui_image_list_t::image_data_t* img_data = new gui_image_list_t::image_data_t(info->get_name(), info->get_base_image());
			convoi_pics.append(img_data);
		}

		/* color bars for current convoi: */
		convoi_pics[0]->lcolor = color_idx_to_rgb(cnv->front()->get_desc()->can_follow(NULL) ? COL_GREEN : COL_YELLOW);
		{
			unsigned i;
			for(  i = 1;  i < cnv->get_vehicle_count();  i++  ) {
				convoi_pics[i - 1]->rcolor = color_idx_to_rgb(cnv->get_vehikel(i-1)->get_desc()->can_lead(cnv->get_vehikel(i)->get_desc()) ? COL_GREEN : COL_RED);
				convoi_pics[i]->lcolor     = color_idx_to_rgb(cnv->get_vehikel(i)->get_desc()->can_follow(cnv->get_vehikel(i-1)->get_desc()) ? COL_GREEN : COL_RED);
			}
			convoi_pics[i - 1]->rcolor = color_idx_to_rgb(cnv->get_vehikel(i-1)->get_desc()->can_lead(NULL) ? COL_GREEN : COL_YELLOW);
		}

		// change green into blue for vehicles that are not available
		for(  unsigned i = 0;  i < cnv->get_vehicle_count();  i++  ) {
			if(  !cnv->get_vehikel(i)->get_desc()->is_available(month_now)  ) {
				if(  convoi_pics[i]->lcolor == color_idx_to_rgb(COL_GREEN)  ) {
					convoi_pics[i]->lcolor = color_idx_to_rgb(COL_BLUE);
				}
				if(  convoi_pics[i]->rcolor == color_idx_to_rgb(COL_GREEN)  ) {
					convoi_pics[i]->rcolor = color_idx_to_rgb(COL_BLUE);
				}
			}
		}

		veh = (veh_action == va_insert ? cnv->front() : cnv->back())->get_desc();
	}

	FOR(vehicle_image_map, const& i, vehicle_map) {
		vehicle_desc_t const* const    info = i.key;
		gui_image_list_t::image_data_t& img  = *i.value;
		const PIXVAL ok_color = color_idx_to_rgb(info->is_available(month_now) ? COL_GREEN : COL_BLUE);

		img.count = 0;
		img.lcolor = ok_color;
		img.rcolor = ok_color;

		/*
		* color bars for current convoi:
		*  green/green	okay to append/insert
		*  red/red		cannot be appended/inserted
		*  green/yellow	append okay, cannot be end of train
		*  yellow/green	insert okay, cannot be start of train
		*/

		if(veh_action == va_insert) {
			if(!info->can_lead(veh)  ||  (veh  &&  !veh->can_follow(info))) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(!info->can_follow(NULL)) {
				img.lcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if(veh_action == va_append) {
			if(!info->can_follow(veh)  ||  (veh  &&  !veh->can_lead(info))) {
				img.lcolor = color_idx_to_rgb(COL_RED);
				img.rcolor = color_idx_to_rgb(COL_RED);
			}
			else if(!info->can_lead(NULL)) {
				img.rcolor = color_idx_to_rgb(COL_YELLOW);
			}
		}
		else if( veh_action == va_sell ) {
			img.lcolor = color_idx_to_rgb(COL_RED);
			img.rcolor = color_idx_to_rgb(COL_RED);
		}
	}

	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		// can fail, if currently not visible
		if (gui_image_list_t::image_data_t* const imgdat = vehicle_map.get(v->get_desc())) {
			imgdat->count++;
			if(veh_action == va_sell) {
				imgdat->lcolor = color_idx_to_rgb(COL_GREEN);
				imgdat->rcolor = color_idx_to_rgb(COL_GREEN);
			}
		}
	}

	// update the line selector
	line_selector.clear_elements();

	if(  !last_selected_line.is_bound()  ) {
		// new line may have a valid line now
		last_selected_line = selected_line;
		// if still nothing, resort to line management dialoge
		if(  !last_selected_line.is_bound()  ) {
			// try last specific line
			last_selected_line = schedule_list_gui_t::selected_line[ depot->get_owner()->get_player_nr() ][ depot->get_line_type() ];
		}
		if(  !last_selected_line.is_bound()  ) {
			// try last general line
			last_selected_line = schedule_list_gui_t::selected_line[ depot->get_owner()->get_player_nr() ][ 0 ];
			if(  last_selected_line.is_bound()  &&  last_selected_line->get_linetype() != depot->get_line_type()  ) {
				last_selected_line = linehandle_t();
			}
		}
	}
	if(  cnv.is_bound()  &&  cnv->get_schedule()  &&  !cnv->get_schedule()->empty()  ) {
		if(  cnv->get_line().is_bound()  ) {
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( clear_schedule_text, SYSCOL_TEXT ) ;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_line_text, SYSCOL_TEXT ) ;
		}
		else {
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( unique_schedule_text, SYSCOL_TEXT ) ;
			line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( promote_to_line_text, SYSCOL_TEXT ) ;
		}
	}
	else {
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( no_schedule_text, SYSCOL_TEXT ) ;
		line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( new_line_text, SYSCOL_TEXT ) ;
	}
	if(  last_selected_line.is_bound()  ) {
		line_selector.new_component<line_scrollitem_t>( last_selected_line ) ;
	}
	if(  !selected_line.is_bound()  ) {
		// select "create new schedule"
		line_selector.set_selection( 0 );
	}
	line_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>( line_seperator, SYSCOL_TEXT ) ;

	// check all matching lines
	if(  cnv.is_bound()  ) {
		selected_line = cnv->get_line();
	}
	vector_tpl<linehandle_t> lines;
	get_line_list(depot, &lines);
	line_selector.set_selection( 0 );
	FOR(  vector_tpl<linehandle_t>,  const line,  lines  ) {
		line_selector.new_component<line_scrollitem_t>(line) ;
		if(  selected_line.is_bound()  &&  selected_line == line  ) {
			line_selector.set_selection( line_selector.count_elements() - 1 );
		}
	}
	if(  line_selector.get_selection() == 0  ) {
		// no line selected
		selected_line = linehandle_t();
	}
	line_selector.sort( last_selected_line.is_bound()+3 );

	// Update vehicle filter
	vehicle_filter.clear_elements();
	vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
	vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("Relevant"), SYSCOL_TEXT);

	FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
		vehicle_filter.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(i->get_name()), SYSCOL_TEXT);
	}

	if(  depot->selected_filter > vehicle_filter.count_elements()  ) {
		depot->selected_filter = VEHICLE_FILTER_RELEVANT;
	}
	vehicle_filter.set_selection(depot->selected_filter);

	sort_by.clear_elements();
	for(int i = 0; i < vehicle_builder_t::sb_length; i++) {
		sort_by.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(vehicle_builder_t::vehicle_sort_by[i]), SYSCOL_TEXT);
	}
	if(  depot->selected_sort_by > sort_by.count_elements()  ) {
		depot->selected_sort_by = vehicle_builder_t::sb_name;
	}
	sort_by.set_selection(depot->selected_sort_by);

	// finally: update text
	uint32 total_pax = 0;
	uint32 total_mail = 0;
	uint32 total_goods = 0;

	uint64 total_power = 0;
	uint32 total_empty_weight = 0;
	uint32 total_selected_weight = 0;
	uint32 total_max_weight = 0;
	uint32 total_min_weight = 0;
	bool use_sel_weight = true;

	if(  cnv.is_bound()  ) {
		if(  cnv->get_vehicle_count() > 0  ) {
			uint8 selected_good_index = 0;
			if(  depot->selected_filter > VEHICLE_FILTER_RELEVANT  ) {
				// Filter is set to specific good
				const uint32 goods_index = depot->selected_filter - VEHICLE_FILTER_GOODS_OFFSET;
				if(  goods_index < welt->get_goods_list().get_count()  ) {
					selected_good_index = welt->get_goods_list()[goods_index]->get_index();
				}
			}

			for(  unsigned i = 0;  i < cnv->get_vehicle_count();  i++  ) {
				const vehicle_desc_t *desc = cnv->get_vehikel(i)->get_desc();

				total_power += desc->get_power()*desc->get_gear();

				uint32 sel_weight = 0; // actual weight using vehicle filter selected good to fill
				uint32 max_weight = 0;
				uint32 min_weight = 100000;
				bool sel_found = false;
				for(  uint32 j=0;  j<goods_manager_t::get_count();  j++  ) {
					const goods_desc_t *ware = goods_manager_t::get_info(j);

					if(  desc->get_freight_type()->get_catg_index() == ware->get_catg_index()  ) {
						max_weight = max(max_weight, (uint32)ware->get_weight_per_unit());
						min_weight = min(min_weight, (uint32)ware->get_weight_per_unit());

						// find number of goods in in this category. TODO: gotta be a better way...
						uint8 catg_count = 0;
						FOR(vector_tpl<goods_desc_t const*>, const i, welt->get_goods_list()) {
							if(  ware->get_catg_index() == i->get_catg_index()  ) {
								catg_count++;
							}
						}

						if(  ware->get_index() == selected_good_index  ||  catg_count < 2  ) {
							sel_found = true;
							sel_weight = ware->get_weight_per_unit();
						}
					}
				}
				if(  !sel_found  ) {
					// vehicle carries more than one good, but not the selected one
					use_sel_weight = false;
				}

				total_empty_weight += desc->get_weight();
				total_selected_weight += desc->get_weight() + sel_weight * desc->get_capacity();
				total_max_weight += desc->get_weight() + max_weight * desc->get_capacity();
				total_min_weight += desc->get_weight() + min_weight * desc->get_capacity();

				const goods_desc_t* const ware = desc->get_freight_type();
				switch(  ware->get_catg_index()  ) {
					case goods_manager_t::INDEX_PAS: {
						total_pax += desc->get_capacity();
						break;
					}
					case goods_manager_t::INDEX_MAIL: {
						total_mail += desc->get_capacity();
						break;
					}
					default: {
						total_goods += desc->get_capacity();
						break;
					}
				}
			}

			sint32 empty_kmh, sel_kmh, max_kmh, min_kmh;
			if(  cnv->front()->get_waytype() == air_wt  ) {
				// flying aircraft have 0 friction --> speed not limited by power, so just use top_speed
				empty_kmh = sel_kmh = max_kmh = min_kmh = speed_to_kmh( cnv->get_min_top_speed() );
			}
			else {
				empty_kmh = speed_to_kmh(convoi_t::calc_max_speed(total_power, total_empty_weight, cnv->get_min_top_speed()));
				sel_kmh =   speed_to_kmh(convoi_t::calc_max_speed(total_power, total_selected_weight, cnv->get_min_top_speed()));
				max_kmh =   speed_to_kmh(convoi_t::calc_max_speed(total_power, total_min_weight,   cnv->get_min_top_speed()));
				min_kmh =   speed_to_kmh(convoi_t::calc_max_speed(total_power, total_max_weight,   cnv->get_min_top_speed()));
			}

			const sint32 convoi_length = (cnv->get_vehicle_count()) * CARUNITS_PER_TILE / 2 - 1;
			convoi_tile_length_sb = convoi_length + (cnv->get_tile_length() * CARUNITS_PER_TILE - cnv->get_length());

			txt_convoi_count.clear();
			if(  cnv->get_vehicle_count()>1  ) {
				txt_convoi_count.printf( translator::translate("%i car(s),"), cnv->get_vehicle_count() );
			}
			txt_convoi_count.append( translator::translate("Station tiles:") );
			txt_convoi_count.append( (double)cnv->get_tile_length(), 0 );

			txt_convoi_speed.clear();
			if(  empty_kmh != (use_sel_weight ? sel_kmh : min_kmh)  ) {
				convoi_length_ok_sb = 0;
				if(  max_kmh != min_kmh  &&  !use_sel_weight  ) {
					txt_convoi_speed.printf("%s %d km/h, %d-%d km/h %s", translator::translate("Max. speed:"), empty_kmh, min_kmh, max_kmh, translator::translate("loaded") );
					if(  max_kmh != empty_kmh  ) {
						convoi_length_slower_sb = 0;
						convoi_length_too_slow_sb = convoi_length;
					}
					else {
						convoi_length_slower_sb = convoi_length;
						convoi_length_too_slow_sb = 0;
					}
				}
				else {
					txt_convoi_speed.printf("%s %d km/h, %d km/h %s", translator::translate("Max. speed:"), empty_kmh, use_sel_weight ? sel_kmh : min_kmh, translator::translate("loaded") );
					convoi_length_slower_sb = 0;
					convoi_length_too_slow_sb = convoi_length;
				}
			}
			else {
					txt_convoi_speed.printf("%s %d km/h", translator::translate("Max. speed:"), empty_kmh );
					convoi_length_ok_sb = convoi_length;
					convoi_length_slower_sb = 0;
					convoi_length_too_slow_sb = 0;
			}

			{
				char buf[128];
				txt_convoi_value.clear();
				money_to_string(  buf, cnv->calc_restwert() / 100.0, false );
				txt_convoi_value.printf("%s %8s", translator::translate("Restwert:"), buf );

				txt_convoi_cost.clear();
				if(  sint64 fix_cost = welt->scale_with_month_length((sint64)cnv->get_fix_cost())  ) {
					money_to_string(  buf, (double)fix_cost / 100.0, false );
					txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km %.2f$/m)\n"), buf, (double)cnv->get_running_cost()/100.0, (double)fix_cost/100.0 );
				}
				else {
					money_to_string(  buf, cnv->get_purchase_cost() / 100.0, false );
					txt_convoi_cost.printf( translator::translate("Cost: %8s (%.2f$/km)\n"), buf, (double)cnv->get_running_cost() / 100.0 );
				}
			}

			txt_convoi_power.clear();
			txt_convoi_power.printf( translator::translate("Power: %4d kW\n"), cnv->get_sum_power() );

			txt_convoi_weight.clear();
			if(  total_empty_weight != (use_sel_weight ? total_selected_weight : total_max_weight)  ) {
				if(  total_min_weight != total_max_weight  &&  !use_sel_weight  ) {
					txt_convoi_weight.printf("%s %.1ft, %.1f-%.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0, total_min_weight / 1000.0, total_max_weight / 1000.0 );
				}
				else {
					txt_convoi_weight.printf("%s %.1ft, %.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0, (use_sel_weight ? total_selected_weight : total_max_weight) / 1000.0 );
				}
			}
			else {
					txt_convoi_weight.printf("%s %.1ft", translator::translate("Weight:"), total_empty_weight / 1000.0 );
			}
		}
		else {
			txt_convoi_count.clear();
			txt_convoi_count.append(translator::translate("keine Fahrzeuge"));
			txt_convoi_value.clear();
			txt_convoi_cost.clear();
			txt_convoi_power.clear();
			txt_convoi_weight.clear();
		}

		sb_convoi_length.set_visible(true);
		cont_convoi_capacity.set_totals( total_pax, total_mail, total_goods );
		cont_convoi_capacity.set_visible(true);
	}
	else {
		txt_convoi_count.clear();
		txt_convoi_speed.clear();
		txt_convoi_value.clear();
		txt_convoi_cost.clear();
		txt_convoi_power.clear();
		txt_convoi_weight.clear();
		sb_convoi_length.set_visible(false);
		cont_convoi_capacity.set_visible(false);
	}
}


sint64 depot_frame_t::calc_restwert(const vehicle_desc_t *veh_type)
{
	sint64 wert = 0;
	FOR(slist_tpl<vehicle_t*>, const v, depot->get_vehicle_list()) {
		if(  v->get_desc() == veh_type  ) {
			wert += v->calc_sale_value();
		}
	}
	return wert;
}


void depot_frame_t::image_from_storage_list(gui_image_list_t::image_data_t *image_data)
{
	if(  image_data->lcolor != color_idx_to_rgb(COL_RED)  &&  image_data->rcolor != color_idx_to_rgb(COL_RED)  ) {
		if(  veh_action == va_sell  ) {
			depot->call_depot_tool('s', convoihandle_t(), image_data->text );
		}
		else {
			convoihandle_t cnv = depot->get_convoi( icnv );
			if(  !cnv.is_bound()  &&   !depot->get_owner()->is_locked()  ) {
				// adding new convoi, block depot actions until command executed
				// otherwise in multiplayer it's likely multiple convois get created
				// rather than one new convoi with multiple vehicles
				depot->set_command_pending();
			}
			depot->call_depot_tool( veh_action == va_insert ? 'i' : 'a', cnv, image_data->text );
		}
	}
}


void depot_frame_t::image_from_convoi_list(uint nr, bool to_end)
{
	const convoihandle_t cnv = depot->get_convoi( icnv );
	if(  cnv.is_bound()  &&  nr < cnv->get_vehicle_count()  ) {
		// we remove all connected vehicles together!
		// find start
		unsigned start_nr = nr;
		while(  start_nr > 0  ) {
			start_nr--;
			const vehicle_desc_t *info = cnv->get_vehikel(start_nr)->get_desc();
			if(  info->get_trailer_count() != 1  ) {
				start_nr++;
				break;
			}
		}

		cbuffer_t start;
		start.printf("%u", start_nr);

		const char tool = to_end ? 'R' : 'r';
		depot->call_depot_tool( tool, cnv, start );
	}
}


bool depot_frame_t::action_triggered( gui_action_creator_t *comp, value_t p)
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  depot->is_command_pending()  ) {
		// block new commands until last command is executed
		return true;
	}

	if(  comp != NULL  ) {	// message from outside!
		if(  comp == &bt_start  ) {
			if(  cnv.is_bound()  ) {
				//first: close schedule (will update schedule on clients)
				destroy_win( (ptrdiff_t)cnv->get_schedule() );
				// only then call the tool to start
				char tool = event_get_last_control_shift() == 2 ? 'B' : 'b'; // start all with CTRL-click
				depot->call_depot_tool( tool, cnv, NULL);
			}
		}
		else if(  comp == &bt_schedule  ) {
			if(  line_selector.get_selection() == 1  &&  !line_selector.is_dropped()  ) { // create new line
				// promote existing individual schedule to line
				cbuffer_t buf;
				if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
					schedule_t* schedule = cnv->get_schedule();
					if(  schedule  ) {
						schedule->sprintf_schedule( buf );
					}
				}
				depot->call_depot_tool('l', convoihandle_t(), buf);
				return true;
			}
			else {
				open_schedule_editor();
				return true;
			}
		}
		else if(  comp == &line_button  ) {
			if(  cnv.is_bound()  ) {
				cnv->get_owner()->simlinemgmt.show_lineinfo( cnv->get_owner(), cnv->get_line() );
				welt->set_dirty();
			}
		}
		else if(  comp == &bt_sell  ) {
			depot->call_depot_tool('v', cnv, NULL);
		}
		// image list selection here ...
		else if(  comp == &convoi  ) {
			image_from_convoi_list( p.i, last_meta_event_get_class() == EVENT_DOUBLE_CLICK);
		}
		else if(  comp == &pas  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(pas_vec[p.i]);
		}
		else if(  comp == &electrics  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(electrics_vec[p.i]);
		}
		else if(  comp == &loks  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(loks_vec[p.i]);
		}
		else if(  comp == &waggons  &&  last_meta_event_get_class() != EVENT_DOUBLE_CLICK  ) {
			image_from_storage_list(waggons_vec[p.i]);
		}
		// convoi filters
		else if(  comp == &bt_obsolete  ) {
			show_retired_vehicles = (show_retired_vehicles == 0);
			bt_obsolete.pressed = show_retired_vehicles;
			depot_t::update_all_win();
		}
		else if(  comp == &bt_show_all  ) {
			show_all = (show_all == 0);
			bt_show_all.pressed = show_all;
			depot_t::update_all_win();
		}
		else if(  comp == &name_filter_input  ) {
			depot_t::update_all_win();
		}
		else if(  comp == &bt_veh_action  ) {
			if(  veh_action == va_sell  ) {
				veh_action = va_append;
			}
			else {
				veh_action++;
			}
		}
		else if(  comp == &sort_by  ) {
			depot->selected_sort_by = sort_by.get_selection();
		}
		else if(  comp == &bt_copy_convoi  ) {
			if(  cnv.is_bound()  ) {
				if(  !welt->use_timeline()  ||  welt->get_settings().get_allow_buying_obsolete_vehicles()  ||  depot->check_obsolete_inventory( cnv )  ) {
					depot->call_depot_tool('c', cnv, NULL);
				}
				else {
					create_win( new news_img("Can't buy obsolete vehicles!"), w_time_delete, magic_none );
				}
			}
			return true;
		}
		else if(  comp == &convoy_selector  ) {
			icnv = p.i - 1;
/*			if(  !depot->get_convoi(icnv).is_bound()  ) {
				set_focus( NULL );
			}
			else {
				set_focus( (gui_component_t *)&convoy_selector );
			}*/
		}
		else if(  comp == &line_selector  ) {
			const int selection = p.i;
			if(  selection == 0  ) { // unique
				if(  selected_line.is_bound()  ) {
					selected_line = linehandle_t();
					apply_line();
				}
				return true;
			}
			else if(  selection == 1  ) { // create new line
				if(  line_selector.is_dropped()  ) { // but not from next/prev buttons
					// promote existing individual schedule to line
					cbuffer_t buf;
					if(  cnv.is_bound()  &&  !selected_line.is_bound()  ) {
						schedule_t* schedule = cnv->get_schedule();
						if(  schedule  ) {
							schedule->sprintf_schedule( buf );
						}
					}
					last_selected_line = linehandle_t();	// clear last selected line so we can get a new one ...
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
			}

			// access the selected element to get selected line
			line_scrollitem_t *item = dynamic_cast<line_scrollitem_t*>(line_selector.get_element(selection));
			if(  item  ) {
				selected_line = item->get_line();
				depot->set_last_selected_line( selected_line );
				last_selected_line = selected_line;
				apply_line();
				return true;
			}
		}
		else if(  comp == &vehicle_filter  ) {
			depot->selected_filter = vehicle_filter.get_selection();
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


bool depot_frame_t::infowin_event(const event_t *ev)
{
	// enable disable button actions
	if(  ev->ev_class < INFOWIN  &&  (depot == NULL  ||  welt->get_active_player() != depot->get_owner()) ) {
		return false;
	}

	const bool swallowed = gui_frame_t::infowin_event(ev);

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {

		bool dir = (ev->ev_code==NEXT_WINDOW);
		depot_t *next_dep = depot_t::find_depot( depot->get_pos(), depot->get_typ(), depot->get_owner(), dir == NEXT_WINDOW );
		if(next_dep == NULL) {
			if(dir == NEXT_WINDOW) {
				// check the next from start of map
				next_dep = depot_t::find_depot( koord3d(-1,-1,0), depot->get_typ(), depot->get_owner(), true );
			}
			else {
				// respective end of map
				next_dep = depot_t::find_depot( koord3d(8192,8192,127), depot->get_typ(), depot->get_owner(), false );
			}
		}

		if(next_dep  &&  next_dep!=this->depot) {
			//  Replace our depot_frame_t with a new at the same position.
			scr_coord const pos = win_get_pos(this);
			destroy_win( this );

			next_dep->show_info();
			win_set_pos(win_get_magic((ptrdiff_t)next_dep), pos.x, pos.y);
			welt->get_viewport()->change_world_position(next_dep->get_pos());
		}
		else {
			// recenter on current depot
			welt->get_viewport()->change_world_position(depot->get_pos());
		}

		return true;
	}

	if(  swallowed  &&  get_focus()==&name_filter_input  &&  (ev->ev_class == EVENT_KEYBOARD  ||  ev->ev_class == EVENT_STRING)  ) {
		depot_t::update_all_win();
	}

	return swallowed;
}


void depot_frame_t::draw(scr_coord pos, scr_size size)
{
	const bool action_allowed = welt->get_active_player() == depot->get_owner();

	bt_new_line.enable( action_allowed );
	bt_change_line.enable( action_allowed );
	bt_copy_convoi.enable( action_allowed );
	bt_apply_line.enable( action_allowed );
	bt_start.enable( action_allowed );
	bt_schedule.enable( action_allowed );
	bt_destroy.enable( action_allowed );
	bt_sell.enable( action_allowed );
	bt_obsolete.enable( action_allowed );
	bt_show_all.enable( action_allowed );
	bt_veh_action.enable( action_allowed );
	line_button.enable( action_allowed );

	convoihandle_t cnv = depot->get_convoi(icnv);
	// check for data inconsistencies (can happen with withdraw-all and vehicle in depot)
	if(  !cnv.is_bound()  &&  !convoi_pics.empty()  ) {
		icnv=0;
		update_data();
		cnv = depot->get_convoi(icnv);
	}

	gui_frame_t::draw(pos, size);
	draw_vehicle_info_text(pos);
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
			dummy->entries.clear();

			cbuffer_t buf;
			dummy->sprintf_schedule(buf);
			cnv->call_convoi_tool('g', (const char*)buf );

			delete dummy;
		}
	}
}


void depot_frame_t::open_schedule_editor()
{
	convoihandle_t cnv = depot->get_convoi( icnv );

	if(  cnv.is_bound()  &&  cnv->get_vehicle_count() > 0  ) {
		if(  selected_line.is_bound()  &&  event_get_last_control_shift() == 2  ) { // update line with CTRL-click
			create_win( new line_management_gui_t( selected_line, depot->get_owner() ), w_info, (ptrdiff_t)selected_line.get_rep() );
		}
		else { // edit individual schedule
			// this can happen locally, since any update of the schedule is done during closing window
			schedule_t *schedule = cnv->create_schedule();
			assert(schedule!=NULL);
			gui_frame_t *schedulewin = win_get_magic( (ptrdiff_t)schedule );
			if(  schedulewin == NULL  ) {
				cnv->open_schedule_window( welt->get_active_player() == cnv->get_owner() );
			}
			else {
				top_win( schedulewin );
			}
		}
	}
	else {
		create_win( new news_img("Please choose vehicles first\n"), w_time_delete, magic_none );
	}
}


void depot_frame_t::draw_vehicle_info_text(scr_coord pos)
{
	cbuffer_t buf;
	const scr_size size = get_windowsize();
	PUSH_CLIP(pos.x, pos.y, size.w-1, size.h-1);

	gui_component_t const* const tab = tabs.get_aktives_tab();
	gui_image_list_t const* const lst =
		tab == &scrolly_pas       ? &pas       :
		tab == &scrolly_electrics ? &electrics :
		tab == &scrolly_loks      ? &loks      :
		&waggons;
	int x = get_mouse_x();
	int y = get_mouse_y();
	double resale_value = -1.0;
	const vehicle_desc_t *veh_type = NULL;
	bool new_vehicle_length_sb_force_zero = false;
	sint16 convoi_number = -1;
	scr_coord relpos = scr_coord(0, ((gui_scrollpane_t *)tabs.get_aktives_tab())->get_scroll_y());
	int sel_index = lst->index_at( pos + tabs.get_pos() - relpos, x, y - D_TITLEBAR_HEIGHT - tabs.get_required_size().h);

	if(  (sel_index != -1)  &&  (tabs.getroffen(x - pos.x, y - pos.y - D_TITLEBAR_HEIGHT))  ) {
		// cursor over a vehicle in the selection list
		const vector_tpl<gui_image_list_t::image_data_t*>& vec = (lst == &electrics ? electrics_vec : (lst == &pas ? pas_vec : (lst == &loks ? loks_vec : waggons_vec)));
		veh_type = vehicle_builder_t::get_info( vec[sel_index]->text );
		if(  vec[sel_index]->lcolor == color_idx_to_rgb(COL_RED)  ||  veh_action == va_sell  ) {
			// don't show new_vehicle_length_sb when can't actually add the highlighted vehicle, or selling from inventory
			new_vehicle_length_sb_force_zero = true;
		}
		if(  vec[sel_index]->count > 0  ) {
			resale_value = (double)calc_restwert( veh_type );
		}
	}
	else {
		// cursor over a vehicle in the convoi
		relpos = scr_coord(scrolly_convoi.get_scroll_x(), 0);

		convoi_number = sel_index = convoi.index_at(pos - relpos + scrolly_convoi.get_pos(), x, y - D_TITLEBAR_HEIGHT);
		if(  sel_index != -1  ) {
			convoihandle_t cnv = depot->get_convoi( icnv );
			veh_type = cnv->get_vehikel( sel_index )->get_desc();
			resale_value = cnv->get_vehikel( sel_index )->calc_sale_value();
			new_vehicle_length_sb_force_zero = true;
		}
	}

	{
		const char *c;
		switch(  const uint32 count = depot->get_vehicle_list().get_count()  ) {
			case 0: {
				c = translator::translate("Keine Einzelfahrzeuge im Depot");
				break;
			}
			case 1: {
				c = translator::translate("1 Einzelfahrzeug im Depot");
				break;
			}
			default: {
				buf.printf( translator::translate("%d Einzelfahrzeuge im Depot"), count );
				c = buf;
				break;
			}
		}
		display_proportional_rgb( pos.x + D_MARGIN_LEFT, pos.y + D_TITLEBAR_HEIGHT + div_tabbottom.get_pos().y + div_tabbottom.get_size().h + 1, c, ALIGN_LEFT, SYSCOL_TEXT, true );
	}

	if(  veh_type  ) {

		// column 1
		buf.clear();
		buf.printf( "%s", translator::translate( veh_type->get_name(), welt->get_settings().get_name_language_id() ) );

		if(  veh_type->get_power() > 0  ) { // LOCO
			buf.printf( " (%s)\n", translator::translate( vehicle_builder_t::engine_type_names[veh_type->get_engine_type()+1] ) );
		}
		else {
			buf.append( "\n" );
		}

		if(  sint64 fix_cost = welt->scale_with_month_length(veh_type->get_maintenance())  ) {
			char tmp[128];
			money_to_string( tmp, veh_type->get_price() / 100.0, false );
			buf.printf( translator::translate("Cost: %8s (%.2f$/km %.2f$/m)\n"), tmp, veh_type->get_running_cost()/100.0, fix_cost/100.0 );
		}
		else {
			char tmp[128];
			money_to_string(  tmp, veh_type->get_price() / 100.0, false );
			buf.printf( translator::translate("Cost: %8s (%.2f$/km)\n"), tmp, veh_type->get_running_cost()/100.0 );
		}

		if(  veh_type->get_capacity() > 0  ) { // must translate as "Capacity: %3d%s %s\n"
			buf.printf( translator::translate("Capacity: %d%s %s\n"),
				veh_type->get_capacity(),
				translator::translate( veh_type->get_freight_type()->get_mass() ),
				veh_type->get_freight_type()->get_catg()==0 ? translator::translate( veh_type->get_freight_type()->get_name() ) : translator::translate( veh_type->get_freight_type()->get_catg_name() )
				);
		}
		else {
			buf.append( "\n" );
		}

		buf.printf( "%s %3d km/h\n", translator::translate("Max. speed:"), veh_type->get_topspeed() );

		if(  veh_type->get_power() > 0  ) {
			if(  veh_type->get_gear() != 64  ){
				buf.printf( "%s %4d kW (x%0.2f)\n", translator::translate("Power:"), veh_type->get_power(), veh_type->get_gear() / 64.0 );
			}
			else {
				buf.printf( translator::translate("Power: %4d kW\n"), veh_type->get_power() );
			}
		}

		int yyy = pos.y + D_TITLEBAR_HEIGHT + div_action_bottom.get_pos().y + div_action_bottom.get_size().h + 2;
		display_multiline_text_rgb( pos.x + D_MARGIN_LEFT, yyy, buf, SYSCOL_TEXT);

		// column 2
		buf.clear();
		buf.printf( "%s %4.1ft\n", translator::translate("Weight:"), veh_type->get_weight() / 1000.0 );
		buf.printf( "%s %s - ", translator::translate("Available:"), translator::get_short_date(veh_type->get_intro_year_month() / 12, veh_type->get_intro_year_month() % 12) );

		if(  veh_type->get_retire_year_month() != DEFAULT_RETIRE_DATE * 12  ) {
			buf.printf( "%s\n", translator::get_short_date(veh_type->get_retire_year_month() / 12, veh_type->get_retire_year_month() % 12) );
		}
		else {
			buf.append( "*\n" );
		}

		if(  char const* const copyright = veh_type->get_copyright()  ) {
			buf.printf( translator::translate("Constructed by %s"), copyright );
		}
		buf.append( "\n" );

		if(  resale_value != -1.0  ) {
			char tmp[128];
			money_to_string(  tmp, resale_value / 100.0, false );
			buf.printf( "%s %8s", translator::translate("Restwert:"), tmp );
		}

		display_multiline_text_rgb( pos.x + second_column_x, yyy + LINESPACE, buf, SYSCOL_TEXT);

		// update speedbar
		new_vehicle_length_sb = new_vehicle_length_sb_force_zero ? 0 : convoi_length_ok_sb + convoi_length_slower_sb + convoi_length_too_slow_sb + veh_type->get_length();

		txt_convoi_number.clear();
		if (convoi_number>-1){
			txt_convoi_number.printf("%d", convoi_number + 1);
			lb_convoi_number.set_pos(scr_coord(((depot->get_x_grid() * get_base_tile_raster_width() / 64 + 4) - (depot->get_x_grid() * get_base_tile_raster_width() / 64 / 2))*convoi_number + 4, 4));
		}
	}
	else {
		txt_convoi_number.clear();
		new_vehicle_length_sb = 0;
	}

	POP_CLIP();
}


void depot_frame_t::update_tabs()
{
	gui_component_t *old_tab = tabs.get_aktives_tab();
	tabs.clear();

	bool one = false;

	// add only if there are any
	if(  !pas_vec.empty()  ) {
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
		one = true;
	}

	// add only if there are any trolleybuses
	if(  !electrics_vec.empty()  ) {
		tabs.add_tab(&scrolly_electrics, translator::translate( depot->get_electrics_name() ) );
		one = true;
	}

	// add, if wagons are there ...
	if(  !loks_vec.empty()  ||  !waggons_vec.empty()  ) {
		tabs.add_tab(&scrolly_loks, translator::translate( depot->get_zieher_name() ) );
		one = true;
	}

	// only add, if there are wagons
	if(  !waggons_vec.empty()  ) {
		tabs.add_tab(&scrolly_waggons, translator::translate( depot->get_haenger_name() ) );
		one = true;
	}

	if(  !one  ) {
		// add passenger as default
		tabs.add_tab(&scrolly_pas, translator::translate( depot->get_passenger_name() ) );
	}

	// Look, if there is our old tab present again (otherwise it will be 0 by tabs.clear()).
	for(  uint8 i = 0;  i < tabs.get_count();  i++  ) {
		if(  old_tab == tabs.get_tab(i)  ) {
			// Found it!
			tabs.set_active_tab_index(i);
			break;
		}
	}
}


depot_convoi_capacity_t::depot_convoi_capacity_t()
{
	total_pax = 0;
	total_mail = 0;
	total_goods = 0;
}


void depot_convoi_capacity_t::set_totals(uint32 pax, uint32 mail, uint32 goods)
{
	total_pax = pax;
	total_mail = mail;
	total_goods = goods;
}


void depot_convoi_capacity_t::draw(scr_coord off)
{
	cbuffer_t cbuf;

	scr_coord_val w = 0;
	cbuf.clear();
	cbuf.printf("%s %d", translator::translate("Capacity:"), total_pax );
	w += display_proportional_clip_rgb( pos.x+off.x + w, pos.y+off.y , cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
	display_color_img( skinverwaltung_t::passengers->get_image_id(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);

	w += 16;
	cbuf.clear();
	cbuf.printf("%d", total_mail );
	w += display_proportional_clip_rgb( pos.x+off.x + w, pos.y+off.y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
	display_color_img( skinverwaltung_t::mail->get_image_id(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);

	w += 16;
	cbuf.clear();
	cbuf.printf("%d", total_goods );
	w += display_proportional_clip_rgb( pos.x+off.x + w, pos.y+off.y, cbuf, ALIGN_LEFT, SYSCOL_TEXT, true);
	display_color_img( skinverwaltung_t::goods->get_image_id(0), pos.x + off.x + w, pos.y + off.y, 0, false, false);
}


uint32 depot_frame_t::get_rdwr_id()
{
	return magic_depot;
}

void  depot_frame_t::rdwr( loadsave_t *file)
{
	if (file->is_version_less(120, 9)) {
		destroy_win(this);
		return;
	}

	// depot position
	koord3d pos;
	if(  file->is_saving()  ) {
		pos = depot->get_pos();
	}
	pos.rdwr( file );
	// window size
	scr_size size = get_windowsize();
	size.rdwr( file );

	if(  file->is_loading()  ) {
		depot_t *dep = welt->lookup(pos)->get_depot();
		if (dep) {
			init(dep);
		}
	}
	tabs.rdwr(file);
	vehicle_filter.rdwr(file);
	file->rdwr_byte(veh_action);
	file->rdwr_long(icnv);
	sort_by.rdwr(file);
	simline_t::rdwr_linehandle_t(file, selected_line);

	if(  depot  &&  file->is_loading()  ) {
		update_data();
		update_tabs();
		reset_min_windowsize();
		set_windowsize(size);

		win_set_magic(this, (ptrdiff_t)depot);
	}

	if (depot == NULL) {
		destroy_win(this);
	}
}
