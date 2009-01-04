/*
 * Copyright (c) 1997 - 2004 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "../simcolor.h"
#include "../simimg.h"
#include "../simworld.h"
#include "../simskin.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simwerkz.h"
#include "../besch/skin_besch.h"
#include "../besch/sound_besch.h"
#include "werkzeug_waehler.h"



werkzeug_waehler_t::werkzeug_waehler_t(karte_t* welt, const char* titel, const char *helpfile, koord icon, bool allow_break) :
	tools(0), groesse(icon)
{
	this->allow_break = allow_break;
    this->welt = welt;
    this->titel  = titel;
    this->hilfe_datei = helpfile;
	this->icon = icon;
    dirty = true;
}


/**
 * Add a new tool with values and tooltip text.
 * w must be created by new werkzeug_t(copy_tool)!
 * @author Hj. Malthaner
 */
void werkzeug_waehler_t::add_werkzeug(werkzeug_t *w)
{
	if(w->get_icon(welt->get_active_player())==IMG_LEER  &&  w!=werkzeug_t::dummy) {
		return;
	}

	// only for non-empty icons ...
	tools.push_back(w);

	int ww = (display_get_width()/icon.x)-2;
	tool_icon_width = tools.get_count();
DBG_DEBUG("werkzeug_waehler_t::add_tool()","ww=%i, tool_icon_width=%i",ww,tool_icon_width);
	if(allow_break  &&  ww<tool_icon_width) {
		//break them
		int rows = (tool_icon_width/ww)+1;
DBG_DEBUG("werkzeug_waehler_t::add_tool()","ww=%i, rows=%i",ww,rows);
		// assure equal distribution if more than a single row is needed
		tool_icon_width = (tool_icon_width+rows-1)/rows;
	}
	dirty = true;
	groesse = koord( tool_icon_width*icon.x, ((tools.get_count()-1)/tool_icon_width) * icon.y + 16 + icon.y );

DBG_DEBUG("werkzeug_waehler_t::add_tool()", "at position %i (width %i)", tools.get_count(), tool_icon_width);
}


// reset the tools to empty state
void werkzeug_waehler_t::reset_tools()
{
	for(  int i=tools.get_count();  i>0;  ) {
		i--;
		tools.remove_at(i);
	}
	groesse = koord( icon.x, 16 );
	tool_icon_width = 0;
}


bool werkzeug_waehler_t::getroffen(int x, int y)
{
	int dx = x/icon.x;
	int	dy = (y-16)/icon.y;
	if(x>=0 && dx<tool_icon_width  &&  y>=0  &&  (y<16  ||  dy<tool_icon_width)) {
		return y < 16 || dx + tool_icon_width * dy < (int)tools.get_count();
	}
	return false;
}


void werkzeug_waehler_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTRELEASE(ev)) {
		// tooltips?
		const int x = (ev->mx) / icon.x;
		const int y = (ev->my-16) / icon.y;

		if(x>=0 && x<tool_icon_width  &&  y>=0) {
			const int wz_idx = x+(tool_icon_width*y);

			if (wz_idx < (int)tools.get_count()) {
				welt->set_werkzeug( tools[wz_idx] );
			}
			dirty = true;
		}
	}
	/* this resets to query-tool, when closing toolsbar ... */
	else if(ev->ev_class==INFOWIN &&  ev->ev_code==WIN_CLOSE) {
		welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE] );
	}
}


void werkzeug_waehler_t::zeichnen(koord pos, koord)
{
	spieler_t *sp = welt->get_active_player();
	for (uint i = 0; i < tools.get_count(); i++) {
		const image_id icon_img = tools[i]->get_icon(sp);

		const koord draw_pos=pos+koord((i%tool_icon_width)*icon.x,16+(i/tool_icon_width)*icon.y);
		if(icon_img == IMG_LEER) {
			// Hajo: no icon image available, draw a blank
			// DDD box as replacement

			// top
			display_fillbox_wh(draw_pos.x, draw_pos.y, icon.x, 1, MN_GREY4, dirty);
			// body
			display_fillbox_wh(draw_pos.x+1, draw_pos.y+1, icon.x-2, icon.y-2, MN_GREY2, dirty);
			// bottom
			display_fillbox_wh(draw_pos.x, draw_pos.y+icon.y-1, icon.x, 1, MN_GREY0, dirty);
			// Left
			display_fillbox_wh(draw_pos.x, draw_pos.y, 1, icon.y, MN_GREY4, dirty);
			// Right
			display_fillbox_wh(draw_pos.x+icon.x-1, draw_pos.y, 1, icon.y, MN_GREY0, dirty);
		}
		else {
			display_color_img(icon_img, draw_pos.x, draw_pos.y, 0, false, dirty);
			if(  tools[i]->is_selected(welt)  ) {
				display_img_blend( icon_img, draw_pos.x, draw_pos.y, TRANSPARENT50_FLAG|OUTLINE_FLAG|COL_BLACK, false, dirty);
			}
		}
	}

	// tooltips?
	const sint16 mx = get_maus_x();
	const sint16 my = get_maus_y();
	const sint16 xdiff = (mx - pos.x) / icon.x;
	const sint16 ydiff = (my - pos.y - 16) / icon.y;
	if(xdiff>=0  &&  xdiff<tool_icon_width  &&  ydiff>=0  &&  mx>=pos.x  &&  my>=pos.y+16) {
		const int tipnr = xdiff+(tool_icon_width*ydiff);
		if (tipnr < (int)tools.get_count()) {
			win_set_tooltip(get_maus_x() + 16, get_maus_y() - 16, tools[tipnr]->get_tooltip(welt->get_active_player()));
		}
	}

	dirty = false;
}
