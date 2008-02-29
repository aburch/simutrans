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
#include "werkzeug_parameter_waehler.h"


werkzeug_parameter_waehler_t::werkzeug_parameter_waehler_t(karte_t* welt, const char* titel) :
	tools(32), groesse(32, 16)
{
    this->welt = welt;
    this->titel  = titel;
    this->hilfe_datei = NULL;
    dirty = true;
}


/**
 * Add a new tool with values and tooltip text.
 * Text must already be translated.
 * @author Hj. Malthaner
 */
void werkzeug_parameter_waehler_t::add_tool(int (* wz1)(spieler_t *, karte_t *, koord),
	      int versatz,
	      int sound_ok,
	      int sound_ko,
	      int icon,
	      int cursor,
	      cstring_t tooltip)
{
	struct werkzeug_t tool;

	tool.has_param = false;
	tool.wzwp = 0;
	tool.wzwop = wz1;
	tool.param.i = 0;
	tool.versatz = versatz;
	tool.sound_ok = sound_ok;
	tool.sound_ko = sound_ko;
	tool.icon = icon;
	tool.cursor = cursor;

	// remove all "\xA" which the tranlator adds ...
	tooltip.replace_character('\n', ' ');
	tool.tooltip = tooltip;

	tools.push_back(tool);

	int ww = (display_get_width()/32)-2;
	tool_icon_width = tools.get_count();
DBG_DEBUG("werkzeug_parameter_waehler_t::add_tool()","ww=%i, tool_icon_width=%i",ww,tool_icon_width);
	if(ww<tool_icon_width) {
		int rows = (tool_icon_width/ww)+1;
DBG_DEBUG("werkzeug_parameter_waehler_t::add_tool()","ww=%i, rows=%i",ww,rows);
		// assure equal distribution if more than a single row is needed
		tool_icon_width = (tool_icon_width+rows-1)/rows;
	}
	dirty = true;
	groesse = koord( tool_icon_width*32, ((tools.get_count()-1)/tool_icon_width) * 32 + 16 + 32 );

	DBG_DEBUG("werkzeug_parameter_waehler_t::add_tool()", "at position %i (width %i)", tools.get_count(), tool_icon_width);
}


/**
 * Add a new tool with parameter values and tooltip text.
 * Text must already be translated.
 * @author Hj. Malthaner
 */
void werkzeug_parameter_waehler_t::add_param_tool(int (* wz1)(spieler_t *, karte_t *, koord, value_t),
		    value_t param,
		    int versatz,
		    int sound_ok,
		    int sound_ko,
		    int icon,
		    int cursor,
		    cstring_t tooltip)
{
	struct werkzeug_t tool;

	tool.has_param = true;
	tool.wzwp = wz1;
	tool.wzwop = 0;
	tool.param = param;
	tool.versatz = versatz;
	tool.sound_ok = sound_ok;
	tool.sound_ko = sound_ko;
	tool.icon = icon;
	tool.cursor = cursor;

	// remove all "\xA" which the tranlator adds ...
	tooltip.replace_character('\n', ' ');
	tool.tooltip = tooltip;

	tools.push_back(tool);
	int ww = (display_get_width()/32)-2;
	tool_icon_width = tools.get_count();
//DBG_DEBUG("werkzeug_parameter_waehler_t::add_param_tool()","ww=%i, tool_icon_width=%i",ww,tool_icon_width);
	if(ww<tool_icon_width) {
		int rows = (tool_icon_width/ww)+1;
		// assure equal distribution if more than a single row is needed
		tool_icon_width = (tool_icon_width+rows-1)/rows;
	}
	groesse = koord( tool_icon_width*32, ((tools.get_count()-1)/tool_icon_width) * 32 + 16 + 32 );
	dirty = true;
//	DBG_DEBUG("werkzeug_parameter_waehler_t::add_param_tool()", "at position %i (width %i)", tools.get_count(), tool_icon_width);
}


// reset the tools to empty state
void werkzeug_parameter_waehler_t::reset_tools()
{
	tools.clear();
	groesse = koord( 32, 16 );
	tool_icon_width = 0;
}


bool werkzeug_parameter_waehler_t::getroffen(int x, int y)
{
	int dx = x>>5;
	int	dy = (y-16)>>5;
	if(x>=0 && dx<tool_icon_width  &&  y>=0  &&  (y<16  ||  dy<tool_icon_width)) {
		return y < 16 || dx + tool_icon_width * dy < (int)tools.get_count();
	}
	return false;
}


void werkzeug_parameter_waehler_t::infowin_event(const event_t *ev)
{
	if(IS_LEFTCLICK(ev)) {
		// tooltips?
		const int x = (ev->mx) >> 5;
		const int y = (ev->my-16) >> 5;

		if(x>=0 && x<tool_icon_width  &&  y>=0) {
			const int wz_idx = x+(tool_icon_width*y);

			if (wz_idx < (int)tools.get_count()) {
				const struct werkzeug_t& tool = tools[wz_idx];

				if(tool.has_param) {
					welt->setze_maus_funktion(tool.wzwp,  tool.cursor, tool.versatz, tool.param, tool.sound_ok, tool.sound_ko);
				}
				else {
					welt->setze_maus_funktion(tool.wzwop, tool.cursor, tool.versatz, tool.sound_ok, tool.sound_ko);
				}
			}
		}
	} else if(ev->ev_class==INFOWIN &&  ev->ev_code==WIN_CLOSE) {
		welt->setze_maus_funktion(wkz_abfrage,
		      skinverwaltung_t::fragezeiger->gib_bild_nr(0),
		      karte_t::Z_PLAN,
		      NO_SOUND,
		      NO_SOUND);
	}
}


void werkzeug_parameter_waehler_t::zeichnen(koord pos, koord)
{
	for (uint i = 0; i < tools.get_count(); i++) {
		const int icon = tools[i].icon;

		const koord draw_pos=pos+koord((i%tool_icon_width)*32,16+(i/tool_icon_width)*32);
		if(icon == -1) {
			// Hajo: no icon image available, draw a blank
			// DDD box as replacement

			// top
			display_fillbox_wh(draw_pos.x, draw_pos.y, 32, 1, MN_GREY4, dirty);
			// body
			display_fillbox_wh(draw_pos.x+1, draw_pos.y+1, 30, 30, MN_GREY2, dirty);
			// bottom
			display_fillbox_wh(draw_pos.x, draw_pos.y+31, 32, 1, MN_GREY0, dirty);
			// Left
			display_fillbox_wh(draw_pos.x, draw_pos.y, 1, 32, MN_GREY4, dirty);
			// Right
			display_fillbox_wh(draw_pos.x+31, draw_pos.y, 1, 32, MN_GREY0, dirty);
		}
		else {
			display_color_img(icon, draw_pos.x, draw_pos.y, 0, false, dirty);
		}
	}

	// tooltips?
	const int xdiff = (gib_maus_x() - pos.x) >> 5;
	const int ydiff = (gib_maus_y() - pos.y - 16) >> 5;
	if(xdiff>=0  &&  xdiff<tool_icon_width  &&  ydiff>=0) {
		const int tipnr = xdiff+(tool_icon_width*ydiff);
		if (tipnr < (int)tools.get_count()) {
			win_set_tooltip(gib_maus_x() + 16, gib_maus_y() - 16, tools[tipnr].tooltip);
		}
	}

	dirty = false;
}
