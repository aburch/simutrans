/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where factory stats are calculated for list dialog
 */

#include "factorylist_stats_t.h"

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simfab.h"
#include "../simworld.h"
#include "../simskin.h"
#include "simwin.h"

#include "gui_frame.h"
#include "components/gui_button.h"

#include "../bauer/goods_manager.h"
#include "../descriptor/skin_desc.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"
#include <algorithm>


scr_coord_val factorylist_stats_t::h = LINESPACE;
int factorylist_stats_t::sort_mode = factorylist::by_name;
bool factorylist_stats_t::reverse = false;



factorylist_stats_t::factorylist_stats_t(fabrik_t *fab)
{
	this->fab = fab;
	h = max( gui_theme_t::gui_pos_button_size.h, LINESPACE );
	mouse_over = false;
}



bool factorylist_stats_t::compare( gui_scrolled_list_t::scrollitem_t *aa, gui_scrolled_list_t::scrollitem_t *bb )
{
	factorylist_stats_t* fa = dynamic_cast<factorylist_stats_t*>(aa);
	factorylist_stats_t* fb = dynamic_cast<factorylist_stats_t*>(bb);
	// good luck with mixed lists
	assert(fa != NULL  &&  fb != NULL);
	fabrik_t *a=fa->fab, *b=fb->fab;

	int cmp;
	switch (sort_mode) {
		default:
		case factorylist::by_name:
			cmp = 0;
			break;

		case factorylist::by_input:
		{
			int a_in = a->get_input().empty() ? -1 : (int)a->get_total_in();
			int b_in = b->get_input().empty() ? -1 : (int)b->get_total_in();
			cmp = a_in - b_in;
			break;
		}

		case factorylist::by_transit:
		{
			int a_transit = a->get_input().empty() ? -1 : (int)a->get_total_transit();
			int b_transit = b->get_input().empty() ? -1 : (int)b->get_total_transit();
			cmp = a_transit - b_transit;
			break;
		}

		case factorylist::by_available:
		{
			int a_in = a->get_input().empty() ? -1 : (int)(a->get_total_in()+a->get_total_transit());
			int b_in = b->get_input().empty() ? -1 : (int)(b->get_total_in()+b->get_total_transit());
			cmp = a_in - b_in;
			break;
		}

		case factorylist::by_output:
		{
			int a_out = a->get_output().empty() ? -1 : (int)a->get_total_out();
			int b_out = b->get_output().empty() ? -1 : (int)b->get_total_out();
			cmp = a_out - b_out;
			break;
		}

		case factorylist::by_maxprod:
			cmp = a->get_base_production()*a->get_prodfactor() - b->get_base_production()*b->get_prodfactor();
			break;

		case factorylist::by_status:
			cmp = a->get_status() - b->get_status();
			break;

		case factorylist::by_power:
			cmp = a->get_prodfactor_electric() - b->get_prodfactor_electric();
			break;
	}
	if (cmp == 0) {
		cmp = STRICMP(a->get_name(), b->get_name());
	}
	return reverse ? cmp > 0 : cmp < 0;
}


bool factorylist_stats_t::sort( vector_tpl<scrollitem_t *>&v, int offset, void * ) const
{
	vector_tpl<scrollitem_t *>::iterator start = v.begin();
	while(  offset-->0  ) {
		++start;
	}
	std::sort( start, v.end(), factorylist_stats_t::compare );
	return true;
}




bool factorylist_stats_t::infowin_event(const event_t * ev)
{
	mouse_over = false;
	if(  ev->ev_class!=EVENT_KEYBOARD  ) {
		// find out, if in pos box
		scr_rect pos_xywh( scr_coord(D_H_SPACE, (h-gui_theme_t::gui_pos_button_size.h)/2), gui_theme_t::gui_pos_button_size );
		bool pos_box_hit = pos_xywh.contains( scr_coord(ev->mx,ev->my) );
		if(  (IS_LEFTRELEASE(ev)  &&  pos_box_hit)  ||  IS_RIGHTRELEASE(ev)  ) {
			world()->get_viewport()->change_world_position( fab->get_pos() );
			mouse_over = true;
		}
		else if(  IS_LEFTRELEASE(ev)  ) {
			fab->open_info_window();
		}
		else {
			mouse_over = (ev->button_state & 3)  &&  pos_box_hit;
		}
	}

	// never notify parent
	return false;
}



scr_coord_val factorylist_stats_t::draw( scr_coord pos, scr_coord_val width, bool selected, bool )
{
	PIXVAL indikatorfarbe = color_idx_to_rgb(fabrik_t::status_to_color[fab->get_status()]);

	cbuffer_t buf;
	buf.append(fab->get_name());
	buf.append(" (");

	if (!fab->get_input().empty()) {
		buf.printf( "%i+%i", fab->get_total_in(), fab->get_total_transit() );
	}
	else {
		buf.append("-");
	}
	buf.append(", ");

	if (!fab->get_output().empty()) {
		buf.append(fab->get_total_out(),0);
	}
	else {
		buf.append("-");
	}
	buf.append(", ");

	buf.append(fab->get_current_production(),0);
	buf.append(") ");

	// goto button
	bool info_open = win_get_magic( (ptrdiff_t)fab );
	scr_rect pos_xywh( scr_coord(pos.x+D_H_SPACE, pos.y+(h-gui_theme_t::gui_pos_button_size.h)/2), gui_theme_t::gui_pos_button_size );
	if(  selected  ||  mouse_over  ||  info_open  ) {
		// still on center?
		selected = world()->get_viewport()->is_on_center( fab->get_pos() );
		if(  mouse_over  ) {
			// still+pressed? (release will be extra event)
			scr_coord_val mx = get_mouse_x(), my = get_mouse_y();
			mouse_over = mx>=pos.x  &&  mx<pos.x+width  &&  my>=pos.y  &&  my<pos.y+h;
			selected |= mouse_over;
		}
	}
	display_img_aligned( gui_theme_t::pos_button_img[ selected ], pos_xywh, ALIGN_CENTER_V | ALIGN_CENTER_H, true );

	int xpos = pos.x + D_MARGIN_LEFT + gui_theme_t::gui_pos_button_size.w + D_H_SPACE;
	display_fillbox_wh_clip_rgb( xpos, pos.y+(h-D_INDICATOR_HEIGHT)/2, D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT, indikatorfarbe, true);

	xpos += D_INDICATOR_WIDTH+D_H_SPACE;
	if(  fab->get_prodfactor_electric()>0  &&  skinverwaltung_t::electricity->get_image(0)  ) {
		display_color_img( skinverwaltung_t::electricity->get_image_id(0), xpos, pos.x+(h-skinverwaltung_t::electricity->get_image(0)->get_pic()->h)/2, 0, false, true);
	}
	xpos += 8+D_H_SPACE;
	if(  fab->get_prodfactor_pax()>0  &&  skinverwaltung_t::passengers->get_image(0)  ) {
		display_color_img( skinverwaltung_t::passengers->get_image_id(0), xpos, pos.x+(h-skinverwaltung_t::passengers->get_image(0)->get_pic()->h)/2, 0, false, true);
	}
	xpos += 8+D_H_SPACE;
	if(  fab->get_prodfactor_mail()>0  &&  skinverwaltung_t::mail->get_image(0)  ) {
		display_color_img( skinverwaltung_t::mail->get_image_id(0), xpos, pos.x+(h-skinverwaltung_t::mail->get_image(0)->get_pic()->h)/2, 0, false, true);
	}
	xpos += 8+D_H_SPACE;
	display_proportional_clip_rgb( xpos, pos.y+(h-LINESPACE)/2, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

	if(  info_open  ) {
		display_blend_wh_rgb( pos.x, pos.y, width, h, color_idx_to_rgb(COL_BLACK), 25 );
	}
	return true;
}
