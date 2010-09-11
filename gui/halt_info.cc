/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "halt_detail.h"
#include "halt_info.h"
#include "../simworld.h"
#include "../simhalt.h"
#include "../simware.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../simskin.h"
#include "../utils/simstring.h"
#include "../freight_list_sorter.h"
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "components/list_button.h"
#include "../besch/skin_besch.h"



karte_t *halt_info_t::welt = NULL;


static const char *sort_text[halt_info_t::SORT_MODES] = {
	"Zielort",
	"via",
	"via Menge",
	"Menge",
	"origin (detail)",
	"origin (amount)"
};

static const char cost_type[MAX_HALT_COST][64] =
{
	"Happy",
	"Unhappy",
	"No Route",
	"Too slow",
	"hl_btn_sort_waiting",
	"Arrived",
	"Departed",
	"Convoys"
};

static const char cost_tooltip[MAX_HALT_COST][128] =
{
	"The number of passengers who have travelled successfully from this stop",
	"The number of passengers who have left because of overcrowding or excess waiting",
	"The number of passengers who could not find a route to their destination",
	"The number of passengers who decline to travel because the journey would take too long",
	"The number of passengers/units of mail/goods waiting at this stop",
	"The number of passengers/units of mail/goods that have arrived at this stop",
	"The number of passengers/units of mail/goods that have departed from this stop",
	"The number of convoys that have serviced this stop"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_NOROUTE,
	HALT_TOO_SLOW,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED
};

const int cost_type_color[MAX_HALT_COST] =
{
	COL_HAPPY,
	COL_UNHAPPY,
	COL_NO_ROUTE,
	COL_PURPLE,
	COL_WAITING,
	COL_ARRIVED,
	COL_DEPARTED,
	COL_VEHICLE_ASSETS
};

halt_info_t::halt_info_t(karte_t *welt, halthandle_t halt)
	: gui_frame_t(edit_name, halt->get_besitzer()),
		scrolly(&text),
		text("                                                                                     "
			" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
			" \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n \n"
		),
		sort_label(translator::translate("Hier warten/lagern:")),
		view(welt, halt->get_basis_pos3d(), koord( max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width()*7)/8) )),
		freight_info(4096),
		info_buf(256)
{
	this->halt = halt;
	this->welt = welt;
	halt->set_sortby( umgebung_t::default_sortmode );

	const sint16 offset_below_viewport = 21 + view.get_groesse().y;
	const sint16 total_width = 3*(BUTTON_WIDTH + BUTTON_SPACER) + max(BUTTON_WIDTH + 2*BUTTON_SPACER, view.get_groesse().x + 32);

	input.set_pos(koord(10,4));
	tstrncpy(edit_name, halt->get_name(), lengthof(edit_name));
	input.set_text(edit_name, lengthof(edit_name));
	add_komponente(&input);

	add_komponente(&view);

	// chart
	chart.set_pos(koord(66,view.get_groesse().y+24));
	chart.set_groesse(koord(BUTTON4_X+BUTTON_WIDTH-70, 100));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(MN_GREY1);
	chart.set_ltr(umgebung_t::left_to_right_graphs);

	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		chart.add_curve(cost_type_color[cost], halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true, 0);
		filterButtons[cost].init(button_t::box_state, cost_type[cost],
			koord(BUTTON1_X+(BUTTON_WIDTH+BUTTON_SPACER)*(cost%4), view.get_groesse().y+142+(BUTTON_HEIGHT+2)*(cost/4) ),
			koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background = cost_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		filterButtons[cost].set_tooltip(cost_tooltip[cost]);
		add_komponente(filterButtons + cost);
	}
	add_komponente(&chart);

	add_komponente(&sort_label);

	// hsiegeln: added sort_button
	sort_button.set_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	sort_button.set_pos(koord(BUTTON1_X, offset_below_viewport));
	sort_button.set_text(sort_text[umgebung_t::default_sortmode]);
	sort_button.set_typ(button_t::roundbox);
	sort_button.set_tooltip("Sort waiting list by");
	add_komponente(&sort_button);

	toggler.set_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	toggler.set_pos(koord(BUTTON3_X, offset_below_viewport));
	toggler.set_text("Chart");
	toggler.set_typ(button_t::roundbox_state);
	toggler.set_tooltip("Show/hide statistics");
	toggler.add_listener(this);
	toggler.pressed = false;
	add_komponente(&toggler);

	button.set_groesse(koord(BUTTON_WIDTH, BUTTON_HEIGHT));
	button.set_pos(koord(BUTTON4_X, offset_below_viewport));
	button.set_text("Details");
	button.set_typ(button_t::roundbox);

	scrolly.set_pos(koord(0, offset_below_viewport+BUTTON_HEIGHT+4));

	add_komponente(&button);

	set_fenstergroesse(koord(total_width, view.get_groesse().y+208));
	set_min_windowsize(koord(total_width, view.get_groesse().y+138));
	set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
	resize(koord(0,0));

	button.add_listener(this);
//	input.add_listener(this);
	sort_button.add_listener(this);

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
	if(halt.is_bound()) {
		if(strcmp(edit_name,halt->get_name())) {
			halt->set_name( edit_name );
			// Knightly : need to update the title text of the associated halt detail dialog, if present
			halt_detail_t *const details_frame = dynamic_cast<halt_detail_t *>( win_get_magic( magic_halt_detail + halt.get_id() ) );
			if(  details_frame  ) {
				details_frame->set_name( halt->get_name() );
			}
		}

		// buffer update now only when needed by halt itself => dedicated buffer for this
		int old_len=freight_info.len();
		halt->get_freight_info(freight_info);
		// will grow as needed
		if (freight_info.is_full()) {
			freight_info.extent( 1024 );
			// tell the halt to give the info in the now larger buffer
			halt->set_sortby((freight_list_sorter_t::sort_mode_t) umgebung_t::default_sortmode);
		}
		if(old_len!=freight_info.len()) {
			text.set_text(freight_info);
			text.recalc_size();
		}

		gui_frame_t::zeichnen(pos, gr);

		sint16 top = pos.y+36;
		COLOR_VAL indikatorfarbe = halt->get_status_farbe();
		display_fillbox_wh_clip(pos.x+10, top+2, INDICATOR_WIDTH, INDICATOR_HEIGHT, indikatorfarbe, true);

		// now what do we accept here?
		int left = 10+INDICATOR_WIDTH+2;
		if (halt->get_pax_enabled()) {
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 10;
		}
		if (halt->get_post_enabled()) {
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 10;
		}
		if (halt->get_ware_enabled()) {
			display_color_img(skinverwaltung_t::waren->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 10;
		}

		// what kind of station?
		left -= 20;
		top -= 44;
		int halttype = halt->get_station_type();
		if (halttype & haltestelle_t::railstation) {
			display_color_img(skinverwaltung_t::zughaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::loadingbay) {
			display_color_img(skinverwaltung_t::autohaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::busstop) {
			display_color_img(skinverwaltung_t::bushaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::dock) {
			display_color_img(skinverwaltung_t::schiffshaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::airstop) {
			display_color_img(skinverwaltung_t::airhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::monorailstop) {
			display_color_img(skinverwaltung_t::monorailhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::tramstop) {
			display_color_img(skinverwaltung_t::tramhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::maglevstop) {
			display_color_img(skinverwaltung_t::maglevhaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}
		if (halttype & haltestelle_t::narrowgaugestop) {
			display_color_img(skinverwaltung_t::narrowgaugehaltsymbol->get_bild_nr(0), pos.x+left, top, 0, false, false);
			left += 23;
		}

		top = pos.y+50;
		info_buf.clear();
		info_buf.append(translator::translate("Storage capacity"));
		info_buf.append(": ");
		left = pos.x+10;
		// passagiere
		info_buf.append(halt->get_capacity(0));
		left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
		if(  welt->get_einstellungen()->is_seperate_halt_capacities()  ) {
			// here only for seperate capacities
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), left, top, 0, false, false);
			left += 10;
			// post
			info_buf.clear();
			info_buf.append(",  ");
			info_buf.append(halt->get_capacity(1));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), left, top, 0, false, false);
			left += 10;
			// goods
			info_buf.clear();
			info_buf.append(",  ");
			info_buf.append(halt->get_capacity(2));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
			display_color_img(skinverwaltung_t::waren->get_bild_nr(0), left, top, 0, false, false);
			left = 53+LINESPACE;
		}

		// Hajo: Reuse of freight_info buffer to get and display
		// information about the convoi itself
		info_buf.clear();
		halt->info(info_buf);
		display_multiline_text(pos.x+10, pos.y+53+LINESPACE, info_buf, COL_BLACK);
	}
}


/**
 * This method is called if an action is triggered
 * @author Hj. Malthaner
 */
bool halt_info_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if (comp == &button) { 			// details button pressed
		create_win( new halt_detail_t(halt), w_info, magic_halt_detail + halt.get_id() );
	} else if (comp == &sort_button) { 	// @author hsiegeln sort button pressed
		umgebung_t::default_sortmode = ((int)(halt->get_sortby())+1)%SORT_MODES;
		halt->set_sortby((freight_list_sorter_t::sort_mode_t) umgebung_t::default_sortmode);
		sort_button.set_text(sort_text[umgebung_t::default_sortmode]);
	} else  if (comp == &toggler) {
		toggler.pressed ^= 1;
		const koord offset = toggler.pressed ? koord(0, 170) : koord(0, -170);
		set_min_windowsize(get_min_windowsize() + offset);
		scrolly.set_pos(scrolly.get_pos() + offset);
		// toggle visibility of components
		chart.set_visible(toggler.pressed);
		set_fenstergroesse(get_fenstergroesse() + offset);
		resize(koord(0,0));
		for (int i=0;i<MAX_HALT_COST;i++) {
			filterButtons[i].set_visible(toggler.pressed);
		}
	}
	else {
		for( int i = 0; i<MAX_HALT_COST; i++) {
			if (comp == &filterButtons[i]) {
				filterButtons[i].pressed = !filterButtons[i].pressed;
				if(filterButtons[i].pressed) {
					chart.show_curve(i);
				}
				else {
					chart.hide_curve(i);
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

	const sint16 yoff = scrolly.get_pos().y-BUTTON_HEIGHT-2;
	input.set_groesse(koord(get_fenstergroesse().x-20, 13));
	toggler.set_pos(koord(BUTTON3_X,yoff));
	button.set_pos(koord(BUTTON4_X,yoff));
	sort_button.set_pos(koord(BUTTON1_X,yoff));
	sort_label.set_pos(koord(BUTTON1_X,yoff-LINESPACE));
	view.set_pos(koord(get_fenstergroesse().x - view.get_groesse().x - 10 , 21));
	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());
}



void halt_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}
