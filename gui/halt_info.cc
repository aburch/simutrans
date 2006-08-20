/*
 * halt_info.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include <stdlib.h>

#include "halt_info.h"

#include "../simworld.h"
#include "../simhalt.h"
#include "../simplay.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simskin.h"
#include "../freight_list_sorter.h"

#include "../dataobj/translator.h"
#include "components/list_button.h"
#include "components/gui_chart.h"
#include "../besch/skin_besch.h"



static const char *sort_text[4] = {
  "Zielort",
  "via",
  "via Menge",
  "Menge"
};

const char cost_type[MAX_HALT_COST][64] =
{
  "Happy",
  "Unhappy",
  "No Route",
  "Waiting",
  "Arrived",
  "Departed",
  "Vehicles"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_NOROUTE,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED
};

const int cost_type_color[MAX_HALT_COST] =
{
  7, 11, 15, 132, 23, 27, 31
};

halt_info_t::halt_info_t(karte_t *welt, halthandle_t halt)
 : gui_frame_t(halt->access_name(), halt->gib_besitzer()->kennfarbe),
  scrolly(&text),
  text("                                                                                     "
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
       " \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
      ),
  sort_label(translator::translate("Hier warten/lagern:")),
   view(welt, halt->gib_basis_pos3d()),
   freight_info(16384)
{
	this->halt = halt;
	this->welt = welt;

	input.setze_pos(koord(11,4));
	input.setze_text(halt->access_name(), 48);

	add_komponente(&sort_label);

	// hsiegeln: added sort_button
	sort_button.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	sort_button.setze_pos(koord(BUTTON1_X, 78));
	sort_button.text = translator::translate("Zielort");
	sort_button.setze_typ(button_t::roundbox);
	sort_button.set_tooltip(translator::translate("Sort waiting list by"));

	toggler.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	toggler.setze_pos(koord(BUTTON3_X, 78));
	toggler.text = translator::translate("Chart");
	toggler.setze_typ(button_t::roundbox);
	toggler.set_tooltip(translator::translate("Show/hide statistics"));
	toggler.add_listener(this);
	add_komponente(&toggler);
	btoggled = false;

	button.setze_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	button.setze_pos(koord(BUTTON4_X, 78));
	button.text = translator::translate("Details");
	button.setze_typ(button_t::roundbox);

	scrolly.setze_pos(koord(1, 78+BUTTON_HEIGHT+4));

	add_komponente(&sort_button);
	add_komponente(&button);
	add_komponente(&input);

	setze_opaque(true);
	setze_fenstergroesse(koord(BUTTON4_X+BUTTON_WIDTH+2, 264));
	set_min_windowsize(koord(BUTTON4_X+BUTTON_WIDTH+2, 194));
	set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
	resize(koord(0,0));

	button.add_listener(this);
	sort_button.add_listener(this);

	// chart
	chart = new gui_chart_t;
	chart->setze_pos(koord(46,80));
	chart->setze_groesse(koord(BUTTON4_X+BUTTON_WIDTH-50, 100));
	chart->set_dimension(12, 10000);
	chart->set_visible(false);
	chart->set_background(MN_GREY1);
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		chart->add_curve(cost_type_color[cost], halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true);

		filterButtons[cost].init(button_t::box, cost_type[cost],
			koord(BUTTON1_X+(BUTTON_WIDTH+BUTTON_SPACER)*(cost%4), 198+(BUTTON_HEIGHT+2)*(cost/4) ),
			koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background = cost_type_color[cost];
		filterButtons[cost].set_visible(false);
		bFilterIsActive[cost] = false;
		add_komponente(filterButtons + cost);
    }
    add_komponente(chart);

    add_komponente(&view);
    add_komponente(&scrolly);
}


/**
 * Komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void
halt_info_t::zeichnen(koord pos, koord gr)
{
	koord viewpos = view.gib_pos(); // 31-May-02  markus weber   added

	if(halt.is_bound()) {
		// buffer update now only when needed by halt itself => dedicated buffer for this
		int old_len=freight_info.len();
		halt->get_freight_info(freight_info);
		if(old_len!=freight_info.len()) {
			text.setze_text(freight_info);
			resize( koord(0,0) );	// recalcs slider
		}

		for (int i = 0;i<MAX_HALT_COST;i++) {
			filterButtons[i].pressed = bFilterIsActive[i];
		}
		toggler.pressed = btoggled;

		gui_frame_t::zeichnen(pos, gr);

		unsigned indikatorfarbe = halt->gib_status_farbe();
//		display_ddd_box_clip(pos.x + view.pos.x, pos.y + view.pos.y + 75, 66, 8, MN_GREY0, MN_GREY4);
		display_fillbox_wh_clip(pos.x+11, pos.y + 42, 16, 6, indikatorfarbe, true);

		// now what do we accept here?
		int left = 30;
		if (halt->get_pax_enabled()) {
			display_color_img(skinverwaltung_t::passagiere->gib_bild_nr(0), pos.x+left, pos.y+40, 0, false, false);
			left += 10;
		}
		if (halt->get_post_enabled()) {
			display_color_img(skinverwaltung_t::post->gib_bild_nr(0), pos.x+left, pos.y+40, 0, false, false);
			left += 10;
		}
		if (halt->get_ware_enabled()) {
			display_color_img(skinverwaltung_t::waren->gib_bild_nr(0), pos.x+left, pos.y+40, 0, false, false);
			left += 10;
		}
		// what kind of station?
		left -= 20;
		int halttype = halt->get_station_type();
		if (halttype & haltestelle_t::railstation) {
			display_color_img(skinverwaltung_t::zughaltsymbol->gib_bild_nr(0), pos.x+left, pos.y-4, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::loadingbay) {
			display_color_img(skinverwaltung_t::autohaltsymbol->gib_bild_nr(0), pos.x+left, pos.y-4, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::busstop) {
			display_color_img(skinverwaltung_t::bushaltsymbol->gib_bild_nr(0), pos.x+left, pos.y-4, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::dock) {
			display_color_img(skinverwaltung_t::schiffshaltsymbol->gib_bild_nr(0), pos.x+left, pos.y-4, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::airstop) {
			display_color_img(skinverwaltung_t::airhaltsymbol->gib_bild_nr(0), pos.x+left, pos.y-4, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::monorailstop) {
			display_color_img(skinverwaltung_t::monorailhaltsymbol->gib_bild_nr(0), pos.x+left, pos.y-4, 0, false, false);
			left += 23;
		}

		// capacity
		static cbuffer_t info_buf(256);
		info_buf.clear();
		info_buf.append(translator::translate("Storage capacity"));
		info_buf.append(": ");
		info_buf.append(halt->get_capacity());
		display_proportional(pos.x+viewpos.x-11, pos.y+40, info_buf, ALIGN_RIGHT, SCHWARZ, true);

		// word view box frame
		display_ddd_box(pos.x+viewpos.x, pos.y+viewpos.y+16, 66, 57, MN_GREY0, MN_GREY4);

		// Hajo: Reuse of freight_info buffer to get and display
		// information about the convoi itself
		info_buf.clear();
		halt->info(info_buf);
		display_multiline_text(pos.x+11, pos.y+56, info_buf, SCHWARZ);
	}
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool halt_info_t::action_triggered(gui_komponente_t *comp)
{
	if (comp == &button) { 			// details button pressed
		halt->open_detail_window();
	} else if (comp == &sort_button) { 	// @author hsiegeln sort button pressed
		int sortby = ((int)(halt->get_sortby())+1)%4;
		halt->set_sortby((freight_list_sorter_t::sort_mode_t) sortby);
		sort_button.text = translator::translate(sort_text[sortby]);
	} else  if (comp == &toggler) {
		btoggled = !btoggled;
		const koord offset = btoggled ? koord(0, 165) : koord(0, -165);
   		set_min_windowsize(koord(BUTTON4_X+BUTTON_WIDTH+2, btoggled ?372:194));
		scrolly.pos.y += offset.y;
		// toggle visibility of components
		chart->set_visible(btoggled);
		setze_fenstergroesse(gib_fenstergroesse() + offset);
		resize(koord(0,0));
		for (int i=0;i<MAX_HALT_COST;i++) {
			filterButtons[i].set_visible(btoggled);
		}
	} else {
		for ( int i = 0; i<MAX_HALT_COST; i++) {

			if (comp == &filterButtons[i]) {

				bFilterIsActive[i] = !bFilterIsActive[i];
				if(bFilterIsActive[i]) {
					chart->show_curve(i);
				} else {
					chart->hide_curve(i);
				}
				break;
			}
		}
	}

	return true;
}


/**
 * Resize the contents of the window
 * @author Markus Weber
 */
void halt_info_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	const sint16 button_offset_y = btoggled?245:80;

	input.setze_groesse(koord(get_client_windowsize().x-23, 13));
	toggler.setze_pos(koord(BUTTON3_X,button_offset_y));
	button.setze_pos(koord(BUTTON4_X,button_offset_y));
	sort_button.setze_pos(koord(BUTTON1_X,button_offset_y));
	sort_label.setze_pos(koord(BUTTON1_X,button_offset_y-LINESPACE));
	view.setze_pos(koord(get_client_windowsize().x - 64 - 16 , 21));
	scrolly.setze_groesse(get_client_windowsize()-scrolly.gib_pos());
}
