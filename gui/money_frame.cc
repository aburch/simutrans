/*
 * money_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

// #include <monetary.h>

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

// @author hsiegeln
const char money_frame_t::cost_type[MAX_COST][64] =
{
  "Construction", "Operation", "New Vehicles", "Revenue",
  "Maintenance", "Assets", "Cash", "Net Wealth", "Gross Profit", "Ops Profit"
};

const int money_frame_t::cost_type_color[MAX_COST] =
{
  7, 11, 15, 132, 23, 27, 31, 35, 241, 61
};


char money_frame_t::digit[4];

/**
 * fills buffer (char array) with finance info
 * @author Owen Rudge, Hj. Malthaner
 */
const char *money_frame_t::display_money(int type, char *buf, int old)
{
	sint64 tmpcash;

	tmpcash = 0;
	tmpcash += sp->get_finance_history_year(old, type);

    // Hajo: Money is counted in credit cents (100 cents = 1 Cr)
    //    sprintf(buf, "%.2f$", tmpcash / 100.0);

    money_to_string(buf, tmpcash / 100.0);

    return(buf);
}

/**
 * Returns the appropriate colour for a certain finance type
 * @author Owen Rudge
 */
int money_frame_t::get_money_colour(int type, int old)
{
	sint64 tmpcash;

	tmpcash = 0;
	tmpcash += sp->get_finance_history_year(old, type);



    if (tmpcash < 0) {
       return(ROT);
    } else {
       return(WHITE);
    }
}

/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten. Based on sound_frame_t()
 * @author Hj. Malthaner, Owen Rudge
 */
money_frame_t::money_frame_t(spieler_t *sp)
  : gui_frame_t("Finances", sp->kennfarbe),
    clabel("Construction:"),
    nvlabel("New Vehicles:"),
    vrlabel("Vehicle Running Costs:"),
    ilabel("Revenue:"),
    tlabel("Total:"),
    mlabel("Maintenance:"),
    gtlabel("Balance:"),
    maintenance_label("Maintenance (monthly):"),
    tylabel("This Year", WEISS, gui_label_t::right),
    lylabel("Last Year", WEISS, gui_label_t::right),
    conmoney(NULL, WEISS, gui_label_t::money),
    nvmoney(NULL, WEISS, gui_label_t::money),
    vrmoney(NULL, WEISS, gui_label_t::money),
    imoney(NULL, WEISS, gui_label_t::money),
    tmoney(NULL, WEISS, gui_label_t::money),
    mmoney(NULL, WEISS, gui_label_t::money),
    maintenance_money(NULL, ROT, gui_label_t::money),
    old_conmoney(NULL, WEISS, gui_label_t::money),
    old_nvmoney(NULL, WEISS, gui_label_t::money),
    old_vrmoney(NULL, WEISS, gui_label_t::money),
    old_imoney(NULL, WEISS, gui_label_t::money),
    old_tmoney(NULL, WEISS, gui_label_t::money),
    old_mmoney(NULL, WEISS, gui_label_t::money),
    gtmoney(NULL, WEISS, gui_label_t::money),
    warn("", ROT)
{
	if(sp->gib_welt()->gib_spieler(0)!=sp) {
		sprintf(money_frame_title,"Finances of %s",translator::translate(sp->gib_name()) );
		setze_name(money_frame_title);
	}

    this->sp = sp;

    const int top = 20;
    const int left = 12;

    //CHART YEAR
    chart = new gui_chart_t();
    chart->setze_pos(koord(1,1));
    chart->setze_groesse(koord(443,120));
    chart->set_dimension(12, 10000);
    chart->set_seed(umgebung_t::starting_year+sp->gib_welt()->get_last_year());
    chart->set_background(MN_GREY1);
    for ( int i = 0; i<MAX_COST; i++)
    {
    	chart->add_curve(cost_type_color[i], (sint64 *)sp->get_finance_history_year(), MAX_COST, i, 12, MONEY, false, false);
    }
    //CHART YEAR END

    //CHART MONTH
    mchart = new gui_chart_t();
    mchart->setze_pos(koord(1,1));
    mchart->setze_groesse(koord(443,120));
    mchart->set_dimension(MAX_HISTORY_MONTHS, 10000);
    mchart->set_seed(0);
    mchart->set_background(MN_GREY1);
    for ( int i = 0; i<MAX_COST; i++)
    {
    	mchart->add_curve(cost_type_color[i], (sint64 *)sp->get_finance_history_month(), MAX_COST, i, 12, MONEY, false, false);
    }
    mchart->set_visible(false);
    //CHART MONTH END

    // tab (month/year)
    year_month_tabs.add_tab(chart, translator::translate("Years"));
    year_month_tabs.add_tab(mchart, translator::translate("Months"));
    year_month_tabs.setze_pos(koord(145, top+11*LINESPACE));
    year_month_tabs.setze_groesse(koord(443, 125));
    add_komponente(&year_month_tabs);

    clabel.setze_pos(koord(left,top));
    nvlabel.setze_pos(koord(left,top+1*LINESPACE));
    vrlabel.setze_pos(koord(left,top+2*LINESPACE));
    mlabel.setze_pos(koord(left,top+3*LINESPACE));
    ilabel.setze_pos(koord(left,top+4*LINESPACE));
    tlabel.setze_pos(koord(left,top+5*LINESPACE));

    lylabel.setze_pos(koord(left+140+80,top-1*LINESPACE-4));
    tylabel.setze_pos(koord(left+240+80,top-1*LINESPACE-4));

    old_conmoney.setze_pos(koord(left+140+55,top));
    old_nvmoney.setze_pos(koord(left+140+55,top+1*LINESPACE));
    old_vrmoney.setze_pos(koord(left+140+55,top+2*LINESPACE));
    old_mmoney.setze_pos(koord(left+140+55,top+3*LINESPACE));
    old_imoney.setze_pos(koord(left+140+55,top+4*LINESPACE));
    old_tmoney.setze_pos(koord(left+140+55,top+5*LINESPACE));

    conmoney.setze_pos(koord(left+240+55,top));
    nvmoney.setze_pos(koord(left+240+55,top+1*LINESPACE));
    vrmoney.setze_pos(koord(left+240+55,top+2*LINESPACE));
    mmoney.setze_pos(koord(left+240+55,top+3*LINESPACE));
    imoney.setze_pos(koord(left+240+55,top+4*LINESPACE));
    tmoney.setze_pos(koord(left+240+55,top+5*LINESPACE));

    gtlabel.setze_pos(koord(left, top+6*LINESPACE+4));
    gtmoney.setze_pos(koord(left+240+55, top+6*LINESPACE));

    maintenance_label.setze_pos(koord(left, top+8*LINESPACE));
    maintenance_money.setze_pos(koord(left+160+55, top+8*LINESPACE));

    warn.setze_pos(koord(left,top+9*LINESPACE+4));

    add_komponente(&clabel);
    add_komponente(&nvlabel);
    add_komponente(&vrlabel);
    add_komponente(&mlabel);
    add_komponente(&ilabel);
    add_komponente(&tlabel);

    add_komponente(&conmoney);
    add_komponente(&nvmoney);
    add_komponente(&vrmoney);
    add_komponente(&mmoney);
    add_komponente(&imoney);
    add_komponente(&tmoney);

    add_komponente(&old_conmoney);
    add_komponente(&old_nvmoney);
    add_komponente(&old_vrmoney);
    add_komponente(&old_mmoney);
    add_komponente(&old_imoney);
    add_komponente(&old_tmoney);

    add_komponente(&gtlabel);
    add_komponente(&gtmoney);

    add_komponente(&lylabel);
    add_komponente(&tylabel);

    add_komponente(&maintenance_label);
    add_komponente(&maintenance_money);

    add_komponente(&warn);

    // add filter buttons
    for (int cost=0;cost<MAX_COST;cost++)
    {
	filterButtons[cost].init(button_t::box, translator::translate(cost_type[cost]), koord(left+335+125*(cost%2), 18*((int)cost/2+1)), koord(120, 15));
	filterButtons[cost].add_listener(this);
	filterButtons[cost].background = cost_type_color[cost];
	bFilterIsActive[cost] = false;
	add_komponente(filterButtons + cost);
    }

    setze_fenstergroesse(koord(605, 300));
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

    old_conmoney.setze_text(display_money(COST_CONSTRUCTION, str_buf[6], 1));
    old_nvmoney.setze_text(display_money(COST_NEW_VEHICLE, str_buf[7], 1));
    old_vrmoney.setze_text(display_money(COST_VEHICLE_RUN, str_buf[8], 1));
    old_mmoney.setze_text(display_money(COST_MAINTENANCE, str_buf[9], 1));
    old_imoney.setze_text(display_money(COST_INCOME, str_buf[10], 1));
    old_tmoney.setze_text(display_money(COST_PROFIT, str_buf[11], 1));

    conmoney.set_color(get_money_colour(COST_CONSTRUCTION, 0));
    nvmoney.set_color(get_money_colour(COST_NEW_VEHICLE, 0));
    vrmoney.set_color(get_money_colour(COST_VEHICLE_RUN, 0));
    mmoney.set_color(get_money_colour(COST_MAINTENANCE, 0));
    imoney.set_color(get_money_colour(COST_INCOME, 0));
    tmoney.set_color(get_money_colour(COST_PROFIT, 0));

    old_conmoney.set_color(get_money_colour(COST_CONSTRUCTION, 1));
    old_nvmoney.set_color(get_money_colour(COST_NEW_VEHICLE, 1));
    old_vrmoney.set_color(get_money_colour(COST_VEHICLE_RUN, 1));
    old_mmoney.set_color(get_money_colour(COST_MAINTENANCE, 1));
    old_imoney.set_color(get_money_colour(COST_INCOME, 1));
    old_tmoney.set_color(get_money_colour(COST_PROFIT, 1));

    gtmoney.setze_text(display_money(COST_CASH, str_buf[12], 0));
    gtmoney.set_color(get_money_colour(COST_CASH, 0));

    if(sp->gib_konto_ueberzogen()) {
	sprintf(str_buf[13], translator::translate("Du hast %d Monate Zeit, deine Schulden zurückzuzahlen"),
	    spieler_t::MAX_KONTO_VERZUG-sp->gib_konto_ueberzogen()+1);
    }
    else {
	str_buf[13][0] = '\0';
    }
    warn.setze_text(str_buf[13]);

    // Hajo: Money is counted in credit cents (100 cents = 1 Cr)
    money_to_string(str_buf[14], sp->add_maintenance(0)/100);
    maintenance_money.setze_text(str_buf[14]);

    for (int i = 0;i<MAX_COST;i++)
    {
    	filterButtons[i].pressed = bFilterIsActive[i];
    	// year_month_toggle.pressed = mchart->is_visible();
    }

    // Hajo: update chart seed
    chart->set_seed(umgebung_t::starting_year+sp->gib_welt()->get_last_year());


    gui_frame_t::zeichnen(pos, gr);

}

bool money_frame_t::action_triggered(gui_komponente_t *komp)
{
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
