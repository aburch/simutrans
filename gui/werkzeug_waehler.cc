/*
 * Copyright (c) 1997 - 2004 Hj. Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * This class defines all toolbar dialogues, i.e. the part the user will see
 */

#include "../simimg.h"
#include "../simworld.h"
#include "../simwin.h"
#include "../simgraph.h"
#include "../simmenu.h"
#include "../dataobj/umgebung.h"
#include "../utils/for.h"
#include "werkzeug_waehler.h"

#define MIN_WIDTH (80)


werkzeug_waehler_t::werkzeug_waehler_t(karte_t* welt, const char* titel, const char *helpfile, uint32 toolbar_id, koord icon, bool allow_break) :
	gui_frame_t( translator::translate(titel) ), tools(0)
{
	this->toolbar_id = toolbar_id;
	this->allow_break = allow_break;
	this->welt = welt;
	this->hilfe_datei = helpfile;
	this->icon = icon;
	this->tool_icon_disp_start = 0;
	this->tool_icon_disp_end = 0;
	this->titel = titel;
	has_prev_next= false;
	set_fenstergroesse( koord(max(icon.x,MIN_WIDTH), D_TITLEBAR_HEIGHT) );
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
	tools.append(w);

	int ww = max(2,(display_get_width()/icon.x)-2);	// to avoid zero or negative ww on posix (no graphic) backends
	tool_icon_width = tools.get_count();
DBG_DEBUG4("werkzeug_waehler_t::add_tool()","ww=%i, tool_icon_width=%i",ww,tool_icon_width);
	if(allow_break  &&  (ww<tool_icon_width  ||  (umgebung_t::toolbar_max_width>0  &&  umgebung_t::toolbar_max_width<tool_icon_width))) {
		//break them
		int rows = (tool_icon_width/ww)+1;
DBG_DEBUG4("werkzeug_waehler_t::add_tool()","ww=%i, rows=%i",ww,rows);
		// assure equal distribution if more than a single row is needed
		tool_icon_width = (tool_icon_width+rows-1)/rows;
		if(  umgebung_t::toolbar_max_width > 0  ) {
			// At least, 3 rows is needed to drag toolbar
			tool_icon_width = min( tool_icon_width, max(umgebung_t::toolbar_max_width, 3) );
		}
	}
	tool_icon_height = max( (display_get_height()/icon.y)-3, 1 );
	if(  umgebung_t::toolbar_max_height > 0  ) {
		tool_icon_height = min(tool_icon_height, umgebung_t::toolbar_max_height);
	}
	dirty = true;
	set_fenstergroesse( koord( tool_icon_width*icon.x, min(tool_icon_height, ((tools.get_count()-1)/tool_icon_width)+1)*icon.y+D_TITLEBAR_HEIGHT ) );
	tool_icon_disp_start = 0;
	tool_icon_disp_end = min( tool_icon_disp_start+tool_icon_width*tool_icon_height, tools.get_count() );
	has_prev_next = ((uint32)tool_icon_width*tool_icon_height < tools.get_count());

DBG_DEBUG4("werkzeug_waehler_t::add_tool()", "at position %i (width %i)", tools.get_count(), tool_icon_width);
}


// reset the tools to empty state
void werkzeug_waehler_t::reset_tools()
{
	tools.clear();
	gui_frame_t::set_fenstergroesse( koord(max(icon.x,MIN_WIDTH), D_TITLEBAR_HEIGHT) );
	tool_icon_width = 0;
	tool_icon_disp_start = 0;
	tool_icon_disp_end = 0;
}


bool werkzeug_waehler_t::getroffen(int x, int y)
{
	int dx = x/icon.x;
	int dy = (y-16)/icon.y;
	// either click in titlebar or on an icon
	if(  x>=0   &&  y>=0  &&  ( (y<D_TITLEBAR_HEIGHT  &&  x<get_fenstergroesse().x)  ||  (dx<tool_icon_width  &&  dy<tool_icon_height) )  ) {
		return y < D_TITLEBAR_HEIGHT || dx + tool_icon_width * dy + tool_icon_disp_start < (int)tools.get_count();
	}
	return false;
}


bool werkzeug_waehler_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTRELEASE(ev)  ||  IS_RIGHTRELEASE(ev)) {
		// tooltips?
		const int x = (ev->mx) / icon.x;
		const int y = (ev->my-16) / icon.y;

		if(x>=0 && x<tool_icon_width  &&  y>=0) {
			const int wz_idx = x+(tool_icon_width*y)+tool_icon_disp_start;

			if (wz_idx < (int)tools.get_count()) {
				// change tool
				werkzeug_t *tool = tools[wz_idx].tool;
				if(IS_LEFTRELEASE(ev)) {
					welt->set_werkzeug( tool, welt->get_active_player() );
				}
				else {
					// right-click on toolbar icon closes toolbars and dialogues. Resets selectable simple and general tools to the query-tool
					if (tool  &&  tool->is_selected(welt)  ) {
						// ->exit triggers werkzeug_waehler_t::infowin_event in the closing toolbar,
						// which resets active tool to query tool
						if(  tool->exit(welt, welt->get_active_player())  ) {
							welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], welt->get_active_player() );
						}
					}
				}
			}
			return true;
		}
	}
	// this resets to query-tool, when closing toolsbar - but only for selected general tools in the closing toolbar
	else if(ev->ev_class==INFOWIN &&  ev->ev_code==WIN_CLOSE) {
		FOR(vector_tpl<tool_data_t>, const i, tools) {
			if (i.tool->is_selected(welt) && i.tool->get_id() & GENERAL_TOOL) {
				welt->set_werkzeug( werkzeug_t::general_tool[WKZ_ABFRAGE], welt->get_active_player() );
				break;
			}
		}
	}
	// reset title, language may have changed
	else if(ev->ev_class==INFOWIN  &&  (ev->ev_code==WIN_TOP  ||  ev->ev_code==WIN_OPEN) ) {
		set_name( translator::translate(titel) );
	}
	if(IS_WINDOW_CHOOSE_NEXT(ev)) {
		if(ev->ev_code==NEXT_WINDOW) {
			tool_icon_disp_start = (tool_icon_disp_start+tool_icon_width*tool_icon_height >= (int)tools.get_count()) ? tool_icon_disp_start : tool_icon_disp_start+tool_icon_width*tool_icon_height;
		}
		else {
			tool_icon_disp_start = (tool_icon_disp_start-tool_icon_width*tool_icon_height < 0) ? 0 : tool_icon_disp_start-tool_icon_width*tool_icon_height;
		}

		int xy = tool_icon_width*tool_icon_height;
		tool_icon_disp_end = min(tool_icon_disp_start+xy, tools.get_count());

		set_fenstergroesse( koord( tool_icon_width*icon.x, min(tool_icon_height, ((tools.get_count()-1)/tool_icon_width)+1)*icon.y+D_TITLEBAR_HEIGHT ) );
		dirty = true;
	}
	return false;
}


void werkzeug_waehler_t::zeichnen(koord pos, koord)
{
	spieler_t *sp = welt->get_active_player();
	for(  uint i = tool_icon_disp_start;  i < tool_icon_disp_end;  i++  ) {
		const image_id icon_img = tools[i].tool->get_icon(sp);

		const koord draw_pos=pos+koord(((i-tool_icon_disp_start)%tool_icon_width)*icon.x,16+((i-tool_icon_disp_start)/tool_icon_width)*icon.y);
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
			bool tool_dirty = dirty  ||  tools[i].tool->is_selected(welt) ^ tools[i].selected;
			display_color_img(icon_img, draw_pos.x, draw_pos.y, 0, false, tool_dirty);
			tools[i].tool->draw_after( welt, draw_pos, tool_dirty);
			// store whether tool was selected
			tools[i].selected = tools[i].tool->is_selected(welt);
		}
	}
	if (dirty  &&  (tool_icon_disp_end-tool_icon_disp_start < tool_icon_width*tool_icon_height) ) {
		// mark empty space empty
		mark_rect_dirty_wc(pos.x, pos.y, pos.x + tool_icon_width*icon.x, pos.y + tool_icon_height*icon.y);
	}

	// tooltips?
	const sint16 mx = get_maus_x();
	const sint16 my = get_maus_y();
	const sint16 xdiff = (mx - pos.x) / icon.x;
	const sint16 ydiff = (my - pos.y - 16) / icon.y;
	if(xdiff>=0  &&  xdiff<tool_icon_width  &&  ydiff>=0  &&  mx>=pos.x  &&  my>=pos.y+16) {
		const int tipnr = xdiff+(tool_icon_width*ydiff)+tool_icon_disp_start;
		if (tipnr < (int)tool_icon_disp_end) {
			win_set_tooltip(get_maus_x() + 16, pos.y + 16 + ((ydiff+1)*icon.y) + 12, tools[tipnr].tool->get_tooltip(welt->get_active_player()), tools[tipnr].tool, this);
		}
	}

	dirty = false;
	//as we do not call gui_frame_t::zeichnen, we reset dirty flag explicitly
	unset_dirty();
}



bool werkzeug_waehler_t::empty(spieler_t *sp) const
{
	FOR(vector_tpl<tool_data_t>, w, tools) {
		if (w.tool->get_icon(sp) != IMG_LEER) {
			return false;
		}
	}
	return true;
}
