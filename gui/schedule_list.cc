/*
 * Copyright (c) 1997 - 2004 Hj. Malthaner
 *
 * Line management
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <algorithm>
#include <stdio.h>

#include "messagebox.h"
#include "schedule_list.h"
#include "line_management_gui.h"
#include "gui_convoiinfo.h"
#include "line_item.h"

#include "../simcolor.h"
#include "../simdepot.h"
#include "../simhalt.h"
#include "../simworld.h"
#include "../simevent.h"
#include "../simgraph.h"
#include "../simskin.h"
#include "../simconvoi.h"
#include "../vehicle/simvehikel.h"
#include "../simwin.h"
#include "../simlinemgmt.h"
#include "../simmenu.h"
#include "../utils/simstring.h"
#include "../player/simplay.h"

#include "../bauer/vehikelbauer.h"

#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"

#include "../boden/wege/kanal.h"
#include "../boden/wege/maglev.h"
#include "../boden/wege/monorail.h"
#include "../boden/wege/narrowgauge.h"
#include "../boden/wege/runway.h"
#include "../boden/wege/schiene.h"
#include "../boden/wege/strasse.h"


#include "halt_list_stats.h"
#include "karte.h"


static const char *cost_type[MAX_LINE_COST] =
{
	"Free Capacity",
	"Transported",
	"Revenue",
	"Operation",
	"Profit",
	"Convoys",
	"Distance",
	"Maxspeed"
};

const int cost_type_color[MAX_LINE_COST] =
{
	COL_FREE_CAPACITY,
	COL_TRANSPORTED,
	COL_REVENUE,
	COL_OPERATION,
	COL_PROFIT,
	COL_COUNVOI_COUNT,
	COL_DISTANCE,
	COL_MAXSPEED
};

static uint8 tabs_to_lineindex[9];
static uint8 max_idx=0;

static uint8 statistic[MAX_LINE_COST]={
	LINE_CAPACITY, LINE_TRANSPORTED_GOODS, LINE_REVENUE, LINE_OPERATIONS, LINE_PROFIT, LINE_CONVOIS, LINE_DISTANCE, LINE_MAXSPEED
};

static uint8 statistic_type[MAX_LINE_COST]={
	STANDARD, STANDARD, MONEY, MONEY, MONEY, STANDARD, STANDARD, STANDARD
};

enum sort_modes_t { SORT_BY_NAME=0, SORT_BY_ID, SORT_BY_PROFIT, SORT_BY_TRANSPORTED, SORT_BY_CONVOIS, SORT_BY_DISTANCE, MAX_SORT_MODES };
static uint8 current_sort_mode = 0;

#define LINE_NAME_COLUMN_WIDTH ((D_BUTTON_WIDTH*3)+11+4)
#define SCL_HEIGHT (15*LINESPACE-1)


static bool compare_lines(line_scrollitem_t* a, line_scrollitem_t* b)
{
	switch(  current_sort_mode  ) {
		case SORT_BY_NAME:	// default
			break;
		case SORT_BY_ID:
			return (a->get_line().get_id(),b->get_line().get_id())<0;
		case SORT_BY_PROFIT:
			return (a->get_line()->get_finance_history(1,LINE_PROFIT) - b->get_line()->get_finance_history(1,LINE_PROFIT))<0;
		case SORT_BY_TRANSPORTED:
			return (a->get_line()->get_finance_history(1,LINE_TRANSPORTED_GOODS) - b->get_line()->get_finance_history(1,LINE_TRANSPORTED_GOODS))<0;
		case SORT_BY_CONVOIS:
			return (a->get_line()->get_finance_history(1,LINE_CONVOIS) - b->get_line()->get_finance_history(1,LINE_CONVOIS))<0;
		case SORT_BY_DISTANCE:
			// normalizing to the number of convoys to get the fastest ones ...
			return (a->get_line()->get_finance_history(1,LINE_DISTANCE)/max(1,a->get_line()->get_finance_history(1,LINE_CONVOIS)) -
			        b->get_line()->get_finance_history(1,LINE_DISTANCE)/max(1,b->get_line()->get_finance_history(1,LINE_CONVOIS)) )<0;
	}
	// default sorting ...

	// first: try to sort by number
	const char *atxt = a->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if(  atxt[0]>='0'  &&  atxt[0]<='9'  ) {
		aint = atoi( atxt );
	}
	else if(  atxt[1]>='0'  &&  atxt[1]<='9'  ) {
		aint = atoi( atxt+1 );
	}
	const char *btxt = b->get_text();
	int bint = 0;
	if(  btxt[0]>='0'  &&  btxt[0]<='9'  ) {
		bint = atoi( btxt );
	}
	else if(  btxt[1]>='0'  &&  btxt[1]<='9'  ) {
		bint = atoi( btxt+1 );
	}
	if(  aint!=bint  ) {
		return (aint-bint)<0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt)<0;
}


/// selected tab per player
static uint8 last_active_player = MAX_PLAYER_COUNT+1;
static uint8 selected_tab = 0;

/// selected line per tab
static linehandle_t selected_line[simline_t::MAX_LINE_TYPE];

// Hajo: 17-Jan-04: changed layout to make components fit into
// a width of 400 pixels -> original size was unuseable in 640x480
schedule_list_gui_t::schedule_list_gui_t(spieler_t *sp_) :
	gui_frame_t( translator::translate("Line Management"), sp_),
	sp(sp_),
	scrolly_convois(&cont),
	scrolly_haltestellen(&cont_haltestellen),
	scl(gui_scrolled_list_t::select),
	lbl_filter("Line Filter")
{
	capacity = load = 0;
	selection = -1;
	loadfactor = 0;
	schedule_filter[0] = 0;
	old_schedule_filter[0] = 0;
	last_schedule = NULL;

	// init scrolled list
	scl.set_pos(koord(0,1));
	scl.set_groesse(koord(LINE_NAME_COLUMN_WIDTH-11-4, SCL_HEIGHT-18));
	scl.set_highlight_color(sp->get_player_color1()+1);
	scl.add_listener(this);

	// reset selected tab / line if player changed
	if (last_active_player != sp->get_player_nr()) {
		last_active_player = sp->get_player_nr();
		selected_tab = 0;
		for(uint i=0; i<lengthof(selected_line); i++) {
			selected_line[i] = linehandle_t();
		}
	}

	// tab panel
	tabs.set_pos(koord(11,5));
	tabs.set_groesse(koord(LINE_NAME_COLUMN_WIDTH-11-4, SCL_HEIGHT));
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
	if (!vehikelbauer_t::get_info(tram_wt).empty()) {
		tabs.add_tab(&scl, translator::translate("Tram"), skinverwaltung_t::tramhaltsymbol, translator::translate("Tram"));
		tabs_to_lineindex[max_idx++] = simline_t::tramline;
	}
	if(strasse_t::default_strasse) {
		tabs.add_tab(&scl, translator::translate("Truck"), skinverwaltung_t::autohaltsymbol, translator::translate("Truck"));
		tabs_to_lineindex[max_idx++] = simline_t::truckline;
	}
	if (!vehikelbauer_t::get_info(water_wt).empty()) {
		tabs.add_tab(&scl, translator::translate("Ship"), skinverwaltung_t::schiffshaltsymbol, translator::translate("Ship"));
		tabs_to_lineindex[max_idx++] = simline_t::shipline;
	}
	if(runway_t::default_runway) {
		tabs.add_tab(&scl, translator::translate("Air"), skinverwaltung_t::airhaltsymbol, translator::translate("Air"));
		tabs_to_lineindex[max_idx++] = simline_t::airline;
	}
	tabs.add_listener(this);
	add_komponente(&tabs);

	// editable line name
	inp_name.add_listener(this);
	inp_name.set_pos(koord(LINE_NAME_COLUMN_WIDTH, 14 + SCL_HEIGHT));
	inp_name.set_visible(false);
	add_komponente(&inp_name);

	// load display
	filled_bar.add_color_value(&loadfactor, COL_GREEN);
	filled_bar.set_pos(koord(LINE_NAME_COLUMN_WIDTH, 6 + SCL_HEIGHT));
	filled_bar.set_visible(false);
	add_komponente(&filled_bar);

	// convoi list
	cont.set_groesse(koord(200, 40));
	scrolly_convois.set_pos(koord(LINE_NAME_COLUMN_WIDTH, 14 + SCL_HEIGHT+D_BUTTON_HEIGHT*2+4+2*LINESPACE+2));
	scrolly_convois.set_show_scroll_x(true);
	scrolly_convois.set_scroll_amount_y(40);
	scrolly_convois.set_visible(false);
	add_komponente(&scrolly_convois);

	// halt list
	cont_haltestellen.set_groesse(koord(LINE_NAME_COLUMN_WIDTH, 28));
	scrolly_haltestellen.set_pos(koord(0, 7 + SCL_HEIGHT+2*D_BUTTON_HEIGHT+2));
	scrolly_haltestellen.set_show_scroll_x(true);
	scrolly_haltestellen.set_scroll_amount_y(28);
	scrolly_haltestellen.set_visible(false);
	add_komponente(&scrolly_haltestellen);

	// filter liens by
	lbl_filter.set_pos( koord( 11, 7+SCL_HEIGHT+2 ) );
	add_komponente(&lbl_filter);

	inp_filter.set_pos( koord( 11+D_BUTTON_WIDTH, 7+SCL_HEIGHT ) );
	inp_filter.set_groesse( koord( D_BUTTON_WIDTH*2, D_BUTTON_HEIGHT ) );
	inp_filter.set_text( schedule_filter, lengthof(schedule_filter) );
//	inp_filter.set_tooltip("Only show lines containing");
	inp_filter.add_listener(this);
	add_komponente(&inp_filter);

	// normal buttons edit new remove
	bt_new_line.init(button_t::roundbox, "New Line", koord(11, 8+SCL_HEIGHT+D_BUTTON_HEIGHT ), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_new_line.add_listener(this);
	add_komponente(&bt_new_line);

	bt_change_line.init(button_t::roundbox, "Update Line", koord(11+D_BUTTON_WIDTH, 8+SCL_HEIGHT+D_BUTTON_HEIGHT ), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_change_line.set_tooltip("Modify the selected line");
	bt_change_line.add_listener(this);
	bt_change_line.disable();
	add_komponente(&bt_change_line);

	bt_delete_line.init(button_t::roundbox, "Delete Line", koord(11+2*D_BUTTON_WIDTH, 8+SCL_HEIGHT+D_BUTTON_HEIGHT ), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_delete_line.set_tooltip("Delete the selected line (if without associated convois).");
	bt_delete_line.add_listener(this);
	bt_delete_line.disable();
	add_komponente(&bt_delete_line);

	bt_withdraw_line.init(button_t::roundbox_state, "Withdraw All", koord(LINE_NAME_COLUMN_WIDTH, 14+SCL_HEIGHT+D_BUTTON_HEIGHT+2), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	bt_withdraw_line.set_tooltip("Convoi is sold when all wagons are empty.");
	bt_withdraw_line.set_visible(false);
	bt_withdraw_line.add_listener(this);
	add_komponente(&bt_withdraw_line);

	//CHART
	chart.set_dimension(12, 1000);
	chart.set_pos( koord(LINE_NAME_COLUMN_WIDTH+50,11) );
	chart.set_seed(0);
	chart.set_background(MN_GREY1);
	add_komponente(&chart);

	// add filter buttons
	for (int i=0; i<MAX_LINE_COST; i++) {
		filterButtons[i].init(button_t::box_state,cost_type[i],koord(0,0), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
		filterButtons[i].add_listener(this);
		filterButtons[i].background = cost_type_color[i];
		add_komponente(filterButtons + i);
	}

	// recover last selected line
	int index = 0;
	for(uint i=0; i<max_idx; i++) {
		if (tabs_to_lineindex[i] == selected_tab) {
			line = selected_line[ selected_tab ];
			index = i;
			break;
		}
	}
	selected_tab = tabs_to_lineindex[index]; // reset if previous selected tab is not there anymore
	tabs.set_active_tab_index(index);
	if (index>0) {
		bt_new_line.enable();
	}
	else {
		bt_new_line.disable();
	}
	update_lineinfo( line );

	// resize button
	set_min_windowsize(koord(488, 300));
	set_resizemode(diagonal_resize);
	resize(koord(0,0));
	resize(koord(0,100));

	build_line_list(index);
}


schedule_list_gui_t::~schedule_list_gui_t()
{
	delete last_schedule;
	// change line name if necessary
	rename_line();
}


/**
 * Mausklicks werden hiermit an die GUI-Komponenten
 * gemeldet
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


bool schedule_list_gui_t::action_triggered( gui_action_creator_t *komp, value_t v )           // 28-Dec-01    Markus Weber    Added
{
	if (komp == &bt_change_line) {
		if (line.is_bound()) {
			create_win( new line_management_gui_t(line, sp), w_info, (ptrdiff_t)line.get_rep() );
		}
	}
	else if (komp == &bt_new_line) {
		// create typed line
		assert(  tabs.get_active_tab_index() > 0  &&  tabs.get_active_tab_index()<max_idx  );
		// update line schedule via tool!
		werkzeug_t *w = create_tool( WKZ_LINE_TOOL | SIMPLE_TOOL );
		cbuffer_t buf;
		int type = tabs_to_lineindex[tabs.get_active_tab_index()];
		buf.printf( "c,0,%i,0,0|%i|", type, type );
		w->set_default_param(buf);
		sp->get_welt()->set_werkzeug( w, sp );
		// since init always returns false, it is safe to delete immediately
		delete w;
		depot_t::update_all_win();
	}
	else if (komp == &bt_delete_line) {
		if (line.is_bound()) {
			werkzeug_t *w = create_tool( WKZ_LINE_TOOL | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "d,%i", line.get_id() );
			w->set_default_param(buf);
			sp->get_welt()->set_werkzeug( w, sp );
			// since init always returns false, it is safe to delete immediately
			delete w;
			depot_t::update_all_win();
		}
	}
	else if (komp == &bt_withdraw_line) {
		bt_withdraw_line.pressed ^= 1;
		if (line.is_bound()) {
			werkzeug_t *w = create_tool( WKZ_LINE_TOOL | SIMPLE_TOOL );
			cbuffer_t buf;
			buf.printf( "w,%i,%i", line.get_id(), bt_withdraw_line.pressed );
			w->set_default_param(buf);
			sp->get_welt()->set_werkzeug( w, sp );
			// since init always returns false, it is safe to delete immediately
			delete w;
		}
	}
	else if (komp == &tabs) {
		int const tab = tabs.get_active_tab_index();
		selected_tab = tabs_to_lineindex[tab];
		update_lineinfo( selected_line[selected_tab] );
		build_line_list(tab);
		if (tab>0) {
			bt_new_line.enable();
		}
		else {
			bt_new_line.disable();
		}
	}
	else if (komp == &scl) {
		if(  line_scrollitem_t *li=(line_scrollitem_t *)scl.get_element(v.i)  ) {
			update_lineinfo( li->get_line() );
		}
		else {
			// no valid line
			update_lineinfo(linehandle_t());
		}
		selected_line[selected_tab] = line;
		// brute force: just recalculate whole list on each click to keep it current
		build_line_list(tabs.get_active_tab_index());
	}
	else if (komp == &inp_filter) {
		if(  strcmp(old_schedule_filter,schedule_filter)  ) {
			build_line_list(tabs.get_active_tab_index());
			strcpy(old_schedule_filter,schedule_filter);
		}
	}
	else if (komp == &inp_name) {
		rename_line();
	}
	else {
		if (line.is_bound()) {
			for ( int i = 0; i<MAX_LINE_COST; i++) {
				if (komp == &filterButtons[i]) {
					filterButtons[i].pressed ^= 1;
					if(filterButtons[i].pressed) {
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
			werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			sp->get_welt()->set_werkzeug( w, line->get_besitzer() );
			// since init always returns false, it is safe to delete immediately
			delete w;
			// do not trigger this command again
			tstrncpy(old_line_name, t, sizeof(old_line_name));
		}
	}
}


void schedule_list_gui_t::zeichnen(koord pos, koord gr)
{
	if(  old_line_count != sp->simlinemgmt.get_line_count()  ) {
		show_lineinfo( line );
	}
	// if search string changed, update line selection
	if(  strcmp( old_schedule_filter, schedule_filter )  ) {
		build_line_list(tabs.get_active_tab_index());
		strcpy( old_schedule_filter, schedule_filter );
	}

	gui_frame_t::zeichnen(pos, gr);

	if(  line.is_bound()  ) {
		if(  (!line->get_schedule()->empty()  &&  !line->get_schedule()->matches( sp->get_welt(), last_schedule ))  ||  last_vehicle_count != line->count_convoys()  ) {
			update_lineinfo( line );
		}
		PUSH_CLIP( pos.x + 1, pos.y + D_TITLEBAR_HEIGHT, gr.x - 2, gr.y - D_TITLEBAR_HEIGHT);
		display(pos);
		POP_CLIP();
	}
}


void schedule_list_gui_t::display(koord pos)
{
	int icnv = line->count_convoys();

	cbuffer_t buf;
	char ctmp[128];

	capacity = load = loadfactor = 0; // total capacity and load of line (=sum of all conv's cap/load)

	sint64 profit = line->get_finance_history(0,LINE_PROFIT);

	for (int i = 0; i<icnv; i++) {
		convoihandle_t const cnv = line->get_convoy(i);
		// we do not want to count the capacity of depot convois
		if (!cnv->in_depot()) {
			for (unsigned j = 0; j<cnv->get_vehikel_anzahl(); j++) {
				capacity += cnv->get_vehikel(j)->get_fracht_max();
				load += cnv->get_vehikel(j)->get_fracht_menge();
			}
		}
	}

	// we check if cap is zero, since theoretically a
	// conv can consist of only 1 vehicle, which has no cap (eg. locomotive)
	// and we do not like to divide by zero, do we?
	if (capacity > 0) {
		loadfactor = (load * 100) / capacity;
	}

	switch(icnv) {
		case 0: {
			buf.append( translator::translate("no convois") );
			break;
		}
		case 1: {
			buf.append( translator::translate("1 convoi") );
			break;
		}
		default: {
			buf.printf( translator::translate("%d convois"), icnv) ;
			break;
		}
	}
	int len=display_proportional_clip(pos.x+LINE_NAME_COLUMN_WIDTH, pos.y+16+14+SCL_HEIGHT+D_BUTTON_HEIGHT*2+4, buf, ALIGN_LEFT, COL_BLACK, true );

	int len2 = display_proportional_clip(pos.x+LINE_NAME_COLUMN_WIDTH, pos.y+16+14+SCL_HEIGHT+D_BUTTON_HEIGHT*2+4+LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, COL_BLACK, true );
	money_to_string(ctmp, profit/100.0);
	len2 += display_proportional_clip(pos.x+LINE_NAME_COLUMN_WIDTH+len2, pos.y+16+14+SCL_HEIGHT+D_BUTTON_HEIGHT*2+4+LINESPACE, ctmp, ALIGN_LEFT, profit>=0?MONEY_PLUS:MONEY_MINUS, true );

	if (capacity > 0) {
		int rest_width = max( (get_fenstergroesse().x-LINE_NAME_COLUMN_WIDTH)/2, max(len2,len) );
		number_to_string(ctmp, capacity, 2);
		buf.clear();
		buf.printf( translator::translate("Capacity: %s\nLoad: %d (%d%%)"), ctmp, load, loadfactor );
		display_multiline_text(pos.x + LINE_NAME_COLUMN_WIDTH + rest_width, pos.y+16 + 14 + SCL_HEIGHT + D_BUTTON_HEIGHT*2 +4 , buf, COL_BLACK);
	}
}


void schedule_list_gui_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse(groesse);

	int rest_width = get_fenstergroesse().x-LINE_NAME_COLUMN_WIDTH;
	int button_per_row=max(1,rest_width/(D_BUTTON_WIDTH+D_H_SPACE));
	int button_rows= MAX_LINE_COST/button_per_row + ((MAX_LINE_COST%button_per_row)!=0);

	scrolly_convois.set_groesse( koord(rest_width, get_client_windowsize().y-scrolly_convois.get_pos().y) );
	scrolly_haltestellen.set_groesse( koord(LINE_NAME_COLUMN_WIDTH-4, get_client_windowsize().y-scrolly_haltestellen.get_pos().y) );

	chart.set_groesse(koord(rest_width-58, SCL_HEIGHT-11-14-(button_rows*(D_BUTTON_HEIGHT+D_H_SPACE))));
	inp_name.set_groesse(koord(rest_width-8, 14));
	filled_bar.set_groesse(koord(rest_width-8, 4));

	int y=SCL_HEIGHT-11-(button_rows*(D_BUTTON_HEIGHT+D_H_SPACE))+14;
	for (int i=0; i<MAX_LINE_COST; i++) {
		filterButtons[i].set_pos( koord(LINE_NAME_COLUMN_WIDTH+(i%button_per_row)*(D_BUTTON_WIDTH+D_H_SPACE),y+(i/button_per_row)*(D_BUTTON_HEIGHT+D_H_SPACE))  );
	}
}


void schedule_list_gui_t::build_line_list(int filter)
{
	const sint32 sb_offset = line.is_bound() ? scl.get_sb_offset() : 0;
	sint32 sel = -1;
	sp->simlinemgmt.sort_lines();	// to take care of renaming ...
	scl.clear_elements();
	sp->simlinemgmt.get_lines(tabs_to_lineindex[filter], &lines);
	vector_tpl<line_scrollitem_t *>selected_lines;

	FOR(vector_tpl<linehandle_t>, const l, lines) {
		// search name
		if (strstr(l->get_name(), schedule_filter))
			selected_lines.append(new line_scrollitem_t(l));
	}

	std::sort(selected_lines.begin(),selected_lines.end(),compare_lines);

	FOR(vector_tpl<line_scrollitem_t*>, const i, selected_lines) {
		scl.append_element(i);
		if (line == i->get_line()) {
			sel = scl.get_count() - 1;
		}
	}

	scl.set_sb_offset( sb_offset );
	if(  sel>=0  ) {
		scl.set_selection( sel );
		scl.show_selection( sel );
	}

	old_line_count = sp->simlinemgmt.get_line_count();
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
		int i, icnv = 0;
		icnv = new_line->count_convoys();
		// display convoys of line
		cont.remove_all();
		int ypos = 0;
		for(i = 0;  i<icnv;  i++  ) {
			gui_convoiinfo_t* const cinfo = new gui_convoiinfo_t(new_line->get_convoy(i));
			cinfo->set_pos(koord(0, ypos));
			cinfo->set_groesse(koord(400, 40));
			cont.add_komponente(cinfo);
			ypos += 40;
		}
		cont.set_groesse(koord(500, ypos));

		bt_delete_line.disable();
		add_komponente(&bt_withdraw_line);
		bt_withdraw_line.disable();
		if(icnv>0) {
			bt_withdraw_line.enable();
		}
		else {
			bt_delete_line.enable();
		}
		bt_change_line.enable();

		bt_withdraw_line.pressed = new_line->get_withdraw();

		// fill haltestellen container with info of line's haltestellen
		cont_haltestellen.remove_all();
		ypos = 0;
		FOR(minivec_tpl<linieneintrag_t>, const& i, new_line->get_schedule()->eintrag) {
			halthandle_t const halt = haltestelle_t::get_halt(sp->get_welt(), i.pos, sp);
			if (halt.is_bound()) {
				halt_list_stats_t* cinfo = new halt_list_stats_t(halt);
				cinfo->set_pos(koord(0, ypos));
				cinfo->set_groesse(koord(500, 28));
				cont_haltestellen.add_komponente(cinfo);
				ypos += 28;
			}
		}
		cont_haltestellen.set_groesse(koord(500, ypos));

		// chart
		chart.remove_curves();
		for(i=0; i<MAX_LINE_COST; i++)  {
			chart.add_curve(cost_type_color[i], new_line->get_finance_history(), MAX_LINE_COST, statistic[i], MAX_MONTHS, statistic_type[i], filterButtons[i].pressed, true, statistic_type[i]*2 );
			if(filterButtons[i].pressed) {
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
		cont.remove_all();
		inp_name.set_visible(false);
		filled_bar.set_visible(false);
		scrolly_convois.set_visible(false);
		scrolly_haltestellen.set_visible(false);
		inp_name.set_visible(false);
		filled_bar.set_visible(false);
		scl.set_selection(-1);
		bt_delete_line.disable();
		bt_change_line.disable();
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
	return magic_line_management_t+sp->get_player_nr();
}


void schedule_list_gui_t::rdwr( loadsave_t *file )
{
	koord gr;
	sint32 cont_xoff, cont_yoff, halt_xoff, halt_yoff;
	if(  file->is_saving()  ) {
		gr = get_fenstergroesse();
		cont_xoff = scrolly_convois.get_scroll_x();
		cont_yoff = scrolly_convois.get_scroll_y();
		halt_xoff = scrolly_haltestellen.get_scroll_x();
		halt_yoff = scrolly_haltestellen.get_scroll_y();
	}
	gr.rdwr( file );
	simline_t::rdwr_linehandle_t(file, line);
	for (int i=0; i<MAX_LINE_COST; i++) {
		bool b = filterButtons[i].pressed;
		file->rdwr_bool( b );
		filterButtons[i].pressed = b;
	}
	file->rdwr_long( cont_xoff );
	file->rdwr_long( cont_yoff );
	file->rdwr_long( halt_xoff );
	file->rdwr_long( halt_yoff );
	// open dialoge
	if(  file->is_loading()  ) {
		show_lineinfo( line );
		set_fenstergroesse( gr );
		resize( koord(0,0) );
		scrolly_convois.set_scroll_position( cont_xoff, cont_yoff );
		scrolly_haltestellen.set_scroll_position( halt_xoff, halt_yoff );
	}
}
