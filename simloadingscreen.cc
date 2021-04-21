/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "simloadingscreen.h"

#include "sys/simsys.h"
#include "descriptor/image.h"
#include "descriptor/skin_desc.h"
#include "simskin.h"
#include "display/simgraph.h"
#include "simevent.h"
#include "dataobj/environment.h"
#include "simticker.h"
#include "gui/simwin.h"
#include "gui/gui_theme.h"
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
	display_blend_wh_rgb(0, 0, display_get_width(), display_get_height(), color_idx_to_rgb(COL_BLACK), 50 );
	mark_screen_dirty();

	display_logo();
}


// show the logo if requested and there
void loadingscreen_t::display_logo()
{
	if(  show_logo  &&  skinverwaltung_t::biglogosymbol  ) {
		const image_t *image0 = skinverwaltung_t::biglogosymbol->get_image(0);
		const int w = image0->w;
		const int h = image0->h + image0->y;
		int x = display_get_width()/2-w;
		int y = display_get_height()/4-w;
		if(y<0) {
			y = 1;
		}

		display_color_img(skinverwaltung_t::biglogosymbol->get_image_id(0), x, y, 0, false, true);
		display_color_img(skinverwaltung_t::biglogosymbol->get_image_id(1), x+w, y, 0, false, true);
		display_color_img(skinverwaltung_t::biglogosymbol->get_image_id(2), x, y+h, 0, false, true);
		display_color_img(skinverwaltung_t::biglogosymbol->get_image_id(3), x+w, y+h, 0, false, true);
	}
}


// show everything but the logo
void loadingscreen_t::display()
{
	const int width = display_get_width();
	const int half_width = width>>1;
	const int quarter_width = width>>2;
	const int half_height = display_get_height()>>1;
	scr_coord_val const bar_height = max(LINESPACE + 10, 20);
	scr_coord_val const bar_y = half_height - bar_height / 2 + 1;
	scr_coord_val const bar_text_y = half_height - LINESPACE / 2 + 1;

	const int bar_len = max_progress>0 ? (int) ( ((double)progress*(double)half_width)/(double)max_progress ) : 0;

	if(  bar_len != last_bar_len  ) {
		last_bar_len = bar_len;

		dr_prepare_flush();

		if(  info  ) {
			display_proportional_rgb( half_width, bar_y - LINESPACE - 2, info, ALIGN_CENTER_H, color_idx_to_rgb(COL_WHITE), true );
		}

		// outline
		display_ddd_box_rgb( quarter_width-2, bar_y, half_width+4, bar_height, color_idx_to_rgb(COL_GREY6), color_idx_to_rgb(COL_GREY4), true );
		display_ddd_box_rgb( quarter_width-1, bar_y + 1, half_width+2, bar_height - 2, color_idx_to_rgb(COL_GREY4), color_idx_to_rgb(COL_GREY6), true );

		// inner
		display_fillbox_wh_rgb( quarter_width, bar_y + 2, half_width, bar_height - 4, SYSCOL_LOADINGBAR_INNER, true);

		// progress
		display_fillbox_wh_rgb( quarter_width, bar_y + 4, bar_len,  bar_height - 8, SYSCOL_LOADINGBAR_PROGRESS, true );

		if(  what  ) {
			display_proportional_rgb( half_width, bar_text_y, what, ALIGN_CENTER_H, SYSCOL_TEXT_HIGHLIGHT, false );
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
			simgraph_resize( ev->new_window_size );
			display_fillbox_wh_rgb( 0, 0, ev->mx, ev->my, color_idx_to_rgb(COL_BLACK), true );
			display_logo();
			// queue the event anyway, so the viewport is correctly updated on world resume (screen will be resized again).
			queued_events.append(ev);
		}
		else if(  ev->ev_code == SYSTEM_QUIT  ) {
			env_t::quit_simutrans = true;
			delete ev;
		}
		else {
			delete ev;
		}
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

	if (progress > max_progress) {
		dbg->error("loadingscreen_t::set_progress", "too much progress: actual = %d max = %d", progress, max_progress);
	}
}


loadingscreen_t::~loadingscreen_t()
{
	if(is_display_init()) {
		win_redraw_world();
		mark_screen_dirty();
		ticker::set_redraw_all(true);
	}
	while(  !queued_events.empty()  ) {
		queue_event( queued_events.remove_first() );
	}
}
