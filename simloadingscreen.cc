/*//
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic license.
 * (see license.txt)
 */

#include "simloadingscreen.h"

#include "simsys.h"
#include "besch/bild_besch.h"
#include "besch/skin_besch.h"
#include "simskin.h"
#include "simgraph.h"
#include "simevent.h"
#include "dataobj/umgebung.h"
#include "simticker.h"
#include "tpl/slist_tpl.h"


loadingscreen_t::loadingscreen_t( const char *w, uint32 max_p, bool logo, bool continueflag )
{
	what = w;
	info = NULL;
	progress = 0;
	max_progress = max_p;
	last_bar_len = -1;
	show_logo = logo;

	if(  !is_display_init()  ||  continueflag  ) {
		return;
	}

	// darkens the current screen
	display_blend_wh(0, 0, display_get_width(), display_get_height(), COL_BLACK, 50 );
	mark_screen_dirty();

	display_logo();
}


// show the logo if requested and there
void loadingscreen_t::display_logo()
{
	if(  show_logo  &&  skinverwaltung_t::biglogosymbol  ) {
		const bild_t *bild0 = skinverwaltung_t::biglogosymbol->get_bild(0)->get_pic();
		const int w = bild0->w;
		const int h = bild0->h + bild0->y;
		int x = display_get_width()/2-w;
		int y = display_get_height()/4-w;
		if(y<0) {
			y = 1;
		}

		display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(0), x, y, 0, false, true);
		display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(1), x+w, y, 0, false, true);
		display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(2), x, y+h, 0, false, true);
		display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(3), x+w, y+h, 0, false, true);
	}
}


// show everything but the logo
void loadingscreen_t::display()
{
	const int width = display_get_width();
	const int half_width = width>>1;
	const int quarter_width = width>>2;
	const int half_height = display_get_height()>>1;

	const int bar_len = (progress*(uint32)half_width)/max_progress;

	if(  bar_len != last_bar_len  ) {
		last_bar_len = bar_len;

		dr_prepare_flush();

		if(  info  ) {
			display_proportional( half_width, half_height - 8 - LINESPACE - 4, info, ALIGN_MIDDLE, COL_WHITE, true );
		}

		// outline
		display_ddd_box( quarter_width-2, half_height-9, half_width+4, 20, COL_GREY6, COL_GREY4, true );
		display_ddd_box( quarter_width-1, half_height-8, half_width+2, 18, COL_GREY4, COL_GREY6, true );

		// inner
		display_fillbox_wh( quarter_width, half_height - 7, half_width, 16, COL_GREY5, true);

		// progress
		display_fillbox_wh( quarter_width, half_height - 5, bar_len,  12, COL_BLUE, true );

		if(  what  ) {
			display_proportional( half_width, half_height-4, what, ALIGN_MIDDLE, COL_WHITE, false );
		}

		dr_flush();
	}
}


void loadingscreen_t::set_progress( uint32 progress )
{
	if(!is_display_init()  &&  (progress != this->progress  ||  progress == 0)  ) {
		return;
	}

	// first: handle events
	event_t *ev = new event_t;

	display_poll_event(ev);
	if(  ev->ev_class == EVENT_SYSTEM  ) {
		if(  ev->ev_code == SYSTEM_RESIZE  ) {
			// main window resized
			simgraph_resize( ev->mx, ev->my );
			display_fillbox_wh( 0, 0, ev->mx, ev->my, COL_BLACK, true );
			display_logo();
		}
		else if(  ev->ev_code == SYSTEM_QUIT  ) {
			umgebung_t::quit_simutrans = true;
		}
		delete ev;
	}
	else {
		if(  ev->ev_class == EVENT_KEYBOARD  ) {
			queued_events.append(ev);
		}
		else {
			delete ev;
		}
	}

	this->progress = progress;
	display();
}


loadingscreen_t::~loadingscreen_t()
{
	if(is_display_init()) {
		mark_screen_dirty();
		ticker::set_redraw_all(true);
	}
	while(  !queued_events.empty()  ) {
		queue_event( queued_events.remove_first() );
	}
}
