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

// @author hsiegeln
const char *money_frame_t::cost_type_name[MAX_PLAYER_COST_BUTTON] =
{
	"Revenue",
	"Operation",
	"Maintenance",
	"Road toll",
	"Powerlines",
	"Ops Profit",
	"New Vehicles",
	"Construction_Btn",
	"Gross Profit",
	"Transported",
	"Cash",
	"Assets",
	"Margin (%)",
	"Net Wealth"
};


const COLOR_VAL money_frame_t::cost_type_color[MAX_PLAYER_COST_BUTTON] =
{
	COL_REVENUE,
	COL_OPERATION,
	COL_MAINTENANCE,
	COL_TOLL,
	COL_POWERLINES,
	COL_OPS_PROFIT,
	COL_NEW_VEHICLES,
	COL_CONSTRUCTION,
	COL_PROFIT,
	COL_TRANSPORTED,
	COL_CASH,
	COL_VEHICLE_ASSETS,
	COL_MARGIN,
	COL_WEALTH
};

const uint8 money_frame_t::cost_type[MAX_PLAYER_COST_BUTTON] =
{
	COST_INCOME,        // Income
	COST_VEHICLE_RUN,   // Vehicle running costs
	COST_MAINTENANCE,   // Upkeep
	COST_WAY_TOLLS,
	COST_POWERLINES,	  // revenue from the power grid
	COST_OPERATING_PROFIT, // COST_POWERLINES+COST_INCOME-(COST_VEHICLE_RUN+COST_MAINTENANCE)
	COST_NEW_VEHICLE,   // New vehicles
	COST_CONSTRUCTION,	// Construction
	COST_PROFIT,        // COST_POWERLINES+COST_INCOME-(COST_CONSTRUCTION+COST_VEHICLE_RUN+COST_NEW_VEHICLE+COST_MAINTENANCE)
	COST_ALL_TRANSPORTED, // all transported goods
	COST_CASH,          // Cash
	COST_ASSETS,        // value of all vehicles and buildings
	COST_MARGIN,        // COST_OPERATING_PROFIT/COST_INCOME
	COST_NETWEALTH,     // Total Cash + Assets
};


/**
 * fills buffer (char array) with finance info
 * @author Owen Rudge, Hj. Malthaner
 */
const char *money_frame_t::display_money(int type, char *buf, int old)
{
	const double cost = (year_month_tabs.get_active_tab_index() ? sp->get_finance_history_month(old, type) : sp->get_finance_history_year(old, type)) / 100.0;
	money_to_string(buf, cost );
	return(buf);
}


const char *money_frame_t::display_number(int type, char *buf, int old)
{
	const double cost = (year_month_tabs.get_active_tab_index() ? sp->get_finance_history_month(old, type) : sp->get_finance_history_year(old, type)) / 1.0;
	money_to_string(buf, cost );
	buf[strlen(buf)-4] = 0;	// remove comma
	return(buf);
}


/**
 * Returns the appropriate colour for a certain finance type
 * @author Owen Rudge
 */
int money_frame_t::get_money_colour(int type, int old)
{
	sint64 i = (year_month_tabs.get_active_tab_index() ? sp->get_finance_history_month(old, type) : sp->get_finance_history_year(old, type));
	if (i < 0) return MONEY_MINUS;
	if (i > 0) return MONEY_PLUS;
	return COL_YELLOW;
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
		old_conmoney(NULL, COL_WHITE, gui_label_t::money),
		old_nvmoney(NULL, COL_WHITE, gui_label_t::money),
		old_vrmoney(NULL, COL_WHITE, gui_label_t::money),
		old_imoney(NULL, COL_WHITE, gui_label_t::money),
		old_tmoney(NULL, COL_WHITE, gui_label_t::money),
		old_mmoney(NULL, COL_WHITE, gui_label_t::money),
		old_omoney(NULL, COL_WHITE, gui_label_t::money),
		tylabel2("This Year", COL_WHITE, gui_label_t::right),
		gtmoney(NULL, COL_WHITE, gui_label_t::money),
		vtmoney(NULL, COL_WHITE, gui_label_t::money),
		money(NULL, COL_WHITE, gui_label_t::money),
		margin(NULL, COL_WHITE, gui_label_t::money),
		transport(NULL, COL_WHITE, gui_label_t::right),
		old_transport(NULL, COL_WHITE, gui_label_t::right),
		toll(NULL, COL_WHITE, gui_label_t::money),
		old_toll(NULL, COL_WHITE, gui_label_t::money),
		powerline(NULL, COL_WHITE, gui_label_t::money),
		old_powerline(NULL, COL_WHITE, gui_label_t::money),
		maintenance_label("This Month",COL_WHITE, gui_label_t::right),
		maintenance_money(NULL, COL_RED, gui_label_t::money),
		warn("", COL_YELLOW, gui_label_t::left),
		scenario("", COL_BLACK, gui_label_t::left),
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

	imoney.set_pos(koord(tyl_x,top+0*BUTTONSPACE));
	old_imoney.set_pos(koord(lyl_x,top+0*BUTTONSPACE));
	vrmoney.set_pos(koord(tyl_x,top+1*BUTTONSPACE));
	old_vrmoney.set_pos(koord(lyl_x,top+1*BUTTONSPACE));
	mmoney.set_pos(koord(tyl_x,top+2*BUTTONSPACE));
	old_mmoney.set_pos(koord(lyl_x,top+2*BUTTONSPACE));
	toll.set_pos(koord(tyl_x,top+3*BUTTONSPACE));
	old_toll.set_pos(koord(lyl_x,top+3*BUTTONSPACE));
	powerline.set_pos(koord(tyl_x,top+4*BUTTONSPACE));
	old_powerline.set_pos(koord(lyl_x,top+4*BUTTONSPACE));
	omoney.set_pos(koord(tyl_x,top+5*BUTTONSPACE));
	old_omoney.set_pos(koord(lyl_x,top+5*BUTTONSPACE));
	nvmoney.set_pos(koord(tyl_x,top+6*BUTTONSPACE));
	old_nvmoney.set_pos(koord(lyl_x,top+6*BUTTONSPACE));
	conmoney.set_pos(koord(tyl_x,top+7*BUTTONSPACE));
	old_conmoney.set_pos(koord(lyl_x,top+7*BUTTONSPACE));
	tmoney.set_pos(koord(tyl_x,top+8*BUTTONSPACE));
	old_tmoney.set_pos(koord(lyl_x,top+8*BUTTONSPACE));
	transport.set_pos(koord(tyl_x+19, top+9*BUTTONSPACE));
	old_transport.set_pos(koord(lyl_x+19, top+9*BUTTONSPACE));

	// right column
	maintenance_label.set_pos(koord(left+340+80, top+1*BUTTONSPACE-2));
	maintenance_money.set_pos(koord(left+340+55, top+2*BUTTONSPACE));

	tylabel2.set_pos(koord(left+140+80+335,top+4*BUTTONSPACE-2));
	gtmoney.set_pos(koord(left+140+335+55, top+5*BUTTONSPACE));
	vtmoney.set_pos(koord(left+140+335+55, top+6*BUTTONSPACE));
	margin.set_pos(koord(left+140+335+55, top+7*BUTTONSPACE));
	money.set_pos(koord(left+140+335+55, top+8*BUTTONSPACE));

	// return money or else stuff ...
	warn.set_pos(koord(left+335, top+9*BUTTONSPACE));
	if(sp->get_player_nr()!=1  &&  sp->get_welt()->get_scenario()->active()) {
		scenario.set_pos( koord( 10,1 ) );
		sp->get_welt()->get_scenario()->update_scenario_texts();
		scenario.set_text( sp->get_welt()->get_scenario()->description_text );
		add_komponente(&scenario);
	}

	//CHART YEAR
	chart.set_pos(koord(105,top+10*BUTTONSPACE+11));
	chart.set_groesse(koord(455,120));
	chart.set_dimension(MAX_PLAYER_HISTORY_YEARS, 10000);
	chart.set_seed(sp->get_welt()->get_last_year());
	chart.set_background(MN_GREY1);
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const int type = cost_type[i];
		chart.add_curve( cost_type_color[i], sp->get_finance_history_year(), MAX_PLAYER_COST, type, 12, (type < COST_ALL_TRANSPORTED  ||  type==COST_POWERLINES)  ||  type==COST_WAY_TOLLS  ? MONEY: STANDARD, false, true, (type < COST_ALL_TRANSPORTED) ||  type==COST_POWERLINES  ||  type==COST_WAY_TOLLS ? 2 : 0 );
	}
	//CHART YEAR END

	//CHART MONTH
	mchart.set_pos(koord(105,top+10*BUTTONSPACE+11));
	mchart.set_groesse(koord(455,120));
	mchart.set_dimension(MAX_PLAYER_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(MN_GREY1);
	for (int i = 0; i<MAX_PLAYER_COST_BUTTON; i++) {
		const int type = cost_type[i];
		mchart.add_curve( cost_type_color[i], sp->get_finance_history_month(), MAX_PLAYER_COST, type, 12, (type < COST_ALL_TRANSPORTED  ||  type==COST_POWERLINES)  ||  type==COST_WAY_TOLLS  ? MONEY: STANDARD, false, true, (type < COST_ALL_TRANSPORTED) ||  type==COST_POWERLINES  ||  type==COST_WAY_TOLLS ? 2 : 0 );
	}
	//CHART MONTH END

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
	add_komponente(&tmoney);
	add_komponente(&omoney);
	add_komponente(&toll);
	add_komponente(&powerline);
	add_komponente(&transport);

	add_komponente(&old_conmoney);
	add_komponente(&old_nvmoney);
	add_komponente(&old_vrmoney);
	add_komponente(&old_mmoney);
	add_komponente(&old_imoney);
	add_komponente(&old_tmoney);
	add_komponente(&old_omoney);
	add_komponente(&old_toll);
	add_komponente(&old_powerline);
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
	for(int ibutton=0;  ibutton<10;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], koord(left, top+ibutton*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		add_komponente(filterButtons + ibutton);
	}
	for(int ibutton=10;  ibutton<MAX_PLAYER_COST_BUTTON;  ibutton++) {
		filterButtons[ibutton].init(button_t::box, cost_type_name[ibutton], koord(left+335, top+(ibutton-5)*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
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

	set_fenstergroesse(koord(582, 340));
}


void money_frame_t::zeichnen(koord pos, koord gr)
{
	// Hajo: each label needs its own buffer
	static char str_buf[26][64];

	sp->calc_finance_history();

	chart.set_visible( year_month_tabs.get_active_tab_index()==0 );
	mchart.set_visible( year_month_tabs.get_active_tab_index()==1 );

	tylabel.set_text( year_month_tabs.get_active_tab_index() ? "This Month" : "This Year" );
	lylabel.set_text( year_month_tabs.get_active_tab_index() ? "Last Month" : "Last Year" );

	conmoney.set_text(display_money(COST_CONSTRUCTION, str_buf[0], 0));
	nvmoney.set_text(display_money(COST_NEW_VEHICLE, str_buf[1], 0));
	vrmoney.set_text(display_money(COST_VEHICLE_RUN, str_buf[2], 0));
	mmoney.set_text(display_money(COST_MAINTENANCE, str_buf[3], 0));
	imoney.set_text(display_money(COST_INCOME, str_buf[4], 0));
	tmoney.set_text(display_money(COST_PROFIT, str_buf[5], 0));
	omoney.set_text(display_money(COST_OPERATING_PROFIT, str_buf[6], 0));

	old_conmoney.set_text(display_money(COST_CONSTRUCTION, str_buf[7], 1));
	old_nvmoney.set_text(display_money(COST_NEW_VEHICLE, str_buf[8], 1));
	old_vrmoney.set_text(display_money(COST_VEHICLE_RUN, str_buf[9], 1));
	old_mmoney.set_text(display_money(COST_MAINTENANCE, str_buf[10], 1));
	old_imoney.set_text(display_money(COST_INCOME, str_buf[11], 1));
	old_tmoney.set_text(display_money(COST_PROFIT, str_buf[12], 1));
	old_omoney.set_text(display_money(COST_OPERATING_PROFIT, str_buf[13], 1));

	// transported goods
	transport.set_text(display_number(COST_ALL_TRANSPORTED, str_buf[20], 0));
	transport.set_color(get_money_colour(COST_ALL_TRANSPORTED, 0));

	old_transport.set_text(display_number(COST_ALL_TRANSPORTED, str_buf[21], 1));
	old_transport.set_color(get_money_colour(COST_ALL_TRANSPORTED, 1));

	toll.set_text(display_money(COST_WAY_TOLLS, str_buf[24], 0));
	toll.set_color(get_money_colour(COST_WAY_TOLLS, 0));
	old_toll.set_text(display_money(COST_WAY_TOLLS, str_buf[25], 1));
	old_toll.set_color(get_money_colour(COST_WAY_TOLLS, 1));

	powerline.set_text(display_money(COST_POWERLINES, str_buf[22], 0));
	powerline.set_color(get_money_colour(COST_POWERLINES, 0));
	old_powerline.set_text(display_money(COST_POWERLINES, str_buf[23], 1));
	old_powerline.set_color(get_money_colour(COST_POWERLINES, 1));

	conmoney.set_color(get_money_colour(COST_CONSTRUCTION, 0));
	nvmoney.set_color(get_money_colour(COST_NEW_VEHICLE, 0));
	vrmoney.set_color(get_money_colour(COST_VEHICLE_RUN, 0));
	mmoney.set_color(get_money_colour(COST_MAINTENANCE, 0));
	imoney.set_color(get_money_colour(COST_INCOME, 0));
	tmoney.set_color(get_money_colour(COST_PROFIT, 0));
	omoney.set_color(get_money_colour(COST_OPERATING_PROFIT, 0));

	old_conmoney.set_color(get_money_colour(COST_CONSTRUCTION, 1));
	old_nvmoney.set_color(get_money_colour(COST_NEW_VEHICLE, 1));
	old_vrmoney.set_color(get_money_colour(COST_VEHICLE_RUN, 1));
	old_mmoney.set_color(get_money_colour(COST_MAINTENANCE, 1));
	old_imoney.set_color(get_money_colour(COST_INCOME, 1));
	old_tmoney.set_color(get_money_colour(COST_PROFIT, 1));
	old_omoney.set_color(get_money_colour(COST_OPERATING_PROFIT, 1));

	gtmoney.set_text(display_money(COST_CASH, str_buf[14], 0));
	gtmoney.set_color(get_money_colour(COST_CASH, 0));

	vtmoney.set_text(display_money(COST_ASSETS, str_buf[17], 0));
	vtmoney.set_color(get_money_colour(COST_ASSETS, 0));

	money.set_text(display_money(COST_NETWEALTH, str_buf[18], 0));
	money.set_color(get_money_colour(COST_NETWEALTH, 0));

	display_money(COST_MARGIN, str_buf[19], 0);
	str_buf[19][strlen(str_buf[19])-1] = '%';	// remove cent sign
	margin.set_text(str_buf[19]);
	margin.set_color(get_money_colour(COST_MARGIN, 0));

	// warning/success messages
	if(sp->get_player_nr()!=1  &&  sp->get_welt()->get_scenario()->active()) {
		warn.set_color( COL_BLACK );
		sint32 percent = sp->get_welt()->get_scenario()->get_completion(sp->get_player_nr());
		if (percent >= 0) {
			sprintf( str_buf[15], translator::translate("Scenario complete: %i%%"), percent );
		}
		else {
			sprintf( str_buf[15], translator::translate("Scenario lost!") );
		}
	}
	else if(sp->get_finance_history_year(0, COST_NETWEALTH)<0) {
		warn.set_color( MONEY_MINUS );
		tstrncpy(str_buf[15], translator::translate("Company bankrupt"), lengthof(str_buf[15]) );
	}
	else if(  sp->get_finance_history_year(0, COST_NETWEALTH)*10 < sp->get_welt()->get_settings().get_starting_money(sp->get_welt()->get_current_month()/12)  ){
		warn.set_color( MONEY_MINUS );
		sprintf(str_buf[15], translator::translate("Net wealth near zero"), sp->get_konto_ueberzogen() );
	}
	else if(  sp->get_konto_ueberzogen()  ) {
		warn.set_color( COL_YELLOW );
		sprintf( str_buf[15], translator::translate("On loan since %i month(s)"), sp->get_konto_ueberzogen() );
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
	money_to_string(str_buf[16],
		sp->get_welt()->ticks_per_world_month_shift>=18 ?
		(double)((sint64)sp->get_maintenance()<<(sp->get_welt()->ticks_per_world_month_shift-18u))/100.0 :
		(double)((sint64)sp->get_maintenance()>>(18u-sp->get_welt()->ticks_per_world_month_shift))/100.0
	);
	maintenance_money.set_text(str_buf[16]);
	maintenance_money.set_color(sp->get_maintenance()>=0?MONEY_PLUS:MONEY_MINUS);

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
