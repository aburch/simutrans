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
#include "../simdepot.h"
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
#include "../boden/grund.h"
#include "messagebox.h"

#include "../utils/simstring.h"

#include "components/list_button.h"

#include "convoi_detail_t.h"

const char cost_type[MAX_CONVOI_COST][64] =
{
	"Free Capacity",
	"Transported",
	"Revenue",
	"Operation",
	"Profit"
};

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = destination
 *         1 = via
 *         2 = via_amount
 *         3 = amount
 * @author prissi
 */
convoi_info_t::sort_mode_t convoi_info_t::global_sortby = by_destination;

const char *convoi_info_t::sort_text[SORT_MODES] = {
  "Zielort",
  "via",
  "via Menge",
  "Menge"
};

const int cost_type_color[MAX_CONVOI_COST] =
{
  COL_FREE_CAPACITY, COL_TRANSPORTED, COL_REVENUE, COL_OPERATION, COL_PROFIT
};


/**
 * Konstruktor.
 * @author Hj. Malthaner
 */
convoi_info_t::convoi_info_t(convoihandle_t cnv)
: gui_frame_t(cnv->gib_name(), cnv->gib_besitzer()),
  scrolly(&text),
  text(" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"),
  view(cnv->gib_welt(), cnv->gib_vehikel(0)),
  sort_label(translator::translate("loaded passenger/freight")),
  freight_info(8192)
{
	this->cnv = cnv;
	this->mean_convoi_speed = speed_to_kmh(cnv->gib_akt_speed()*4);
	this->max_convoi_speed = speed_to_kmh(cnv->gib_min_top_speed()*4);
	this->sortby = global_sortby;

	input.setze_pos(koord(11,4));
	input.setze_text( cnv->access_internal_name(), 116);
	add_komponente(&input);

	add_komponente(&sort_label);

	toggler.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	toggler.setze_text("Chart");
	toggler.setze_typ(button_t::roundbox_state);
	toggler.add_listener(this);
	toggler.set_tooltip("Show/hide statistics");
	add_komponente(&toggler);
	toggler.pressed = false;

	sort_button.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	sort_button.setze_text(sort_text[sortby]);
	sort_button.setze_typ(button_t::roundbox);
	sort_button.add_listener(this);
	sort_button.set_tooltip("Sort by");
	add_komponente(&sort_button);

	details_button.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	details_button.setze_text("Details");
	details_button.setze_typ(button_t::roundbox);
	details_button.add_listener(this);
	details_button.set_tooltip("Vehicle details");
	add_komponente(&details_button);

	scrolly.setze_pos(koord(0, 122));
	add_komponente(&scrolly);

	filled_bar.add_color_value(&cnv->get_loading_limit(), COL_YELLOW);
	filled_bar.add_color_value(&cnv->get_loading_level(), COL_GREEN);
	add_komponente(&filled_bar);

	speed_bar.set_base(max_convoi_speed);
	speed_bar.set_vertical(false);
	speed_bar.add_color_value(&mean_convoi_speed, COL_GREEN);
	add_komponente(&speed_bar);

	// we update this ourself!
	route_bar.add_color_value(&cnv_route_index, COL_GREEN);
	add_komponente(&route_bar);

	setze_opaque(true);
	setze_fenstergroesse(koord(TOTAL_WIDTH, 278));

	// chart
	chart.setze_pos(koord(44,76+BUTTON_HEIGHT+8));
	chart.setze_groesse(koord(TOTAL_WIDTH-44-4, 100));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(MN_GREY1);
	for (int cost = 0; cost<MAX_CONVOI_COST; cost++) {
		chart.add_curve(cost_type_color[cost], cnv->get_finance_history(), MAX_CONVOI_COST, cost, MAX_MONTHS, cost<MAX_CONVOI_NON_MONEY_TYPES ? 0 : 1, false, true);
		filterButtons[cost].init(button_t::box_state, cost_type[cost], koord(BUTTON1_X+(BUTTON_WIDTH+BUTTON_SPACER)*(cost%4), 230+(BUTTON_HEIGHT+2)*(cost/4)), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background = cost_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_komponente(filterButtons + cost);
	}
	add_komponente(&chart);
	add_komponente(&view);

	// this convoi belongs not to an AI
	button.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	button.setze_text("Fahrplan");
	button.setze_typ(button_t::roundbox);
	button.set_tooltip("Alters a schedule.");
	add_komponente(&button);
	button.setze_pos(koord(BUTTON1_X,76));
	button.add_listener(this);

	go_home_button.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	go_home_button.setze_pos(koord(BUTTON2_X,76));
	go_home_button.setze_text("go home");
	go_home_button.setze_typ(button_t::roundbox);
	go_home_button.set_tooltip("Sends the convoi to the last depot it departed from!");
	add_komponente(&go_home_button);
	go_home_button.add_listener(this);

	follow_button.setze_groesse(koord(66, BUTTON_HEIGHT));
	follow_button.setze_text("follow me");
	follow_button.setze_typ(button_t::roundbox_state);
	follow_button.set_tooltip("Follow the convoi on the map.");
	add_komponente(&follow_button);
	follow_button.add_listener(this);

	cnv->set_sort( sortby );

	set_min_windowsize(koord(TOTAL_WIDTH, 194));
	set_resizemode(diagonal_resize);
	resize(koord(0,0));
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
	if(!cnv.is_bound()) {
		destroy_win(dynamic_cast <gui_fenster_t *> (this));
	}
	else {
		if(cnv->gib_besitzer()==cnv->gib_welt()->get_active_player()) {
			button.enable();
			go_home_button.enable();
		}
		else {
			button.disable();
			go_home_button.disable();
		}
		follow_button.pressed = (cnv->gib_welt()->get_follow_convoi()==cnv);

		// buffer update now only when needed by convoi itself => dedicated buffer for this
		cnv->get_freight_info(freight_info);
		text.setze_text(freight_info);

		route_bar.set_base(cnv->get_route()->gib_max_n());
		cnv_route_index = cnv->gib_vehikel(0)->gib_route_index()-1;

		// all gui stuff set => display it
		gui_frame_t::zeichnen(pos, gr);

		// convoi information
		static char tmp[256];
		static cbuffer_t info_buf(256);

		// use median speed to avoid flickering
		mean_convoi_speed += speed_to_kmh(cnv->gib_akt_speed()*4);
		mean_convoi_speed /= 2;
		sprintf(tmp,translator::translate("%i km/h (max. %ikm/h)"), (mean_convoi_speed+3)/4, speed_to_kmh(cnv->gib_min_top_speed()) );
		display_proportional( pos.x+11, pos.y+16+20, tmp, ALIGN_LEFT, COL_BLACK, true );

		// next important: income stuff
		info_buf.clear();
		info_buf.append( translator::translate("Gewinn") );
		int len = display_proportional( pos.x+11, pos.y+16+20+1*LINESPACE, info_buf, ALIGN_LEFT, COL_BLACK, true )+5;
		money_to_string( tmp, cnv->gib_jahresgewinn()/100.0 );
		len += display_proportional( pos.x+11+len, pos.y+16+20+1*LINESPACE, tmp, ALIGN_LEFT, cnv->gib_jahresgewinn()>0?MONEY_PLUS:MONEY_MINUS, true )+5;
		sprintf(tmp," (%1.2f$/km)", cnv->get_running_cost()/100.0 );
		display_proportional( pos.x+11+len, pos.y+16+20+1*LINESPACE, tmp, ALIGN_LEFT, COL_BLACK, true );

		// the weight entry
		info_buf.clear();
		info_buf.append( translator::translate("Gewicht") );
		info_buf.append( ": " );
		info_buf.append( cnv->gib_sum_gesamtgewicht() );
		info_buf.append( " (" );
		info_buf.append( cnv->gib_sum_gesamtgewicht()-cnv->gib_sum_gewicht() );
		info_buf.append( ") t" );
		display_proportional( pos.x+11, pos.y+16+20+2*LINESPACE, info_buf, ALIGN_LEFT, COL_BLACK, true );

		// next stop
		const fahrplan_t * fpl = cnv->gib_fahrplan();
		info_buf.clear();
		info_buf.append(translator::translate("Fahrtziel"));
		fahrplan_gui_t::gimme_short_stop_name(info_buf, cnv->gib_welt(), fpl, fpl->aktuell, 34);
		len = display_proportional( pos.x+11, pos.y+16+20+3*LINESPACE, info_buf, ALIGN_LEFT, COL_BLACK, true );

		// convoi load indicator
		const int offset = max( len, 167)+3;
		route_bar.setze_pos(koord(offset,22+3*LINESPACE));
		route_bar.setze_groesse(koord(view.gib_pos().x-offset-5, 4));

		/*
		* only show assigned line, if there is one!
		* @author hsiegeln
		*/
		if (cnv->get_line().is_bound()) {
			sint16 w = display_proportional( pos.x+11, pos.y+16+20+4*LINESPACE, translator::translate("Serves Line:"), ALIGN_LEFT, COL_BLACK, true );
			display_proportional( pos.x+11+w+5, pos.y+16+20+4*LINESPACE, cnv->get_line()->get_name(), ALIGN_LEFT, cnv->get_line()->get_state_color(), true );
		}
	}
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool convoi_info_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	// follow convoi on map?
	if(komp == &follow_button) {
		if(cnv->gib_welt()->get_follow_convoi()==cnv) {
			// stop following
			cnv->gib_welt()->set_follow_convoi( convoihandle_t() );
		}
		else {
			cnv->gib_welt()->set_follow_convoi(cnv);
		}
		return true;
	}

	// datails?
	if(komp == &details_button) {
		create_win(20, 20, -1, new convoi_detail_t(cnv), w_autodelete);
		return true;
	}

	// sort by what
	if(komp == &sort_button) {
		// sort by what
		sortby = (sort_mode_t)((int)(sortby+1)%(int)SORT_MODES);
		sort_button.setze_text(sort_text[sortby]);
		global_sortby = sortby;
		cnv->set_sort( sortby );
//		sorteddir.setze_text(translator::translate( sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc"));
	}

	// some actions only allowed, when I am the player
	if(cnv->gib_besitzer()==cnv->gib_welt()->get_active_player()) {

		if(komp == &button)     //Fahrplan
		{
			cnv->open_schedule_window();
			return true;
		}

		if(komp == &go_home_button)
		{
			// limit update to certain states that are considered to be save for fahrplan updates
			int state = cnv->get_state();
			if(state==convoi_t::FAHRPLANEINGABE  /*||  state==convoi_t::ROUTING_4  ||  state==convoi_t::ROUTING_5*/) {
DBG_MESSAGE("convoi_info_t::action_triggered()","convoi state %i => cannot change schedule ... ", state );
				return true;
			}

			// find all depots
			slist_tpl<koord3d> all_depots;
			karte_t * welt = cnv->gib_welt();
			depot_t * depot = NULL;
			const waytype_t waytype = cnv->gib_vehikel(0)->gib_besch()->gib_typ();
			for (int x = 0; x < welt->gib_groesse_x(); x++) {
				for (int y = 0; y < welt->gib_groesse_y(); y++) {
					grund_t * gr = welt->lookup(koord(x,y))->gib_kartenboden();
					if (gr) {
						depot = gr->gib_depot();
						if(depot  &&  (depot->get_wegtyp()!=waytype  ||  depot->gib_besitzer()!=cnv->gib_besitzer())) {
							// that was not the right depot for this vehicle
							depot = NULL;
						}
					}
					if (depot) {
						all_depots.append(gr->gib_pos());
DBG_MESSAGE("convoi_info_t::action_triggered()","search depot: found on %i,%i",gr->gib_pos().x,gr->gib_pos().y);
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
				bool found = cnv->gib_vehikel(0)->calc_route(welt, cnv->gib_pos(), pos,  50, route );	// do not care about speed
				if (found) {
					if (route->gib_max_n() < shortest_route->gib_max_n() || shortest_route->gib_max_n() == -1) {
						shortest_route->kopiere(route);
						home = pos;
					}
				}
			}
			delete route;
			DBG_MESSAGE("shortest route has ", "%i hops", shortest_route->gib_max_n());

			// if route to a depot has been found, update the convoi's schedule
			bool b_depot_found = false;
			if (shortest_route->gib_max_n() > -1) {
				fahrplan_t *fpl = cnv->gib_fahrplan();
				fpl->insert(welt, welt->lookup(home) );
				b_depot_found = cnv->setze_fahrplan(fpl);
			}
			delete shortest_route;

			// show result
			if (b_depot_found) {
				create_win(-1, -1, 120, new nachrichtenfenster_t(cnv->gib_welt(), translator::translate("Convoi has been sent\nto the nearest depot\nof appropriate type.\n")), w_autodelete);
			} else {
				create_win(-1, -1, 120, new nachrichtenfenster_t(cnv->gib_welt(), translator::translate("Home depot not found!\nYou need to send the\nconvoi to the depot\nmanually.")), w_autodelete);
			}
		} // end go home button
	}

	if (komp == &toggler) {
		toggler.pressed = !toggler.pressed;
		const koord offset = toggler.pressed ? koord(0, 170) : koord(0, -170);
		set_min_windowsize( koord(TOTAL_WIDTH, toggler.pressed ? 364: 194));
		scrolly.pos.y += offset.y;
		// toggle visibility of components
		chart.set_visible(toggler.pressed);
		setze_fenstergroesse(gib_fenstergroesse() + offset);
		resize(koord(0,0));
		for (int i=0;i<MAX_CONVOI_COST;i++) {
			filterButtons[i].set_visible(toggler.pressed);
		}
		return true;
	}

	for ( int i = 0; i<MAX_CONVOI_COST; i++)
	{
		if (komp == &filterButtons[i])
		{
		  filterButtons[i].pressed = !filterButtons[i].pressed;
		  if(filterButtons[i].pressed) {
		    chart.show_curve(i);
		  } else {
		    chart.hide_curve(i);
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

	input.setze_groesse(koord(gib_fenstergroesse().x-22, 13));

	view.setze_pos(koord(gib_fenstergroesse().x - 64 - 12 , 21));
	follow_button.setze_pos(koord(view.gib_pos().x-1,77));

	scrolly.setze_groesse(get_client_windowsize()-scrolly.gib_pos());

	const int yoff = scrolly.gib_pos().y-BUTTON_HEIGHT-2;
	sort_button.setze_pos(koord(BUTTON1_X,yoff));
	toggler.setze_pos(koord(BUTTON3_X,yoff));
	details_button.setze_pos(koord(BUTTON4_X,yoff));
	sort_label.setze_pos(koord(BUTTON1_X,yoff-LINESPACE));

	// convoi speed indicator
	speed_bar.setze_pos(koord(170,22+0*LINESPACE));
	speed_bar.setze_groesse(koord(view.gib_pos().x - 175, 4));

	// convoi load indicator
	filled_bar.setze_pos(koord(170,22+2*LINESPACE));
	filled_bar.setze_groesse(koord(view.gib_pos().x - 175, 4));
}
