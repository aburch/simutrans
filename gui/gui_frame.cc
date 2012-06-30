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

#include "gui_frame.h"
#include "../simcolor.h"
#include "../simgraph.h"
#include "../simwin.h"
#include "../simworld.h"
#include "../player/simplay.h"

#include "../besch/reader/obj_reader.h"
#include "../simskin.h"
#include "../besch/skin_besch.h"

// default button sizes
KOORD_VAL gui_frame_t::gui_button_width = 92;
KOORD_VAL gui_frame_t::gui_button_height = 14;

// default titlebar height
KOORD_VAL gui_frame_t::gui_titlebar_height = 16;

// dialog borders
KOORD_VAL gui_frame_t::gui_frame_left = 10;
KOORD_VAL gui_frame_t::gui_frame_top = 10;
KOORD_VAL gui_frame_t::gui_frame_right = 10;
KOORD_VAL gui_frame_t::gui_frame_bottom = 10;

// space between two elements
KOORD_VAL gui_frame_t::gui_hspace = 4;
KOORD_VAL gui_frame_t::gui_vspace = 4;

// size of staus indicator elements (colored boxes)
KOORD_VAL gui_frame_t::gui_indicator_width = 20;
KOORD_VAL gui_frame_t::gui_indicator_height = 4;



gui_frame_t::gui_frame_t(char const* const name, spieler_t const* const sp)
{
	this->name = name;
	groesse = koord(200, 100);
	min_windowsize = koord(0,0);
	owner = sp;
	container.set_pos(koord(0,D_TITLEBAR_HEIGHT));
	set_resizemode(no_resize); //25-may-02	markus weber	added
	dirty = true;
}



/**
 * Setzt die Fenstergroesse
 * @author Hj. Malthaner
 */
void gui_frame_t::set_fenstergroesse(koord groesse)
{
	if(  groesse != this->groesse  ) {
		// mark old size dirty
		koord const& pos = win_get_pos(this);
		mark_rect_dirty_wc( pos.x, pos.y, pos.x+this->groesse.x, pos.y+this->groesse.y );

		// minimal width //25-may-02	markus weber	added
		if(  groesse.x < min_windowsize.x  ) {
			groesse.x = min_windowsize.x;
		}

		// minimal heigth //25-may-02	markus weber	added
		if(  groesse.y < min_windowsize.y  ) {
			groesse.y = min_windowsize.y;
		}

		this->groesse = groesse;
		dirty = true;
	}
}


/**
 * gibt farbinformationen fuer Fenstertitel, -ränder und -körper
 * zurück
 * @author Hj. Malthaner
 */
PLAYER_COLOR_VAL gui_frame_t::get_titelcolor() const
{
	return owner ? PLAYER_FLAG|(owner->get_player_color1()+1) : WIN_TITEL;
}


/**
 * Events werden hiermit an die GUI-Komponenten
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_frame_t::infowin_event(const event_t *ev)
{
	// %DB0 printf( "\nMessage: gui_frame_t::infowin_event( event_t const * ev ) : Fenster|Window %p : Event is %d", (void*)this, ev->ev_class );
	if(IS_WINDOW_RESIZE(ev)) {
		koord delta (  resize_mode & horizonal_resize ? ev->mx - ev->cx : 0,
		               resize_mode & vertical_resize  ? ev->my - ev->cy : 0);
		resize(delta);
		return true;	// not pass to childs!
	}
	else if(IS_WINDOW_MAKE_MIN_SIZE(ev)) {
		set_fenstergroesse( get_min_windowsize() ) ;
		resize( koord(0,0) ) ;
		return true;	// not pass to childs!
	}
	else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_CLOSE  ||  ev->ev_code==WIN_OPEN  ||  ev->ev_code==WIN_TOP)) {
		dirty = true;
		container.clear_dirty();
	}
	event_t ev2 = *ev;
	translate_event(&ev2, 0, -D_TITLEBAR_HEIGHT);
	return container.infowin_event(&ev2);
}



/**
 * resize window in response to a resize event
 * @author Markus Weber, Hj. Malthaner
 * @date 11-may-02
 */
void gui_frame_t::resize(const koord delta)
{
	koord size_change = delta;
	koord new_size = groesse + delta;

	// resize window to the minimal width
	if (new_size.x < min_windowsize.x) {
		size_change.x = min_windowsize.x - groesse.x;
		new_size.x = min_windowsize.x;
	}

	// resize window to the minimal heigth
	if (new_size.y < min_windowsize.y) {
		size_change.y = min_windowsize.y - groesse.y;
		new_size.y = min_windowsize.y;
	}

	// resize window
	set_fenstergroesse(new_size);

	// change drag start
	change_drag_start(size_change.x, size_change.y);
}


/**
 * komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void gui_frame_t::zeichnen(koord pos, koord gr)
{
	// ok, resized, move or draw for the first time
	if(dirty) {
		mark_rect_dirty_wc(pos.x,pos.y,pos.x+gr.x,pos.y+gr.y);
		dirty = false;
	}

	// draw background
	PUSH_CLIP(pos.x+1,pos.y+D_TITLEBAR_HEIGHT,gr.x-2,gr.y-D_TITLEBAR_HEIGHT);

	// Hajo: skinned windows code
	if(skinverwaltung_t::window_skin!=NULL) {
		const int img = skinverwaltung_t::window_skin->get_bild_nr(0);

		for(int j=0; j<gr.y; j+=64) {
			for(int i=0; i<gr.x; i+=64) {
				// the background will not trigger a redraw!
				display_color_img( img, pos.x+1 + i, pos.y+D_TITLEBAR_HEIGHT + j, 0, false, false );
			}
		}
	}
	else {
		// empty box
		display_fillbox_wh( pos.x+1, pos.y+D_TITLEBAR_HEIGHT, gr.x-2, gr.y-D_TITLEBAR_HEIGHT, MN_GREY1, false );
	}

	// Hajo: left, right
	display_vline_wh( pos.x, pos.y+D_TITLEBAR_HEIGHT, gr.y-D_TITLEBAR_HEIGHT, MN_GREY4, false );
	display_vline_wh( pos.x+gr.x-1, pos.y+D_TITLEBAR_HEIGHT, gr.y-D_TITLEBAR_HEIGHT, MN_GREY0, false );

	// Hajo: bottom line
	display_fillbox_wh( pos.x, pos.y+gr.y-1, gr.x, 1, MN_GREY0, false );

	// shadows

	container.zeichnen(pos);

	POP_CLIP();

// for shadows
//	display_blend_wh( pos.x+gr.x, pos.y+1, 2, gr.y, COL_BLACK, 50 );
//	display_blend_wh( pos.x+1, pos.y+gr.y, gr.x, 2, COL_BLACK, 50 );
}
