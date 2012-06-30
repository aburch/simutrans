/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "halt_detail.h"
#include "halt_info.h"
#include "../simworld.h"
#include "../simware.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simmenu.h"
#include "../simskin.h"

#include "../freight_list_sorter.h"

#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../utils/simstring.h"

#include "../besch/skin_besch.h"



karte_t *halt_info_t::welt = NULL;


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
	"hl_btn_sort_waiting",
	"Arrived",
	"Departed",
	"Convoys",
	"Walked"
};

const uint8 index_of_haltinfo[MAX_HALT_COST] = {
	HALT_HAPPY,
	HALT_UNHAPPY,
	HALT_NOROUTE,
	HALT_WAITING,
	HALT_ARRIVED,
	HALT_DEPARTED,
	HALT_CONVOIS_ARRIVED,
	HALT_WALKED
};

#define COL_HAPPY COL_WHITE
#define COL_UNHAPPY COL_RED
#define COL_NO_ROUTE COL_BLUE
#define COL_WAITING COL_YELLOW
#define COL_ARRIVED COL_DARK_ORANGE
#define COL_DEPARTED COL_DARK_YELLOW

const int cost_type_color[MAX_HALT_COST] =
{
	COL_HAPPY,
	COL_UNHAPPY,
	COL_NO_ROUTE,
	COL_WAITING,
	COL_ARRIVED,
	COL_DEPARTED,
	COL_COUNVOI_COUNT,
	COL_LILAC
};

halt_info_t::halt_info_t(karte_t *welt, halthandle_t halt) :
		gui_frame_t( halt->get_name(), halt->get_besitzer() ),
		scrolly(&text),
		text(&freight_info),
		sort_label(translator::translate("Hier warten/lagern:")),
		view(welt, halt->get_basis_pos3d(), koord(max(64, get_base_tile_raster_width()), max(56, get_base_tile_raster_width() * 7 / 8)))
{
	this->halt = halt;
	this->welt = welt;
	halt->set_sortby( umgebung_t::default_sortmode );

	const sint16 offset_below_viewport = 21 + view.get_groesse().y;
	const sint16 total_width = D_MARGIN_LEFT + 3*(D_BUTTON_WIDTH + D_H_SPACE) + max( D_BUTTON_WIDTH, view.get_groesse().x ) + D_MARGIN_RIGHT;

	input.set_pos(koord(10,4));
	tstrncpy(edit_name, halt->get_name(), lengthof(edit_name));
	input.set_text(edit_name, lengthof(edit_name));
	input.add_listener(this);
	add_komponente(&input);

	add_komponente(&view);

	// chart
	chart.set_pos(koord(66,offset_below_viewport+2));
	chart.set_groesse(koord(total_width-66-10, 100));
	chart.set_dimension(12, 10000);
	chart.set_visible(false);
	chart.set_background(MN_GREY1);
	for (int cost = 0; cost<MAX_HALT_COST; cost++) {
		chart.add_curve(cost_type_color[cost], halt->get_finance_history(), MAX_HALT_COST, index_of_haltinfo[cost], MAX_MONTHS, 0, false, true, 0);
		filterButtons[cost].init(button_t::box_state, cost_type[cost],
			koord(BUTTON1_X+(D_BUTTON_WIDTH+D_H_SPACE)*(cost%4), view.get_groesse().y+141+(D_BUTTON_HEIGHT+2)*(cost/4) ),
			koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
		filterButtons[cost].add_listener(this);
		filterButtons[cost].background = cost_type_color[cost];
		filterButtons[cost].set_visible(false);
		filterButtons[cost].pressed = false;
		add_komponente(filterButtons + cost);
	}
	add_komponente(&chart);

	add_komponente(&sort_label);

	const sint16 yoff = offset_below_viewport+D_BUTTON_HEIGHT+1-D_BUTTON_HEIGHT-2;

	// hsiegeln: added sort_button
	sort_button.init(button_t::roundbox, sort_text[umgebung_t::default_sortmode],koord(BUTTON1_X, yoff), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	sort_button.set_tooltip("Sort waiting list by");
	sort_button.add_listener(this);
	add_komponente(&sort_button);

	toggler.init(button_t::roundbox_state, "Chart", koord(BUTTON3_X, yoff), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	toggler.set_tooltip("Show/hide statistics");
	toggler.add_listener(this);
	add_komponente(&toggler);

	button.init(button_t::roundbox, "Details", koord(BUTTON4_X, yoff), koord(D_BUTTON_WIDTH, D_BUTTON_HEIGHT));
	button.add_listener(this);
	add_komponente(&button);

	scrolly.set_pos(koord(0, offset_below_viewport+D_BUTTON_HEIGHT+3));
	scrolly.set_show_scroll_x(true);
	add_komponente(&scrolly);

	set_fenstergroesse(koord(total_width, view.get_groesse().y+208+scrollbar_t::BAR_SIZE));
	set_min_windowsize(koord(total_width, view.get_groesse().y+131+scrollbar_t::BAR_SIZE));

	set_resizemode(diagonal_resize);     // 31-May-02	markus weber	added
	resize(koord(0,0));
}


halt_info_t::~halt_info_t()
{
	if(  halt.is_bound()  &&  strcmp(halt->get_name(),edit_name)  &&  edit_name[0]  ) {
		// text changed => call tool
		cbuffer_t buf;
		buf.printf( "h%u,%s", halt.get_id(), edit_name );
		werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
		w->set_default_param( buf );
		halt->get_welt()->set_werkzeug( w, halt->get_besitzer() );
		// since init always returns false, it is save to delete immediately
		delete w;
	}
}


koord3d halt_info_t::get_weltpos(bool)
{
	return halt->get_basis_pos3d();
}


bool halt_info_t::is_weltpos()
{
	return ( welt->get_x_off() | welt->get_y_off()) == 0  &&
		welt->get_world_position() == welt->calculate_world_position( get_weltpos(false) );
}


/**
 * Komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void halt_info_t::zeichnen(koord pos, koord gr)
{
	if(halt.is_bound()) {

		// buffer update now only when needed by halt itself => dedicated buffer for this
		int old_len=freight_info.len();
		halt->get_freight_info(freight_info);
		if(old_len!=freight_info.len()) {
			text.recalc_size();
		}

		gui_frame_t::zeichnen(pos, gr);

		sint16 top = pos.y+36;
		COLOR_VAL indikatorfarbe = halt->get_status_farbe();
		display_fillbox_wh_clip(pos.x+10, top+2, D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT, indikatorfarbe, true);

		// now what do we accept here?
		int left = 10+D_INDICATOR_WIDTH+2;
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
		haltestelle_t::stationtyp const halttype = halt->get_station_type();
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
		info_buf.printf("%s: %u", translator::translate("Storage capacity"), halt->get_capacity(0));
		left = pos.x+10;
		// passagiere
		left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
		if (welt->get_settings().is_seperate_halt_capacities()) {
			// here only for seperate capacities
			display_color_img(skinverwaltung_t::passagiere->get_bild_nr(0), left, top, 0, false, false);
			left += 10;
			// post
			info_buf.clear();
			info_buf.printf(",  %u", halt->get_capacity(1));
			left += display_proportional(left, top, info_buf, ALIGN_LEFT, COL_BLACK, true);
			display_color_img(skinverwaltung_t::post->get_bild_nr(0), left, top, 0, false, false);
			left += 10;
			// goods
			info_buf.clear();
			info_buf.printf(",  %u", halt->get_capacity(2));
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


// activate the statistic
void halt_info_t::show_hide_statistics( bool show )
{
	toggler.pressed = show;
	const koord offset = show ? koord(0, 165) : koord(0, -165);
	set_min_windowsize(get_min_windowsize() + offset);
	scrolly.set_pos(scrolly.get_pos() + offset);
	chart.set_visible(show);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
	for (int i=0;i<MAX_HALT_COST;i++) {
		filterButtons[i].set_visible(toggler.pressed);
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
		umgebung_t::default_sortmode = ((int)(halt->get_sortby())+1)%4;
		halt->set_sortby((freight_list_sorter_t::sort_mode_t) umgebung_t::default_sortmode);
		sort_button.set_text(sort_text[umgebung_t::default_sortmode]);
	} else  if (comp == &toggler) {
		show_hide_statistics( toggler.pressed^1 );
	}
	else if(  comp == &input  ) {
		if(  strcmp(halt->get_name(),edit_name)  ) {
			// text changed => call tool
			cbuffer_t buf;
			buf.printf( "h%u,%s", halt.get_id(), edit_name );
			werkzeug_t *w = create_tool( WKZ_RENAME_TOOL | SIMPLE_TOOL );
			w->set_default_param( buf );
			halt->get_welt()->set_werkzeug( w, halt->get_besitzer() );
			// since init always returns false, it is save to delete immediately
			delete w;
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
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void halt_info_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse(groesse);

	input.set_groesse(koord(get_fenstergroesse().x-20, 13));

	view.set_pos(koord(get_fenstergroesse().x - view.get_groesse().x - 10 , 21));

	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());

	const sint16 yoff = scrolly.get_pos().y-D_BUTTON_HEIGHT-3;
	sort_button.set_pos(koord(BUTTON1_X,yoff));
	toggler.set_pos(koord(BUTTON3_X,yoff));
	button.set_pos(koord(BUTTON4_X,yoff));
	sort_label.set_pos(koord(2,yoff-LINESPACE-1));
}



void halt_info_t::map_rotate90( sint16 new_ysize )
{
	view.map_rotate90(new_ysize);
}


halt_info_t::halt_info_t(karte_t *welt):
	gui_frame_t("", NULL),
	scrolly(&text),
	text(&freight_info),
	sort_label(NULL),
	view(welt, koord3d::invalid, koord(64, 64))
{
	// just a dummy
	this->welt = welt;
}


void halt_info_t::rdwr(loadsave_t *file)
{
	koord3d halt_pos;
	koord gr = get_fenstergroesse();
	uint32 flags = 0;
	bool stats = toggler.pressed;
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	if(  file->is_saving()  ) {
		halt_pos = halt->get_basis_pos3d();
		for( int i = 0; i<MAX_HALT_COST; i++) {
			if(  filterButtons[i].pressed  ) {
				flags |= (1<<i);
			}
		}
	}
	halt_pos.rdwr( file );
	gr.rdwr( file );
	file->rdwr_long( flags );
	file->rdwr_byte( umgebung_t::default_sortmode );
	file->rdwr_bool( stats );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );
	if(  file->is_loading()  ) {
		halt = welt->lookup( halt_pos )->get_halt();
		// now we can open the window ...
		koord const& pos = win_get_pos(this);
		halt_info_t *w = new halt_info_t(welt,halt);
		create_win(pos.x, pos.y, w, w_info, magic_halt_info + halt.get_id());
		if(  stats  ) {
			gr.y -= 170;
		}
		w->set_fenstergroesse( gr );
		for( int i = 0; i<MAX_HALT_COST; i++) {
			w->filterButtons[i].pressed = (flags>>i)&1;
			if(w->filterButtons[i].pressed) {
				w->chart.show_curve(i);
			}
		}
		if(  stats  ) {
			w->show_hide_statistics( true );
		}
		halt->get_freight_info(w->freight_info);
		w->text.recalc_size();
		w->scrolly.set_scroll_position( xoff, yoff );
		// we must invalidate halthandle
		halt = halthandle_t();
		destroy_win( this );
	}
}
