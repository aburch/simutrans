/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>
#include <algorithm>

#include "messagebox.h"
#include "schedule_list.h"
#include "times_history.h"
#include "line_management_gui.h"
#include "components/gui_convoiinfo.h"
#include "line_item.h"
#include "simwin.h"

#include "../simcolor.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../simskin.h"
#include "../simconvoi.h"
#include "../vehicle/simvehicle.h"
#include "../simlinemgmt.h"
#include "../simmenu.h"
#include "../utils/simstring.h"
#include "../player/simplay.h"
#include "../gui/line_class_manager.h"

#include "../bauer/vehikelbauer.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/livery_scheme.h"
#include "../dataobj/environment.h"

#include "../boden/wege/kanal.h"
#include "../boden/wege/maglev.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/narrowgauge.h"
#include "../boden/wege/runway.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"


#include "karte.h"

uint16 schedule_list_gui_t::livery_scheme_index = 0;

static const char *cost_type[MAX_LINE_COST] =
{
	"Free Capacity",
	"Transported",
	"Distance",
	"Average speed",
//	"Maxspeed",
	"Comfort",
	"Revenue",
	"Operation",
	"Refunds",
	"Road toll",
	"Profit",
	"Convoys",
	"Departures",
	"Scheduled"
};

const int cost_type_color[MAX_LINE_COST] =
{
	COL_FREE_CAPACITY,
	COL_TRANSPORTED,
	COL_DISTANCE,
	COL_AVERAGE_SPEED,
	COL_COMFORT,
	COL_REVENUE,
	COL_OPERATION,
	COL_CAR_OWNERSHIP,
	COL_TOLL,
	COL_PROFIT,
	COL_VEHICLE_ASSETS,
	//COL_COUNVOI_COUNT,
	COL_MAXSPEED,
	COL_LILAC
};
static uint32 bFilterStates = 0;

static uint8 tabs_to_lineindex[8];
static uint8 max_idx=0;

static uint8 statistic[MAX_LINE_COST]={
	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_DISTANCE, LINE_AVERAGE_SPEED, LINE_COMFORT, LINE_REVENUE, LINE_OPERATIONS, LINE_REFUNDS, LINE_WAYTOLL, LINE_PROFIT, LINE_CONVOIS, LINE_DEPARTURES, LINE_DEPARTURES_SCHEDULED
//std	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_CONVOIS, LINE_DISTANCE, LINE_MAXSPEED, LINE_WAYTOLL
};

static uint8 statistic_type[MAX_LINE_COST]={
	STANDARD, STANDARD, STANDARD, STANDARD, STANDARD, MONEY, MONEY, MONEY, MONEY, MONEY, STANDARD, STANDARD, STANDARD
//std	STANDARD, STANDARD, MONEY, MONEY, MONEY, STANDARD, STANDARD, STANDARD, MONEY
};

static const char * line_alert_helptexts[5] =
{
  "line_nothing_moved",
  "line_missing_scheduled_slots",
  "line_has_obsolete_vehicles",
  "line_overcrowded",
  "line_has_upgradeable_vehicles"
};


enum sort_modes_t { SORT_BY_NAME=0, SORT_BY_ID, SORT_BY_PROFIT, SORT_BY_TRANSPORTED, SORT_BY_CONVOIS, SORT_BY_DISTANCE, MAX_SORT_MODES };

static uint8 current_sort_mode = 0;

#define LINE_NAME_COLUMN_WIDTH ((D_BUTTON_WIDTH*3)+11+4)
#define SCL_HEIGHT (15*LINESPACE)

/// selected convoy tab
static uint8 selected_convoy_tab = 0;

/// selected line tab per player
static uint8 selected_tab[MAX_PLAYER_COUNT] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

/// selected line per tab (static)
linehandle_t schedule_list_gui_t::selected_line[MAX_PLAYER_COUNT][simline_t::MAX_LINE_TYPE];

// selected convoy list display mode
static uint8 selected_cnvlist_mode[MAX_PLAYER_COUNT] = {0};

// sort stuff
schedule_list_gui_t::sort_mode_t schedule_list_gui_t::sortby = by_name;
static uint8 default_sortmode = 0;
bool schedule_list_gui_t::sortreverse = false;

const char *schedule_list_gui_t::sort_text[SORT_MODES] = {
	"cl_btn_sort_name",
	"cl_btn_sort_schedule",
	"cl_btn_sort_income",
	"cl_btn_sort_loading_lvl",
	"cl_btn_sort_max_speed",
	/*"cl_btn_sort_power",*/
	"cl_btn_sort_value",
	"cl_btn_sort_age"
};

schedule_list_gui_t::schedule_list_gui_t(player_t *player_) :
	gui_frame_t( translator::translate("Line Management"), player_),
	player(player_),
	scrolly_convois(&cont),
	scrolly_haltestellen(&cont_haltestellen),
	scl(gui_scrolled_list_t::listskin),
	lbl_filter("Line Filter"),
	convoy_infos(),
	stop_infos()
{
	capacity = load = 0;
	selection = -1;
	loadfactor = 0;
	schedule_filter[0] = 0;
	old_schedule_filter[0] = 0;
	last_schedule = NULL;

	// init scrolled list
	scl.set_pos(scr_coord(0,1));
	scl.set_size(scr_size(LINE_NAME_COLUMN_WIDTH-11-4, SCL_HEIGHT-18));
	scl.set_highlight_color(player->get_player_color1()+1);
	scl.add_listener(this);

	// tab panel
	tabs.set_pos(scr_coord(11,5));
	tabs.set_size(scr_size(LINE_NAME_COLUMN_WIDTH-11-4, SCL_HEIGHT));
	tabs.add_tab(&scl, translator::translate("All"));
	max_idx = 0;
	tabs_to_lineindex[max_idx++] = simline_t::line;

	// now add all specific tabs
	if(maglev_t::default_maglev) {
		tabs.add_tab(&scl, translator::translate("Maglev"), skinverwaltung_t::maglevhaltsymbol, translator::translate("Maglev"));
		tabs_to_lineindex[max_idx++] = simline_t::maglevline;
	}
	if(monorail_t::default_monorail) {
		tabs.add_tab(&scl, translator::translate("Monorail"), skinverwaltung_t::monorailhaltsymbol, translator::translate("Monorail"));
		tabs_to_lineindex[max_idx++] = simline_t::monorailline;
	}
	if(schiene_t::default_schiene) {
		tabs.add_tab(&scl, translator::translate("Train"), skinverwaltung_t::zughaltsymbol, translator::translate("Train"));
		tabs_to_lineindex[max_idx++] = simline_t::trainline;
	}
	if(narrowgauge_t::default_narrowgauge) {
		tabs.add_tab(&scl, translator::translate("Narrowgauge"), skinverwaltung_t::narrowgaugehaltsymbol, translator::translate("Narrowgauge"));
		tabs_to_lineindex[max_idx++] = simline_t::narrowgaugeline;
	}
	if (!vehicle_builder_t::get_info(tram_wt).empty()) {
		tabs.add_tab(&scl, translator::translate("Tram"), skinverwaltung_t::tramhaltsymbol, translator::translate("Tram"));
		tabs_to_lineindex[max_idx++] = simline_t::tramline;
	}
	if(strasse_t::default_strasse) {
		tabs.add_tab(&scl, translator::translate("Truck"), skinverwaltung_t::autohaltsymbol, translator::translate("Truck"));
		tabs_to_lineindex[max_idx++] = simline_t::truckline;
	}
	if (!vehicle_builder_t::get_info(water_wt).empty()) {
		tabs.add_tab(&scl, translator::translate("Ship"), skinverwaltung_t::schiffshaltsymbol, translator::translate("Ship"));
		tabs_to_lineindex[max_idx++] = simline_t::shipline;
	}
	if(runway_t::default_runway) {
		tabs.add_tab(&scl, translator::translate("Air"), skinverwaltung_t::airhaltsymbol, translator::translate("Air"));
		tabs_to_lineindex[max_idx++] = simline_t::airline;
	}
	tabs.add_listener(this);
	add_component(&tabs);

	// editable line name
	inp_name.add_listener(this);
	inp_name.set_pos(scr_coord(LINE_NAME_COLUMN_WIDTH+23, D_MARGIN_TOP));
	inp_name.set_visible(false);
	add_component(&inp_name);

	// load display
	filled_bar.add_color_value(&loadfactor, COL_GREEN);
	filled_bar.set_visible(false);
	add_component(&filled_bar);

	// sort button on convoy list, define this first to prevent overlapping
	sortedby.set_pos(scr_coord(BUTTON1_X, 2));
	sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_BUTTON_HEIGHT));
	sortedby.set_max_size(scr_size(D_BUTTON_WIDTH * 1.5, LINESPACE * 8));
	for (int i = 0; i < SORT_MODES; i++) {
		sortedby.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(sort_text[i]), SYSCOL_TEXT));
	}
	sortedby.set_selection(default_sortmode);
	sortedby.add_listener(this);
	cont_convoys.add_component(&sortedby);

	// convoi list
	cont.set_size(scr_size(200, 80));
	cont.set_pos(scr_coord(D_H_SPACE, D_V_SPACE));
	scrolly_convois.set_pos(scr_coord(0, D_BUTTON_HEIGHT+D_H_SPACE));
	scrolly_convois.set_show_scroll_y(true);
	scrolly_convois.set_scroll_amount_y(40);
	scrolly_convois.set_visible(false);
	cont_convoys.add_component(&scrolly_convois);

	// halt list
	cont_haltestellen.set_size(scr_size(LINE_NAME_COLUMN_WIDTH, 28));
	scrolly_haltestellen.set_pos(scr_coord(0, 8 + SCL_HEIGHT + D_BUTTON_HEIGHT + D_BUTTON_HEIGHT + 2));
	scrolly_haltestellen.set_show_scroll_x(true);
	scrolly_haltestellen.set_scroll_amount_y(28);
	scrolly_haltestellen.set_visible(false);
	add_component(&scrolly_haltestellen);

	// filter liens by
	lbl_filter.set_pos( scr_coord( 11, 7+SCL_HEIGHT+2 ) );
	add_component(&lbl_filter);

	inp_filter.set_pos( scr_coord( 11+D_BUTTON_WIDTH, 7+SCL_HEIGHT ) );
	inp_filter.set_size( scr_size( D_BUTTON_WIDTH*2- D_BUTTON_HEIGHT *3, D_BUTTON_HEIGHT ) );
	inp_filter.set_text( schedule_filter, lengthof(schedule_filter) );
//	inp_filter.set_tooltip("Only show lines containing");
	inp_filter.add_listener(this);
	add_component(&inp_filter);

	filter_btn_all_pas.init(button_t::roundbox_state, NULL, scr_coord(inp_filter.get_pos() + scr_coord(inp_filter.get_size().w, 0)), scr_size(D_BUTTON_HEIGHT, D_BUTTON_HEIGHT));
	filter_btn_all_pas.set_image(skinverwaltung_t::passengers->get_image_id(0));
	filter_btn_all_pas.set_tooltip("filter_pas_line");
	filter_btn_all_pas.pressed = line_type_flags & (1 << simline_t::all_pas);
	add_component(&filter_btn_all_pas);
	filter_btn_all_pas.add_listener(this);

	filter_btn_all_mails.init(button_t::roundbox_state, NULL, scr_coord(filter_btn_all_pas.get_pos() + scr_coord(D_BUTTON_HEIGHT, 0)), scr_size(D_BUTTON_HEIGHT, D_BUTTON_HEIGHT));
	filter_btn_all_mails.set_image(skinverwaltung_t::mail->get_image_id(0));
	filter_btn_all_mails.set_tooltip("filter_mail_line");
	filter_btn_all_mails.pressed = line_type_flags & (1 << simline_t::all_mail);
	filter_btn_all_mails.add_listener(this);
	add_component(&filter_btn_all_mails);

	filter_btn_all_freights.init(button_t::roundbox_state, NULL, scr_coord(filter_btn_all_mails.get_pos() + scr_coord(D_BUTTON_HEIGHT, 0)), scr_size(D_BUTTON_HEIGHT, D_BUTTON_HEIGHT));
	filter_btn_all_freights.set_image(skinverwaltung_t::goods->get_image_id(0));
	filter_btn_all_freights.set_tooltip("filter_freight_line");
	filter_btn_all_freights.pressed = line_type_flags & (1 << simline_t::all_freight);
	filter_btn_all_freights.add_listener(this);
	add_component(&filter_btn_all_freights);


	// normal buttons edit new remove
	bt_new_line.init(button_t::roundbox, "New Line", scr_coord(11, 8+SCL_HEIGHT+D_BUTTON_HEIGHT ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_new_line.add_listener(this);
	add_component(&bt_new_line);

	bt_change_line.init(button_t::roundbox, "Update Line", scr_coord(11+D_BUTTON_WIDTH, 8+SCL_HEIGHT+D_BUTTON_HEIGHT ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_change_line.set_tooltip("Modify the selected line");
	bt_change_line.add_listener(this);
	bt_change_line.disable();
	add_component(&bt_change_line);

	bt_delete_line.init(button_t::roundbox, "Delete Line", scr_coord(11+2*D_BUTTON_WIDTH, 8+SCL_HEIGHT+D_BUTTON_HEIGHT ), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_delete_line.set_tooltip("Delete the selected line (if without associated convois).");
	bt_delete_line.add_listener(this);
	bt_delete_line.disable();
	add_component(&bt_delete_line);


	int offset_y = D_MARGIN_TOP + D_BUTTON_HEIGHT*2 + LINESPACE*2;
	bt_line_class_manager.init(button_t::roundbox_state, "line_class_manager", scr_coord(LINE_NAME_COLUMN_WIDTH, offset_y), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	bt_line_class_manager.set_tooltip("change_the_classes_for_the_entire_line");
	bt_line_class_manager.set_visible(false);
	bt_line_class_manager.add_listener(this);
	add_component(&bt_line_class_manager);

	bt_times_history.init(button_t::roundbox, "times_history", scr_coord(LINE_NAME_COLUMN_WIDTH + D_BUTTON_WIDTH, offset_y), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	bt_times_history.set_tooltip("view_journey_times_history_of_this_line");
	bt_times_history.set_visible(true);
	bt_times_history.add_listener(this);
	add_component(&bt_times_history);

	bt_withdraw_line.init(button_t::roundbox_state, "Withdraw All", scr_coord(LINE_NAME_COLUMN_WIDTH + D_BUTTON_WIDTH * 2, offset_y), scr_size(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	bt_withdraw_line.set_tooltip("Convoi is sold when all wagons are empty.");
	bt_withdraw_line.set_visible(false);
	bt_withdraw_line.add_listener(this);
	add_component(&bt_withdraw_line);

	offset_y += D_BUTTON_HEIGHT;
	// Select livery
	livery_selector.set_pos(scr_coord(LINE_NAME_COLUMN_WIDTH, offset_y));
	livery_selector.set_focusable(false);
	livery_selector.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_BUTTON_HEIGHT));
	livery_selector.set_max_size(scr_size(D_BUTTON_WIDTH - 8, LINESPACE * 8 + 2 + 16));
	livery_selector.set_highlight_color(1);
	livery_selector.clear_elements();
	livery_selector.add_listener(this);
	add_component(&livery_selector);

	// sort button
	sort_asc.init(button_t::arrowup_state, "", scr_coord(BUTTON1_X + D_BUTTON_WIDTH * 1.5 + D_H_SPACE, 3), scr_size(D_ARROW_UP_WIDTH, D_ARROW_UP_HEIGHT));
	sort_asc.set_tooltip(translator::translate("cl_btn_sort_asc"));
	sort_asc.add_listener(this);
	sort_asc.pressed = sortreverse;
	cont_convoys.add_component(&sort_asc);

	sort_desc.init(button_t::arrowdown_state, "", sort_asc.get_pos() + scr_coord(D_ARROW_UP_WIDTH+2, 0), scr_size(D_ARROW_DOWN_WIDTH, D_ARROW_DOWN_HEIGHT));
	sort_desc.set_tooltip(translator::translate("cl_btn_sort_desc"));
	sort_desc.add_listener(this);
	sort_desc.pressed = !sortreverse;
	cont_convoys.add_component(&sort_desc);

	bt_mode_convois.init(button_t::roundbox, gui_convoiinfo_t::cnvlist_mode_button_texts[selected_cnvlist_mode[player->get_player_nr()]], scr_coord(BUTTON3_X, 2), scr_size(D_BUTTON_WIDTH+15, D_BUTTON_HEIGHT));
	bt_mode_convois.add_listener(this);
	cont_convoys.add_component(&bt_mode_convois);
	info_tabs.add_tab(&cont_convoys, tab_name);


	offset_y += D_BUTTON_HEIGHT;
	// right tabs
	info_tabs.set_pos(scr_coord(LINE_NAME_COLUMN_WIDTH-4, offset_y));
	info_tabs.add_listener(this);
	info_tabs.set_size(scr_size(get_windowsize().w- LINE_NAME_COLUMN_WIDTH+4+D_MARGIN_RIGHT, get_windowsize().h-offset_y));
	add_component(&info_tabs);

	tab_name.clear();
	tab_name.printf("%s%5s",translator::translate("Convoys"),"(0)");

	//CHART
	chart.set_dimension(12, 1000);
	chart.set_pos( scr_coord(68, LINESPACE) );
	chart.set_seed(0);
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_ltr(env_t::left_to_right_graphs);
	cont_charts.add_component(&chart);

	// add filter buttons
	for (int i=0; i<MAX_LINE_COST; i++) {
		filterButtons[i].init(button_t::box_state,cost_type[i],scr_coord(0,0), scr_size(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
		filterButtons[i].add_listener(this);
		filterButtons[i].background_color = cost_type_color[i];
		cont_charts.add_component(filterButtons + i);
	}
	info_tabs.add_tab(&cont_charts, translator::translate("Chart"));
	info_tabs.set_active_tab_index(selected_convoy_tab);

	// recover last selected line
	int index = 0;
	for(  uint i=0;  i<max_idx;  i++  ) {
		if(  tabs_to_lineindex[i] == selected_tab[player->get_player_nr()]  ) {
			line = selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]];
			index = i;
			break;
		}
	}
	selected_tab[player->get_player_nr()] = tabs_to_lineindex[index]; // reset if previous selected tab is not there anymore
	tabs.set_active_tab_index(index);
	if(index>0) {
		bt_new_line.enable();
	}
	else {
		bt_new_line.disable();
	}
	update_lineinfo( line );

	// resize button
	set_min_windowsize(scr_size(LINE_NAME_COLUMN_WIDTH + D_BUTTON_WIDTH*3 + D_MARGIN_LEFT*2, 305));
	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
	resize(scr_coord(D_BUTTON_WIDTH, LINESPACE*3+D_V_SPACE)); // suitable for 4 buttons horizontally and 5 convoys vertically

	build_line_list(index);
}


schedule_list_gui_t::~schedule_list_gui_t()
{
	delete last_schedule;
	// change line name if necessary
	rename_line();
}


/**
 * Mouse clicks are hereby reported to the GUI-Components
 */
bool schedule_list_gui_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN) {
		if(ev->ev_code == WIN_CLOSE) {
			// hide schedule on minimap (may not current, but for safe)
			reliefkarte_t::get_karte()->set_current_cnv( convoihandle_t() );
		}
		else if(  (ev->ev_code==WIN_OPEN  ||  ev->ev_code==WIN_TOP)  &&  line.is_bound() ) {
			if(  line->count_convoys()>0  ) {
				// set this schedule as current to show on minimap if possible
				reliefkarte_t::get_karte()->set_current_cnv( line->get_convoy(0) );
			}
			else {
				// set this schedule as current to show on minimap if possible
				reliefkarte_t::get_karte()->set_current_cnv( convoihandle_t() );
			}
		}
	}
	return gui_frame_t::infowin_event(ev);
}


bool schedule_list_gui_t::action_triggered( gui_action_creator_t *comp, value_t v )           // 28-Dec-01    Markus Weber    Added
{
	if(comp == &bt_change_line) {
		if(line.is_bound()) {
			create_win( new line_management_gui_t(line, player), w_info, (ptrdiff_t)line.get_rep() );
		}
	}
	else if(comp == &bt_new_line) {
		// create typed line
		assert(  tabs.get_active_tab_index() > 0  &&  tabs.get_active_tab_index()<max_idx  );
		// update line schedule via tool!
		tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
		cbuffer_t buf;
		int type = tabs_to_lineindex[tabs.get_active_tab_index()];
		buf.printf( "c,0,%i,0,0|%i|", type, type );
		tmp_tool->set_default_param(buf);
		welt->set_tool( tmp_tool, player );
		// since init always returns false, it is safe to delete immediately
		delete tmp_tool;
		depot_t::update_all_win();
	}
	else if(comp == &bt_delete_line) {
		if(line.is_bound()) {
			tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "d,%i", line.get_id() );
			tmp_tool->set_default_param(buf);
			welt->set_tool( tmp_tool, player );
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
			depot_t::update_all_win();
		}
	}
	else if(comp == &bt_withdraw_line) {
		bt_withdraw_line.pressed ^= 1;
		if (line.is_bound()) {
			tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "w,%i,%i", line.get_id(), bt_withdraw_line.pressed );
			tmp_tool->set_default_param(buf);
			welt->set_tool( tmp_tool, player );
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
		}
	}
	else if (comp == &bt_line_class_manager)
	{
		create_win(20, 20, new line_class_manager_t(line), w_info, magic_line_class_manager + line.get_id());
		return true;
        }
	else if (comp == &bt_times_history) {
		if(line.is_bound()) {
			create_win( new times_history_t(line, convoihandle_t()), w_info, (ptrdiff_t)line.get_rep() + 1 );
		}
	}
	else if (comp == &sortedby) {
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			set_sortierung((sort_mode_t)tmp);
		}
		else {
			sortedby.set_selection(0);
			set_sortierung(by_name);
		}
		default_sortmode = (uint8)tmp;
		update_lineinfo(line);
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		set_reverse(!get_reverse());
		update_lineinfo(line);
		sort_asc.pressed = sortreverse;
		sort_desc.pressed = !sortreverse;
	}
	else if (comp == &bt_mode_convois) {
		selected_cnvlist_mode[player->get_player_nr()] = (selected_cnvlist_mode[player->get_player_nr()] + 1) % gui_convoiinfo_t::DISPLAY_MODES;
		bt_mode_convois.set_text(gui_convoiinfo_t::cnvlist_mode_button_texts[selected_cnvlist_mode[player->get_player_nr()]]);
		update_lineinfo(line);
	}
	else if(comp == &livery_selector)
	{
			sint32 livery_selection = livery_selector.get_selection();
			if(livery_selection < 0)
			{
				livery_selector.set_selection(0);
				livery_selection = 0;
			}
			livery_scheme_index = livery_scheme_indices.empty()? 0 : livery_scheme_indices[livery_selection];
			if (line.is_bound())
			{
				tool_t *tmp_tool = create_tool( TOOL_CHANGE_LINE | SIMPLE_TOOL );
				cbuffer_t buf;
				buf.printf( "V,%i,%i", line.get_id(), livery_scheme_index );
				tmp_tool->set_default_param(buf);
				welt->set_tool( tmp_tool, player );
				// since init always returns false, it is safe to delete immediately
				delete tmp_tool;
			}
	}
	else if (comp == &tabs) {
		int const tab = tabs.get_active_tab_index();
		uint8 old_selected_tab = selected_tab[player->get_player_nr()];
		selected_tab[player->get_player_nr()] = tabs_to_lineindex[tab];
		if(  old_selected_tab == simline_t::line  &&  selected_line[player->get_player_nr()][0].is_bound()  &&  selected_line[player->get_player_nr()][0]->get_linetype() == selected_tab[player->get_player_nr()]  ) {
			// switching from general to same waytype tab while line is seletced => use current line instead
			selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]] = selected_line[player->get_player_nr()][0];
		}
		update_lineinfo( selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]] );
		build_line_list(tab);
		if (tab>0) {
			bt_new_line.enable();
		}
		else {
			bt_new_line.disable();
		}
	}
	else if(comp == &info_tabs){
		selected_convoy_tab = (uint8)info_tabs.get_active_tab_index();
	}
	else if (comp == &scl) {
		if(  line_scrollitem_t *li=(line_scrollitem_t *)scl.get_element(v.i)  ) {
			update_lineinfo( li->get_line() );
		}
		else {
			// no valid line
			update_lineinfo(linehandle_t());
		}
		selected_line[player->get_player_nr()][selected_tab[player->get_player_nr()]] = line;
		selected_line[player->get_player_nr()][0] = line; // keep these the same in overview
	}
	else if (comp == &inp_filter) {
		if(  strcmp(old_schedule_filter,schedule_filter)  ) {
			build_line_list(tabs.get_active_tab_index());
			strcpy(old_schedule_filter,schedule_filter);
		}
	}
	else if (comp == &inp_name) {
		rename_line();
	}
	else if (comp == &filter_btn_all_pas) {
		line_type_flags ^= (1 << simline_t::all_pas);
		filter_btn_all_pas.pressed = line_type_flags & (1 << simline_t::all_pas);
		build_line_list(tabs.get_active_tab_index());
	}
	else if (comp == &filter_btn_all_mails) {
		line_type_flags ^= (1 << simline_t::all_mail);
		filter_btn_all_mails.pressed = line_type_flags & (1 << simline_t::all_mail);
		build_line_list(tabs.get_active_tab_index());
	}
	else if (comp == &filter_btn_all_freights) {
		line_type_flags ^= (1 << simline_t::all_freight);
		filter_btn_all_freights.pressed = line_type_flags & (1 << simline_t::all_freight);
		build_line_list(tabs.get_active_tab_index());
	}
	else {
		if (line.is_bound()) {
			for ( int i = 0; i<MAX_LINE_COST; i++) {
				if (comp == &filterButtons[i]) {
					bFilterStates ^= (1 << i);
					if(bFilterStates & (1 << i)) {
						chart.show_curve(i);
					}
					else {
						chart.hide_curve(i);
					}
					break;
				}
			}
		}
	}

	return true;
}


void schedule_list_gui_t::reset_line_name()
{
	// change text input of selected line
	if (line.is_bound()) {
		tstrncpy(old_line_name, line->get_name(), sizeof(old_line_name));
		tstrncpy(line_name, line->get_name(), sizeof(line_name));
		inp_name.set_text(line_name, sizeof(line_name));
	}
}


void schedule_list_gui_t::rename_line()
{
	if (line.is_bound()) {
		const char *t = inp_name.get_text();
		// only change if old name and current name are the same
		// otherwise some unintended undo if renaming would occur
		if(  t  &&  t[0]  &&  strcmp(t, line->get_name())  &&  strcmp(old_line_name, line->get_name())==0) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "l%u,%s", line.get_id(), t );
			tool_t *tmp_tool = create_tool( TOOL_RENAME | SIMPLE_TOOL );
			tmp_tool->set_default_param( buf );
			welt->set_tool( tmp_tool, line->get_owner() );
			// since init always returns false, it is safe to delete immediately
			delete tmp_tool;
			// do not trigger this command again
			tstrncpy(old_line_name, t, sizeof(old_line_name));
		}
	}
}


void schedule_list_gui_t::draw(scr_coord pos, scr_size size)
{
	if(  old_line_count != player->simlinemgmt.get_line_count()  ) {
		show_lineinfo( line );
	}
	// if search string changed, update line selection
	if(  strcmp( old_schedule_filter, schedule_filter )  ) {
		build_line_list(tabs.get_active_tab_index());
		strcpy( old_schedule_filter, schedule_filter );
	}

	gui_frame_t::draw(pos, size);

	if(  line.is_bound()  ) {
		tab_name.clear();
		tab_name.append(translator::translate("Convoys"));
		tab_name.printf(" (%u)", line->count_convoys());

		if(  (!line->get_schedule()->empty()  &&  !line->get_schedule()->matches( welt, last_schedule ))  ||  last_vehicle_count != line->count_convoys()  ) {
			update_lineinfo( line );
		}

		// line type symbol
		display_color_img(line->get_linetype_symbol(), pos.x + LINE_NAME_COLUMN_WIDTH -23, pos.y + D_TITLEBAR_HEIGHT + D_MARGIN_TOP - 42, 0, false, false);

		PUSH_CLIP( pos.x + 1, pos.y + D_TITLEBAR_HEIGHT, size.w - 2, size.h - D_TITLEBAR_HEIGHT);
		display(pos);
		POP_CLIP();
	}

	for (int i = 0; i < MAX_LINE_COST; i++) {
		filterButtons[i].pressed = ((bFilterStates & (1 << i)) != 0);
	}
}

#define GOODS_SYMBOL_CELL_WIDTH 14 // TODO: This will be used in common with halt detail in the future
void schedule_list_gui_t::display(scr_coord pos)
{
	int icnv = line->count_convoys();

	cbuffer_t buf;
	char ctmp[128];
	int top = D_TITLEBAR_HEIGHT + D_MARGIN_TOP + D_BUTTON_HEIGHT;
	int left = LINE_NAME_COLUMN_WIDTH;

	// line handling goods (symbol)
	FOR(minivec_tpl<uint8>, const catg_index, line->get_goods_catg_index()) {
		uint8 temp = catg_index;
		display_color_img(goods_manager_t::get_info_catg_index(temp)->get_catg_symbol(), pos.x + left, pos.y + top+2, 0, false, false);
		left += GOODS_SYMBOL_CELL_WIDTH;
	}

	top += D_BUTTON_HEIGHT;

	capacity = load = loadfactor = 0; // total capacity and load of line (=sum of all conv's cap/load)

	sint64 profit = line->get_finance_history(0,LINE_PROFIT);

	sint64 total_trip_times = 0;
	sint64 convoys_with_trip_data = 0;
	for (int i = 0; i<icnv; i++) {
		convoihandle_t const cnv = line->get_convoy(i);
		// we do not want to count the capacity of depot convois
		if (!cnv->in_depot()) {
			total_trip_times += cnv->get_average_round_trip_time();
			if(cnv->get_average_round_trip_time())
			{
				convoys_with_trip_data++;
			}
			for (unsigned j = 0; j<cnv->get_vehicle_count(); j++) {
				capacity += cnv->get_vehicle(j)->get_cargo_max();
				load += cnv->get_vehicle(j)->get_total_cargo();
			}
		}
	}

	// we check if cap is zero, since theoretically a
	// conv can consist of only 1 vehicle, which has no cap (eg. locomotive)
	// and we do not like to divide by zero, do we?
	if (capacity > 0) {
		loadfactor = (load * 100) / capacity;
	}

	sint64 service_frequency = convoys_with_trip_data ? total_trip_times / convoys_with_trip_data : 0; // In ticks.
	if(icnv)
	{
		service_frequency /= icnv;
	}

	const int spacing = line->get_schedule()->get_spacing();
	if(icnv && spacing > 0)
	{
		// Check whether the spacing setting affects things.
		sint64 spacing_ticks = welt->ticks_per_world_month / (sint64)spacing;
		const uint32 spacing_time = welt->ticks_to_tenths_of_minutes(spacing_ticks);
		service_frequency = max(spacing_time, service_frequency);
	}

	buf.clear();
	// Display service frequency
	if(service_frequency)
	{
		buf.printf(translator::translate("Service frequency"));
		char as_clock[32];
		welt->sprintf_ticks(as_clock, sizeof(as_clock), service_frequency);
		buf.printf(" %s",  as_clock);
	}
	int len=display_proportional_clip(pos.x+LINE_NAME_COLUMN_WIDTH, pos.y + top, buf, ALIGN_LEFT, line->get_state() & simline_t::line_missing_scheduled_slots ? COL_DARK_TURQUOISE : SYSCOL_TEXT, true );
	if (line->get_state() & simline_t::line_missing_scheduled_slots) {
		if (skinverwaltung_t::missing_scheduled_slot) {
			display_color_img_with_tooltip(skinverwaltung_t::missing_scheduled_slot->get_image_id(0), pos.x + LINE_NAME_COLUMN_WIDTH + len + D_H_SPACE, pos.y + top, 0, false, false, translator::translate(line_alert_helptexts[1]));
		}
		else if (skinverwaltung_t::alerts) {
			display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), pos.x + LINE_NAME_COLUMN_WIDTH + len + D_H_SPACE, pos.y + top, 0, false, false, translator::translate(line_alert_helptexts[1]));
		}
	}

	int len2 = display_proportional_clip(pos.x+LINE_NAME_COLUMN_WIDTH, pos.y+top + LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, SYSCOL_TEXT, true );
	money_to_string(ctmp, profit/100.0);
	len2 += display_proportional_clip(pos.x+LINE_NAME_COLUMN_WIDTH+len2+5, pos.y+top+LINESPACE, ctmp, ALIGN_LEFT, profit>=0?MONEY_PLUS:MONEY_MINUS, true );


	int rest_width = max( (get_windowsize().w-LINE_NAME_COLUMN_WIDTH)/2, max(len2,len) );
	filled_bar.set_pos(scr_coord(LINE_NAME_COLUMN_WIDTH + rest_width + 24, top - LINESPACE - D_BUTTON_HEIGHT));
	if (capacity > 0) {
		number_to_string(ctmp, capacity, 0);
		buf.clear();
		buf.printf( translator::translate("Capacity: %s\nLoad: %d (%d%%)"), ctmp, load, loadfactor );
		display_multiline_text(pos.x + LINE_NAME_COLUMN_WIDTH + rest_width + 24, pos.y+top, buf, SYSCOL_TEXT);
	}
	bt_line_class_manager.disable();
	for (unsigned convoy = 0; convoy < line->count_convoys(); convoy++)
	{
		convoihandle_t cnv = line->get_convoy(convoy);
		for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++)
		{
			vehicle_t* v = cnv->get_vehicle(veh);
			if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS || v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
			{
				bt_line_class_manager.enable();
			}
		}
	}

	top += D_BUTTON_HEIGHT + LINESPACE * 2 + 1;
	left = LINE_NAME_COLUMN_WIDTH + D_BUTTON_WIDTH*1.5 + D_V_SPACE;
	buf.clear();
	// show the state of the line, if interresting
	if (line->get_state() & simline_t::line_nothing_moved) {
		if (skinverwaltung_t::alerts) {
			display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), pos.x + left, pos.y + top, 0, false, false, translator::translate(line_alert_helptexts[0]));
			left += GOODS_SYMBOL_CELL_WIDTH;
		}
		else {
			buf.append(translator::translate(line_alert_helptexts[0]));
		}
	}
	if (line->count_convoys() && line->get_state() & simline_t::line_has_upgradeable_vehicles) {
		if (skinverwaltung_t::upgradable) {
			display_color_img_with_tooltip(skinverwaltung_t::upgradable->get_image_id(1), pos.x + left, pos.y + top, 0, false, false, translator::translate(line_alert_helptexts[4]));
			left += GOODS_SYMBOL_CELL_WIDTH;
		}
		else if (!buf.len() && line->get_state_color() == COL_PURPLE) {
			buf.append(translator::translate(line_alert_helptexts[4]));
		}
	}
	if (line->count_convoys() && line->get_state() & simline_t::line_has_obsolete_vehicles) {
		if (skinverwaltung_t::alerts) {
			display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(1), pos.x + left, pos.y + top, 0, false, false, translator::translate(line_alert_helptexts[2]));
			left += GOODS_SYMBOL_CELL_WIDTH;
		}
		else if (!buf.len()) {
			buf.append(translator::translate(line_alert_helptexts[2]));
		}
	}
	if (line->count_convoys() && line->get_state() & simline_t::line_overcrowded) {
		if (skinverwaltung_t::pax_evaluation_icons) {
			display_color_img_with_tooltip(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), pos.x + left, pos.y + top, 0, false, false, translator::translate(line_alert_helptexts[3]));
			left += GOODS_SYMBOL_CELL_WIDTH;
		}
		else if (!buf.len() && line->get_state_color() == COL_DARK_PURPLE) {
			buf.append(translator::translate(line_alert_helptexts[3]));
		}
	}
	if (buf.len() > 0) {
		display_proportional_clip(pos.x + left, pos.y + top, buf, ALIGN_LEFT, line->get_state_color(), true);
	}
}


void schedule_list_gui_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);

	int rest_width = get_windowsize().w-LINE_NAME_COLUMN_WIDTH;
	int button_per_row=max(1,rest_width/(D_BUTTON_WIDTH+D_H_SPACE));
	int button_rows= MAX_LINE_COST/button_per_row + ((MAX_LINE_COST%button_per_row)!=0);

	scrolly_haltestellen.set_size( scr_size(LINE_NAME_COLUMN_WIDTH-4, get_client_windowsize().h-scrolly_haltestellen.get_pos().y-1) );

	info_tabs.set_size(scr_size(rest_width+2, get_windowsize().h - LINESPACE*2 - D_BUTTON_HEIGHT*5 - D_MARGIN_TOP - D_TITLEBAR_HEIGHT));
	scrolly_convois.set_size(scr_size(info_tabs.get_size().w+1, info_tabs.get_size().h - scrolly_convois.get_pos().y-D_H_SPACE-1));
	chart.set_size(scr_size(rest_width-68-D_MARGIN_RIGHT, SCL_HEIGHT-14-(button_rows*(D_BUTTON_HEIGHT+D_H_SPACE))));
	inp_name.set_size(scr_size(rest_width - 31, 14));
	filled_bar.set_size(scr_size(rest_width/2-24-D_MARGIN_RIGHT, 4));
	bt_withdraw_line.set_pos(scr_coord(get_windowsize().w - D_BUTTON_WIDTH - D_MARGIN_LEFT, bt_withdraw_line.get_pos().y));

	int y=SCL_HEIGHT-(button_rows*(D_BUTTON_HEIGHT+D_H_SPACE))+18;
	for (int i=0; i<MAX_LINE_COST; i++) {
		filterButtons[i].set_pos( scr_coord((i%button_per_row)*(D_BUTTON_WIDTH+D_H_SPACE)+D_H_SPACE, y+(i/button_per_row)*(D_BUTTON_HEIGHT+D_H_SPACE)) );
	}
}

void schedule_list_gui_t::build_line_list(int selected_tab)
{
	sint32 sel = -1;
	scl.clear_elements();
	player->simlinemgmt.get_lines(tabs_to_lineindex[selected_tab], &lines, get_filter_type_bits());
	vector_tpl<line_scrollitem_t *>selected_lines;


	FOR(vector_tpl<linehandle_t>, const l, lines) {
		// search name
		if(  strstr(l->get_name(), schedule_filter)  ) {
			selected_lines.append(new line_scrollitem_t(l));
		}
	}

	FOR(vector_tpl<line_scrollitem_t*>, const i, selected_lines) {
		scl.append_element(i);
		if(  line == i->get_line()  ) {
			sel = scl.get_count() - 1;
		}
	}

	scl.set_selection( sel );
	line_scrollitem_t::sort_mode = (line_scrollitem_t::sort_modes_t)current_sort_mode;
	scl.sort( 0, NULL );

	old_line_count = player->simlinemgmt.get_line_count();
}


/* hides show components */
void schedule_list_gui_t::update_lineinfo(linehandle_t new_line)
{
	// change line name if necessary
	if (line.is_bound()) {
		rename_line();
	}
	if(new_line.is_bound()) {
		// ok, this line is visible
		scrolly_convois.set_visible(true);
		scrolly_haltestellen.set_visible(true);
		inp_name.set_visible(true);
		filled_bar.set_visible(true);

		// fill container with info of line's convoys
		// we do it here, since this list needs only to be
		// refreshed when the user selects a new line
		line_convoys.clear();
		line_convoys = new_line->get_convoys();
		sort_list();

		int i, icnv = 0;
		icnv = new_line->count_convoys();
		// display convoys of line
		cont.remove_all();
		while (!convoy_infos.empty()) {
			delete convoy_infos.pop_back();
		}
		convoy_infos.resize(icnv);
		int ypos = 0;
		int cinfo_height;
		switch (selected_cnvlist_mode[player->get_player_nr()]) {
			case gui_convoiinfo_t::cnvlist_formation:
				cinfo_height = 55;
				break;
			case gui_convoiinfo_t::cnvlist_payload:
				cinfo_height = 40;
				break;
			case gui_convoiinfo_t::cnvlist_normal:
			default:
				cinfo_height = 40;
				break;
		}
		for(i = 0;  i<icnv;  i++  ) {
			gui_convoiinfo_t* const cinfo = new gui_convoiinfo_t(line_convoys.get_element(i), false);
			cinfo->set_pos(scr_coord(0, ypos));
			cinfo->set_size(scr_size(600, cinfo_height));
			cinfo->set_mode(selected_cnvlist_mode[player->get_player_nr()]);
			convoy_infos.append(cinfo);
			cont.add_component(cinfo);
			ypos += cinfo_height;
		}
		cont.set_size(scr_size(600, ypos));

		bt_delete_line.disable();
		add_component(&bt_withdraw_line);
		bt_withdraw_line.disable();
		if(icnv>0) {
			bt_withdraw_line.enable();
		}
		else {
			bt_delete_line.enable();
		}
		bt_change_line.enable();

		bt_withdraw_line.pressed = new_line->get_withdraw();

		livery_selector.set_focusable(true);
		livery_selector.clear_elements();
		livery_scheme_indices.clear();
		if(icnv > 0)
		{
			// build available livery schemes list for this line
			if (new_line->count_convoys()) {
				const uint16 month_now = welt->get_timeline_year_month();
				vector_tpl<livery_scheme_t*>* schemes = welt->get_settings().get_livery_schemes();

				ITERATE_PTR(schemes, i)
				{
					bool found = false;
					livery_scheme_t* scheme = schemes->get_element(i);
					if (scheme->is_available(month_now))
					{
						for (uint j = 0; j < new_line->count_convoys(); j++)
						{
							convoihandle_t const cnv_in_line = new_line->get_convoy(j);
							for (int k = 0; k < cnv_in_line->get_vehicle_count(); k++) {
								const vehicle_desc_t *desc = cnv_in_line->get_vehicle(k)->get_desc();
								if (desc->get_livery_count()) {
									const char* test_livery = scheme->get_latest_available_livery(month_now, desc);
									if (test_livery) {
										if (scheme->is_contained(test_livery, month_now)) {
											livery_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(scheme->get_name()), SYSCOL_TEXT));
											livery_scheme_indices.append(i);
											found = true;
											break;
										}
									}
								}
							}
							if (found) {
								livery_selector.set_selection(livery_scheme_indices.index_of(i));
								break;
							}
						}
					}
				}
			}
		}
		if(livery_scheme_indices.empty()) {
			livery_selector.append_element(new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate("No livery"), SYSCOL_TEXT));
			livery_selector.set_selection(0);
		}


		// fill haltestellen container with info of line's haltestellen
		cont_haltestellen.remove_all();
		while (!stop_infos.empty()) {
			delete stop_infos.pop_back();
		}
		stop_infos.resize(new_line->get_schedule()->entries.get_count());
		ypos = 0;
		FOR(minivec_tpl<schedule_entry_t>, const& i, new_line->get_schedule()->entries) {
			halthandle_t const halt = haltestelle_t::get_halt(i.pos, player);
			if (halt.is_bound()) {
				halt_list_stats_t* cinfo = new halt_list_stats_t(halt);
				cinfo->set_pos(scr_coord(0, ypos));
				cinfo->set_size(scr_size(500, 28));
				stop_infos.append(cinfo);
				cont_haltestellen.add_component(cinfo);
				ypos += 28;
			}
		}
		cont_haltestellen.set_size(scr_size(500, ypos));

		// chart
		chart.remove_curves();
		for(i=0; i<MAX_LINE_COST; i++)  {
			chart.add_curve(cost_type_color[i], new_line->get_finance_history(), MAX_LINE_COST, statistic[i], MAX_MONTHS, statistic_type[i], filterButtons[i].pressed, true, statistic_type[i]*2 );
			if (bFilterStates & (1 << i)) {
				chart.show_curve(i);
			}
		}
		chart.set_visible(true);

		// has this line a single running convoi?
		if(  new_line.is_bound()  &&  new_line->count_convoys() > 0  ) {
			// set this schedule as current to show on minimap if possible
			reliefkarte_t::get_karte()->set_current_cnv( new_line->get_convoy(0) );
		}
		else {
			reliefkarte_t::get_karte()->set_current_cnv( convoihandle_t() );
		}

		delete last_schedule;
		last_schedule = new_line->get_schedule()->copy();
		last_vehicle_count = new_line->count_convoys();
	}
	else if(  inp_name.is_visible()  ) {
		// previously a line was visible
		// thus the need to hide everything
		line_convoys.clear();
		cont.remove_all();
		scrolly_convois.set_visible(false);
		scrolly_haltestellen.set_visible(false);
		inp_name.set_visible(false);
		filled_bar.set_visible(false);
		scl.set_selection(-1);
		bt_delete_line.disable();
		bt_change_line.disable();
		tab_name.clear();
		tab_name.printf("%s (0)",translator::translate("Convoys"));
		for(int i=0; i<MAX_LINE_COST; i++)  {
			chart.hide_curve(i);
		}
		chart.set_visible(true);

		// hide schedule on minimap (may not current, but for safe)
		reliefkarte_t::get_karte()->set_current_cnv( convoihandle_t() );

		delete last_schedule;
		last_schedule = NULL;
		last_vehicle_count = 0;
	}
	line = new_line;
	bt_withdraw_line.set_visible( line.is_bound() );
	bt_line_class_manager.set_visible(line.is_bound());
	bt_times_history.set_visible( line.is_bound() );

	reset_line_name();
}


void schedule_list_gui_t::show_lineinfo(linehandle_t line)
{
	update_lineinfo(line);

	if(  line.is_bound()  ) {
		// rebuilding line list will also show selection
		for(  uint8 i=0;  i<max_idx;  i++  ) {
			if(  tabs_to_lineindex[i]==line->get_linetype()  ) {
				tabs.set_active_tab_index( i );
				build_line_list( i );
				break;
			}
		}
	}
}


bool schedule_list_gui_t::compare_convois(convoihandle_t const cnv1, convoihandle_t const cnv2)
{
	sint32 cmp = 0;

	switch (sortby) {
		default:
		case by_name:
			cmp = strcmp(cnv1->get_internal_name(), cnv2->get_internal_name());
			break;
		case by_schedule:
			cmp = cnv1->get_schedule()->get_current_stop() - cnv2->get_schedule()->get_current_stop();
			break;
		case by_profit:
			cmp = sgn(cnv1->get_jahresgewinn() - cnv2->get_jahresgewinn());
			break;
		case by_loading_lvl:
			cmp = cnv1->get_loading_level() - cnv2->get_loading_level();
			break;
		case by_max_speed:
			cmp = cnv1->get_min_top_speed() - cnv2->get_min_top_speed();
			break;
		//case by_power:
		//	cmp = cnv1->get_sum_power() - cnv2->get_sum_power();
		//	break;
		case by_age:
			cmp = cnv1->get_average_age() - cnv2->get_average_age();
			break;
		case by_value:
			cmp = sgn(cnv1->get_purchase_cost() - cnv2->get_purchase_cost());
			break;
	}

	return sortreverse ? cmp > 0 : cmp < 0;
}


// sort the line convoy list
void schedule_list_gui_t::sort_list()
{
	if (!line_convoys.get_count()) {
		return;
	}
	std::sort(line_convoys.begin(), line_convoys.end(), compare_convois);
}


void schedule_list_gui_t::update_data(linehandle_t changed_line)
{
	if (changed_line.is_bound()) {
		const uint16 i = tabs.get_active_tab_index();
		if (tabs_to_lineindex[i] == simline_t::line  ||  tabs_to_lineindex[i] == changed_line->get_linetype()) {
			// rebuilds the line list, but does not change selection
			build_line_list(i);
		}

		// change text input of selected line
		if (changed_line.get_id() == line.get_id()) {
			reset_line_name();
		}
	}
}


uint32 schedule_list_gui_t::get_rdwr_id()
{
	return magic_line_management_t+player->get_player_nr();
}


void schedule_list_gui_t::rdwr( loadsave_t *file )
{
	scr_size size;
	sint32 cont_xoff, cont_yoff, halt_xoff, halt_yoff;
	if(  file->is_saving()  ) {
		size = get_windowsize();
		cont_xoff = scrolly_convois.get_scroll_x();
		cont_yoff = scrolly_convois.get_scroll_y();
		halt_xoff = scrolly_haltestellen.get_scroll_x();
		halt_yoff = scrolly_haltestellen.get_scroll_y();
	}
	size.rdwr( file );
	simline_t::rdwr_linehandle_t(file, line);
	int chart_records = line_cost_t::MAX_LINE_COST;
	if (file->get_version() < 112008) {
		chart_records = 8;
	}
	else if (file->get_extended_version() < 14 || (file->get_extended_version() == 14 && file->get_extended_revision() < 25)) {
		chart_records = 12;
	}
	for (int i=0; i<chart_records; i++) {
		bool b = filterButtons[i].pressed;
		file->rdwr_bool( b );
		filterButtons[i].pressed = b;
	}
	file->rdwr_long( cont_xoff );
	file->rdwr_long( cont_yoff );
	file->rdwr_long( halt_xoff );
	file->rdwr_long( halt_yoff );
	// open dialogue
	if(  file->is_loading()  ) {
		show_lineinfo( line );
		set_windowsize( size );
		resize( scr_coord(0,0) );
		scrolly_convois.set_scroll_position( cont_xoff, cont_yoff );
		scrolly_haltestellen.set_scroll_position( halt_xoff, halt_yoff );
	}
}
