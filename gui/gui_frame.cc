/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * The window frame all dialogs are based
 * [Mathew Hounsell] Min Size Button On Map Window 20030313
 */

#include <stdio.h>

#include "gui_frame.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../gui/simwin.h"
#include "../simworld.h"
#include "../player/simplay.h"

#include "../descriptor/reader/obj_reader.h"
#include "../descriptor/skin_desc.h"
#include "../simskin.h"

floating_cursor_t::floating_cursor_t(const scr_coord& initial,	scr_coord_val min_left,	scr_coord_val max_right)
	: cursor(initial)
	, left(min_left)
	, right(max_right)
	, row_height(0) 
{}

void floating_cursor_t::new_line()
{
	cursor.x = left;
	cursor.y += row_height + D_V_SPACE;
	row_height = 0;
}

scr_coord floating_cursor_t::next_pos(const scr_size& size)
{
	if (cursor.x + size.w > right)
	{
		new_line();
	}
	scr_coord curr = cursor;
	cursor.x += size.w + D_H_SPACE;
	if (row_height < size.h)
		row_height = size.h;
	return curr;
}


//// default button sizes
//KOORD_VAL gui_frame_t::gui_button_width = 92;
//KOORD_VAL gui_frame_t::gui_button_height = 14;
//
//// default titlebar height
//KOORD_VAL gui_frame_t::gui_titlebar_height = 16;
//
//// dialog borders
//KOORD_VAL gui_frame_t::gui_frame_left = 10;
//KOORD_VAL gui_frame_t::gui_frame_top = 10;
//KOORD_VAL gui_frame_t::gui_frame_right = 10;
//KOORD_VAL gui_frame_t::gui_frame_bottom = 10;
//
//// space between two elements
//KOORD_VAL gui_frame_t::gui_hspace = 4;
//KOORD_VAL gui_frame_t::gui_vspace = 4;
//
//// size of status indicator elements (colored boxes in factories, station and others)
//KOORD_VAL gui_frame_t::gui_indicator_width = 20;
//KOORD_VAL gui_frame_t::gui_indicator_height = 4;
//

karte_ptr_t gui_frame_t::welt;


// Insert the container
gui_frame_t::gui_frame_t(char const* const name, player_t const* const player)
{
	this->name = name;
	size = scr_size(200, 100);
	min_windowsize = scr_size(0,0);
	owner = player;
	container.set_pos(scr_coord(0,D_TITLEBAR_HEIGHT));
	set_resizemode(no_resize);  //25-may-02  markus weber  added
	opaque = true;
	dirty = true;
}


/**
 * Set the window size
 * @author Hj. Malthaner
 */
void gui_frame_t::set_windowsize(scr_size size)
{
	if(  size != this->size  ) {
		// mark old size dirty
		scr_coord const& pos = win_get_pos(this);
		mark_rect_dirty_wc( pos.x, pos.y, pos.x+this->size.w, pos.y+this->size.h );

		// minimum size //25-may-02  markus weber  added
		size.clip_lefttop(min_windowsize);

		this->size = size;
		dirty = true;
	}
}


/**
 * get color information for the window title
 * -borders and -body background
 * @author Hj. Malthaner
 */
PLAYER_COLOR_VAL gui_frame_t::get_titlecolor() const
{
	return owner ? PLAYER_FLAG|(owner->get_player_color1()+1) : WIN_TITLE;
}


/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 * @author Hj. Malthaner
 */
bool gui_frame_t::infowin_event(const event_t *ev)
{
	// %DB0 printf( "\nMessage: gui_frame_t::infowin_event( event_t const * ev ) : Fenster|Window %p : Event is %d", (void*)this, ev->ev_class );
	if(IS_WINDOW_RESIZE(ev)) {
		scr_coord delta (  resize_mode & horizonal_resize ? ev->mx - ev->cx : 0,
		               resize_mode & vertical_resize  ? ev->my - ev->cy : 0);
		resize(delta);
		return true;  // don't pass to children!
	}
	else if(IS_WINDOW_MAKE_MIN_SIZE(ev)) {
		set_windowsize( get_min_windowsize() ) ;
		resize( scr_coord(0,0) ) ;
		return true;  // don't pass to children!
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
void gui_frame_t::resize(const scr_coord delta)
{
	dirty = true;
	scr_size new_size = size + delta;

	// resize window to the minimum size
	new_size.clip_lefttop(min_windowsize);

	scr_coord size_change = new_size - size;

	// resize window
	set_windowsize(new_size);

	// change drag start
	change_drag_start(size_change.x, size_change.y);
}


/**
 * Draw new component. The values to be passed refer to the window
 * i.e. It's the screen coordinates of the window where the
 * component is displayed.
 *
 * @author Hj. Malthaner
 */
void gui_frame_t::draw(scr_coord pos, scr_size size)
{
	// draw background
	if(  opaque  ) {
		display_img_stretch( gui_theme_t::windowback, scr_rect( pos + scr_coord(0,D_TITLEBAR_HEIGHT), size - scr_size(0,D_TITLEBAR_HEIGHT) ) );
		if(  dirty  ) {
			mark_rect_dirty_wc(pos.x, pos.y, pos.x + size.w, pos.y + D_TITLEBAR_HEIGHT );
		}
	}
	else {
		if(  dirty  ) {
			mark_rect_dirty_wc(pos.x, pos.y, pos.x + size.w, pos.y + size.h + D_TITLEBAR_HEIGHT );
		}
		display_blend_wh( pos.x+1, pos.y+D_TITLEBAR_HEIGHT, size.w-2, size.h-D_TITLEBAR_HEIGHT, color_transparent, percent_transparent );
	}
	dirty = false;

	PUSH_CLIP(pos.x+1, pos.y+D_TITLEBAR_HEIGHT, size.w-2, size.h-D_TITLEBAR_HEIGHT);
	container.draw(pos);
	POP_CLIP();

	// for shadows of the windows
	if(  gui_theme_t::gui_drop_shadows  ) {
		display_blend_wh( pos.x+size.w, pos.y+1, 2, size.h, COL_BLACK, 50 );
		display_blend_wh( pos.x+1, pos.y+size.h, size.w, 2, COL_BLACK, 50 );
	}
}

void  gui_frame_t::set_name(const char *name)
{
	this->name = translator::translate(name); 
}
