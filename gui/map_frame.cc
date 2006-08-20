/*
 * map_frame.cc
 *
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>
#include <cmath>

#include "map_frame.h"
#include "../ifc/karte_modell.h"
#include "components/list_button.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cstring_t.h"
#include "../dataobj/translator.h"
#include "../besch/fabrik_besch.h"


koord map_frame_t::size;
uint8 map_frame_t::legend_visible=false;

// Hajo: we track our position onscreen
koord map_frame_t::screenpos;

// hopefully these are enough ...
static minivec_tpl <cstring_t> legend_names (128);
static minivec_tpl <int> legend_colors (128);

// @author hsiegeln
const char map_frame_t::map_type[MAX_MAP_TYPE][64] =
{
    "Towns",
    "Passagiere",
    "Post",
    "Fracht",
    "Status",
    "Service",
    "Traffic",
    "Origin",
    "Destination",
    "Waiting",
    "Tracks",
    "Speedlimit",
    "Powerlines",
    "Tourists",
    "Factories",
    "Depots"
};

const int map_frame_t::map_type_color[MAX_MAP_TYPE] =
{
  7, 11, 15, 132, 23, 27, 31, 35, 241, 7, 11, 71, 57, 81
};

/**
 * Konstruktor. Erzeugt alle notwendigen Subkomponenten.
 * @author Hj. Malthaner
 */
map_frame_t::map_frame_t(const karte_modell_t *welt) :
	gui_frame_t("Reliefkarte"),
	scrolly(reliefkarte_t::gib_karte())
{
	// init factories names
	legend_names.clear();
	legend_colors.clear();
	const stringhashtable_tpl<const fabrik_besch_t *> & fabesch = fabrikbauer_t::gib_fabesch();
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

	// add factory names; shorten too long names
	while( iter.next() &&  iter.get_current_value()->gib_gewichtung()>0) {
		int i;

		cstring_t label (translator::translate(iter.get_current_value()->gib_name()));
		for(  i=12;  i<label.len()  &&  display_calc_proportional_string_len_width(label,i,true)<100;  i++  )
			;
		if(  i<label.len()  ) {
			label.set_at(i++, '.');
			label.set_at(i++, '.');
			label.set_at(i++, '\0');
		}

		legend_names.append(label);
		legend_colors.append(iter.get_current_value()->gib_kennfarbe());
	}

	// init the rest
	// size will be set during resize ...
	add_komponente(&scrolly);

	// and now the buttons
	// these will be only added to the window, when they are visible
	// this is the only legal way to hide them
	for (int type=0; type<MAX_MAP_TYPE; type++) {
		filter_buttons[type].init(button_t::box,translator::translate(map_type[type]), koord(0,0), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filter_buttons[type].add_listener(this);
		filter_buttons[type].background = map_type_color[type];
		is_filter_active[type] = reliefkarte_t::gib_karte()->get_mode() == type;
		if(legend_visible) {
			add_komponente(filter_buttons + type);
		}
	}

	// Hajo: Hack: use static size if set by a former object
	if(size != koord(0,0)) {
		setze_fenstergroesse(size);
	}
	resize( koord(0,0) );

	// Clipping geändert - max. 250 war zu knapp für grosse Karten - V.Meyer
	reliefkarte_t *karte = reliefkarte_t::gib_karte();
	const koord gr = karte->gib_groesse();
	const koord s_gr=scrolly.gib_groesse();
	const koord ij = welt->gib_ij_off();
	const koord win_size = gr-s_gr;	// this is the visible area
	scrolly.setze_scroll_position(  max(0,min(ij.x-win_size.x/2,gr.x)), max(0, min(ij.y-win_size.y/2,gr.y)) );

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);

	is_dragging = false;
}



// switches updates off
map_frame_t::~map_frame_t()
{
}



// button pressed
bool
map_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	bool all_inactive=true;
	reliefkarte_t::gib_karte()->calc_map();
	for (int i=0;i<MAX_MAP_TYPE;i++) {
		if (komp == &filter_buttons[i]) {
			if (is_filter_active[i] == true) {
				is_filter_active[i] = false;
			} else {
				all_inactive = false;
				reliefkarte_t::gib_karte()->set_mode((reliefkarte_t::MAP_MODES)i);
				is_filter_active[i] = true;
			}
		} else {
			is_filter_active[i] = false;
		}
	}
	if(all_inactive) {
		reliefkarte_t::gib_karte()->set_mode(reliefkarte_t::PLAIN);
	}
	return true;
}



/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
void map_frame_t::infowin_event(const event_t *ev)
{
	if(ev->ev_class == INFOWIN) {
		if(ev->ev_code == WIN_OPEN) {
			reliefkarte_t::gib_karte()->is_visible = true;
			reliefkarte_t::gib_karte()->calc_map();
		}
		else if(ev->ev_code == WIN_CLOSE) {
			reliefkarte_t::gib_karte()->is_visible = false;
		}
	}

	if(IS_WHEELUP(ev) || IS_WHEELDOWN(ev)  &&  reliefkarte_t::gib_karte()->getroffen(ev->mx,ev->my)) {
		// otherwise these would go to the vertical scroll bar
		reliefkarte_t::gib_karte()->infowin_event(ev);
		return;
	}

	if(!is_dragging) {
		gui_frame_t::infowin_event(ev);
	}

	if(IS_WINDOW_CHOOSE_NEXT(ev)) {
		// open close legend ...
		if(ev->ev_code==NEXT_WINDOW) {
			if(legend_visible==0) {
				for (int type=0; type<MAX_MAP_TYPE; type++) {
					add_komponente(filter_buttons + type);
				}
			}
			legend_visible = min(3,legend_visible+1);
			resize( koord(0,0) );
		}
		else {
			// do not draw legend anymore
			if(legend_visible==1) {
				for (int type=0; type<MAX_MAP_TYPE; type++) {
					remove_komponente(filter_buttons + type);
				}
			}
			legend_visible = max(0,legend_visible-1);
			resize( koord(0,0) );
		}
	}

	// Hajo: hack: relief map can resize upon right click
	// we track this here, and adjust size.
	if(IS_RIGHTCLICK(ev)) {
		is_dragging = false;
		reliefkarte_t::gib_karte()->gib_welt()->set_scroll_lock(false);
	}

	if(IS_RIGHTRELEASE(ev)) {
		if(!is_dragging) {
			resize( koord(0,0) );
		}
		is_dragging = false;
		reliefkarte_t::gib_karte()->gib_welt()->set_scroll_lock(false);
	}

	if(reliefkarte_t::gib_karte()->getroffen(ev->mx,ev->my)  &&  IS_RIGHTDRAG(ev)) {
		int x = scrolly.get_scroll_x();
		int y = scrolly.get_scroll_y();

		x += (ev->mx - ev->cx)*2;
		y += (ev->my - ev->cy)*2;

		is_dragging = true;
		reliefkarte_t::gib_karte()->gib_welt()->set_scroll_lock(true);

		scrolly.setze_scroll_position(  max(0, x),  max(0, y) );
//		scrolly.setze_scroll_position(  max(0, min(size.x, x)),  max(0, min(size.y, y)) );

		// Hajo: re-center mouse pointer
		display_move_pointer(screenpos.x+ev->cx, screenpos.y+ev->cy);
	}
}



/**
 * size window in response and save it in static size
 * @author (Mathew Hounsell)
 * @date   11-Mar-2003
 */
void map_frame_t::setze_fenstergroesse(koord groesse)
{
	gui_frame_t::setze_fenstergroesse( groesse );
	groesse = gib_fenstergroesse();
	// set scroll area size
	scrolly.setze_groesse( groesse-scrolly.gib_pos()-koord(0,16) );
	map_frame_t::size = gib_fenstergroesse();
//DBG_MESSAGE("map_frame_t::setze_fenstergroesse()","gr.x=%i, gr.y=%i",size.x,size.y );
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_frame_t::resize(const koord delta)
{
	karte_t *welt=reliefkarte_t::gib_karte()->gib_welt();

	koord groesse = gib_fenstergroesse()+delta;
	gui_frame_t::resize(delta);

	if(legend_visible) {
		// calculate space with legend
		col = max( 1, min( (groesse.x-2)/BUTTON_WIDTH, MAX_MAP_TYPE ) );
		row = ((MAX_MAP_TYPE-1)/col)+1;
		int offset_y=row*14+8;

		switch(legend_visible) {
			case 1:
				// new minimum size/scroll offset only buttons
				break;
			case 2:
				// plus scale bar
				offset_y += LINESPACE+4;
				break;
			case 3:
				{
					// full program including factory texts
					const int fac_cols = max( 1, min( (groesse.x-2)/110, legend_names.get_count() ) );
					const int fac_rows = ((legend_names.get_count()-1)/fac_cols)+1;
					offset_y += (LINESPACE+4) + (fac_rows*14+4);
				}
				break;
		}

		// offset of map
		scrolly.setze_pos( koord(0,offset_y) );
		// min size
		offset_y += min(welt->gib_groesse_y(),256)+16+12;
		if(offset_y>display_get_height()-64) {
			// avoid negative values for min size
			offset_y = max(10,display_get_height()-64);
		}
		set_min_windowsize(  koord(min(256,max(BUTTON_WIDTH+4,welt->gib_groesse_x()))+12, offset_y) );
		setze_opaque(true);

		// set button pos
		for (int type=0; type<MAX_MAP_TYPE; type++) {
			koord pos = koord( 2+BUTTON_WIDTH*(type%col), 16+4+BUTTON_HEIGHT*((int)type/col-1) );
			filter_buttons[type].setze_pos( pos );
		}

	}
	else {
		scrolly.setze_pos( koord(0,0) );
		setze_opaque(false);
		set_min_windowsize( koord(min(256,welt->gib_groesse_x())+11, min(welt->gib_groesse_y(),256)+16+12) );
	}
	setze_fenstergroesse( groesse );
}



/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void map_frame_t::zeichnen(koord pos, koord gr)
{
	if(legend_visible==0) {
		// scrollbar "opaqueness" mechanism has changed. So we must draw grey background here
		// if not handled automatically
		screenpos = pos;

		display_fillbox_wh(pos.x+gr.x-14,pos.y+16,14, gr.y-16, MN_GREY2, true);
		display_fillbox_wh(pos.x,pos.y+gr.y-12,gr.x,12, MN_GREY2, true);
		display_fillbox_wh(pos.x+gr.x, pos.y+16, 1, gr.y-16, MN_GREY0, true);
		display_fillbox_wh(pos.x, pos.y+gr.y, gr.x, 1, MN_GREY0, true);
	}
	else {
		// button state
		for (int i = 0;i<MAX_MAP_TYPE;i++) {
			filter_buttons[i].pressed = is_filter_active[i];
		}
	}

	gui_frame_t::zeichnen(pos, gr);

	// draw scale
	if(legend_visible>1) {
		koord bar_pos = pos+scrolly.gib_pos()-koord(0,LINESPACE+4-16);
		// color bar
		for( int i=0;  i<MAX_SEVERITY_COLORS;  i++) {
			display_fillbox_wh(bar_pos.x + 30 + i*(gr.x-60)/MAX_SEVERITY_COLORS, bar_pos.y+2,  (gr.x-60)/(MAX_SEVERITY_COLORS-1), 7, reliefkarte_t::calc_severity_color(i,MAX_SEVERITY_COLORS), false);
		}
		display_proportional(bar_pos.x + 26, bar_pos.y, translator::translate("min"), ALIGN_RIGHT, COL_BLACK, false);
		display_proportional(bar_pos.x + size.x - 26, bar_pos.y, translator::translate("max"), ALIGN_LEFT, COL_BLACK, false);
	}

	// draw factory descriptions
	if(legend_visible==3) {

		const int offset_y=row*14+12+16;
		const int fac_cols = max( 1, min( (gr.x-2)/110, legend_names.get_count() ) );
//		const int fac_rows = ((legend_names.get_count()-1)/fac_cols)+1;
		const int width = (gr.x-10)/fac_cols;
		for(unsigned u=0; u<legend_names.get_count(); u++) {

			const int xpos = pos.x + (u%fac_cols)*width + 8;
			const int ypos = pos.y+offset_y+(u/fac_cols)*14;
			const int color = legend_colors.at(u);

			if(ypos+14>pos.y+gr.y) {
				break;
			}
			display_fillbox_wh(xpos, ypos+1, 7, 7, color, false);
			display_proportional( xpos+8, ypos, legend_names.get(u), ALIGN_LEFT, COL_BLACK, false);
		}
	}
}
