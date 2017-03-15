/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Where the citylist status are calculated (for graphs and statistics)
 */

#include "citylist_stats_t.h"
#include "city_info.h"

#include "../simcity.h"
#include "../simcolor.h"
#include "../simevent.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../gui/simwin.h"
#include "../simworld.h"

#include "../descriptor/skin_desc.h"
#include <algorithm>

#include "../dataobj/translator.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"


scr_coord_val citylist_stats_t::h = LINESPACE;

citylist_stats_t::citylist_stats_t(stadt_t *c)
{
	city = c;
	h = max( gui_theme_t::gui_pos_button_size.h, LINESPACE );
	mouse_over = false;
}


scr_coord_val citylist_stats_t::draw( scr_coord pos, scr_coord_val width, bool selected, bool )
{
	sint32 bev = city->get_einwohner();
	sint32 growth = city->get_wachstum();

	cbuffer_t buf;
	buf.printf( "%s: ", city->get_name() );
	buf.append( bev, 0 );
	buf.append( " (" );
	buf.append( growth/10.0, 1 );
	buf.append( ")" );
	display_proportional_clip_rgb( pos.x+D_H_SPACE*2 + gui_theme_t::gui_pos_button_size.w, pos.y+(h-LINESPACE)/2, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

	// goto button
	bool info_open = win_get_magic( (ptrdiff_t)city );
	scr_rect pos_xywh( scr_coord(pos.x+D_H_SPACE, pos.y+(h-gui_theme_t::gui_pos_button_size.h)/2), gui_theme_t::gui_pos_button_size );
	if(  selected  ||  mouse_over  ||  info_open  ) {
		// still on center?
		if(  grund_t *gr = world()->lookup_kartenboden( city->get_center() )  ) {
			selected = world()->get_viewport()->is_on_center( gr->get_pos() );
		}
		if(  mouse_over  ) {
			// still+pressed? (release will be extra event)
			scr_coord_val mx = get_mouse_x(), my = get_mouse_y();
			mouse_over = mx>=pos.x  &&  mx<pos.x+width  &&  my>=pos.y  &&  my<pos.y+h;
			selected |= mouse_over;
		}
	}
	display_img_aligned( gui_theme_t::pos_button_img[ selected ], pos_xywh, ALIGN_CENTER_V | ALIGN_CENTER_H, true );

	if(  info_open  ) {
		display_blend_wh_rgb( pos.x, pos.y, width, h, color_idx_to_rgb(COL_BLACK), 25 );
	}
	return true;
}



bool citylist_stats_t::infowin_event(const event_t *ev)
{
	mouse_over = false;
	if(  ev->ev_class!=EVENT_KEYBOARD  ) {
		// find out, if in pos box
		scr_rect pos_xywh( scr_coord(D_H_SPACE, (h-gui_theme_t::gui_pos_button_size.h)/2), gui_theme_t::gui_pos_button_size );
		bool pos_box_hit = pos_xywh.contains( scr_coord(ev->mx,ev->my) );
		if(  (IS_LEFTRELEASE(ev)  &&  pos_box_hit)  ||  IS_RIGHTRELEASE(ev)  ) {
			if(  grund_t *gr = world()->lookup_kartenboden( city->get_center() )  ) {
				world()->get_viewport()->change_world_position( gr->get_pos() );
			}
		}
		else if(  IS_LEFTRELEASE(ev)  ) {
			city->open_info_window();
		}
		else {
			mouse_over = (ev->button_state &1)  &&  pos_box_hit;
		}
	}

	// never notify parent
	return false;
}



citylist_stats_t::sort_mode_t citylist_stats_t::sort_mode = citylist_stats_t::SORT_BY_NAME;


bool citylist_stats_t::compare( gui_scrolled_list_t::scrollitem_t *aa, gui_scrolled_list_t::scrollitem_t *bb )
{
	bool reverse = citylist_stats_t::sort_mode > citylist_stats_t::SORT_MODES;
	int sort_mode = citylist_stats_t::sort_mode & 0x1F;

	citylist_stats_t* a = dynamic_cast<citylist_stats_t*>(aa);
	citylist_stats_t* b = dynamic_cast<citylist_stats_t*>(bb);
	// good luck with mixed lists
	assert(a != NULL  &&  b != NULL);

	if(  reverse  ) {
		citylist_stats_t *temp = a;
		a = b;
		b = temp;
	}

	if(  sort_mode != SORT_BY_NAME  ) {
		switch(  sort_mode  ) {
			case SORT_BY_NAME:	// default
				break;
			case SORT_BY_SIZE:
				return a->city->get_einwohner() < b->city->get_einwohner();
			case SORT_BY_GROWTH:
				return a->city->get_wachstum() < b->city->get_wachstum();
			default: break;
		}
		// default sorting ...
	}

	// first: try to sort by number
	const char *atxt =a->get_text();
	int aint = 0;
	// isdigit produces with UTF8 assertions ...
	if(  atxt[0]>='0'  &&  atxt[0]<='9'  ) {
		aint = atoi( atxt );
	}
	else if(  atxt[0]=='('  &&  atxt[1]>='0'  &&  atxt[1]<='9'  ) {
		aint = atoi( atxt+1 );
	}
	const char *btxt = b->get_text();
	int bint = 0;
	if(  btxt[0]>='0'  &&  btxt[0]<='9'  ) {
		bint = atoi( btxt );
	}
	else if(  btxt[0]=='('  &&  btxt[1]>='0'  &&  btxt[1]<='9'  ) {
		bint = atoi( btxt+1 );
	}
	if(  aint!=bint  ) {
		return (aint-bint)<0;
	}
	// otherwise: sort by name
	return strcmp(atxt, btxt)<0;
}


bool citylist_stats_t::sort( vector_tpl<scrollitem_t *>&v, int offset, void * ) const
{
	vector_tpl<scrollitem_t *>::iterator start = v.begin();
	while(  offset-->0  ) {
		++start;
	}
	std::sort( start, v.end(), citylist_stats_t::compare );
	return true;
}
