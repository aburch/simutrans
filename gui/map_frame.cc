/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <string>
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
#include "../dataobj/umgebung.h"
#include "../dataobj/translator.h"
#include "../dataobj/koord.h"
#include "../dataobj/loadsave.h"
#include "../besch/fabrik_besch.h"


static koord old_ij=koord::invalid;

koord map_frame_t::size=koord(0,0);
bool map_frame_t::legend_visible=false;
bool map_frame_t::scale_visible=false;
bool map_frame_t::directory_visible=false;
bool map_frame_t::is_cursor_hidden=false;

// Hajo: we track our position onscreen
koord map_frame_t::screenpos;


struct legend_entry
{
	legend_entry() {}
	legend_entry(const std::string &text_, int colour_) : text(text_), colour(colour_) {}

	std::string text;
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
	// show the various objects
	b_show_legend.init(button_t::roundbox_state, "Show legend", koord(BUTTON1_X,0), koord(BUTTON_WIDTH-1,BUTTON_HEIGHT));
	b_show_legend.add_listener(this);
	add_komponente(&b_show_legend);

	b_show_scale.init(button_t::roundbox_state, "Show map scale", koord(BUTTON2_X-1,0), koord(BUTTON_WIDTH+2,BUTTON_HEIGHT));
	b_show_scale.add_listener(this);
	add_komponente(&b_show_scale);

	b_show_directory.init(button_t::roundbox_state, "Show industry", koord(BUTTON3_X+1,0), koord(BUTTON_WIDTH-1,BUTTON_HEIGHT));
	b_show_directory.add_listener(this);
	add_komponente(&b_show_directory);

	// zoom levels
	zoom_buttons[0].init(button_t::repeatarrowleft, NULL, koord(2,BUTTON_HEIGHT+4));
	zoom_buttons[0].add_listener( this );
	add_komponente( zoom_buttons+0 );

	zoom_buttons[1].init(button_t::repeatarrowright, NULL, koord(40,BUTTON_HEIGHT+4));
	zoom_buttons[1].add_listener( this );
	add_komponente( zoom_buttons+1 );

	zoom_label.set_pos( koord(54,BUTTON_HEIGHT+4) );
	add_komponente( &zoom_label );

	// rotate map 45°
	b_rotate45.init(button_t::square_state, "isometric map", koord(BUTTON_WIDTH+40,BUTTON_HEIGHT+4));
	b_rotate45.add_listener(this);
	add_komponente(&b_rotate45);

	// show/hide schedule
	b_show_schedule.init(button_t::square_state, "Show schedules", koord(BUTTON_WIDTH+40,BUTTON_HEIGHT*2+4)); // right align
	b_show_schedule.set_tooltip("Shows the currently selected schedule");
	b_show_schedule.add_listener(this);
	add_komponente( &b_show_schedule );

	// show/hide factory links
	b_show_fab_connections.init(button_t::square_state, "factory details", koord(2,BUTTON_HEIGHT*2+4)); // right align
	b_show_fab_connections.set_tooltip("Shows consumer/suppliers for factories");
	b_show_fab_connections.add_listener(this);
	add_komponente( &b_show_fab_connections );

	// init factories names
	legend.clear();
	const stringhashtable_tpl<const fabrik_besch_t *> & fabesch = fabrikbauer_t::get_fabesch();
	stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

	while(iter.next()) {
		if(iter.get_current_value()->get_gewichtung()>0) {
			legend.append(legend_entry(translator::translate(iter.get_current_value()->get_name()), iter.get_current_value()->get_kennfarbe()));
		}
	}

	// init the rest
	scrolly.set_pos( koord(0, BUTTON_HEIGHT*3 + 2) );
	scrolly.set_show_scroll_x(true);
	scrolly.set_scroll_discrete_y(false);
	add_komponente(&scrolly);

	// and now the buttons
	for (int type=0; type<MAX_BUTTON_TYPE; type++) {
		filter_buttons[type].init(button_t::box_state, translator::translate(map_type[type]), koord(0,0), koord(BUTTON_WIDTH, BUTTON_HEIGHT));
		filter_buttons[type].background = map_type_color[type];
		filter_buttons[type].set_visible(false);
		filter_buttons[type].pressed = type==umgebung_t::default_mapmode;
		filter_buttons[type].add_listener(this);
		add_komponente(filter_buttons + type);
	}

 	// Hajo: Hack: use static size if set by a former object
	const koord size2 = size;

	set_fenstergroesse(koord(TOTAL_WIDTH, TITLEBAR_HEIGHT+256+6));
	set_min_windowsize(koord(BUTTON4_X, TITLEBAR_HEIGHT+2*BUTTON_HEIGHT+4+64));

	if(  size2 != koord(0,0)  ) {
		if(  legend_visible  ) {
			show_hide_legend( legend_visible );
		}
		if(  scale_visible  ) {
			show_hide_scale( scale_visible );
		}
		if(  directory_visible  ) {
			show_hide_directory( directory_visible );
		}

		resize( size2 - get_fenstergroesse() );
	}

	set_resizemode(diagonal_resize);
	resize(koord(0,0));

	reliefkarte_t *karte = reliefkarte_t::get_karte();
	karte->set_welt( welt );

	const koord gr = karte->get_groesse();
	const koord s_gr=scrolly.get_groesse();
	const koord ij = welt->get_world_position();
	const koord win_size = gr-s_gr;	// this is the visible area
	scrolly.set_scroll_position(  max(0,min(ij.x-win_size.x/2,gr.x)), max(0, min(ij.y-win_size.y/2,gr.y)) );

	old_ij = koord::invalid;

	// Hajo: Trigger layouting
	is_dragging = false;
	zoomed = false;

	karte->set_mode( (reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode );
}


void map_frame_t::show_hide_legend(const bool show){
	b_show_legend.pressed = show;
	legend_visible = show;

	const int col = max( 1, min( (get_fenstergroesse().x-2)/(BUTTON_WIDTH+BUTTON_SPACER), MAX_BUTTON_TYPE ) );
	const int row = ((MAX_BUTTON_TYPE-1)/col)+1;
	const int offset_y = (BUTTON_HEIGHT+2)*row;
	const koord offset = show ? koord(0, offset_y) : koord(0, -offset_y);

	for (int type=0; type<MAX_BUTTON_TYPE; type++) {
		filter_buttons[type].set_visible(show);
	}
	scrolly.set_pos(scrolly.get_pos() + offset);

	set_min_windowsize(get_min_windowsize() + offset);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
}


void map_frame_t::show_hide_scale(const bool show){
	b_show_scale.pressed = show;
	scale_visible = show;

	const koord offset = show ? koord(0, (LINESPACE+4)) : koord(0, -(LINESPACE+4));

	scrolly.set_pos(scrolly.get_pos() + offset);

	set_min_windowsize(get_min_windowsize() + offset);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
}


void map_frame_t::show_hide_directory(const bool show){
	b_show_directory.pressed = show;
	directory_visible = show;

	const int fac_cols = clamp(legend.get_count(), 1, get_fenstergroesse().x / (TOTAL_WIDTH/3));
	const int fac_rows = (legend.get_count() - 1) / fac_cols + 1;
	const koord offset = show ? koord(0, (fac_rows*14)) : koord(0, -(fac_rows*14));

	scrolly.set_pos(scrolly.get_pos() + offset);

	set_min_windowsize(get_min_windowsize() + offset);
	set_fenstergroesse(get_fenstergroesse() + offset);
	resize(koord(0,0));
}


bool map_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp==&b_show_legend) {
		show_hide_legend( !b_show_legend.pressed );
	}
	else if(komp==&b_show_scale) {
		show_hide_scale( !b_show_scale.pressed );
	}
	else if(komp==&b_show_directory) {
		show_hide_directory( !b_show_directory.pressed );
	}
	else if(komp==zoom_buttons+1) {
		// zoom out
		zoom(true);
	}
	else if(komp==zoom_buttons+0) {
		// zoom in
		zoom(false);
	}
	else if(komp==&b_rotate45) {
		// rotated/straight map
		reliefkarte_t::get_karte()->rotate45 ^= 1;
		b_rotate45.pressed = reliefkarte_t::get_karte()->rotate45;
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );
	}
	else if(komp==&b_show_schedule) {
		//	show/hide schedule of convoi
		reliefkarte_t::get_karte()->is_show_schedule = !reliefkarte_t::get_karte()->is_show_schedule;
		b_show_schedule.pressed = reliefkarte_t::get_karte()->is_show_schedule;
		}
	else if(komp==&b_show_fab_connections) {
		//	show/hide factory connections
		reliefkarte_t::get_karte()->is_show_fab = !reliefkarte_t::get_karte()->is_show_fab;
		b_show_fab_connections.pressed = reliefkarte_t::get_karte()->is_show_fab;
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
	// recalc map size
	reliefkarte_t::get_karte()->calc_map_groesse();
	// recalc all the other data incl scrollbars
	resize(koord(0,0));

	zoomed = true;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool map_frame_t::infowin_event(const event_t *ev)
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
		return true;
	}

	// Hajo: hack: relief map can resize upon right click
	// we track this here, and adjust size.
	if(  IS_RIGHTCLICK(ev)  ) {
		is_dragging = false;
		display_show_pointer(false);
		is_cursor_hidden = true;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(false);
		return true;
	}
	else if(  IS_RIGHTRELEASE(ev)  ) {
		if(!is_dragging) {
			resize( koord(0,0) );
		}
		is_dragging = false;
		display_show_pointer(true);
		is_cursor_hidden = false;
		reliefkarte_t::get_karte()->get_welt()->set_scroll_lock(false);
		return true;
	}
	else if(  IS_RIGHTDRAG(ev)  &&  reliefkarte_t::get_karte()->getroffen(ev->mx,ev->my)  ) {
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
		return true;
	}
	else if(  is_cursor_hidden  )
	{
		display_show_pointer(true);
		is_cursor_hidden = false;
	}

	return gui_frame_t::infowin_event(ev);
}


/**
 * size window in response and save it in static size
 * @author (Mathew Hounsell)
 * @date   11-Mar-2003
 */
void map_frame_t::set_fenstergroesse(koord groesse)
{
	gui_frame_t::set_fenstergroesse( groesse );

	scrolly.set_groesse(get_client_windowsize()-scrolly.get_pos());
	map_frame_t::size = get_fenstergroesse();
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   01-Jun-2002
 */
void map_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);

	const KOORD_VAL old_offset_y = scrolly.get_pos().y;
	int offset_y = BUTTON_HEIGHT*3 + 2;

	if(legend_visible) {
		// calculate space with legend
		const int col = max( 1, min( (get_fenstergroesse().x-2)/(BUTTON_WIDTH+BUTTON_SPACER), MAX_BUTTON_TYPE ) );
		const int row = ((MAX_BUTTON_TYPE-1)/col)+1;

		// set button pos
		for (int type=0; type<MAX_BUTTON_TYPE; type++) {
			koord pos = koord( 2+(BUTTON_WIDTH+BUTTON_SPACER)*(type%col), offset_y+(BUTTON_HEIGHT+2)*((int)type/col) );
			filter_buttons[type].set_pos( pos );
		}
		offset_y += (BUTTON_HEIGHT+2)*row;
	}

	if(scale_visible) {
		// plus scale bar
		offset_y += LINESPACE+4;
	}

	if(directory_visible) {
		// full program including factory texts
		const int fac_cols = clamp(legend.get_count(), 1, get_fenstergroesse().x / (TOTAL_WIDTH/3));
		const int fac_rows = (legend.get_count() - 1) / fac_cols + 1;
		offset_y += fac_rows*14;

		// factory names; shorten too long names
		legend.clear();
		const stringhashtable_tpl<const fabrik_besch_t *> & fabesch = fabrikbauer_t::get_fabesch();
		stringhashtable_iterator_tpl<const fabrik_besch_t *> iter (fabesch);

		while(iter.next()) {
			if(iter.get_current_value()->get_gewichtung()>0) {
				size_t i;
				const int dot_len = proportional_string_width("..");

				std::string label (translator::translate(iter.get_current_value()->get_name()));
				for(  i=12;  i<label.size()  &&  display_calc_proportional_string_len_width(label.c_str(),i)<get_fenstergroesse().x/fac_cols-dot_len-3-9-1;  i++  )
					;
				if(  i<label.size()  ) {
					label = label.substr(0, i);
					label.append("..");
				}

				legend.append(legend_entry(label, iter.get_current_value()->get_kennfarbe()));
			}
		}
	}

	// offset of map
	scrolly.set_pos( koord(0,offset_y) );
	set_dirty();

	if(  offset_y != old_offset_y  ) {
		set_min_windowsize(get_min_windowsize() + koord(0,offset_y - old_offset_y));
		resize(koord(0,0));
	}
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
		// only recenter if zoomed or world position has changed and its outside visible area
		const koord groesse = scrolly.get_groesse();
		if(zoomed  ||  ( old_ij != ij  &&
				( scrolly.get_scroll_x()>ij.x  ||  scrolly.get_scroll_x()+groesse.x<=ij.x  ||
				  scrolly.get_scroll_y()>ij.y  ||  scrolly.get_scroll_y()+groesse.y<=ij.y ) ) ) {
				// recenter cursor by scrolling
				scrolly.set_scroll_position( max(0,ij.x-(groesse.x/2)), max(0,ij.y-(groesse.y/2)) );
				zoomed = false;
		}
		// remember world position, we do not want to have surprises when scrolling later on
		old_ij = ij;
	}

	gui_frame_t::zeichnen(pos, gr);

	char buf[16];
	sprintf( buf, "%i:%i", reliefkarte_t::get_karte()->zoom_in, reliefkarte_t::get_karte()-> zoom_out );
	display_proportional( pos.x+18, pos.y+16+BUTTON_HEIGHT+5, buf, ALIGN_LEFT, COL_WHITE, true);

	int offset_y = BUTTON_HEIGHT*3 + 2 + 16;
	if(legend_visible) {
		offset_y = 16+filter_buttons[MAX_BUTTON_TYPE-1].get_pos().y+BUTTON_HEIGHT+2;
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
		const int fac_cols = clamp(legend.get_count(), 1, gr.x / (TOTAL_WIDTH/3));
		uint u = 0;
		for (vector_tpl<legend_entry>::const_iterator i = legend.begin(), end = legend.end(); i != end; ++i, ++u) {
			const int xpos = pos.x + (u % fac_cols) * (gr.x-fac_cols/2-1)/fac_cols + 3;
			const int ypos = pos.y + (u / fac_cols) * 14 + offset_y+2;

			if(ypos+LINESPACE>pos.y+gr.y) {
				break;
			}
			display_fillbox_wh(xpos, ypos + 1 , 7, 7, i->colour, false);
			display_proportional(xpos + 9, ypos, i->text.c_str(), ALIGN_LEFT, COL_BLACK, false);
		}
	}
}


void map_frame_t::rdwr( loadsave_t *file )
{
	file->rdwr_bool( reliefkarte_t::get_karte()->rotate45 );
	file->rdwr_bool( reliefkarte_t::get_karte()->is_show_schedule );
	file->rdwr_bool( reliefkarte_t::get_karte()->is_show_fab );
	file->rdwr_short( reliefkarte_t::get_karte()->zoom_in );
	file->rdwr_short( reliefkarte_t::get_karte()->zoom_out );
	bool show_legend_state = legend_visible;
	file->rdwr_bool( show_legend_state );
	file->rdwr_bool( scale_visible );
	file->rdwr_bool( directory_visible );
	file->rdwr_byte( umgebung_t::default_mapmode );

	if(  file->is_loading()  ) {
		koord savesize;
		savesize.rdwr(file);
		set_fenstergroesse( savesize );
		resize( koord(0,0) );
		// notify minimap of new settings
		reliefkarte_t::get_karte()->calc_map_groesse();
		scrolly.set_groesse( scrolly.get_groesse() );

		sint32 xoff;
		file->rdwr_long( xoff );
		sint32 yoff;
		file->rdwr_long( yoff );
		scrolly.set_scroll_position( xoff, yoff );

		reliefkarte_t::get_karte()->set_mode((reliefkarte_t::MAP_MODES)umgebung_t::default_mapmode);
		for (int i=0;i<MAX_BUTTON_TYPE;i++) {
			filter_buttons[i].pressed = i==umgebung_t::default_mapmode;
		}
		if(  legend_visible!=show_legend_state  ) {
			action_triggered( &b_show_legend, (long)0 );
		}
	}
	else {
		koord gr = get_fenstergroesse();
		gr.rdwr(file);
		sint32 xoff = scrolly.get_scroll_x();
		file->rdwr_long( xoff );
		sint32 yoff = scrolly.get_scroll_y();
		file->rdwr_long( yoff );
	}
}
