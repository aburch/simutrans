/*
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


static bool hidden = true;
static const char *copyright = NULL;
static const char *label = NULL;
static unsigned int level;
static unsigned int level_max;
static slist_tpl<event_t *> queued_events;

void loadingscreen::bootstrap()
{
	hidden = true;
	label = NULL;
	copyright = NULL;
}


void loadingscreen::show_logo()
{
	if( hidden  ||  !skinverwaltung_t::biglogosymbol ) {
		return;
	}

	const bild_t *bild0 = skinverwaltung_t::biglogosymbol->get_bild(0)->get_pic();
	const int w = bild0->w;
	const int h = bild0->h + bild0->y;
	int x = display_get_width()/2-w;
	int y = display_get_height()/4-w;
	if(y<0) {
		y = 1;
	}
	dr_prepare_flush();
	display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(0), x, y, 0, false, true);
	display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(1), x+w, y, 0, false, true);
	display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(2), x, y+h, 0, false, true);
	display_color_img(skinverwaltung_t::biglogosymbol->get_bild_nr(3), x+w, y+h, 0, false, true);
#if 0
	display_free_all_images_above( skinverwaltung_t::biglogosymbol->get_bild_nr(0) );
#endif
	dr_flush();
}


void loadingscreen::set_copyright(const char *message)
{
	copyright = message;
}


void loadingscreen::set_label(const char *message){
	label = message;
}


void loadingscreen::show_copyright()
{
	if( hidden  ||  !copyright ) {
		return;
	}

	display_proportional(display_get_width() / 2, display_get_height() / 2 - 8 - LINESPACE - 4, copyright, ALIGN_MIDDLE, COL_WHITE, true);
}


void loadingscreen::shadow_screen()
{
	display_blend_wh(0, 0, display_get_width(), display_get_height(), COL_BLACK, 32 );
	mark_screen_dirty();
}


void loadingscreen::handle_events()
{
	event_t *ev = new event_t;

	display_poll_event(ev);
	if(ev->ev_class==EVENT_SYSTEM) {
		if (ev->ev_code==SYSTEM_RESIZE) {
			// main window resized
			simgraph_resize( ev->mx, ev->my );
			dr_prepare_flush();
			display_fillbox_wh( 0, 0, ev->mx, ev->my, COL_BLACK, true );
			dr_flush();

			//redraw loading screen

			hidden=true;
			set_progress(level, level_max);
		}
		else if (ev->ev_code == SYSTEM_QUIT) {
			umgebung_t::quit_simutrans = true;
		}
		delete ev;
	}
	else {
		if ( ev->ev_class == EVENT_KEYBOARD ) {
			queued_events.append(ev);
		}
		else {
			delete ev;
		}
	}
}


void loadingscreen::set_progress(const unsigned int level_in,const unsigned int max_in)
{

	level = level_in;
	level_max = max_in;

	if(hidden) {
		show();
	}

	if(hidden) {
		//this means display is not initialized. Exit.
		return;
	}

	handle_events();

	const int width = display_get_width();
	const int half_width = width>>1;
	const int quarter_width = width>>2;
	const int half_height = display_get_height()>>1;

	const int part = (level*half_width)/max_in;

	dr_prepare_flush();

	// outline
	display_ddd_box(quarter_width-2, half_height-9, half_width+4, 20, COL_GREY6, COL_GREY4);
	display_ddd_box(quarter_width-1, half_height-8, half_width+2, 18, COL_GREY4, COL_GREY6);

	// inner
	display_fillbox_wh(quarter_width, half_height - 7, half_width, 16, COL_GREY5, true);

	// progress
	display_fillbox_wh(quarter_width, half_height - 5, part,  12, COL_BLUE,  true);

	if(label) {
		display_proportional(half_width, half_height-4,label,ALIGN_MIDDLE,COL_WHITE,0);
	}
	dr_flush();

}


void loadingscreen::show()
{
	if (!is_display_init()) {
		hidden=true;
		return;
	}

	if(!hidden) {
		return;
	}

	hidden = false;
	shadow_screen();
	show_logo();
	show_copyright();
}


void loadingscreen::hide()
{
	hidden = true;
	if (is_display_init()) {
		mark_screen_dirty();
	}
	ticker::set_redraw_all(true);
	queue_events(queued_events);
}
