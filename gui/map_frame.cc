/*
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
#include "components/list_button.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cstring_t.h"
#include "../dataobj/translator.h"
#include "../dataobj/koord.h"
#include "../besch/fabrik_besch.h"


static koord old_ij=koord::invalid;

koord map_frame_t::size=koord(0,0);
uint8 map_frame_t::legend_visible=false;
uint8 map_frame_t::scale_visible=false;
uint8 map_frame_t::directory_visible=false;

// Hajo: we track our position onscreen
koord map_frame_t::screenpos;


struct legend_entry
{
	legend_entry() {}
	legend_entry(const cstring_t& text_, int colour_) : text(text_), colour(colour_) {}

	cstring_t text;
	int       colour;
};

static vector_tpl<legend_entry> legend(16);


// @author hsiegeln
const char map_frame_t::map_type[MAX_BUTTON_TYPE][64] =
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
    "hl_btn_sort_waiting",
    "Tracks",
    "Speedlimit",
    "Powerlines",
    "Tourists",
    "Factories",
    "Depots",
		"Forest"
};

const uint8 map_frame_t::map_type_color[MAX_BUTTON_TYPE] =
{
	15, 23, 31, 157, 46, 55, 63, 133, 79, 191, 207, 11, 123, 221, 71, 135, 127
};


map_frame_t::map_frame_t(const karte_t *welt) :
	gui_frame_t("Reliefkarte"),
	scrolly(reliefkarte_t::gib_karte()),
	zoom_label("map zoom")
{
	// zoom levels
	zoom_buttons[0].setze_pos( koord(2,BUTTON_HEIGHT+4) );
	zoom_buttons[0].setze_typ( button_t::repeatarrowleft );
	zoom_buttons[0].add_listener( this );
	add_komponente( zoom_buttons+0 );
	zoom_buttons[1].setze_pos( koord(40,BUTTON_HEIGHT+4) );
	zoom_buttons[1].setze_typ( button_t::repeatarrowright );
	zoom_buttons[1].add_listener( this );
	add_komponente( zoom_buttons+1 );
	zoom_label.setze_pos( koord(54,BUTTON_HEIGHT+4) );
	add_komponente( &zoom_label );

	// rotate map 45°
	b_rotate45.init(button_t::square, "isometric map", koord(BUTTON_WIDTH+40,BUTTON_HEIGHT+4), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	b_rotate45.add_listener(this);
	add_komponente(&b_rotate45);

	// show the various objects
	b_show_legend.init(button_t::roundbox, "Show legend", koord(0,0), koord(BUTTON_WIDTH+1,BUTTON_HEIGHT));
	b_show_legend.add_listener(this);
	add_komponente(&b_show_legend);
	b_show_scale.init(button_t::roundbox, "Show map scale", koord(BUTTON_WIDTH+1,0), koord(BUTTON_WIDTH+2,BUTTON_HEIGHT));
	b_show_scale.add_listener(this);
	add_komponente(&b_show_scale);
	b_show_directory.init(button_t::roundbox, "Show industry", koord(2*BUTTON_WIDTH+3,0), koord(BUTTON_WIDTH+1,BUTTON_HEIGHT));
	b_show_directory.add_listener(this);
	add_komponente(&b_show_directory);

	// init factories names
	legend.clear();
	const stringhashtable_tpl<const fabrik_besch_t *> & fabesch = fabrikbauer_t::gib_fabesch();
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

	// add factory names; shorten too long names
	while(iter.next()) {
		if(iter.get_current_value()->gib_gewichtung()>0) {
			int i;

			cstring_t label (translator::translate(iter.get_current_value()->gib_name()));
			for(  i=12;  i<label.len()  &&  display_calc_proportional_string_len_width(label,i)<100;  i++  )
				;
			if(  i<label.len()  ) {
				label.set_at(i++, '.');
				label.set_at(i++, '.');
				label.set_at(i++, '\0');
			}

			legend.push_back(legend_entry(label, iter.get_current_value()->gib_kennfarbe()));
		}
	}

	// init the rest
	// size will be set during resize ...
	add_komponente(&scrolly);

	// and now the buttons
	// these will be only added to the window, when they are visible
	// this is the only legal way to hide them
	for (int type=0; type<MAX_BUTTON_TYPE; type++) {
		filter_buttons[type].init(button_t::box,translator::translate(map_type[type]), koord(0,0), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filter_buttons[type].add_listener(this);
		filter_buttons[type].background = map_type_color[type];
		is_filter_active[type] = reliefkarte_t::gib_karte()->get_mode() == type;
		if(legend_visible) {
			add_komponente(filter_buttons + type);
		}
	}

	// Hajo: Hack: use static size if set by a former object
	if(size == koord(0,0)) {
		size = koord(min(256,welt->gib_groesse_x())+11, min(welt->gib_groesse_y(),256)+16+12);
		set_min_windowsize( size );
		gui_frame_t::setze_fenstergroesse( size );
	}
	setze_fenstergroesse(size);
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
	setze_opaque( true );

	is_dragging = false;
}



// button pressed
bool
map_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	bool all_inactive=true;
	reliefkarte_t::gib_karte()->calc_map();

	if(komp==&b_show_legend) {
		if(!legend_visible) {
			for (int type=0; type<MAX_BUTTON_TYPE; type++) {
				add_komponente(filter_buttons + type);
			}
			legend_visible = 1;
		}
		else {
			// do not draw legend anymore
			for (int type=0; type<MAX_BUTTON_TYPE; type++) {
				remove_komponente(filter_buttons + type);
			}
			legend_visible = 0;
		}
		resize( koord(0,0) );
	}
	else if(komp==&b_show_scale) {
		scale_visible = !scale_visible;
		resize( koord(0,0) );
	}
	else if(komp==&b_show_directory) {
		directory_visible = !directory_visible;
		resize( koord(0,0) );
	}
	else if(komp==zoom_buttons+1) {
		// zoom out
		if(reliefkarte_t::gib_karte()->zoom_in>1) {
			reliefkarte_t::gib_karte()->zoom_in--;
		}
		else if(reliefkarte_t::gib_karte()->zoom_out<4) {
			reliefkarte_t::gib_karte()->zoom_out++;
		}
		reliefkarte_t::gib_karte()->calc_map();
		// recalc sliders
		scrolly.setze_groesse( scrolly.gib_groesse() );
	}
	else if(komp==zoom_buttons+0) {
		// zoom in
		if(reliefkarte_t::gib_karte()->zoom_out>1) {
			reliefkarte_t::gib_karte()->zoom_out--;
		}
		else if(reliefkarte_t::gib_karte()->zoom_in<4) {
			reliefkarte_t::gib_karte()->zoom_in++;
		}
		reliefkarte_t::gib_karte()->calc_map();
		// recalc sliders
		scrolly.setze_groesse( scrolly.gib_groesse() );
	}
	else if(komp==&b_rotate45) {
		// rotated/straight map
		reliefkarte_t::gib_karte()->rotate45 ^= 1;
		reliefkarte_t::gib_karte()->calc_map();
		scrolly.setze_groesse( scrolly.gib_groesse() );
	}
	else {
		for (int i=0;i<MAX_BUTTON_TYPE;i++) {
			if (komp == &filter_buttons[i]) {
				if (is_filter_active[i]) {
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
DBG_MESSAGE("map_frame_t::setze_fenstergroesse()","gr.x=%i, gr.y=%i",size.x,size.y );
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

	// mark old size dirty
	const koord pos = koord( win_get_posx(this), win_get_posy(this) );
	mark_rect_dirty_wc(pos.x,pos.y,pos.x+groesse.x,pos.y+groesse.y);

	gui_frame_t::resize(delta);

	int offset_y = BUTTON_HEIGHT*2 + 2;
	groesse.x = max( BUTTON_WIDTH*3+4, groesse.x );	// not smaller than 192 allow

	// find out about the button pos for the additiona objects

	if(legend_visible) {
		// calculate space with legend
		col = max( 1, min( (groesse.x-2)/BUTTON_WIDTH, MAX_BUTTON_TYPE ) );
		row = ((MAX_BUTTON_TYPE-1)/col)+1;

		// set button pos
		for (int type=0; type<MAX_BUTTON_TYPE; type++) {
			koord pos = koord( 2+BUTTON_WIDTH*(type%col), 4+offset_y+BUTTON_HEIGHT*((int)type/col) );
			filter_buttons[type].setze_pos( pos );
		}
		offset_y += BUTTON_HEIGHT*row+8;
	}

	if(scale_visible) {
		// plus scale bar
		offset_y += LINESPACE+4;
	}

	if(directory_visible) {
		// full program including factory texts
		const int fac_cols = clamp(legend.get_count(), 1, (groesse.x - 2) / 110);
		const int fac_rows = (legend.get_count() - 1) / fac_cols + 1;
		offset_y +=(fac_rows*14+4);
	}

	// set allowed min size
	if(groesse.y<offset_y+64) {
		groesse.y = offset_y+64;
	}
	set_min_windowsize(  koord( BUTTON_WIDTH*3+4, offset_y+12+64) );

	// offset of map
	scrolly.setze_pos( koord(0,offset_y) );

	// max size
	offset_y += min(welt->gib_groesse_y(),256)+16+12;
	if(offset_y>display_get_height()-64) {
		// avoid negative values for min size
		offset_y = max(10,display_get_height()-64);
	}
	if(offset_y>groesse.y) {
		groesse.y = offset_y;
	}
	old_ij = koord::invalid;

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
	// update our stored screen position
	screenpos = pos;

	// first: check if cursor within map screen size
	karte_t *welt=reliefkarte_t::gib_karte()->gib_welt();
	koord ij = welt->gib_ij_off();
	if(welt->ist_in_kartengrenzen(ij)) {
		reliefkarte_t::gib_karte()->karte_to_screen(ij);
		// only recenter by zoom or position change; we want still be able to scroll
		if(old_ij!=ij) {
			koord groesse = scrolly.gib_groesse();
			if(
				(scrolly.get_scroll_x()>ij.x  ||  scrolly.get_scroll_x()+groesse.x<=ij.x) ||
				(scrolly.get_scroll_y()>ij.y  ||  scrolly.get_scroll_y()+groesse.y<=ij.y) ) {
				// recenter cursor by scrolling
				scrolly.setze_scroll_position( max(0,ij.x-(groesse.x/2)), max(0,ij.y-(groesse.y/2)) );
				old_ij = ij;
			}
		}
	}

	// button state
	for (int i = 0;i<MAX_BUTTON_TYPE;i++) {
		filter_buttons[i].pressed = is_filter_active[i];
	}

	b_rotate45.pressed = reliefkarte_t::gib_karte()->rotate45;

	b_show_legend.pressed = legend_visible;
	b_show_scale.pressed = scale_visible;
	b_show_directory.pressed = directory_visible;

	gui_frame_t::zeichnen(pos, gr);

	char buf[16];
	sprintf( buf, "%i:%i", reliefkarte_t::gib_karte()->zoom_in, reliefkarte_t::gib_karte()-> zoom_out );
	display_proportional( pos.x+20, pos.y+16+BUTTON_HEIGHT+4, buf, ALIGN_LEFT, COL_WHITE, true);

	int offset_y = BUTTON_HEIGHT*2 + 2 +16;
	if(legend_visible) {
		offset_y = 16+filter_buttons[MAX_BUTTON_TYPE-1].gib_pos().y+4+BUTTON_HEIGHT;
	}

	// draw scale
	if(scale_visible) {
		koord bar_pos = pos + koord( 0, offset_y+2 );
		// color bar
		for( int i=0;  i<MAX_SEVERITY_COLORS;  i++) {
			display_fillbox_wh(bar_pos.x + 30 + i*(gr.x-60)/MAX_SEVERITY_COLORS, bar_pos.y+2,  (gr.x-60)/(MAX_SEVERITY_COLORS-1), 7, reliefkarte_t::calc_severity_color(i,MAX_SEVERITY_COLORS), false);
		}
		display_proportional(bar_pos.x + 26, bar_pos.y, translator::translate("min"), ALIGN_RIGHT, COL_BLACK, false);
		display_proportional(bar_pos.x + size.x - 26, bar_pos.y, translator::translate("max"), ALIGN_LEFT, COL_BLACK, false);
		offset_y += LINESPACE+4;
	}

	// draw factory descriptions
	if(directory_visible) {
		const int fac_cols = clamp(legend.get_count(), 1, (gr.x - 2) / 110);
		const int width = (gr.x-10)/fac_cols;
		uint u = 0;
		for (vector_tpl<legend_entry>::const_iterator i = legend.begin(), end = legend.end(); i != end; ++i, ++u) {
			const int xpos = pos.x + (u % fac_cols) * width + 8;
			const int ypos = pos.y + (u / fac_cols) * 14    + offset_y;

			if(ypos+LINESPACE>pos.y+gr.y) {
				break;
			}
			display_fillbox_wh(xpos, ypos+ 1 , 7, 7, i->colour, false);
			display_proportional(xpos + 8, ypos, i->text, ALIGN_LEFT, COL_BLACK, false);
		}
	}
}
