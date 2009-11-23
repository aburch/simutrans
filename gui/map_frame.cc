/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>
#include <cmath>

#include "karte.h"
#include "map_frame.h"
#include "components/list_button.h"

#include "../simworld.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simcolor.h"
#include "../bauer/fabrikbauer.h"
#include "../utils/cstring_t.h"
#include "../dataobj/umgebung.h"
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
	"Forest",
	"CityLimit",
	"PaxDest"
};

const uint8 map_frame_t::map_type_color[MAX_BUTTON_TYPE] =
{
	215, 23, 31, 157, 46, 55, 63, 133, 79, 191, 207, 11, 123, 221, 71, 135, 127, 198, 23
};


map_frame_t::map_frame_t(karte_t *welt) :
	gui_frame_t("Reliefkarte"),
	scrolly(reliefkarte_t::get_karte()),
	zoom_label("map zoom")
{
	// zoom levels
	zoom_buttons[0].set_pos( koord(2,BUTTON_HEIGHT+4) );
	zoom_buttons[0].set_typ( button_t::repeatarrowleft );
	zoom_buttons[0].add_listener( this );
	add_komponente( zoom_buttons+0 );
	zoom_buttons[1].set_pos( koord(40,BUTTON_HEIGHT+4) );
	zoom_buttons[1].set_typ( button_t::repeatarrowright );
	zoom_buttons[1].add_listener( this );
	add_komponente( zoom_buttons+1 );
	zoom_label.set_pos( koord(54,BUTTON_HEIGHT+4) );
	add_komponente( &zoom_label );

	// rotate map 45°
	b_rotate45.init(button_t::square, "isometric map", koord(BUTTON_WIDTH+40,BUTTON_HEIGHT+4), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	b_rotate45.set_tooltip("Show the map in the same isometric orientation as the main game window");
	b_rotate45.add_listener(this);
	add_komponente(&b_rotate45);

	// show/hide schedule
	b_show_schedule.init(button_t::square, "Show schedules", koord(BUTTON_WIDTH+40,BUTTON_HEIGHT*2+4), koord(BUTTON_WIDTH,BUTTON_HEIGHT)); // right align
	b_show_schedule.set_tooltip("Shows the currently selected schedule");
	b_show_schedule.add_listener(this);
	add_komponente( &b_show_schedule );

	// show/hide schedule
	b_show_fab_connections.init(button_t::square, "factory details", koord(2,BUTTON_HEIGHT*2+4), koord(BUTTON_WIDTH,BUTTON_HEIGHT)); // right align
	b_show_fab_connections.set_tooltip("Shows consumer/suppliers for factories");
	b_show_fab_connections.add_listener(this);
	add_komponente( &b_show_fab_connections );

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
	const stringhashtable_tpl<const fabrik_besch_t *> & fabesch = fabrikbauer_t::get_fabesch();
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

	minivec_tpl<uint8> colours;

	// add factory names; shorten too long names
	while(iter.next()) 
	{
		if(iter.get_current_value()->get_gewichtung()>0) 
		{
			int i;

			// Do not show multiple factories with the same colour.
			// @author: jamespetts, July 2009
			if(colours.append_unique(iter.get_current_value()->get_kennfarbe()))
			{
				cstring_t label (translator::translate(iter.get_current_value()->get_name()));
				for(  i=12;  i<label.len()  &&  display_calc_proportional_string_len_width(label,i)<100;  i++  )
					;
				if(  i<label.len()  ) {
					label.set_at(i++, '.');
					label.set_at(i++, '.');
					label.set_at(i++, '\0');
				}

				legend.append(legend_entry(label, iter.get_current_value()->get_kennfarbe()));
			}
		}
	}

	// init the rest
	// size will be set during resize ...
	add_komponente(&scrolly);

	// and now the buttons
	// these will be only added to the window, when they are visible
	// this is the only legal way to hide them
	for (int type=0; type<MAX_BUTTON_TYPE; type++) {
		filter_buttons[type].init(button_t::box_state,translator::translate(map_type[type]), koord(0,0), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filter_buttons[type].add_listener(this);
		filter_buttons[type].background = map_type_color[type];
		if(legend_visible) {
			add_komponente(filter_buttons + type);
		}
		filter_buttons[type].pressed = type==umgebung_t::default_mapmode;
	}

	// Hajo: Hack: use static size if set by a former object
	if(size == koord(0,0)) {
		size = koord(min(256,welt->get_groesse_x())+11, min(welt->get_groesse_y(),256)+16+12);
		set_min_windowsize( size );
		gui_frame_t::set_fenstergroesse( size );
	}
	set_fenstergroesse(size);
	resize( koord(0,0) );

	reliefkarte_t *karte = reliefkarte_t::get_karte();
	karte->set_welt( welt );

	const koord gr = karte->get_groesse();
	const koord s_gr=scrolly.get_groesse();
	const koord ij = welt->get_world_position();
	const koord win_size = gr-s_gr;	// this is the visible area
	scrolly.set_scroll_position(  max(0,min(ij.x-win_size.x/2,gr.x)), max(0, min(ij.y-win_size.y/2,gr.y)) );

	// Hajo: Trigger layouting
	set_resizemode(diagonal_resize);

	is_dragging = false;

	karte->set_mode( (reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode );
}



// button pressed
bool
map_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
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
		if(reliefkarte_t::get_karte()->zoom_in>1) {
			reliefkarte_t::get_karte()->zoom_in--;
		}
		else if(reliefkarte_t::get_karte()->zoom_out<4  ) {
			reliefkarte_t::get_karte()->zoom_out++;
		}
		// recalc sliders
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );
	}
	else if(komp==zoom_buttons+0) {
		// zoom in
		if(reliefkarte_t::get_karte()->zoom_out>1) {
			reliefkarte_t::get_karte()->zoom_out--;
		}
		else if(reliefkarte_t::get_karte()->zoom_in<4  ) {
			reliefkarte_t::get_karte()->zoom_in++;
		}
		// recalc sliders
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );
	}
	else if(komp==&b_rotate45) {
		// rotated/straight map
		reliefkarte_t::get_karte()->rotate45 ^= 1;
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );
	}
	else if(komp==&b_show_schedule) {
		//	show/hide schedule of convoi
		reliefkarte_t::get_karte()->is_show_schedule = !reliefkarte_t::get_karte()->is_show_schedule;
	}
	else if(komp==&b_show_fab_connections) {
		//	show/hide schedule of convoi
		reliefkarte_t::get_karte()->is_show_fab = !reliefkarte_t::get_karte()->is_show_fab;
	}

	else {
		for (int i=0;i<MAX_BUTTON_TYPE;i++) {
			if (komp == &filter_buttons[i]) {
				if(filter_buttons[i].pressed) {
					umgebung_t::default_mapmode = -1;
				}
				else {
					umgebung_t::default_mapmode = i;
				}
			}
		}
		reliefkarte_t::get_karte()->set_mode((reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode);
		for (int i=0;i<MAX_BUTTON_TYPE;i++) {
			filter_buttons[i].pressed = i==umgebung_t::default_mapmode;
		}
	}
	filter_buttons[18].set_tooltip("Passenger destinations");
	return true;
}

void map_frame_t::zoom(bool zoom_out)
{
	if (zoom_out) {
		// zoom out
		if(reliefkarte_t::get_karte()->zoom_in>1) {
			reliefkarte_t::get_karte()->zoom_in--;
		}
		else if(reliefkarte_t::get_karte()->zoom_out<4  ) {
			reliefkarte_t::get_karte()->zoom_out++;
		}
	}
	else {
		// zoom in
		if(reliefkarte_t::get_karte()->zoom_out>1) {
			reliefkarte_t::get_karte()->zoom_out--;
		}
		else if(reliefkarte_t::get_karte()->zoom_in<8  ) {
			reliefkarte_t::get_karte()->zoom_in++;
		}
	}
	// recalc sliders
	reliefkarte_t::get_karte()->calc_map_groesse();
	scrolly.set_groesse( scrolly.get_groesse() );
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
			reliefkarte_t::get_karte()->set_xy_offset_size( koord(0,0), koord(0,0) );
		}
		else if(ev->ev_code == WIN_CLOSE) {
			reliefkarte_t::get_karte()->is_visible = false;
		}
	}

	if(  (IS_WHEELUP(ev) || IS_WHEELDOWN(ev))  &&  reliefkarte_t::get_karte()->getroffen(ev->mx-scrolly.get_pos().x,ev->my-scrolly.get_pos().y-16)) {
		// otherwise these would go to the vertical scroll bar
		zoom(IS_WHEELUP(ev));
		return;
	}

	if(!is_dragging) {
		gui_frame_t::infowin_event(ev);
	}

	// Hajo: hack: relief map can resize upon right click
	// we track this here, and adjust size.
	if(IS_RIGHTCLICK(ev)) {
		is_dragging = false;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(false);
	}

	if(IS_RIGHTRELEASE(ev)) {
		if(!is_dragging) {
			resize( koord(0,0) );
		}
		is_dragging = false;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(false);
	}

	if(reliefkarte_t::get_karte()->getroffen(ev->mx,ev->my)  &&  IS_RIGHTDRAG(ev)) {
		int x = scrolly.get_scroll_x();
		int y = scrolly.get_scroll_y();
		const int scroll_direction = ( umgebung_t::scroll_multi>0 ? 1 : -1 );

		x += (ev->mx - ev->cx)*scroll_direction*2;
		y += (ev->my - ev->cy)*scroll_direction*2;

		is_dragging = true;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(true);

		scrolly.set_scroll_position(  max(0, x),  max(0, y) );

		// Hajo: re-center mouse pointer
		display_move_pointer(screenpos.x+ev->cx, screenpos.y+ev->cy);
	}
}



/**
 * size window in response and save it in static size
 * @author (Mathew Hounsell)
 * @date   11-Mar-2003
 */
void map_frame_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse( groesse );
	groesse = get_fenstergroesse();
	// set scroll area size
	scrolly.set_groesse( groesse-scrolly.get_pos()-koord(0,16) );
	map_frame_t::size = get_fenstergroesse();
DBG_MESSAGE("map_frame_t::set_fenstergroesse()","gr.x=%i, gr.y=%i",size.x,size.y );
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_frame_t::resize(const koord delta)
{
	karte_t *welt = reliefkarte_t::get_karte()->get_welt();

	koord groesse = get_fenstergroesse()+delta;

	// mark old size dirty
	const koord pos = koord( win_get_posx(this), win_get_posy(this) );
	mark_rect_dirty_wc(pos.x,pos.y,pos.x+groesse.x,pos.y+groesse.y);

	gui_frame_t::resize(delta);

	int offset_y = BUTTON_HEIGHT*3 + 2;
	groesse.x = max( BUTTON_WIDTH*3+4, groesse.x );	// not smaller than 192 allow

	if(legend_visible) {
		// calculate space with legend
		col = max( 1, min( (groesse.x-2)/BUTTON_WIDTH, MAX_BUTTON_TYPE ) );
		row = ((MAX_BUTTON_TYPE-1)/col)+1;

		// set button pos
		for (int type=0; type<MAX_BUTTON_TYPE; type++) {
			koord pos = koord( 2+BUTTON_WIDTH*(type%col), 4+offset_y+BUTTON_HEIGHT*((int)type/col) );
			filter_buttons[type].set_pos( pos );
		}
		offset_y += BUTTON_HEIGHT*row+8;
	}

	if(scale_visible) {
		// plus scale bar
		offset_y += LINESPACE+24;
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
	scrolly.set_pos( koord(0,offset_y) );

	// max size
	offset_y += min(welt->get_groesse_y(),256)+16+12;
	if(offset_y>display_get_height()-64) {
		// avoid negative values for min size
		offset_y = max(10,display_get_height()-64);
	}
	if(offset_y>groesse.y) {
		groesse.y = offset_y;
	}
	old_ij = koord::invalid;

	set_fenstergroesse( groesse );
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

	reliefkarte_t::get_karte()->set_xy_offset_size( koord(scrolly.get_scroll_x(), scrolly.get_scroll_y()), koord(scrolly.get_groesse()-koord(12,11)) );

	// first: check if cursor within map screen size
	karte_t *welt=reliefkarte_t::get_karte()->get_welt();
	koord ij = welt->get_world_position();
	if(welt->ist_in_kartengrenzen(ij)) {
		reliefkarte_t::get_karte()->karte_to_screen(ij);
		// only recenter by zoom or position change; we want still be able to scroll
		if(old_ij!=ij) {
			koord groesse = scrolly.get_groesse();
			if(
				(scrolly.get_scroll_x()>ij.x  ||  scrolly.get_scroll_x()+groesse.x<=ij.x) ||
				(scrolly.get_scroll_y()>ij.y  ||  scrolly.get_scroll_y()+groesse.y<=ij.y) ) {
				// recenter cursor by scrolling
				scrolly.set_scroll_position( max(0,ij.x-(groesse.x/2)), max(0,ij.y-(groesse.y/2)) );
				old_ij = ij;
			}
		}
	}

	b_rotate45.pressed = reliefkarte_t::get_karte()->rotate45;
	b_show_schedule.pressed = reliefkarte_t::get_karte()->is_show_schedule;
	b_show_fab_connections.pressed = reliefkarte_t::get_karte()->is_show_fab;

	b_show_legend.pressed = legend_visible;
	b_show_scale.pressed = scale_visible;
	b_show_directory.pressed = directory_visible;

	gui_frame_t::zeichnen(pos, gr);

	char buf[16];
	sprintf( buf, "%i:%i", reliefkarte_t::get_karte()->zoom_in, reliefkarte_t::get_karte()-> zoom_out );
	display_proportional( pos.x+20, pos.y+16+BUTTON_HEIGHT+4, buf, ALIGN_LEFT, COL_WHITE, true);

	int offset_y = BUTTON_HEIGHT*3 + 2 + 16;
	if(legend_visible) {
		offset_y = 16+filter_buttons[MAX_BUTTON_TYPE-1].get_pos().y+4+BUTTON_HEIGHT;
	}

	// draw scale
	if(scale_visible) 
	{
		koord bar_pos = pos + koord( 0, offset_y+2 );
		// color bar
		for(uint8 i = 0; i < MAX_SEVERITY_COLORS; i++) 
		{
			display_fillbox_wh(bar_pos.x + 30 + i*(gr.x-60)/MAX_SEVERITY_COLORS, bar_pos.y+2,  (gr.x-60)/(MAX_SEVERITY_COLORS-1), 7, reliefkarte_t::calc_severity_color(i,MAX_SEVERITY_COLORS), false);
		}
		display_proportional(bar_pos.x + 26, bar_pos.y, translator::translate("min"), ALIGN_RIGHT, COL_BLACK, false);
		display_proportional(bar_pos.x + size.x - 26, bar_pos.y, translator::translate("max"), ALIGN_LEFT, COL_BLACK, false);
		char scale_text[64] = "NULL";
		if(fmod(1, welt->get_einstellungen()->get_distance_per_tile()) == 0)
		{
			// Can use integer
			sprintf(scale_text, "%i %s %s", (uint16)(1 / welt->get_einstellungen()->get_distance_per_tile()), translator::translate("tiles"), translator::translate("per 1 km"));
		}
		else
		{
			// Otherwise, must use float
			
			sprintf(scale_text, "%f %s %s", (1 / welt->get_einstellungen()->get_distance_per_tile()), translator::translate("tiles"), translator::translate("per 1 km"));
		}
		display_proportional(bar_pos.x + 4, bar_pos.y + 16, scale_text, ALIGN_LEFT, COL_BLACK, false);
		offset_y += LINESPACE+24;
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
