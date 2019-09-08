/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "money_frame.h"
#include "ai_option_t.h"
#include "simwin.h"

#include "../simworld.h"
#include "../simdebug.h"
#include "../display/simgraph.h"
#include "../simcolor.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../dataobj/scenario.h"
#include "../dataobj/loadsave.h"

// for headquarters construction only ...
#include "../simmenu.h"
#include "../bauer/hausbauer.h"


// remembers last settings
static uint32 bFilterStates[MAX_PLAYER_COUNT];

#define COST_BALANCE    10 // bank balance

#define BUTTONWIDTH 120
#define BUTTONSPACE 14

#define COLUMN_TWO_START 11

// @author hsiegeln
const char *money_frame_t::cost_type_name[MAX_PLAYER_COST_BUTTON] =
{
	"Revenue",
	"Operation",
	"Vehicle maintenance",
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
  "Vehicle running costs per km",
  "Vehicle maintenance costs per month",
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
	COL_VEH_MAINTENANCE,
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
	ATV_VEHICLE_MAINTENANCE,		TT_ALL, MONEY,	  // Vehicle monthly maintenance
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


/* order has to be same as in enum transport_type in file player/finance.h */
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
	const finance_t* finance = player->get_finance();
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
 * @param always_monthly: reference monthly values even when in "year" mode
 */
void money_frame_t::update_label(gui_label_t &label, char *buf, int transport_type, uint8 type, int yearmonth, int label_type, bool always_monthly)
{
	bool monthly = always_monthly || year_month_tabs.get_active_tab_index()==1;
	sint64 value = get_statistics_value(transport_type, type, yearmonth, monthly);
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
	if (player->get_finance()->get_maintenance_with_bits((transport_type)ttoption) != 0) {
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


money_frame_t::money_frame_t(player_t *player)
  : gui_frame_t( translator::translate("Finanzen"), player),
		interest(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_interest(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		cash_money(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		assets(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		net_wealth(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		margin(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		soft_credit_limit(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		hard_credit_limit(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		maintenance_label2("Fixed Costs", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		tylabel("This Year", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		lylabel("Last Year", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		conmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		nvmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		vehicle_maintenance_money(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),	
		vrmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		imoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		tmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		mmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		omoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_conmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_nvmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_vehicle_maintenance_money(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),	
		old_vrmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_imoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_tmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_mmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		old_omoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		tylabel2("This Year", SYSCOL_TEXT_HIGHLIGHT, gui_label_t::right),
		//gtmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		//vtmoney(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
		//money(NULL, SYSCOL_TEXT_HIGHLIGHT, gui_label_t::money),
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
	if(welt->get_player(0)!=player) {
		sprintf(money_frame_title,translator::translate("Finances of %s"),translator::translate(player->get_name()) );
		set_name(money_frame_title);
	}

	this->player = player;

	const scr_coord_val top =  30;
	const scr_coord_val left = D_MARGIN_LEFT;

	// Button components are left-aligned, but number components aren't.
	// Number components are aligned at the *decimal point*,
	// which is 25 to the left of the *right* edge!
	const scr_coord_val  c1_x = left + BUTTONWIDTH + D_H_SPACE - 25; // center column left edge (fixed costs)
	const scr_coord_val tyl_x = c1_x + 1 * 100; // "this month" column numbers
	const scr_coord_val lyl_x = c1_x + 2 * 100;	// "last month" column numbers

	const scr_coord_val c2_x     = lyl_x + D_H_SPACE; // center column left edge (fixed costs)
	const scr_coord_val c2_num_x = c2_x + 100; // center column number alignment

	const scr_coord_val c3_btn_x = c2_num_x + 25 + D_H_SPACE; // right column buttons
	const scr_coord_val c3_num_x = c3_btn_x + BUTTONWIDTH + 100 - 25; // numbers for right column of buttons
	const scr_coord_val WINDOW_WIDTH = c3_num_x + 25 + D_MARGIN_RIGHT;

	const scr_size lbl_size(BUTTONWIDTH, D_LABEL_HEIGHT);
	// left column
	tylabel.set_pos(scr_coord(c1_x,top-1*BUTTONSPACE));
	tylabel.set_size(lbl_size);
	lylabel.set_pos(scr_coord(c1_x+100,top-1*BUTTONSPACE));
	lylabel.set_size(lbl_size);

	imoney.set_pos(scr_coord(tyl_x,top+0*BUTTONSPACE));  // revenue
	old_imoney.set_pos(scr_coord(lyl_x,top+0*BUTTONSPACE));
	vrmoney.set_pos(scr_coord(tyl_x,top+1*BUTTONSPACE)); // running costs
	old_vrmoney.set_pos(scr_coord(lyl_x,top+1*BUTTONSPACE));
	vehicle_maintenance_money.set_pos(scr_coord(tyl_x,top+2*BUTTONSPACE)); 
	old_vehicle_maintenance_money.set_pos(scr_coord(lyl_x,top+2*BUTTONSPACE));
	mmoney.set_pos(scr_coord(tyl_x,top+3*BUTTONSPACE)); // inf. maintenance
	old_mmoney.set_pos(scr_coord(lyl_x,top+3*BUTTONSPACE));
	toll.set_pos(scr_coord(tyl_x,top+4*BUTTONSPACE));
	old_toll.set_pos(scr_coord(lyl_x,top+4*BUTTONSPACE));
	omoney.set_pos(scr_coord(tyl_x,top+5*BUTTONSPACE)); // op. profit
	old_omoney.set_pos(scr_coord(lyl_x,top+5*BUTTONSPACE));
	nvmoney.set_pos(scr_coord(tyl_x,top+6*BUTTONSPACE)); // new vehicles
	old_nvmoney.set_pos(scr_coord(lyl_x,top+6*BUTTONSPACE));
	conmoney.set_pos(scr_coord(tyl_x,top+7*BUTTONSPACE)); // construction costs
	old_conmoney.set_pos(scr_coord(lyl_x,top+7*BUTTONSPACE));
	interest.set_pos(scr_coord(tyl_x,top+8*BUTTONSPACE)); // interest
	old_interest.set_pos(scr_coord(lyl_x,top+8*BUTTONSPACE));
	tmoney.set_pos(scr_coord(tyl_x,top+9*BUTTONSPACE));  // cash flow
	old_tmoney.set_pos(scr_coord(lyl_x,top+9*BUTTONSPACE));
	transport.set_pos(scr_coord(c1_x, top+10*BUTTONSPACE)); // units transported
	transport.set_size(lbl_size);
	old_transport.set_pos(scr_coord(c1_x + 100, top+10*BUTTONSPACE));
	old_transport.set_size(lbl_size);

	
	// center column (above selector box)
	maintenance_label.set_pos(scr_coord(c2_x, top-1*BUTTONSPACE));
	maintenance_label.set_size(lbl_size);
	maintenance_label2.set_pos(scr_coord(c2_x, top+0*BUTTONSPACE));
	maintenance_label2.set_size(lbl_size);
	// vehicle maintenance money should be the same height as running costs
	//vehicle_maintenance_money.set_pos(scr_coord(c2_num_x, top+1*BUTTONSPACE));
	//maintenance money should be the same height as inf. maintenance (mmoney)
	maintenance_money.set_pos(scr_coord(c2_num_x, top+3*BUTTONSPACE));
	

	// right column (lower)
	tylabel2.set_pos(scr_coord(c3_btn_x, top+4*BUTTONSPACE-2));
	cash_money.set_pos(scr_coord(c3_num_x, top+5*BUTTONSPACE));
	assets.set_pos(scr_coord(c3_num_x, top+6*BUTTONSPACE));
	net_wealth.set_pos(scr_coord(c3_num_x, top+7*BUTTONSPACE));
	soft_credit_limit.set_pos(scr_coord(c3_num_x, top+8*BUTTONSPACE));
	hard_credit_limit.set_pos(scr_coord(c3_num_x, top+9*BUTTONSPACE));
	margin.set_pos(scr_coord(c3_num_x, top+10*BUTTONSPACE));


	// Scenario and warning location
	warn.set_pos(scr_coord(c2_x, top+11*BUTTONSPACE));
	if(player->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		scenario.set_pos( scr_coord( 10,1 ) );
		welt->get_scenario()->update_scenario_texts();
		scenario.set_text( welt->get_scenario()->description_text );
		add_component(&scenario);
	}

	const int TOP_OF_CHART = top + 12 * BUTTONSPACE + 11; // extra space is for chart labels
	const int HEIGHT_OF_CHART = 120;

	//CHART YEAR
	chart.set_pos(scr_coord(104,TOP_OF_CHART));
	chart.set_size(scr_size(457,HEIGHT_OF_CHART));
	chart.set_dimension(MAX_PLAYER_HISTORY_YEARS, 10000);
	chart.set_seed(welt->get_last_year());
	chart.set_background(SYSCOL_CHART_BACKGROUND);
	chart.set_ltr(env_t::left_to_right_graphs);
	//CHART YEAR END

	//CHART MONTH
	mchart.set_pos(scr_coord(104,TOP_OF_CHART));
	mchart.set_size(scr_size(457,HEIGHT_OF_CHART));
	mchart.set_dimension(MAX_PLAYER_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(SYSCOL_CHART_BACKGROUND);
	mchart.set_ltr(env_t::left_to_right_graphs);
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
	add_component(&year_month_tabs);

	add_component(&conmoney);
	add_component(&nvmoney);
	add_component(&vrmoney);
	add_component(&vehicle_maintenance_money); 
	add_component(&mmoney);
	add_component(&imoney);
	add_component(&interest);
	add_component(&tmoney);
	add_component(&omoney);
	add_component(&toll);
	add_component(&transport);

	add_component(&old_conmoney);
	add_component(&old_nvmoney);
	add_component(&old_vrmoney);
	add_component(&old_vehicle_maintenance_money); 
	add_component(&old_mmoney);
	add_component(&old_imoney);
	add_component(&old_interest);
	add_component(&old_tmoney);
	add_component(&old_omoney);
	add_component(&old_toll);
	add_component(&old_transport);
	
	add_component(&lylabel);
	add_component(&tylabel);

	add_component(&tylabel2);
	add_component(&cash_money);
	add_component(&assets);
	add_component(&net_wealth);
	add_component(&margin);
	add_component(&soft_credit_limit);
	add_component(&hard_credit_limit);

	add_component(&maintenance_label);
	add_component(&maintenance_label2);
	add_component(&maintenance_money);
	// add_component(&vehicle_maintenance_money);

	add_component(&warn);

	add_component(&chart);
	add_component(&mchart);

	// easier headquarters access
	old_level = player->get_headquarters_level();
	old_pos = player->get_headquarter_pos();
	headquarter_tooltip[0] = 0;

	// Headquarters is at upper right
	if(  player->get_ai_id()!=player_t::HUMAN  ) {
		// misuse headquarters button for AI configure
		headquarters.init(button_t::box, "Configure AI", scr_coord(c3_btn_x, 0), scr_size(BUTTONWIDTH, BUTTONSPACE));
		headquarters.add_listener(this);
		add_component(&headquarters);
		headquarters.set_tooltip( "Configure AI setttings" );
	}
	else if(old_level > 0  ||  hausbauer_t::get_headquarter(0,welt->get_timeline_year_month())!=NULL) {

		headquarters.init(button_t::box, old_pos!=koord::invalid ? "upgrade HQ" : "build HQ", scr_coord(c3_btn_x, 0), scr_size(BUTTONWIDTH, BUTTONSPACE));
		headquarters.add_listener(this);
		add_component(&headquarters);
		headquarters.set_tooltip( NULL );
		headquarters.disable();

		// reuse tooltip from tool_headquarter_t
		const char * c = tool_t::general_tool[TOOL_HEADQUARTER]->get_tooltip(player);
		if(c) {
			// only true, if the headquarters can be built/updated
			tstrncpy(headquarter_tooltip, c, lengthof(headquarter_tooltip));
			headquarters.set_tooltip( headquarter_tooltip );
			headquarters.enable();
		}
	}

	if(old_pos!=koord::invalid) {
		headquarter_view.set_location( welt->lookup_kartenboden( player->get_headquarter_pos() )->get_pos() );
	}
	headquarter_view.set_pos( scr_coord(c3_btn_x, BUTTONSPACE) );
	add_component(&headquarter_view);

	// add filter buttons
	for(int ibutton=0;  ibutton<COLUMN_TWO_START;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], scr_coord(left, top+ibutton*BUTTONSPACE-2), scr_size(BUTTONWIDTH, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background_color = cost_type_color[ibutton];
		filterButtons[ibutton].set_tooltip(cost_tooltip[ibutton]);
		add_component(filterButtons + ibutton);
	}
	for(int ibutton=COLUMN_TWO_START;  ibutton<MAX_PLAYER_COST_BUTTON;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], scr_coord(c3_btn_x, top+(ibutton-6)*BUTTONSPACE-2), scr_size(BUTTONWIDTH, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background_color = cost_type_color[ibutton];
		filterButtons[ibutton].set_tooltip(cost_tooltip[ibutton]);
		add_component(filterButtons + ibutton);
	}

	// states ...
	for ( int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		if (bFilterStates[player->get_player_nr()] & (1<<i)) {
			chart.show_curve(i);
			mchart.show_curve(i);
		}
		else {
			chart.hide_curve(i);
			mchart.hide_curve(i);
		}
	}

	transport_type_c.set_pos( scr_coord(c2_x - 14 - D_H_SPACE, 0) ); // below fixed costs
	transport_type_c.set_size( scr_size( 85 + 14 + 14, D_BUTTON_HEIGHT) ); // width of column plus spacing
	transport_type_c.set_max_size( scr_size( 85 + 14 + 14, TT_MAX * BUTTONSPACE ) );
	for(int i=0, count=0; i<TT_MAX; ++i) {
		if (!is_chart_table_zero(i)) {
			transport_type_c.append_element( new gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(transport_type_values[i]), SYSCOL_TEXT));
			transport_types[ count++ ] = i;
		}
	}
	transport_type_c.set_selection(0);
	transport_type_c.set_focusable( false );
	add_component(&transport_type_c);
	transport_type_c.add_listener( this );

	set_focus( &transport_type_c );

	const int WINDOW_HEIGHT = TOP_OF_CHART + HEIGHT_OF_CHART + 10 + BUTTONSPACE * 2 ; // formerly 340
	// The extra room below the chart is for (a) labels, (b) year label (BUTTONSPACE), (c) empty space
	set_windowsize(scr_size(WINDOW_WIDTH, WINDOW_HEIGHT));
}


void money_frame_t::draw(scr_coord pos, scr_size size)
{
	// Hajo: each label needs its own buffer
	static char str_buf[37][256];

	player->get_finance()->calc_finance_history();
	fill_chart_tables();

	chart.set_visible( year_month_tabs.get_active_tab_index()==0 );
	mchart.set_visible( year_month_tabs.get_active_tab_index()==1 );

	tylabel.set_text( year_month_tabs.get_active_tab_index() ? "This Month" : "This Year", false );
	lylabel.set_text( year_month_tabs.get_active_tab_index() ? "Last Month" : "Last Year", false );

	// update_label(gui_label_t &label, char *buf, int transport_type, uint8 type, int yearmonth)
	update_label(conmoney, str_buf[0], transport_type_option, ATV_CONSTRUCTION_COST, 0);
	update_label(nvmoney,  str_buf[1], transport_type_option, ATV_NEW_VEHICLE, 0);
	update_label(vrmoney,  str_buf[2], transport_type_option, ATV_RUNNING_COST, 0);
	update_label(vehicle_maintenance_money,  str_buf[3], transport_type_option, ATV_VEHICLE_MAINTENANCE, 0);
	update_label(mmoney,   str_buf[4], transport_type_option, ATV_INFRASTRUCTURE_MAINTENANCE, 0);
	update_label(imoney,   str_buf[5], transport_type_option, ATV_REVENUE_TRANSPORT, 0);
	update_label(tmoney,   str_buf[6], transport_type_option, ATV_PROFIT, 0);
	update_label(omoney,   str_buf[7], transport_type_option, ATV_OPERATING_PROFIT, 0);

	update_label(old_conmoney, str_buf[8], transport_type_option, ATV_CONSTRUCTION_COST, 1);
	update_label(old_nvmoney,  str_buf[9], transport_type_option, ATV_NEW_VEHICLE, 1);
	update_label(old_vrmoney,  str_buf[10], transport_type_option, ATV_RUNNING_COST, 1);
	update_label(old_vehicle_maintenance_money,  str_buf[11], transport_type_option, ATV_VEHICLE_MAINTENANCE, 1);
	update_label(old_mmoney,   str_buf[12], transport_type_option, ATV_INFRASTRUCTURE_MAINTENANCE, 1);
	update_label(old_imoney,   str_buf[13], transport_type_option, ATV_REVENUE_TRANSPORT, 1);
	update_label(old_tmoney,   str_buf[14], transport_type_option, ATV_PROFIT, 1);
	update_label(old_omoney,   str_buf[15], transport_type_option, ATV_OPERATING_PROFIT, 1);

	// transported goods
	update_label(transport,     str_buf[22], transport_type_option, ATV_TRANSPORTED, 0, STANDARD);
	update_label(old_transport, str_buf[23], transport_type_option, ATV_TRANSPORTED, 1, STANDARD);

	update_label(toll,     str_buf[26], transport_type_option, ATV_WAY_TOLL, 0);
	update_label(old_toll, str_buf[27], transport_type_option, ATV_WAY_TOLL, 1);

	// It causes confusion when interest shows up with transport types other than "all" or "other".
	// Arguably interest should be registered by transport type (for borrowing against railcars etc.)
	// but that is a complex enhancement which require savefile format changes.
	if (transport_type_option == TT_ALL || transport_type_option == TT_OTHER) {
		update_label(interest, str_buf[33], TT_MAX, ATC_INTEREST, 0);
		update_label(old_interest, str_buf[34], TT_MAX, ATC_INTEREST, 1);
	} else {
		// Print interest as zero.  (Doesn't affect graph, only number entries.)
		money_to_string(str_buf[33], 0 );
		interest.set_text(str_buf[33]);
		interest.set_color(COL_YELLOW);
		money_to_string(str_buf[34], 0 );
		old_interest.set_text(str_buf[34]);
		old_interest.set_color(COL_YELLOW);
	}

	update_label(cash_money, str_buf[16], TT_MAX, ATC_CASH, 0);
	update_label(assets, str_buf[19], transport_type_option, ATV_NON_FINANCIAL_ASSETS, 0);
	update_label(net_wealth, str_buf[20], TT_MAX, ATC_NETWEALTH, 0);

	// To avoid confusion, these should always show the current month's credit limit, even when "year" is selected
	update_label(soft_credit_limit, str_buf[35], TT_MAX, ATC_SOFT_CREDIT_LIMIT, 0, true);
	update_label(hard_credit_limit, str_buf[36], TT_MAX, ATC_HARD_CREDIT_LIMIT, 0, true);

	update_label(margin, str_buf[21], transport_type_option, ATV_PROFIT_MARGIN, 0);
	str_buf[21][strlen(str_buf[21])-1] = '%';	// remove cent sign

	// warning/success messages
	if(player->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		warn.set_color( SYSCOL_TEXT );
		sint32 percent = welt->get_scenario()->get_completion(player->get_player_nr());
		if (percent >= 0) {
			sprintf( str_buf[17], translator::translate("Scenario complete: %i%%"), percent );
		}
		else {
			tstrncpy(str_buf[17], translator::translate("Scenario lost!"), lengthof(str_buf[17]) );
		}
	}
	else if(player->get_finance()->get_history_com_month(0, ATC_CASH)<player->get_finance()->get_history_com_month(0, ATC_HARD_CREDIT_LIMIT)) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[17], translator::translate("Company bankrupt"), lengthof(str_buf[17]) );
	}
	else if(player->get_finance()->get_history_com_month(0, ATC_CASH)<player->get_finance()->get_history_com_month(0, ATC_SOFT_CREDIT_LIMIT)) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[17], translator::translate("Credit limit exceeded"), lengthof(str_buf[17]) );
	}
	else if(  player->get_finance()->get_history_com_year(0, ATC_NETWEALTH)*10 < welt->get_settings().get_starting_money(welt->get_current_month()/12)  ){
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[17], translator::translate("Net wealth near zero"), lengthof(str_buf[17]) );
	}
	else if(  player->get_account_overdrawn()  ) {
		warn.set_color( COL_YELLOW );
		sprintf( str_buf[17], translator::translate("On loan since %i month(s)"), player->get_account_overdrawn() );
	}
	else {
		str_buf[17][0] = '\0';
	}
	warn.set_text(str_buf[17]);

	// scenario description
	if(player->get_player_nr()!=1  &&  welt->get_scenario()->active()) {
		dynamic_string& desc = welt->get_scenario()->description_text;
		if (desc.has_changed()) {
			scenario.set_text( desc );
			desc.clear_changed();
		}
	}

	headquarters.disable();
	if(  player->get_ai_id()!=player_t::HUMAN  ) {
		headquarters.set_tooltip( "Configure AI setttings" );
		headquarters.set_text( "Configure AI" );
		headquarters.enable();
	}
	else if(player!=welt->get_active_player()) {
		headquarters.set_tooltip( NULL );
	}
	else {
		// I am on my own => allow upgrading
		if(old_level!=player->get_headquarters_level()) {

			// reuse tooltip from tool_headquarter_t
			const char * c = tool_t::general_tool[TOOL_HEADQUARTER]->get_tooltip(player);
			if(c) {
				// only true, if the headquarters can be built/updated
				tstrncpy(headquarter_tooltip, c, lengthof(headquarter_tooltip));
				headquarters.set_tooltip( headquarter_tooltip );
			}
			else {
				headquarter_tooltip[0] = 0;
				headquarters.set_tooltip( NULL );
			}
		}
		// HQ construction still possible ...
		if(  headquarter_tooltip[0]  ) {
			headquarters.enable();
		}
	}

	if(old_level!=player->get_headquarters_level()  ||  old_pos!=player->get_headquarter_pos()) {
		if( player->get_ai_id() == player_t::HUMAN ) {
			headquarters.set_text( player->get_headquarter_pos()!=koord::invalid ? "upgrade HQ" : "build HQ" );
		}
		remove_component(&headquarter_view);
		old_level = player->get_headquarters_level();
		old_pos = player->get_headquarter_pos();
		if(  old_pos!=koord::invalid  ) {
			headquarter_view.set_location( welt->lookup_kartenboden(old_pos)->get_pos() );
			add_component(&headquarter_view);
		}
	}

	// Hajo: Money is counted in credit cents (100 cents = 1 Cr)
	sint64 maintenance = player->get_finance()->get_maintenance_with_bits((transport_type)transport_type_option);
	money_to_string(str_buf[18],
		(double)(maintenance)/100.0
	);
	maintenance_money.set_text(str_buf[18]);
	maintenance_money.set_color(maintenance>=0?MONEY_PLUS:MONEY_MINUS);

	for (int i = 0;  i<MAX_PLAYER_COST_BUTTON;  i++) {
		filterButtons[i].pressed = ( (bFilterStates[player->get_player_nr()]&(1<<i)) != 0 );
		// year_month_toggle.pressed = mchart.is_visible();
	}

	// Hajo: update chart seed
	chart.set_seed(welt->get_last_year());

	gui_frame_t::draw(pos, size);
}


bool money_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp==&headquarters) {
		if(  player->get_ai_id()!=player_t::HUMAN  ) {
			create_win( new ai_option_t(player), w_info, magic_ai_options_t+player->get_player_nr() );
		}
		else {
			welt->set_tool( tool_t::general_tool[TOOL_HEADQUARTER], player );
		}
		return true;
	}

	for ( int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		if (comp == &filterButtons[i]) {
			bFilterStates[player->get_player_nr()] ^= (1<<i);
			if (bFilterStates[player->get_player_nr()] & (1<<i)) {
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
	if(comp == &transport_type_c) {
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
	return magic_finances_t+player->get_player_nr();
}


void money_frame_t::rdwr( loadsave_t *file )
{
	bool monthly = mchart.is_visible();;
	file->rdwr_bool( monthly );

	// button state already collected
	file->rdwr_long( bFilterStates[player->get_player_nr()] );

	if(  file->is_loading()  ) {
		// states ...
		for( uint32 i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
			if (bFilterStates[player->get_player_nr()] & (1<<i)) {
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
