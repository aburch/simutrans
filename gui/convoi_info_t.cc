/*
 * convoi_info_t.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include <stdio.h>

#include "convoi_info_t.h"

#include "../simplay.h"
#include "../simconvoi.h"
#include "../simvehikel.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simworld.h"
#include "../dataobj/fahrplan.h"
#include "../dataobj/translator.h"
#include "fahrplan_gui.h"
// @author hsiegeln
#include "../simlinemgmt.h"
#include "../simline.h"
#include "components/gui_chart.h"
#include "../boden/grund.h"
#include "messagebox.h"

const char cost_type[MAX_CONVOI_COST][64] =
{
  "Free Capacity", "Transported Goods", "Convoi Size", "Revenue",
  "Operation", "Profit"
};


const int cost_type_color[MAX_CONVOI_COST] =
{
  7, 11, 15, 132, 23, 27
};


/**
 * Konstruktor.
 * @author Hj. Malthaner
 */
convoi_info_t::convoi_info_t(convoihandle_t cnv)
: gui_frame_t(cnv->gib_name(), cnv->gib_besitzer()->kennfarbe),
  scrolly(&text),
  text(" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"),
  view(cnv->gib_welt(), cnv->gib_pos().gib_2d()),
  freight_info(32768)
{
    this->cnv = cnv;

    input.setze_pos(koord(11,4));
    input.setze_groesse(koord(189, 13));
    input.setze_text(cnv->access_name(), 48);

     toggler.setze_groesse(koord(10*8+4, 13));
    toggler.setze_pos(view.gib_pos() + koord(11, 105));
    toggler.text = translator::translate("Chart");
    toggler.setze_typ(button_t::roundbox);
    toggler.add_listener(this);
    toggler.set_tooltip(translator::translate("Show/hide statistics"));
    add_komponente(&toggler);
    btoggled = false;

    scrolly.setze_pos(koord(0, 122));

    filled_bar.add_color_value(&cnv->get_loading_limit(), GELB);
    filled_bar.add_color_value(&cnv->get_loading_level(), GREEN);
    add_komponente(&filled_bar);

    speed_bar.set_base(cnv->gib_vehikel(0)->gib_speed());
    speed_bar.set_vertical(true);
    speed_bar.add_color_value(&cnv->gib_akt_speed(), GREEN);
    add_komponente(&speed_bar);

    setze_opaque(true);
    setze_fenstergroesse(koord(304, 278));

    // chart
    chart = new gui_chart_t;
    chart->setze_pos(koord(50,130));
    chart->setze_groesse(koord(280, 100));
    chart->set_dimension(12, 10000);
    chart->set_visible(false);
    chart->set_background(MN_GREY1);

    for (int i = 0; i<MAX_CONVOI_COST; i++)
    {
    	chart->add_curve(cost_type_color[i],
			 cnv->get_finance_history(),
			 MAX_CONVOI_COST,
			 i,
			 MAX_MONTHS,
			 i < MAX_CONVOI_NON_MONEY_TYPES ? 0 : 1,
			 false,
			 true);
    }
    add_komponente(chart);

    for (int cost=0;cost<MAX_CONVOI_COST;cost++) {

      filterButtons[cost].init(button_t::box,
			       cost_type[cost],
			       koord(11+107*(cost%3),
				     235+15*((int)cost/3+1)),
			       koord(106, 14));
      filterButtons[cost].add_listener(this);
      filterButtons[cost].background = cost_type_color[cost];
      filterButtons[cost].set_visible(false);
      bFilterIsActive[cost] = false;
      add_komponente(filterButtons + cost);
    }

    add_komponente(&scrolly);
    add_komponente(&input);
    add_komponente(&view);

    set_min_windowsize(koord(304, 194));
    set_resizemode(diagonal_resize);
    resize(koord(0,0));

    if(cnv->gib_besitzer()->automat==false) {
       // this convoi belongs not to an AI
       button.setze_groesse(koord(10*8+4, 14));
       button.text = translator::translate("Fahrplan");
       button.setze_typ(button_t::roundbox);

       go_home_button.setze_groesse(koord(10*8+4, 14));
       go_home_button.text = translator::translate("go home");
       go_home_button.setze_typ(button_t::roundbox);
       go_home_button.set_tooltip(translator::translate("Sends the convoi to the last depot it departed from!"));

       kill_button.setze_groesse(koord(15, 14));
       kill_button.text = "X";
       kill_button.setze_typ(button_t::roundbox);
       kill_button.set_tooltip(translator::translate("Remove vehicle from map. Use with care!"));

      add_komponente(&button);
       add_komponente(&go_home_button);
       add_komponente(&kill_button);
       button.add_listener(this);
       go_home_button.add_listener(this);
       kill_button.add_listener(this);
    }
}


/**
 * Destruktor.
 * @author Hj. Malthaner
 */
convoi_info_t::~convoi_info_t()
{

}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void
convoi_info_t::zeichnen(koord pos, koord gr)
{
    koord viewpos = view.gib_pos(); // 01-June-02  markus weber   added

    if(cnv.is_bound()) {

	/* // Hajo: Newly build convois do not have vehicles and thus
	// they do not have names. At create time of this window the
        // name might be NULL. Because of that problem we need to
        // dynamically set the name here, input field and window title.
	input.setze_text(cnv->access_name(), 48);
	setze_name(input.gib_text());*/

	// update window contents
	view.set_location(cnv->gib_pos().gib_2d());

	freight_info.clear();
	cnv->get_freight_info(freight_info);

	text.setze_text(freight_info);

	for (int i = 0; i<MAX_CONVOI_COST; i++) {
	  filterButtons[i].pressed = bFilterIsActive[i];
	}
	toggler.pressed = btoggled;

	gui_frame_t::zeichnen(pos, gr);

	display_ddd_box(pos.x+viewpos.x-1,
			pos.y+viewpos.y+16,
			66,
			57,
			MN_GREY0,
			MN_GREY4);

	// Hajo: Reuse of freight_info buffer to get and display
	// information about the convoi itself
	freight_info.clear();
	cnv->info(freight_info);

	freight_info.append(" ");
	freight_info.append(translator::translate("Ladegrad:"));
	freight_info.append("\n");

	const fahrplan_t * fpl = cnv->gib_fahrplan();

	freight_info.append(" ");
	freight_info.append(translator::translate("Fahrtziel:"));
	freight_info.append(" ");


	fahrplan_gui_t::gimme_stop_name(freight_info,
					cnv->gib_welt(),
					fpl,
					fpl->aktuell,
					26);

	freight_info.append("\n");


	/*
	 * only show assigned line, if there is one!
	 * @author hsiegeln
	 */
	if (cnv->get_line() != NULL)
	{
	  freight_info.append(" ");
	  freight_info.append(translator::translate("Serves Line:"));
	  freight_info.append(" ");
	  freight_info.append(cnv->get_line()->get_name());
	}

	display_multiline_text(pos.x+11,
			       pos.y+30,
			       freight_info,
			       SCHWARZ);
    }
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool convoi_info_t::action_triggered(gui_komponente_t *komp)
{
	if(komp == &button)     //Fahrplan
	{
		cnv->open_schedule_window();
		return true;
	}

	if(komp == &go_home_button)
	{
		// limit update to certain states that are considered to be save for fahrplan updates
		int state = cnv->get_state();
		if ((state == 4) || (state == 5)) {
			return true;
		}
/*	bool b_depot_found = false;
		koord3d home = cnv->get_home_depot();
		const grund_t *gr = cnv->gib_welt()->lookup(home);
		if (gr) {
			depot_t * dp = gr->gib_depot();
			if(dp) {
				fahrplan_t *fpl = cnv->gib_fahrplan();
				fpl->insert(cnv->gib_welt(), home);
				b_depot_found = cnv->setze_fahrplan(fpl);
			}
		}
		if (!b_depot_found) {
			create_win(-1, -1, 120, new nachrichtenfenster_t(cnv->gib_welt(), translator::translate("Home depot not found!\nYou need to send the\nconvoi to the depot\nmanually.")), w_autodelete);
		}
		*/

		// find all depots
		slist_tpl<koord3d> all_depots;
		karte_t * welt = cnv->gib_welt();
		int world_size = welt->gib_groesse();
		depot_t * depot = NULL;
		for (int x = 0; x < world_size; x++) {
			for (int y = 0; y < world_size; y++) {
				grund_t * gr = welt->lookup(koord(x,y))->gib_kartenboden();
				if (gr) {
					fahrer_t * fahr = cnv->gib_vehikel(0);
					// only add depots to list, which are compatible with convoi
					if (fahr->ist_befahrbar(gr)) {
						depot = gr->gib_depot();
					}
				}
				if (depot) {
					all_depots.append(gr->gib_pos());
					depot = NULL;
				}
			}
		}

		// iterate over all depots and try to find shortest route
		slist_iterator_tpl<koord3d> depot_iter(all_depots);
		route_t * shortest_route = new route_t();
		route_t * route = new route_t();
		koord3d home = koord3d(0,0,0);
		while (depot_iter.next()) {
			koord3d pos = depot_iter.get_current();
			bool found = route->calc_route(welt, cnv->gib_pos(), pos, cnv->gib_vehikel(0));
			if (found) {
				if (route->gib_max_n() < shortest_route->gib_max_n() || shortest_route->gib_max_n() == -1) {
					shortest_route->kopiere(route);
					home = pos;
				}
			}
		}
		dbg->message("shortest route has ", "%i hops", shortest_route->gib_max_n());

		// shortest path by Hajo
/*		karte_t * welt = cnv->gib_welt();
		route_t * shortest_route = new route_t();
		shortest_route->find_path(welt, cnv->gib_pos(), cnv->gib_vehikel(0), ding_t::bahndepot);
		const koord3d home = shortest_route->position_bei(0);
		dbg->message("Depot at: ", "%i,%i,%i", home.x, home.y, home.z);
*/
		// if route to a depot has been found, update the convoi's schedule
		bool b_depot_found = false;
		if (shortest_route->gib_max_n() > -1) {
			fahrplan_t *fpl = cnv->gib_fahrplan();
			fpl->insert(welt, home);
			b_depot_found = cnv->setze_fahrplan(fpl);
		}

		// show result
		if (b_depot_found) {
			create_win(-1, -1, 120, new nachrichtenfenster_t(cnv->gib_welt(), translator::translate("Convoi has been sent\nto the nearest depot\nof appropriate type.\n")), w_autodelete);
		} else {
			create_win(-1, -1, 120, new nachrichtenfenster_t(cnv->gib_welt(), translator::translate("Home depot not found!\nYou need to send the\nconvoi to the depot\nmanually.")), w_autodelete);
		}
	} // end go home button

	if(komp == &kill_button)     // Destroy convoi -> deletes us, too!
	{
	        destroy_win(dynamic_cast <gui_fenster_t *> (this));
		cnv->self_destruct();
		return true;
	}

	if (komp == &toggler)
        {
		btoggled = !btoggled;
		const koord offset = btoggled ? koord(40, 170) : koord(-40, -170);
		set_min_windowsize(btoggled ? koord(344, 364) : koord(304, 194));
		scrolly.pos.y += offset.y;
		// toggle visibility of components
		chart->set_visible(btoggled);
		setze_fenstergroesse(gib_fenstergroesse() + offset);
		resize(koord(0,0));
		for (int i=0;i<MAX_CONVOI_COST;i++)
		{
			filterButtons[i].set_visible(btoggled);
		}
		return true;
	}

	for ( int i = 0; i<MAX_CONVOI_COST; i++)
	{
		if (komp == &filterButtons[i])
		{
		  bFilterIsActive[i] = !bFilterIsActive[i];
		  if(bFilterIsActive[i]) {
		    chart->show_curve(i);
		  } else {
		    chart->hide_curve(i);
		  }

		  return true;
		}
	}

  	return false;
}


/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void convoi_info_t::resize(const koord delta)
{
    gui_frame_t::resize(delta);

    scrolly.setze_groesse(get_client_windowsize()-scrolly.gib_pos());
    button.setze_pos(koord(get_client_windowsize().x - 93,4));
    view.setze_pos(koord(get_client_windowsize().x - 64 - 20 , 21));

    kill_button.setze_pos(koord(get_client_windowsize().x - 30 , 105));
    go_home_button.setze_pos(koord(get_client_windowsize().x - 93 , 80));

    filled_bar.setze_pos(koord(100, 72));
    filled_bar.setze_groesse(koord(view.gib_pos().x - 120, 4));

    speed_bar.setze_pos(koord(view.gib_pos().x + view.gib_groesse().x + 4, view.gib_pos().y));
    speed_bar.setze_groesse(koord(4, view.gib_groesse().y));
}
