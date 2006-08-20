/*
 * money_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <string.h>

#include "../simworld.h"
#include "money_frame.h"
#include "../simdebug.h"
#include "gui_label.h"
#include "button.h"
#include "../simintr.h"
#include "../simplay.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../utils/simstring.h"
#include "../dataobj/translator.h"
#include "../dataobj/umgebung.h"
#include "components/gui_chart.h"

#define COST_BALANCE    10 // bank balance

#define BUTTONSPACE 14

// @author hsiegeln
const char money_frame_t::cost_type[MAX_COST][64] =
{
  "Construction", "Operation", "New Vehicles", "Revenue",
  "Maintenance", "Assets", "Cash", "Net Wealth", "Gross Profit", "Ops Profit", "Margin (%)", "Transported"
};

const int money_frame_t::cost_type_color[MAX_COST] =
{
  7, 11, 15, 132, 23, 27, 31, 35, 241, 61, 62, 63
};

const uint8 button_order[MAX_COST] =
{
	3, 1, 4, 9, 2, 0, 8,
	6, 5, 7, 10, 11
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
	return (sp->get_finance_history_year(old, type) < 0) ? MONEY_MINUS : MONEY_PLUS;
}

/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten. Based on sound_frame_t()
 * @author Hj. Malthaner, Owen Rudge
 */
money_frame_t::money_frame_t(spieler_t *sp)
  : gui_frame_t("Finanzen", sp->kennfarbe),
    tylabel("This Year", WEISS, gui_label_t::right),
    lylabel("Last Year", WEISS, gui_label_t::right),
    conmoney(NULL, WEISS, gui_label_t::money),
    nvmoney(NULL, WEISS, gui_label_t::money),
    vrmoney(NULL, WEISS, gui_label_t::money),
    imoney(NULL, WEISS, gui_label_t::money),
    tmoney(NULL, WEISS, gui_label_t::money),
    mmoney(NULL, WEISS, gui_label_t::money),
    omoney(NULL, WEISS, gui_label_t::money),
    old_conmoney(NULL, WEISS, gui_label_t::money),
    old_nvmoney(NULL, WEISS, gui_label_t::money),
    old_vrmoney(NULL, WEISS, gui_label_t::money),
    old_imoney(NULL, WEISS, gui_label_t::money),
    old_tmoney(NULL, WEISS, gui_label_t::money),
    old_mmoney(NULL, WEISS, gui_label_t::money),
    old_omoney(NULL, WEISS, gui_label_t::money),
    tylabel2("This Year", WEISS, gui_label_t::right),
    gtmoney(NULL, WEISS, gui_label_t::money),
    vtmoney(NULL, WEISS, gui_label_t::money),
    money(NULL, WEISS, gui_label_t::money),
    margin(NULL, WEISS, gui_label_t::money),
    maintenance_label("Maintenance (monthly):"),
    maintenance_money(NULL, ROT, gui_label_t::money),
    warn("", ROT)
{
	if(sp->gib_welt()->gib_spieler(0)!=sp) {
		sprintf(money_frame_title,translator::translate("Finances of %s"),translator::translate(sp->gib_name()) );
		setze_name(money_frame_title);
	}

    this->sp = sp;

    const int top = 20;
    const int left = 12;

    //CHART YEAR
    chart = new gui_chart_t();
    chart->setze_pos(koord(1,1));
    chart->setze_groesse(koord(443,120));
    chart->set_dimension(MAX_HISTORY_YEARS, 10000);
    chart->set_seed(sp->gib_welt()->get_last_year());
    chart->set_background(MN_GREY1);
    int i;
    for (i = 0; i<MAX_COST; i++)
    {
    	chart->add_curve(cost_type_color[i], (sint64 *)sp->get_finance_history_year(), MAX_COST, i, 12, i<MAX_COST-2 ? MONEY: STANDARD, false, false);
    }
    //CHART YEAR END

    //CHART MONTH
    mchart = new gui_chart_t();
    mchart->setze_pos(koord(1,1));
    mchart->setze_groesse(koord(443,120));
    mchart->set_dimension(MAX_HISTORY_MONTHS, 10000);
    mchart->set_seed(0);
    mchart->set_background(MN_GREY1);
    for (i = 0; i<MAX_COST; i++)
    {
    	mchart->add_curve(cost_type_color[i], (sint64 *)sp->get_finance_history_month(), MAX_COST, i, 12, i<MAX_COST-2 ? MONEY: STANDARD, false, false);
    }
    mchart->set_visible(false);
    //CHART MONTH END

    // tab (month/year)
    year_month_tabs.add_tab(chart, translator::translate("Years"));
    year_month_tabs.add_tab(mchart, translator::translate("Months"));
    year_month_tabs.setze_pos(koord(112, top+9*BUTTONSPACE-6));
    year_month_tabs.setze_groesse(koord(443, 125));
    add_komponente(&year_month_tabs);

   // left column
    tylabel.setze_pos(koord(left+140+80,top-1*BUTTONSPACE-2));
    lylabel.setze_pos(koord(left+240+80,top-1*BUTTONSPACE-2));

    imoney.setze_pos(koord(left+140+55,top+0*BUTTONSPACE));
    old_imoney.setze_pos(koord(left+240+55,top+0*BUTTONSPACE));
    vrmoney.setze_pos(koord(left+140+55,top+1*BUTTONSPACE));
    old_vrmoney.setze_pos(koord(left+240+55,top+1*BUTTONSPACE));
    mmoney.setze_pos(koord(left+140+55,top+2*BUTTONSPACE));
    old_mmoney.setze_pos(koord(left+240+55,top+2*BUTTONSPACE));
    omoney.setze_pos(koord(left+140+55,top+3*BUTTONSPACE));
    old_omoney.setze_pos(koord(left+240+55,top+3*BUTTONSPACE));
    nvmoney.setze_pos(koord(left+140+55,top+4*BUTTONSPACE));
    old_nvmoney.setze_pos(koord(left+240+55,top+4*BUTTONSPACE));
    conmoney.setze_pos(koord(left+140+55,top+5*BUTTONSPACE));
    old_conmoney.setze_pos(koord(left+240+55,top+5*BUTTONSPACE));
    tmoney.setze_pos(koord(left+140+55,top+6*BUTTONSPACE));
    old_tmoney.setze_pos(koord(left+240+55,top+6*BUTTONSPACE));

	// right column
    tylabel2.setze_pos(koord(left+140+80+335,top-1*BUTTONSPACE-2));

    gtmoney.setze_pos(koord(left+140+335+55, top+0*BUTTONSPACE));
    vtmoney.setze_pos(koord(left+140+335+55, top+1*BUTTONSPACE));
    money.setze_pos(koord(left+140+335+55, top+2*BUTTONSPACE));
    margin.setze_pos(koord(left+140+335+55, top+4*BUTTONSPACE));

    maintenance_label.setze_pos(koord(left+335, top+6*BUTTONSPACE));
    maintenance_money.setze_pos(koord(left+335+140+55, top+6*BUTTONSPACE));

    warn.setze_pos(koord(left,top+8*BUTTONSPACE-8));

    add_komponente(&conmoney);
    add_komponente(&nvmoney);
    add_komponente(&vrmoney);
    add_komponente(&mmoney);
    add_komponente(&imoney);
    add_komponente(&tmoney);
    add_komponente(&omoney);

    add_komponente(&old_conmoney);
    add_komponente(&old_nvmoney);
    add_komponente(&old_vrmoney);
    add_komponente(&old_mmoney);
    add_komponente(&old_imoney);
    add_komponente(&old_tmoney);
    add_komponente(&old_omoney);

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

	// add filter buttons
	for(int i=0;  i<7;  i++) {
		int ibutton=button_order[i];
		filterButtons[ibutton].init(button_t::box, translator::translate(cost_type[ibutton]), koord(left, top+i*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		bFilterIsActive[ibutton] = false;
		add_komponente(filterButtons + ibutton);
	}
	for(int i=7;  i<10;  i++) {
		int ibutton=button_order[i];
		filterButtons[ibutton].init(button_t::box, translator::translate(cost_type[ibutton]), koord(left+335, top+(i-7)*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		bFilterIsActive[ibutton] = false;
		add_komponente(filterButtons + ibutton);
	}
	// Button 10: Marge
	{
		int ibutton=button_order[10];
		filterButtons[ibutton].init(button_t::box, translator::translate(cost_type[ibutton]), koord(left+335, top+4*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		bFilterIsActive[ibutton] = false;
		add_komponente(filterButtons + ibutton);
	}

	// Button 11: Transport
	{
		int ibutton=button_order[11];
		filterButtons[ibutton].init(button_t::box, translator::translate(cost_type[ibutton]), koord(left+335+100, top+8*BUTTONSPACE-2), koord(120, BUTTONSPACE));
		filterButtons[ibutton].add_listener(this);
		filterButtons[ibutton].background = cost_type_color[ibutton];
		bFilterIsActive[ibutton] = false;
		add_komponente(filterButtons + ibutton);
	}

    setze_fenstergroesse(koord(582, 300));
    setze_opaque( true );
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void money_frame_t::zeichnen(koord pos, koord gr)
{
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

    if(sp->gib_konto_ueberzogen()) {
	sprintf(str_buf[15], translator::translate("Du hast %d Monate Zeit, deine Schulden zurueckzuzahlen"),
	    spieler_t::MAX_KONTO_VERZUG-sp->gib_konto_ueberzogen()+1);
    }
    else {
	str_buf[15][0] = '\0';
    }
    warn.setze_text(str_buf[15]);

    // Hajo: Money is counted in credit cents (100 cents = 1 Cr)
    money_to_string(str_buf[16], sp->add_maintenance(0)/100);
    maintenance_money.setze_text(str_buf[16]);
    maintenance_money.set_color(sp->add_maintenance(0)>=0?MONEY_PLUS:MONEY_MINUS);

    for (int i = 0;i<MAX_COST;i++)
    {
    	filterButtons[i].pressed = bFilterIsActive[i];
    	// year_month_toggle.pressed = mchart->is_visible();
    }

    // Hajo: update chart seed
    chart->set_seed(sp->gib_welt()->get_last_year());


    gui_frame_t::zeichnen(pos, gr);

}

bool money_frame_t::action_triggered(gui_komponente_t *komp)
{
		sp->calc_finance_history();
    for ( int i = 0; i<MAX_COST; i++)
    {
    	if (komp == &filterButtons[i])
    	{
    		bFilterIsActive[i] == true ? bFilterIsActive[i] = false : bFilterIsActive[i] = true;
    		if (bFilterIsActive[i] == true) {
				chart->show_curve(i);
				mchart->show_curve(i);
			} else {
				chart->hide_curve(i);
				mchart->hide_curve(i);
			}
    		return true;
    	}
    }
    return false;
}
