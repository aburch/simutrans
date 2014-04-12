/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "money_frame.h"
#include "ai_option_t.h"

#include "../simworld.h"
#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../simcolor.h"
#include "../gui/simwin.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/scenario.h"
#include "../dataobj/loadsave.h"

// for headquarter construction only ...
#include "../simmenu.h"
#include "../bauer/hausbauer.h"


// remembers last settings
static uint32 bFilterStates[MAX_PLAYER_COUNT];

#define COST_BALANCE    10 // bank balance

#define BUTTONSPACE 14

// @author hsiegeln
const char *money_frame_t::cost_type_name[MAX_PLAYER_COST_BUTTON] =
{
	"Transported",
	"Revenue",
	"Operation",
	"Maintenance",
	"Road toll",
	"Ops Profit",
	"New Vehicles",
	"Construction_Btn",
	"Gross Profit",
	"Cash",
	"Assets",
	"Margin (%)",
	"Net Wealth"
};


const COLOR_VAL money_frame_t::cost_type_color[MAX_PLAYER_COST_BUTTON] =
{
	COL_TRANSPORTED,
	COL_REVENUE,
	COL_OPERATION,
	COL_MAINTENANCE,
	COL_TOLL,
	COL_OPS_PROFIT,
	COL_NEW_VEHICLES,
	COL_CONSTRUCTION,
	COL_PROFIT,
	COL_CASH,
	COL_VEHICLE_ASSETS,
	COL_MARGIN,
	COL_WEALTH
};


const uint8 money_frame_t::cost_type[3*MAX_PLAYER_COST_BUTTON] =
{
	ATV_TRANSPORTED,                TT_ALL, STANDARD, // all transported goods
	ATV_REVENUE_TRANSPORT,          TT_ALL, MONEY,    // Income
	ATV_RUNNING_COST,               TT_ALL, MONEY,    // Vehicle running costs
	ATV_INFRASTRUCTURE_MAINTENANCE, TT_ALL, MONEY,    // Upkeep
	ATV_WAY_TOLL,                   TT_ALL, MONEY,
	ATV_OPERATING_PROFIT,           TT_ALL, MONEY,
	ATV_NEW_VEHICLE,                TT_ALL, MONEY,   // New vehicles
	ATV_CONSTRUCTION_COST,	        TT_ALL, MONEY,   // Construction
	ATV_PROFIT,                     TT_ALL, MONEY,
	ATC_CASH,                       TT_MAX, MONEY,   // Cash
	ATV_NON_FINANCIAL_ASSETS,       TT_ALL, MONEY,   // value of all vehicles and buildings
	ATV_PROFIT_MARGIN,              TT_ALL, STANDARD,
	ATC_NETWEALTH,                  TT_MAX, MONEY,   // Total Cash + Assets
};


/* order has to be same as in enum transport_type in file simtypes.h */
const char * money_frame_t::transport_type_values[TT_MAX] = {
	"All",
	"Truck",
	"Train",
	"Ship",
	"Monorail",
	"Maglev",
	"Tram",
	"Narrowgauge",
	"Air",
	"tt_Other",
	"Powerlines",
};

/// Helper method to query data from players statistics
sint64 money_frame_t::get_statistics_value(int tt, uint8 type, int yearmonth, bool monthly)
{
	const finance_t* finance = sp->get_finance();
	if (tt == TT_MAX) {
		return monthly ? finance->get_history_com_month(yearmonth, type)
		               : finance->get_history_com_year( yearmonth, type);
	}
	else {
		assert(0 <= tt  &&  tt < TT_MAX);
		return monthly ? finance->get_history_veh_month((transport_type)tt, yearmonth, type)
		               : finance->get_history_veh_year( (transport_type)tt, yearmonth, type);
	}
}


/**
 * Updates label text and color.
 * @param buf used to save the string
 * @param transport_type transport type to query statistics for (or TT_ALL)
 * @param type value from accounting_type_common (if transport_type==TT_ALL) or accounting_type_vehicles
 * @param yearmonth how many months/years back in history (0 == current)
 * @param label_type MONEY or STANDARD
 */
void money_frame_t::update_label(gui_label_t &label, char *buf, int transport_type, uint8 type, int yearmonth, int label_type)
{
	sint64 value = get_statistics_value(transport_type, type, yearmonth, year_month_tabs.get_active_tab_index()==1);
	int color = value >= 0 ? (value > 0 ? MONEY_PLUS : COL_YELLOW) : MONEY_MINUS;

	if (label_type == MONEY) {
		const double cost = value / 100.0;
		money_to_string(buf, cost );
	}
	else {
		const double cost = value / 1.0;
		money_to_string(buf, cost );
		buf[strlen(buf)-4] = 0;	// remove comma
	}

	label.set_text(buf,false);
	label.set_color(color);
}


void money_frame_t::fill_chart_tables()
{
	// fill tables for chart curves
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const uint8 tt = cost_type[3*i+1] == TT_ALL ? transport_type_option : cost_type[3*i+1];

		for(int j=0; j<MAX_PLAYER_HISTORY_MONTHS; j++) {
			chart_table_month[j][i] = get_statistics_value(tt, cost_type[3*i], j, true);
		}

		for(int j=0; j<MAX_PLAYER_HISTORY_YEARS; j++) {
			chart_table_year[j][i] =  get_statistics_value(tt, cost_type[3*i], j, false);
		}
	}
}


bool money_frame_t::is_chart_table_zero(int ttoption)
{
	if (sp->get_finance()->get_maintenance_with_bits((transport_type)ttoption) != 0) {
		return false;
	}
	// search for any non-zero values
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const uint8 tt = cost_type[3*i+1] == TT_ALL ? ttoption : cost_type[3*i+1];

		if (tt == TT_MAX  &&  ttoption != TT_ALL) {
			continue;
		}

		for(int j=0; j<MAX_PLAYER_HISTORY_MONTHS; j++) {
			if (get_statistics_value(tt, cost_type[3*i], j, true) != 0) {
				return false;
			}
		}

		for(int j=0; j<MAX_PLAYER_HISTORY_YEARS; j++) {
			if (get_statistics_value(tt, cost_type[3*i], j, false) != 0) {
				return false;
			}
		}
	}
	return true;
}


money_frame_t::money_frame_t(spieler_t *sp)
  : gui_frame_t( translator::translate("Finanzen"), sp),
		tylabel("This Year", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		lylabel("Last Year", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		conmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		nvmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		vrmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		imoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		tmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		mmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		omoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_conmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_nvmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_vrmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_imoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_tmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_mmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_omoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		tylabel2("This Year", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		gtmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		vtmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		money(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		margin(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		transport(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		old_transport(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		toll(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_toll(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		maintenance_label("This Month",SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		maintenance_money(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		warn("", COL_YELLOW, gui_label_t::left),
		scenario("", COL_BLACK, gui_label_t::left),
		transport_type_option(0),
		headquarter_view(koord3d::invalid, scr_size(120, 64))
{
	if(welt->get_spieler(0)!=sp) {
		sprintf(money_frame_title,translator::translate("Finances of %s"),translator::translate(sp->get_name()) );
		set_name(money_frame_title);
	}

	this->sp = sp;

	const scr_coord_val top =  30;
	const scr_coord_val left = 12;

	const scr_coord_val tyl_x = left+140+55;
	const scr_coord_val lyl_x = left+240+55;

	// left column
	tylabel.set_pos(scr_coord(left+120,top-1*BUTTONSPACE));
	tylabel.set_width(tyl_x-left-120+25);
	lylabel.align_to(&tylabel, ALIGN_LEFT | ALIGN_EXTERIOR_H | ALIGN_TOP);
	lylabel.set_width(lyl_x+25-lylabel.get_pos().x);

	//transport.set_pos(scr_coord(tyl_x+19, top+0*BUTTONSPACE));
	transport.align_to(&tylabel,ALIGN_LEFT,scr_coord(0,top));
	transport.set_width(tylabel.get_size().w);

	//old_transport.set_pos(scr_coord(lyl_x+19, top+0*BUTTONSPACE));
	old_transport.align_to(&lylabel,ALIGN_LEFT,scr_coord(0,top));
	old_transport.set_width(lylabel.get_size().w);

	imoney.set_pos(scr_coord(tyl_x,top+1*BUTTONSPACE));
	old_imoney.set_pos(scr_coord(lyl_x,top+1*BUTTONSPACE));
	vrmoney.set_pos(scr_coord(tyl_x,top+2*BUTTONSPACE));
	old_vrmoney.set_pos(scr_coord(lyl_x,top+2*BUTTONSPACE));
	mmoney.set_pos(scr_coord(tyl_x,top+3*BUTTONSPACE));
	old_mmoney.set_pos(scr_coord(lyl_x,top+3*BUTTONSPACE));
	toll.set_pos(scr_coord(tyl_x,top+4*BUTTONSPACE));
	old_toll.set_pos(scr_coord(lyl_x,top+4*BUTTONSPACE));
	omoney.set_pos(scr_coord(tyl_x,top+5*BUTTONSPACE));
	old_omoney.set_pos(scr_coord(lyl_x,top+5*BUTTONSPACE));
	nvmoney.set_pos(scr_coord(tyl_x,top+6*BUTTONSPACE));
	old_nvmoney.set_pos(scr_coord(lyl_x,top+6*BUTTONSPACE));
	conmoney.set_pos(scr_coord(tyl_x,top+7*BUTTONSPACE));
	old_conmoney.set_pos(scr_coord(lyl_x,top+7*BUTTONSPACE));
	tmoney.set_pos(scr_coord(tyl_x,top+8*BUTTONSPACE));
	old_tmoney.set_pos(scr_coord(lyl_x,top+8*BUTTONSPACE));

	// right column
	maintenance_label.set_pos(scr_coord(left+340+80-maintenance_label.get_size().w, top+2*BUTTONSPACE-2));
	maintenance_money.set_pos(scr_coord(left+340+55, top+3*BUTTONSPACE));

	tylabel2.set_pos(scr_coord(left+140+80+335-tylabel2.get_size().w,top+4*BUTTONSPACE-2));
	gtmoney.set_pos(scr_coord(left+140+335+55, top+5*BUTTONSPACE));
	vtmoney.set_pos(scr_coord(left+140+335+55, top+6*BUTTONSPACE));
	margin.set_pos(scr_coord(left+140+335+55, top+7*BUTTONSPACE));
	money.set_pos(scr_coord(left+140+335+55, top+8*BUTTONSPACE));

	// return money or else stuff ...
	warn.set_pos(scr_coord(left+335, top+9*BUTTONSPACE));
	if(sp->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		scenario.set_pos( scr_coord( 10,1 ) );
		welt->get_scenario()->update_scenario_texts();
		scenario.set_text( welt->get_scenario()->description_text );
		add_komponente(&scenario);
	}

	//CHART YEAR
	chart.set_pos(scr_coord(104,top+10*BUTTONSPACE+11));
	chart.set_size(scr_size(457,120));
	chart.set_dimension(MAX_PLAYER_HISTORY_YEARS, 10000);
	chart.set_seed(welt->get_last_year());
	chart.set_background(MN_GREY1);
	//CHART YEAR END

	//CHART MONTH
	mchart.set_pos(scr_coord(104,top+10*BUTTONSPACE+11));
	mchart.set_size(scr_size(457,120));
	mchart.set_dimension(MAX_PLAYER_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(MN_GREY1);
	//CHART MONTH END

	// add chart curves
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const int curve_type = cost_type[3*i+2];
		const int curve_precision = curve_type == MONEY ? 2 : 0;
		mchart.add_curve( cost_type_color[i], *chart_table_month, MAX_PLAYER_COST_BUTTON, i, MAX_PLAYER_HISTORY_MONTHS, curve_type, false, true, curve_precision);
		chart.add_curve(  cost_type_color[i], *chart_table_year,  MAX_PLAYER_COST_BUTTON, i, MAX_PLAYER_HISTORY_YEARS,  curve_type, false, true, curve_precision);
	}

	// tab (month/year)
	year_month_tabs.add_tab( &year_dummy, translator::translate("Years"));
	year_month_tabs.add_tab( &month_dummy, translator::translate("Months"));
	year_month_tabs.set_pos(scr_coord(0, LINESPACE-2));
	year_month_tabs.set_size(scr_size(lyl_x+25, D_TAB_HEADER_HEIGHT));
	add_komponente(&year_month_tabs);

	add_komponente(&conmoney);
	add_komponente(&nvmoney);
	add_komponente(&vrmoney);
	add_komponente(&mmoney);
	add_komponente(&imoney);
	add_komponente(&tmoney);
	add_komponente(&omoney);
	add_komponente(&toll);
	add_komponente(&transport);

	add_komponente(&old_conmoney);
	add_komponente(&old_nvmoney);
	add_komponente(&old_vrmoney);
	add_komponente(&old_mmoney);
	add_komponente(&old_imoney);
	add_komponente(&old_tmoney);
	add_komponente(&old_omoney);
	add_komponente(&old_toll);
	add_komponente(&old_transport);

	add_komponente(&lylabel);
	add_komponente(&tylabel);

	add_komponente(&tylabel2);
	add_komponente(&gtmoney);
	add_komponente(&vtmoney);
	add_komponente(&money);
	add_komponente(&margin);

	add_komponente(&maintenance_label);
	add_komponente(&maintenance_money);

	add_komponente(&warn);

	add_komponente(&chart);
	add_komponente(&mchart);

	// easier headquarter access
	old_level = sp->get_headquarter_level();
	old_pos = sp->get_headquarter_pos();
	headquarter_tooltip[0] = 0;

	if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
		// misuse headquarter button for AI configure
		headquarter.init(button_t::box, "Configure AI", scr_coord(582-12-120, 0), scr_size(120, BUTTONSPACE));
		headquarter.add_listener(this);
		add_komponente(&headquarter);
		headquarter.set_tooltip( "Configure AI setttings" );
	}
	else if(old_level > 0  ||  hausbauer_t::get_headquarter(0,welt->get_timeline_year_month())!=NULL) {

		headquarter.init(button_t::box, old_pos!=koord::invalid ? "upgrade HQ" : "build HQ", scr_coord(582-12-120, 0), scr_size(120, BUTTONSPACE));
		headquarter.add_listener(this);
		add_komponente(&headquarter);
		headquarter.set_tooltip( NULL );
		headquarter.disable();

		// reuse tooltip from wkz_headquarter_t
		const char * c = werkzeug_t::general_tool[WKZ_HEADQUARTER]->get_tooltip(sp);
		if(c) {
			// only true, if the headquarter can be built/updated
			tstrncpy(headquarter_tooltip, c, lengthof(headquarter_tooltip));
			headquarter.set_tooltip( headquarter_tooltip );
			headquarter.enable();
		}
	}

	if(old_pos!=koord::invalid) {
		headquarter_view.set_location( welt->lookup_kartenboden( sp->get_headquarter_pos() )->get_pos() );
	}
	headquarter_view.set_pos( scr_coord(582-12-120, BUTTONSPACE) );
	add_komponente(&headquarter_view);

	// add filter buttons
	for(int ibutton=0;  ibutton<9;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], scr_coord(left, top+ibutton*BUTTONSPACE-2), scr_size(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background_color = cost_type_color[ibutton];
		add_komponente(filterButtons + ibutton);
	}
	for(int ibutton=9;  ibutton<MAX_PLAYER_COST_BUTTON;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], scr_coord(left+335, top+(ibutton-4)*BUTTONSPACE-2), scr_size(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background_color = cost_type_color[ibutton];
		add_komponente(filterButtons + ibutton);
	}

	// states ...
	for ( int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		if (bFilterStates[sp->get_player_nr()] & (1<<i)) {
			chart.show_curve(i);
			mchart.show_curve(i);
		}
		else {
			chart.hide_curve(i);
			mchart.hide_curve(i);
		}
	}

	transport_type_c.set_pos( scr_coord(left+335-12-2, 0) );
	transport_type_c.set_size( scr_size(116,D_BUTTON_HEIGHT) );
	transport_type_c.set_max_size( scr_size( 116, 1*BUTTONSPACE ) );
	for(int i=0, count=0; i<TT_MAX; ++i) {
		if (!is_chart_table_zero(i)) {
			transport_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(transport_type_values[i]), COL_BLACK));
			transport_types[ count++ ] = i;
		}
	}
	transport_type_c.set_selection(0);
	transport_type_c.set_focusable( false );
	add_komponente(&transport_type_c);
	transport_type_c.add_listener( this );


	set_windowsize(scr_size(582, 340));
}


void money_frame_t::draw(scr_coord pos, scr_size size)
{
	// Hajo: each label needs its own buffer
	static char str_buf[26][64];

	sp->get_finance()->calc_finance_history();
	fill_chart_tables();

	chart.set_visible( year_month_tabs.get_active_tab_index()==0 );
	mchart.set_visible( year_month_tabs.get_active_tab_index()==1 );

	tylabel.set_text( year_month_tabs.get_active_tab_index() ? "This Month" : "This Year", false );
	lylabel.set_text( year_month_tabs.get_active_tab_index() ? "Last Month" : "Last Year", false );

	// update_label(gui_label_t &label, char *buf, int transport_type, uint8 type, int yearmonth)
	update_label(conmoney, str_buf[0], transport_type_option, ATV_CONSTRUCTION_COST, 0);
	update_label(nvmoney,  str_buf[1], transport_type_option, ATV_NEW_VEHICLE, 0);
	update_label(vrmoney,  str_buf[2], transport_type_option, ATV_RUNNING_COST, 0);
	update_label(mmoney,   str_buf[3], transport_type_option, ATV_INFRASTRUCTURE_MAINTENANCE, 0);
	update_label(imoney,   str_buf[4], transport_type_option, ATV_REVENUE_TRANSPORT, 0);
	update_label(tmoney,   str_buf[5], transport_type_option, ATV_PROFIT, 0);
	update_label(omoney,   str_buf[6], transport_type_option, ATV_OPERATING_PROFIT, 0);

	update_label(old_conmoney, str_buf[7], transport_type_option, ATV_CONSTRUCTION_COST, 1);
	update_label(old_nvmoney,  str_buf[8], transport_type_option, ATV_NEW_VEHICLE, 1);
	update_label(old_vrmoney,  str_buf[9], transport_type_option, ATV_RUNNING_COST, 1);
	update_label(old_mmoney,   str_buf[10], transport_type_option, ATV_INFRASTRUCTURE_MAINTENANCE, 1);
	update_label(old_imoney,   str_buf[11], transport_type_option, ATV_REVENUE_TRANSPORT, 1);
	update_label(old_tmoney,   str_buf[12], transport_type_option, ATV_PROFIT, 1);
	update_label(old_omoney,   str_buf[13], transport_type_option, ATV_OPERATING_PROFIT, 1);

	// transported goods
	update_label(transport,     str_buf[20], transport_type_option, ATV_TRANSPORTED, 0, STANDARD);
	update_label(old_transport, str_buf[21], transport_type_option, ATV_TRANSPORTED, 1, STANDARD);

	update_label(toll,     str_buf[24], transport_type_option, ATV_WAY_TOLL, 0);
	update_label(old_toll, str_buf[25], transport_type_option, ATV_WAY_TOLL, 1);

	update_label(gtmoney, str_buf[14], TT_MAX, ATC_CASH, 0);
	update_label(vtmoney, str_buf[17], transport_type_option, ATV_NON_FINANCIAL_ASSETS, 0);

	update_label(money, str_buf[18], TT_MAX, ATC_NETWEALTH, 0);
	update_label(margin, str_buf[19], transport_type_option, ATV_PROFIT_MARGIN, 0);
	str_buf[19][strlen(str_buf[19])-1] = '%';	// remove cent sign

	// warning/success messages
	if(sp->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		warn.set_color( COL_BLACK );
		sint32 percent = welt->get_scenario()->get_completion(sp->get_player_nr());
		if (percent >= 0) {
			sprintf( str_buf[15], translator::translate("Scenario complete: %i%%"), percent );
		}
		else {
			tstrncpy(str_buf[15], translator::translate("Scenario lost!"), lengthof(str_buf[15]) );
		}
	}
	else if(sp->get_finance()->get_history_com_year(0, ATC_NETWEALTH)<0) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[15], translator::translate("Company bankrupt"), lengthof(str_buf[15]) );
	}
	else if(  sp->get_finance()->get_history_com_year(0, ATC_NETWEALTH)*10 < welt->get_settings().get_starting_money(welt->get_current_month()/12)  ){
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[15], translator::translate("Net wealth near zero"), lengthof(str_buf[15]) );
	}
	else if(  sp->get_account_overdrawn()  ) {
		warn.set_color( COL_YELLOW );
		sprintf( str_buf[15], translator::translate("On loan since %i month(s)"), sp->get_account_overdrawn() );
	}
	else {
		str_buf[15][0] = '\0';
	}
	warn.set_text(str_buf[15]);

	// scenario description
	if(sp->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		dynamic_string& desc = welt->get_scenario()->description_text;
		if (desc.has_changed()) {
			scenario.set_text( desc );
			desc.clear_changed();
		}
	}

	headquarter.disable();
	if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
		headquarter.set_tooltip( "Configure AI setttings" );
		headquarter.set_text( "Configure AI" );
		headquarter.enable();
	}
	else if(sp!=welt->get_active_player()) {
		headquarter.set_tooltip( NULL );
	}
	else {
		// I am on my own => allow upgrading
		if(old_level!=sp->get_headquarter_level()) {

			// reuse tooltip from wkz_headquarter_t
			const char * c = werkzeug_t::general_tool[WKZ_HEADQUARTER]->get_tooltip(sp);
			if(c) {
				// only true, if the headquarter can be built/updated
				tstrncpy(headquarter_tooltip, c, lengthof(headquarter_tooltip));
				headquarter.set_tooltip( headquarter_tooltip );
			}
			else {
				headquarter_tooltip[0] = 0;
				headquarter.set_tooltip( NULL );
			}
		}
		// HQ construction still possible ...
		if(  headquarter_tooltip[0]  ) {
			headquarter.enable();
		}
	}

	if(old_level!=sp->get_headquarter_level()  ||  old_pos!=sp->get_headquarter_pos()) {
		if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
			headquarter.set_text( sp->get_headquarter_pos()!=koord::invalid ? "upgrade HQ" : "build HQ" );
		}
		remove_komponente(&headquarter_view);
		old_level = sp->get_headquarter_level();
		old_pos = sp->get_headquarter_pos();
		if(  old_pos!=koord::invalid  ) {
			headquarter_view.set_location( welt->lookup_kartenboden(old_pos)->get_pos() );
			add_komponente(&headquarter_view);
		}
	}

	// Hajo: Money is counted in credit cents (100 cents = 1 Cr)
	money_to_string(str_buf[16],
		(double)((sint64)sp->get_finance()->get_maintenance_with_bits((transport_type)transport_type_option))/100.0
	);
	maintenance_money.set_text(str_buf[16]);
	maintenance_money.set_color(sp->get_finance()->get_maintenance((transport_type)transport_type_option)>=0?MONEY_PLUS:MONEY_MINUS);

	for (int i = 0;  i<MAX_PLAYER_COST_BUTTON;  i++) {
		filterButtons[i].pressed = ( (bFilterStates[sp->get_player_nr()]&(1<<i)) != 0 );
		// year_month_toggle.pressed = mchart.is_visible();
	}

	// Hajo: update chart seed
	chart.set_seed(welt->get_last_year());

	gui_frame_t::draw(pos, size);
}


bool money_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp==&headquarter) {
		if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
			create_win( new ai_option_t(sp), w_info, magic_ai_options_t+sp->get_player_nr() );
		}
		else {
			welt->set_werkzeug( werkzeug_t::general_tool[WKZ_HEADQUARTER], sp );
		}
		return true;
	}

	for ( int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		if (komp == &filterButtons[i]) {
			bFilterStates[sp->get_player_nr()] ^= (1<<i);
			if (bFilterStates[sp->get_player_nr()] & (1<<i)) {
				chart.show_curve(i);
				mchart.show_curve(i);
			}
			else {
				chart.hide_curve(i);
				mchart.hide_curve(i);
			}
			return true;
		}
	}
	if(komp == &transport_type_c) {
		int tmp = transport_type_c.get_selection();
		if((0 <= tmp) && (tmp < transport_type_c.count_elements())) {
			transport_type_option = transport_types[tmp];
		}
		return true;
	}
	return false;
}


uint32 money_frame_t::get_rdwr_id()
{
	return magic_finances_t+sp->get_player_nr();
}


void money_frame_t::rdwr( loadsave_t *file )
{
	bool monthly = mchart.is_visible();;
	file->rdwr_bool( monthly );

	// button state already collected
	file->rdwr_long( bFilterStates[sp->get_player_nr()] );

	if(  file->is_loading()  ) {
		// states ...
		for( uint32 i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
			if (bFilterStates[sp->get_player_nr()] & (1<<i)) {
				chart.show_curve(i);
				mchart.show_curve(i);
			}
			else {
				chart.hide_curve(i);
				mchart.hide_curve(i);
			}
		}
		year_month_tabs.set_active_tab_index( monthly );
	}
}
