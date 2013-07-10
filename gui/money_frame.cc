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
#include "../simgraph.h"
#include "../simcolor.h"
#include "../simwin.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/scenario.h"
#include "../dataobj/loadsave.h"

// for headquarter construction only ...
#include "../simmenu.h"
#include "../bauer/hausbauer.h"


// remebers last settings
static uint32 bFilterStates[MAX_PLAYER_COUNT];

#define COST_BALANCE    10 // bank balance

#define BUTTONSPACE 14

#define COLUMN_TWO_START 10

// @author hsiegeln
const char *money_frame_t::cost_type_name[MAX_PLAYER_COST_BUTTON] =
{
	"Revenue",
	"Operation",
	"Maintenance",
	"Road toll",
	"Ops Profit",
	"New Vehicles",
	"Construction_Btn",
	"Interest",
	"Gross Profit",
	"Transported",
	"Cash",
	"Assets",
	"Net Wealth",
	"Credit Limit",
	"Solvency Limit",
	"Margin (%)"
};

const char money_frame_t::cost_tooltip[MAX_PLAYER_COST_BUTTON][256] =
{
  "Gross revenue",
  "Vehicle running costs (both fixed and distance related)",
  "Recurring expenses of infrastructure maintenance", 
  "The charges incurred or revenues earned by running on other players' ways",
  "Operating revenue less operating expenditure", 
  "Capital expenditure on vehicle purchases and upgrades", 	
  "Capital expenditure on infrastructure", 
  "Cost of overdraft interest payments", 
  "Total income less total expenditure", 
  "Number of units of passengers and goods transported", 
  "Total liquid assets", 
  "Total capital assets, excluding liabilities", 
  "Total assets less total liabilities", 
  "The maximum amount that can be borrowed without prohibiting further capital outlays",
  "The maximum amount that can be borrowed without going bankrupt",
  "Percentage of revenue retained as profit"
};


const COLOR_VAL money_frame_t::cost_type_color[MAX_PLAYER_COST_BUTTON] =
{
	COL_REVENUE,
	COL_OPERATION,
	COL_MAINTENANCE,
	COL_TOLL,
	COL_OPS_PROFIT,
	COL_NEW_VEHICLES,
	COL_CONSTRUCTION,
	COL_INTEREST,
	COL_PROFIT,
	COL_TRANSPORTED,
	COL_CASH,
	COL_VEHICLE_ASSETS,
	COL_WEALTH,
	COL_SOFT_CREDIT_LIMIT,
	COL_HARD_CREDIT_LIMIT,
	COL_MARGIN
};

const uint8 money_frame_t::cost_type[3*MAX_PLAYER_COST_BUTTON] =
{
	ATV_REVENUE_TRANSPORT,          TT_ALL, MONEY,    // Income
	ATV_RUNNING_COST,               TT_ALL, MONEY,    // Vehicle running costs
	ATV_INFRASTRUCTURE_MAINTENANCE, TT_ALL, MONEY,    // Upkeep
	ATV_WAY_TOLL,                   TT_ALL, MONEY,
	ATV_OPERATING_PROFIT,           TT_ALL, MONEY,
	ATV_NEW_VEHICLE,                TT_ALL, MONEY,   // New vehicles
	ATV_CONSTRUCTION_COST,	        TT_ALL, MONEY,   // Construction
	ATC_INTEREST,					TT_MAX, MONEY,	// Interest paid servicing debt
	ATV_PROFIT,                     TT_ALL, MONEY,
	ATV_TRANSPORTED,                TT_ALL, STANDARD, // all transported goods
	ATC_CASH,                       TT_MAX, MONEY,   // Cash
	ATV_NON_FINANCIAL_ASSETS,       TT_ALL, MONEY,   // value of all vehicles and buildings
	ATC_NETWEALTH,                  TT_MAX, MONEY,   // Total Cash + Assets
	ATC_SOFT_CREDIT_LIMIT,			TT_MAX, MONEY,	// Maximum amount that can be borrowed
	ATC_HARD_CREDIT_LIMIT,			TT_MAX, MONEY,	// Borrowing which will lead to bankruptcy
	ATV_PROFIT_MARGIN,              TT_ALL, STANDARD
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

	label.set_text(buf);
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
		tylabel("This Year", COL_WHITE, gui_label_t::right),
		lylabel("Last Year", COL_WHITE, gui_label_t::right),
		conmoney(NULL, COL_WHITE, gui_label_t::money),
		nvmoney(NULL, COL_WHITE, gui_label_t::money),
		vrmoney(NULL, COL_WHITE, gui_label_t::money),
		imoney(NULL, COL_WHITE, gui_label_t::money),
		tmoney(NULL, COL_WHITE, gui_label_t::money),
		mmoney(NULL, COL_WHITE, gui_label_t::money),
		omoney(NULL, COL_WHITE, gui_label_t::money),
		interest(NULL, COL_WHITE, gui_label_t::money),
		old_conmoney(NULL, COL_WHITE, gui_label_t::money),
		old_nvmoney(NULL, COL_WHITE, gui_label_t::money),
		old_vrmoney(NULL, COL_WHITE, gui_label_t::money),
		old_imoney(NULL, COL_WHITE, gui_label_t::money),
		old_tmoney(NULL, COL_WHITE, gui_label_t::money),
		old_mmoney(NULL, COL_WHITE, gui_label_t::money),
		old_omoney(NULL, COL_WHITE, gui_label_t::money),
		old_interest(NULL, COL_WHITE, gui_label_t::money),
		tylabel2("This Year", COL_WHITE, gui_label_t::right),
		cash_money(NULL, COL_WHITE, gui_label_t::money),
		assets(NULL, COL_WHITE, gui_label_t::money),
		net_wealth(NULL, COL_WHITE, gui_label_t::money),
		margin(NULL, COL_WHITE, gui_label_t::money),
		soft_credit_limit(NULL, COL_WHITE, gui_label_t::money),
		hard_credit_limit(NULL, COL_WHITE, gui_label_t::money),
		transport(NULL, COL_WHITE, gui_label_t::right),
		old_transport(NULL, COL_WHITE, gui_label_t::right),
		toll(NULL, COL_WHITE, gui_label_t::money),
		old_toll(NULL, COL_WHITE, gui_label_t::money),
		maintenance_label("Next Month:",COL_WHITE, gui_label_t::right),
		maintenance_label2("Fixed Costs",COL_WHITE, gui_label_t::right),
		maintenance_money(NULL, COL_RED, gui_label_t::money),
		// vehicle_maintenance_money(NULL, COL_RED, gui_label_t::money),
		warn("", COL_YELLOW, gui_label_t::left),
		scenario("", COL_BLACK, gui_label_t::left),
		transport_type_option(0),
		headquarter_view(sp->get_welt(), koord3d::invalid, koord(120, 64))
{
	if(sp->get_welt()->get_spieler(0)!=sp) {
		sprintf(money_frame_title,translator::translate("Finances of %s"),translator::translate(sp->get_name()) );
		set_name(money_frame_title);
	}

	this->sp = sp;

	const int top = 30;
	const int left = 12;

	const sint16 tyl_x = left+140+55;
	const sint16 lyl_x = left+240+55;

	// left column
	tylabel.set_pos(koord(tyl_x+25,top-1*BUTTONSPACE));
	lylabel.set_pos(koord(lyl_x+25,top-1*BUTTONSPACE));

	imoney.set_pos(koord(tyl_x,top+0*BUTTONSPACE));  // revenue
	old_imoney.set_pos(koord(lyl_x,top+0*BUTTONSPACE));
	vrmoney.set_pos(koord(tyl_x,top+1*BUTTONSPACE)); // running costs
	old_vrmoney.set_pos(koord(lyl_x,top+1*BUTTONSPACE));
	mmoney.set_pos(koord(tyl_x,top+2*BUTTONSPACE)); // inf. maintenance
	old_mmoney.set_pos(koord(lyl_x,top+2*BUTTONSPACE));
	toll.set_pos(koord(tyl_x,top+3*BUTTONSPACE));
	old_toll.set_pos(koord(lyl_x,top+3*BUTTONSPACE));
	omoney.set_pos(koord(tyl_x,top+4*BUTTONSPACE)); // op. profit
	old_omoney.set_pos(koord(lyl_x,top+4*BUTTONSPACE));
	nvmoney.set_pos(koord(tyl_x,top+5*BUTTONSPACE)); // new vehicles
	old_nvmoney.set_pos(koord(lyl_x,top+5*BUTTONSPACE));
	conmoney.set_pos(koord(tyl_x,top+6*BUTTONSPACE)); // construction costs
	old_conmoney.set_pos(koord(lyl_x,top+6*BUTTONSPACE));
	interest.set_pos(koord(tyl_x,top+7*BUTTONSPACE)); // interest
	old_interest.set_pos(koord(lyl_x,top+7*BUTTONSPACE));
	tmoney.set_pos(koord(tyl_x,top+8*BUTTONSPACE));  // cash flow
	old_tmoney.set_pos(koord(lyl_x,top+8*BUTTONSPACE));
	transport.set_pos(koord(tyl_x+19, top+9*BUTTONSPACE)); // units transported
	old_transport.set_pos(koord(lyl_x+19, top+9*BUTTONSPACE));

	// right column (upper)
	maintenance_label.set_pos(koord(left+140+335+55, top-1*BUTTONSPACE-2));
	maintenance_label2.set_pos(koord(left+140+335+55, top+0*BUTTONSPACE-2));
	// vehicle maintenance money should be the same height as running costs
	// vehicle_maintenance_money.set_pos(koord(left+140+335+55, top+1*BUTTONSPACE));
	// maintenance money should be the same height as inf. maintenance (mmoney)
	maintenance_money.set_pos(koord(left+140+335+55, top+2*BUTTONSPACE));

	// right column (lower)
	tylabel2.set_pos(koord(left+140+80+335,top+3*BUTTONSPACE-2));
	cash_money.set_pos(koord(left+140+335+55, top+4*BUTTONSPACE));
	assets.set_pos(koord(left+140+335+55, top+5*BUTTONSPACE));
	net_wealth.set_pos(koord(left+140+335+55, top+6*BUTTONSPACE));
	soft_credit_limit.set_pos(koord(left+140+335+55, top+7*BUTTONSPACE));
	hard_credit_limit.set_pos(koord(left+140+335+55, top+8*BUTTONSPACE));
	margin.set_pos(koord(left+140+335+55, top+9*BUTTONSPACE));


	// Scenario and warning location
	warn.set_pos(koord(left+335, top+10*BUTTONSPACE));
	if(sp->get_player_nr()!=1  &&  sp->get_welt()->get_scenario()->active()) {
		scenario.set_pos( koord( 10,1 ) );
		sp->get_welt()->get_scenario()->update_scenario_texts();
		scenario.set_text( sp->get_welt()->get_scenario()->description_text );
		add_komponente(&scenario);
	}

	const int TOP_OF_CHART = top + 11 * BUTTONSPACE + 10; // extra space is for chart labels
	const int HEIGHT_OF_CHART = 120;

	//CHART YEAR
	chart.set_pos(koord(104,TOP_OF_CHART));
	chart.set_groesse(koord(457,HEIGHT_OF_CHART));
	chart.set_dimension(MAX_PLAYER_HISTORY_YEARS, 10000);
	chart.set_seed(sp->get_welt()->get_last_year());
	chart.set_background(MN_GREY1);
	chart.set_ltr(umgebung_t::left_to_right_graphs);
	//CHART YEAR END

	//CHART MONTH
	mchart.set_pos(koord(104,TOP_OF_CHART));
	mchart.set_groesse(koord(457,HEIGHT_OF_CHART));
	mchart.set_dimension(MAX_PLAYER_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(MN_GREY1);
	mchart.set_ltr(umgebung_t::left_to_right_graphs);
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
	year_month_tabs.set_pos(koord(0, LINESPACE-2));
	year_month_tabs.set_groesse(koord(lyl_x+25, gui_tab_panel_t::HEADER_VSIZE));
	add_komponente(&year_month_tabs);

	add_komponente(&conmoney);
	add_komponente(&nvmoney);
	add_komponente(&vrmoney);
	add_komponente(&mmoney);
	add_komponente(&imoney);
	add_komponente(&interest);
	add_komponente(&tmoney);
	add_komponente(&omoney);
	add_komponente(&toll);
	add_komponente(&transport);

	add_komponente(&old_conmoney);
	add_komponente(&old_nvmoney);
	add_komponente(&old_vrmoney);
	add_komponente(&old_mmoney);
	add_komponente(&old_imoney);
	add_komponente(&old_interest);
	add_komponente(&old_tmoney);
	add_komponente(&old_omoney);
	add_komponente(&old_toll);
	add_komponente(&old_transport);
	
	add_komponente(&lylabel);
	add_komponente(&tylabel);

	add_komponente(&tylabel2);
	add_komponente(&cash_money);
	add_komponente(&assets);
	add_komponente(&net_wealth);
	add_komponente(&margin);
	add_komponente(&soft_credit_limit);
	add_komponente(&hard_credit_limit);

	add_komponente(&maintenance_label);
	add_komponente(&maintenance_label2);
	add_komponente(&maintenance_money);
	// add_komponente(&vehicle_maintenance_money);

	add_komponente(&warn);

	add_komponente(&chart);
	add_komponente(&mchart);

	// easier headquarter access
	old_level = sp->get_headquarter_level();
	old_pos = sp->get_headquarter_pos();
	headquarter_tooltip[0] = 0;

	if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
		// misuse headquarter button for AI configure
		headquarter.init(button_t::box, "Configure AI", koord(582-12-120, 0), koord(120, BUTTONSPACE));
		headquarter.add_listener(this);
		add_komponente(&headquarter);
		headquarter.set_tooltip( "Configure AI setttings" );
	}
	else if(old_level > 0  ||  hausbauer_t::get_headquarter(0,sp->get_welt()->get_timeline_year_month())!=NULL) {

		headquarter.init(button_t::box, old_pos!=koord::invalid ? "upgrade HQ" : "build HQ", koord(582-12-120, 0), koord(120, BUTTONSPACE));
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
		headquarter_view.set_location( sp->get_welt()->lookup_kartenboden( sp->get_headquarter_pos() )->get_pos() );
	}
	headquarter_view.set_pos( koord(582-12-120, BUTTONSPACE) );
	add_komponente(&headquarter_view);

	// add filter buttons
	for(int ibutton=0;  ibutton<COLUMN_TWO_START;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], koord(left, top+ibutton*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		filterButtons[ibutton].set_tooltip(cost_tooltip[ibutton]);
		add_komponente(filterButtons + ibutton);
	}
	for(int ibutton=COLUMN_TWO_START;  ibutton<MAX_PLAYER_COST_BUTTON;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], koord(left+335, top+(ibutton-6)*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		filterButtons[ibutton].set_tooltip(cost_tooltip[ibutton]);
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

	transport_type_c.set_pos(koord(koord(left+335-12-2, 0)));
	transport_type_c.set_groesse( koord(116,D_BUTTON_HEIGHT) );
	transport_type_c.set_max_size( koord( 116, 1*BUTTONSPACE ) );
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

	const int WINDOW_HEIGHT = TOP_OF_CHART + HEIGHT_OF_CHART + 10 + BUTTONSPACE * 2 ; // formerly 340
	// The extra room below the chart is for (a) labels, (b) year label (BUTTONSPACE), (c) empty space
	set_fenstergroesse(koord(582, WINDOW_HEIGHT));
}


void money_frame_t::zeichnen(koord pos, koord gr)
{
	// Hajo: each label needs its own buffer
	static char str_buf[35][256];

	sp->get_finance()->calc_finance_history();
	fill_chart_tables();

	chart.set_visible( year_month_tabs.get_active_tab_index()==0 );
	mchart.set_visible( year_month_tabs.get_active_tab_index()==1 );

	tylabel.set_text( year_month_tabs.get_active_tab_index() ? "This Month" : "This Year" );
	lylabel.set_text( year_month_tabs.get_active_tab_index() ? "Last Month" : "Last Year" );

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

	update_label(interest, str_buf[31], TT_MAX, ATC_INTEREST, 0);
	update_label(old_interest, str_buf[32], TT_MAX, ATC_INTEREST, 1);

	update_label(cash_money, str_buf[14], TT_MAX, ATC_CASH, 0);
	update_label(assets, str_buf[17], transport_type_option, ATV_NON_FINANCIAL_ASSETS, 0);
	update_label(net_wealth, str_buf[18], TT_MAX, ATC_NETWEALTH, 0);
	update_label(soft_credit_limit, str_buf[33], TT_MAX, ATC_SOFT_CREDIT_LIMIT, 0);
	update_label(hard_credit_limit, str_buf[34], TT_MAX, ATC_HARD_CREDIT_LIMIT, 0);
	update_label(margin, str_buf[19], transport_type_option, ATV_PROFIT_MARGIN, 0);
	str_buf[19][strlen(str_buf[19])-1] = '%';	// remove cent sign

	// warning/success messages
	if(sp->get_player_nr()!=1  &&  sp->get_welt()->get_scenario()->active()) {
		warn.set_color( COL_BLACK );
		sint32 percent = sp->get_welt()->get_scenario()->get_completion(sp->get_player_nr());
		if (percent >= 0) {
			sprintf( str_buf[15], translator::translate("Scenario complete: %i%%"), percent );
		}
		else {
			tstrncpy(str_buf[15], translator::translate("Scenario lost!"), lengthof(str_buf[15]) );
		}
	}
	else if(sp->get_finance()->get_history_com_month(0, ATC_CASH)<sp->get_finance()->get_history_com_month(0, ATC_HARD_CREDIT_LIMIT)) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[15], translator::translate("Company bankrupt"), lengthof(str_buf[15]) );
	}
	else if(sp->get_finance()->get_history_com_month(0, ATC_CASH)<sp->get_finance()->get_history_com_month(0, ATC_SOFT_CREDIT_LIMIT)) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[15], translator::translate("Credit limit exceeded"), lengthof(str_buf[15]) );
	}
	else if(sp->get_finance()->get_history_com_month(0, ATC_NETWEALTH)<0) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[15], translator::translate("Net wealth negative"), lengthof(str_buf[15]) );
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
	if(sp->get_player_nr()!=1  &&  sp->get_welt()->get_scenario()->active()) {
		dynamic_string& desc = sp->get_welt()->get_scenario()->description_text;
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
	else if(sp!=sp->get_welt()->get_active_player()) {
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
			headquarter_view.set_location( sp->get_welt()->lookup_kartenboden(old_pos)->get_pos() );
			add_komponente(&headquarter_view);
		}
	}

	// Hajo: Money is counted in credit cents (100 cents = 1 Cr)
	sint64 maintenance = sp->get_finance()->get_maintenance_with_bits((transport_type)transport_type_option);
	money_to_string(str_buf[16],
		(double)(maintenance)/100.0
	);
	maintenance_money.set_text(str_buf[16]);
	maintenance_money.set_color(maintenance>=0?MONEY_PLUS:MONEY_MINUS);

	for (int i = 0;  i<MAX_PLAYER_COST_BUTTON;  i++) {
		filterButtons[i].pressed = ( (bFilterStates[sp->get_player_nr()]&(1<<i)) != 0 );
		// year_month_toggle.pressed = mchart.is_visible();
	}

	// Hajo: update chart seed
	chart.set_seed(sp->get_welt()->get_last_year());

	gui_frame_t::zeichnen(pos, gr);
}


bool money_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp==&headquarter) {
		if(  sp->get_ai_id()!=spieler_t::HUMAN  ) {
			create_win( new ai_option_t(sp), w_info, magic_ai_options_t+sp->get_player_nr() );
		}
		else {
			sp->get_welt()->set_werkzeug( werkzeug_t::general_tool[WKZ_HEADQUARTER], sp );
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

	// button state already cooledcted
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
