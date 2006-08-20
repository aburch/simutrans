/*
 * citylist_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "citylist_frame_t.h"
#include "citylist_stats_t.h"

#include "components/list_button.h"
#include "../simcolor.h"


/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool citylist_frame_t::sortreverse = false;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
citylist::sort_mode_t citylist_frame_t::sortby = citylist::by_name;

const char *citylist_frame_t::sort_text[citylist::SORT_MODES] = {
    "Name",
    "citicens",
    "Growth"
};

citylist_frame_t::citylist_frame_t(karte_t * welt) :
	gui_frame_t("City list"),
	sort_label(translator::translate("hl_txt_sort")),
	stats(welt,sortby,sortreverse),
	scrolly(&stats)
{
	sort_label.setze_pos(koord(BUTTON1_X, 40-BUTTON_HEIGHT-LINESPACE));
	add_komponente(&sort_label);

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 40-BUTTON_HEIGHT), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 40-BUTTON_HEIGHT), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	// name buttons
	sortedby.setze_text(translator::translate(sort_text[gib_sortierung()]));
	sorteddir.setze_text(translator::translate(gib_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc"));

	scrolly.set_show_scroll_x(false);
	scrolly.setze_pos(koord(1,42));
	add_komponente(&scrolly);

	setze_fenstergroesse(koord(TOTAL_WIDTH, 240));
	// a min-size for the window
	set_min_windowsize(koord(TOTAL_WIDTH, 240));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));

	setze_opaque(true);
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool citylist_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
    if(komp == &sortedby) {
	setze_sortierung((citylist::sort_mode_t)((gib_sortierung() + 1) % citylist::SORT_MODES));
	sortedby.setze_text(translator::translate(sort_text[gib_sortierung()]));
	stats.sort(gib_sortierung(),gib_reverse());
    }
    else if(komp == &sorteddir) {
	setze_reverse(!gib_reverse());
	sorteddir.setze_text(translator::translate(gib_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc"));
	stats.sort(gib_sortierung(),gib_reverse());
    }
    return true;
}

/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void citylist_frame_t::resize(const koord delta)
{
  gui_frame_t::resize(delta);

  // fensterhoehe - 16(title) -42 (header)
  koord groesse = gib_fenstergroesse()-koord(0,58);
  scrolly.setze_groesse(groesse);
}



/**
 * Komponente neu zeichnen. Die übergebenen Werte beziehen sich auf
 * das Fenster, d.h. es sind die Bildschirkoordinaten des Fensters
 * in dem die Komponente dargestellt wird.
 * @author Hj. Malthaner
 */
void
citylist_frame_t::zeichnen(koord pos, koord gr)
{
	gui_frame_t::zeichnen(pos,gr);

	display_proportional(pos.x+2, pos.y+18, citylist_stats_t::total_bev_string, ALIGN_LEFT,COL_BLACK,true);
}
