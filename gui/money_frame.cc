/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include <string.h>

#include "money_frame.h"

#include "../simworld.h"
#include "../simdebug.h"
#include "../simintr.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/scenario.h"

// for headquarter construction only ...
#include "../simskin.h"
#include "../simwerkz.h"
#include "../bauer/hausbauer.h"
#include "../besch/haus_besch.h"


// remebers last settings
static uint32 bFilterStates[MAX_PLAYER_COUNT];

#define COST_BALANCE    10 // bank balance

#define BUTTONSPACE 14

// @author hsiegeln
const char money_frame_t::cost_type[MAX_PLAYER_COST][64] =
{
  "Construction_Btn", "Operation", "New Vehicles", "Revenue",
  "Maintenance", "Assets", "Cash", "Net Wealth", "Gross Profit", "Ops Profit", "Margin (%)", "Transported", "Powerlines"
};

const int money_frame_t::cost_type_color[MAX_PLAYER_COST] =
{
	COL_CONSTRUCTION,
	COL_OPERATION,
	COL_NEW_VEHICLES,
	COL_REVENUE,
	COL_MAINTENANCE,
	COL_VEHICLE_ASSETS,
	COL_CASH,
	COL_WEALTH,
	COL_PROFIT,
	COL_OPS_PROFIT,
	COL_MARGIN,
	COL_TRANSPORTED,
	COL_POWERLINES
};

const uint8 button_order[MAX_PLAYER_COST] =
{
	3, 1, 4, 9, 2, 0, 8, 12, 11,
	6, 5, 10, 7
};

char money_frame_t::digit[4];

/**
 * fills buffer (char array) with finance info
 * @author Owen Rudge, Hj. Malthaner
 */
const char *money_frame_t::display_money(int type, char *buf, int old)
{
	money_to_string(buf, sp->get_finance_history_year(old, type) / 100.0 );
	return(buf);
}

/**
 * Returns the appropriate colour for a certain finance type
 * @author Owen Rudge
 */
int money_frame_t::get_money_colour(int type, int old)
{
	sint64 i = sp->get_finance_history_year(old, type);
	if (i < 0) return MONEY_MINUS;
	if (i > 0) return MONEY_PLUS;
	return COL_YELLOW;
}


money_frame_t::money_frame_t(spieler_t *sp)
  : gui_frame_t("Finanzen", sp),
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
		powerline(NULL, COL_WHITE, gui_label_t::right),
		old_powerline(NULL, COL_WHITE, gui_label_t::right),
		maintenance_label("This Month",COL_WHITE, gui_label_t::right),
		maintenance_money(NULL, COL_RED, gui_label_t::money),
		warn("", COL_YELLOW, gui_label_t::left),
		scenario("", COL_BLACK, gui_label_t::left),
		headquarter_view(sp->get_welt(), koord3d::invalid)
{
	if(sp->get_welt()->gib_spieler(0)!=sp) {
		sprintf(money_frame_title,translator::translate("Finances of %s"),translator::translate(sp->gib_name()) );
		setze_name(money_frame_title);
	}

	this->sp = sp;

	const int top = 30;
	const int left = 12;

	//CHART YEAR
	chart.setze_pos(koord(1,1));
	chart.setze_groesse(koord(443,120));
	chart.set_dimension(MAX_PLAYER_HISTORY_YEARS, 10000);
	chart.set_seed(sp->get_welt()->get_last_year());
	chart.set_background(MN_GREY1);
	for (int i = 0; i<MAX_PLAYER_COST; i++) {
		chart.add_curve(cost_type_color[i], sp->get_finance_history_year(), MAX_PLAYER_COST, i, 12, (i < 10) ||  i==COST_POWERLINES ? MONEY: STANDARD, false, false);
	}
	//CHART YEAR END

	//CHART MONTH
	mchart.setze_pos(koord(1,1));
	mchart.setze_groesse(koord(443,120));
	mchart.set_dimension(MAX_PLAYER_HISTORY_MONTHS, 10000);
	mchart.set_seed(0);
	mchart.set_background(MN_GREY1);
	for (int i = 0; i<MAX_PLAYER_COST; i++) {
		mchart.add_curve(cost_type_color[i], sp->get_finance_history_month(), MAX_PLAYER_COST, i, 12, (i < 10) ||  i==COST_POWERLINES ? MONEY: STANDARD, false, false);
	}
	mchart.set_visible(false);
	//CHART MONTH END

	// tab (month/year)
	year_month_tabs.add_tab(&chart, translator::translate("Years"));
	year_month_tabs.add_tab(&mchart, translator::translate("Months"));
	year_month_tabs.setze_pos(koord(112, top+11*BUTTONSPACE-6));
	year_month_tabs.setze_groesse(koord(443, 125));
	add_komponente(&year_month_tabs);

	const sint16 tyl_x = left+140+55;
	const sint16 lyl_x = left+240+55;

	// left column
	tylabel.setze_pos(koord(tyl_x+25,top-1*BUTTONSPACE-2));
	lylabel.setze_pos(koord(lyl_x+25,top-1*BUTTONSPACE-2));

	imoney.setze_pos(koord(tyl_x,top+0*BUTTONSPACE));
	old_imoney.setze_pos(koord(lyl_x,top+0*BUTTONSPACE));
	vrmoney.setze_pos(koord(tyl_x,top+1*BUTTONSPACE));
	old_vrmoney.setze_pos(koord(lyl_x,top+1*BUTTONSPACE));
	mmoney.setze_pos(koord(tyl_x,top+2*BUTTONSPACE));
	old_mmoney.setze_pos(koord(lyl_x,top+2*BUTTONSPACE));
	omoney.setze_pos(koord(tyl_x,top+3*BUTTONSPACE));
	old_omoney.setze_pos(koord(lyl_x,top+3*BUTTONSPACE));
	nvmoney.setze_pos(koord(tyl_x,top+4*BUTTONSPACE));
	old_nvmoney.setze_pos(koord(lyl_x,top+4*BUTTONSPACE));
	conmoney.setze_pos(koord(tyl_x,top+5*BUTTONSPACE));
	old_conmoney.setze_pos(koord(lyl_x,top+5*BUTTONSPACE));
	tmoney.setze_pos(koord(tyl_x,top+6*BUTTONSPACE));
	old_tmoney.setze_pos(koord(lyl_x,top+6*BUTTONSPACE));
	powerline.setze_pos(koord(tyl_x+19,top+7*BUTTONSPACE));
	old_powerline.setze_pos(koord(lyl_x+19,top+7*BUTTONSPACE));
	transport.setze_pos(koord(tyl_x+19, top+8*BUTTONSPACE));
	old_transport.setze_pos(koord(lyl_x+19, top+8*BUTTONSPACE));

	// right column
	maintenance_label.setze_pos(koord(left+340+80, top+1*BUTTONSPACE-2));
	maintenance_money.setze_pos(koord(left+340+55, top+2*BUTTONSPACE));

	tylabel2.setze_pos(koord(left+140+80+335,top+4*BUTTONSPACE-2));
	gtmoney.setze_pos(koord(left+140+335+55, top+5*BUTTONSPACE));
	vtmoney.setze_pos(koord(left+140+335+55, top+6*BUTTONSPACE));
	margin.setze_pos(koord(left+140+335+55, top+7*BUTTONSPACE));
	money.setze_pos(koord(left+140+335+55, top+8*BUTTONSPACE));

	// return money or else stuff ...
	warn.setze_pos(koord(left+335, top+10*BUTTONSPACE));
	if(sp->get_player_nr()==0  &&  sp->get_welt()->get_scenario()->active()) {
		scenario.setze_pos( koord( 10,0 ) );
		scenario.setze_text( sp->get_welt()->get_scenario()->get_description() );
		add_komponente(&scenario);
	}

	add_komponente(&conmoney);
	add_komponente(&nvmoney);
	add_komponente(&vrmoney);
	add_komponente(&mmoney);
	add_komponente(&imoney);
	add_komponente(&tmoney);
	add_komponente(&omoney);
	add_komponente(&powerline);
	add_komponente(&transport);

	add_komponente(&old_conmoney);
	add_komponente(&old_nvmoney);
	add_komponente(&old_vrmoney);
	add_komponente(&old_mmoney);
	add_komponente(&old_imoney);
	add_komponente(&old_tmoney);
	add_komponente(&old_omoney);
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

	// easier headquarter access
	if (!hausbauer_t::headquarter.empty()) {
		headquarter.init(button_t::box, "build HQ", koord(582-12-120, 0), koord(120, BUTTONSPACE));
		headquarter.add_listener(this);
		add_komponente(&headquarter);
		headquarter.set_tooltip( NULL );
		headquarter.disable();

		// get new costs
		for(  vector_tpl<const haus_besch_t *>::const_iterator iter = hausbauer_t::headquarter.begin(), end = hausbauer_t::headquarter.end();  iter != end;  ++iter  ) {
			const haus_besch_t* besch = (*iter);
			if (besch->gib_extra() <= sp->get_headquarter_level() ) {
				tstrncpy( headquarter_tooltip, werkzeug_t::general_tool[WKZ_HEADQUARTER]->get_tooltip(sp), 900 );
			}
			else {
				headquarter.enable();
				headquarter.set_tooltip( headquarter_tooltip );
			}
		}


		if(sp->get_headquarter_pos()!=koord::invalid) {
			headquarter_view.set_location( sp->get_welt()->lookup_kartenboden( sp->get_headquarter_pos() )->gib_pos() );
		}
		headquarter_view.setze_pos( koord(582-12-120, BUTTONSPACE) );
		headquarter_view.setze_groesse( koord(120, 64) );
		add_komponente(&headquarter_view);
	}
	old_level = sp->get_headquarter_level();

	// add filter buttons
	for(int i=0;  i<9;  i++) {
		int ibutton=button_order[i];
		filterButtons[ibutton].init(button_t::box, cost_type[ibutton], koord(left, top+i*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		add_komponente(filterButtons + ibutton);
	}
	for(int i=9;  i<13;  i++) {
		int ibutton=button_order[i];
		filterButtons[ibutton].init(button_t::box, cost_type[ibutton], koord(left+335, top+(i-4)*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		add_komponente(filterButtons + ibutton);
	}

	// states ...
	for ( int i = 0; i<MAX_PLAYER_COST; i++) {
		if (bFilterStates[sp->get_player_nr()] & (1<<i)) {
			chart.show_curve(i);
			mchart.show_curve(i);
		}
		else {
			chart.hide_curve(i);
			mchart.hide_curve(i);
		}
	}

	setze_fenstergroesse(koord(582, 340));
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void money_frame_t::zeichnen(koord pos, koord gr)
{
	// Hajo: each label needs its own buffer
	static char str_buf[24][256];

	sp->calc_finance_history();

	conmoney.setze_text(display_money(COST_CONSTRUCTION, str_buf[0], 0));
	nvmoney.setze_text(display_money(COST_NEW_VEHICLE, str_buf[1], 0));
	vrmoney.setze_text(display_money(COST_VEHICLE_RUN, str_buf[2], 0));
	mmoney.setze_text(display_money(COST_MAINTENANCE, str_buf[3], 0));
	imoney.setze_text(display_money(COST_INCOME, str_buf[4], 0));
	tmoney.setze_text(display_money(COST_PROFIT, str_buf[5], 0));
	omoney.setze_text(display_money(COST_OPERATING_PROFIT, str_buf[6], 0));

	old_conmoney.setze_text(display_money(COST_CONSTRUCTION, str_buf[7], 1));
	old_nvmoney.setze_text(display_money(COST_NEW_VEHICLE, str_buf[8], 1));
	old_vrmoney.setze_text(display_money(COST_VEHICLE_RUN, str_buf[9], 1));
	old_mmoney.setze_text(display_money(COST_MAINTENANCE, str_buf[10], 1));
	old_imoney.setze_text(display_money(COST_INCOME, str_buf[11], 1));
	old_tmoney.setze_text(display_money(COST_PROFIT, str_buf[12], 1));
	old_omoney.setze_text(display_money(COST_OPERATING_PROFIT, str_buf[13], 1));

	// transported goods
	money_to_string(str_buf[20], sp->get_finance_history_year(0, COST_ALL_TRANSPORTED) );
	str_buf[20][strlen(str_buf[20])-4] = 0;	// remove comma
	transport.setze_text(str_buf[20]);
	transport.set_color(get_money_colour(COST_ALL_TRANSPORTED, 0));

	money_to_string(str_buf[21], sp->get_finance_history_year(1, COST_ALL_TRANSPORTED) );
	str_buf[21][strlen(str_buf[21])-4] = 0;	// remove comma
	old_transport.setze_text(str_buf[21]);
	old_transport.set_color(get_money_colour(COST_ALL_TRANSPORTED, 0));

	money_to_string(str_buf[22], sp->get_finance_history_year(0, COST_POWERLINES) );
	str_buf[22][strlen(str_buf[22])-4] = 0;	// remove comma
	powerline.setze_text(str_buf[22]);
	powerline.set_color(get_money_colour(COST_POWERLINES, 0));

	money_to_string(str_buf[23], sp->get_finance_history_year(1, COST_POWERLINES) );
	str_buf[23][strlen(str_buf[23])-4] = 0;	// remove comma
	old_powerline.setze_text(str_buf[23]);
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

	gtmoney.setze_text(display_money(COST_CASH, str_buf[14], 0));
	gtmoney.set_color(get_money_colour(COST_CASH, 0));

	vtmoney.setze_text(display_money(COST_ASSETS, str_buf[17], 0));
	vtmoney.set_color(get_money_colour(COST_ASSETS, 0));

	money.setze_text(display_money(COST_NETWEALTH, str_buf[18], 0));
	money.set_color(get_money_colour(COST_NETWEALTH, 0));

	display_money(COST_MARGIN, str_buf[19], 0);
	str_buf[19][strlen(str_buf[19])-1] = 0;	// remove percent sign
	margin.setze_text(str_buf[19]);
	margin.set_color(get_money_colour(COST_MARGIN, 0));

	// warning/success messages
	if(sp->get_player_nr()==0  &&  sp->get_welt()->get_scenario()->active()) {
		sprintf( str_buf[15], translator::translate("Scenario complete: %i%%"), sp->get_welt()->get_scenario()->completed(0) );
	}
	else if(sp->gib_konto_ueberzogen()) {
		warn.set_color( COL_RED );
		if(sp->get_finance_history_year(0, COST_NETWEALTH)<0) {
			sprintf(str_buf[15], translator::translate("Company bankrupt") );
		}
		else {
			sprintf(str_buf[15], translator::translate("On loan since %i month(s)"), sp->gib_konto_ueberzogen() );
		}
	}
	else {
		str_buf[15][0] = '\0';
	}
	warn.setze_text(str_buf[15]);

	if(sp!=sp->get_welt()->get_active_player()) {
		headquarter.disable();
		headquarter.set_tooltip( NULL );
	}
	else {
		headquarter.enable();
		if(old_level!=sp->get_headquarter_level()) {

			if(sp->get_headquarter_level()==(uint16)hausbauer_t::headquarter.get_count()) {
				headquarter.disable();
				headquarter.set_tooltip( NULL );
			}
			else {
				// get new costs
				for(  vector_tpl<const haus_besch_t *>::const_iterator iter = hausbauer_t::headquarter.begin(), end = hausbauer_t::headquarter.end();  iter != end;  ++iter  ) {
					const haus_besch_t* besch = (*iter);
					if (besch->gib_extra() == sp->get_headquarter_level()) {
						tstrncpy( headquarter_tooltip, werkzeug_t::general_tool[WKZ_HEADQUARTER]->get_tooltip(sp), 900 );
						break;
					}
				}
			}
		}
	}

	if(old_level!=sp->get_headquarter_level()  &&  sp->get_headquarter_pos()!=koord::invalid) {

		old_level = sp->get_headquarter_level();
		remove_komponente(&headquarter_view);
		headquarter_view.set_location( sp->get_welt()->lookup_kartenboden(sp->get_headquarter_pos())->gib_pos() );
		headquarter.setze_text( "upgrade HQ" );
		add_komponente(&headquarter_view);
	}

	// Hajo: Money is counted in credit cents (100 cents = 1 Cr)
	money_to_string(str_buf[16], (spieler_t::add_maintenance( sp, 0)<<((sint64)sp->get_welt()->ticks_bits_per_tag-18l))/100);
	maintenance_money.setze_text(str_buf[16]);
	maintenance_money.set_color(spieler_t::add_maintenance( sp, 0)>=0?MONEY_PLUS:MONEY_MINUS);

	for (int i = 0;  i<MAX_PLAYER_COST;  i++) {
		filterButtons[i].pressed = ( (bFilterStates[sp->get_player_nr()]&(1<<i)) != 0 );
		// year_month_toggle.pressed = mchart.is_visible();
	}

	// Hajo: update chart seed
	chart.set_seed(sp->get_welt()->get_last_year());

	gui_frame_t::zeichnen(pos, gr);
}



bool money_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	if(komp==&headquarter) {
		sp->get_welt()->set_werkzeug( werkzeug_t::general_tool[WKZ_HEADQUARTER] );
		return true;
	}

	for ( int i = 0; i<MAX_PLAYER_COST; i++) {
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
