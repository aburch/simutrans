/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */

#include "factorylist_frame_t.h"
#include "components/list_button.h"

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool factorylist_frame_t::sortreverse = false;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
factorylist::sort_mode_t factorylist_frame_t::sortby = factorylist::by_name;

const char *factorylist_frame_t::sort_text[factorylist::SORT_MODES] = {
    "Fabrikname",
    "Input",
    "Output",
    "Produktion",
    "Rating",
    "Power"
};

factorylist_frame_t::factorylist_frame_t(karte_t * welt) :
    gui_frame_t(translator::translate("fl_title")),
    sort_label(translator::translate("hl_txt_sort")),
	stats(welt,sortby,sortreverse),
	scrolly(&stats)
{
	sort_label.setze_pos(koord(BUTTON1_X, 4));
	add_komponente(&sort_label);

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	// name buttons
	sortedby.setze_text(sort_text[gib_sortierung()]);
	sorteddir.setze_text(gib_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	scrolly.setze_pos(koord(1, 14+BUTTON_HEIGHT+2));
	scrolly.set_show_scroll_x(false);
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
bool factorylist_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
    if(komp == &sortedby) {
	setze_sortierung((factorylist::sort_mode_t)((gib_sortierung() + 1) % factorylist::SORT_MODES));
	sortedby.setze_text(sort_text[gib_sortierung()]);
	stats.sort(gib_sortierung(),gib_reverse());
    }
    else if(komp == &sorteddir) {
	setze_reverse(!gib_reverse());
	sorteddir.setze_text(gib_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	stats.sort(gib_sortierung(),gib_reverse());
    }
    return true;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void factorylist_frame_t::resize(const koord delta)
{
    gui_frame_t::resize(delta);
    // fensterhoehe - 16(title) -offset (header)
    koord groesse = gib_fenstergroesse()-koord(0,14+BUTTON_HEIGHT+2+16);
    scrolly.setze_groesse(groesse);
}
