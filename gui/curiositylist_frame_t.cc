/*
 * curiositylist_frame_t.cpp
 *
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project and may not be used
 * in other projects without written permission of the author.
 */


#include "curiositylist_frame_t.h"
#include "curiositylist_stats_t.h"
#include "components/list_button.h"
#include "../simcolor.h"

/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool curiositylist_frame_t::sortreverse = false;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */
curiositylist::sort_mode_t curiositylist_frame_t::sortby = curiositylist::by_name;

const char *curiositylist_frame_t::sort_text[curiositylist::SORT_MODES] = {
    "hl_btn_sort_name",
    "Passagierrate"/*,
		     "Postrate"*/
};

curiositylist_frame_t::curiositylist_frame_t(karte_t * welt) :
    gui_frame_t(translator::translate("curlist_title")),
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

	setze_opaque(true);
	setze_fenstergroesse(koord(TOTAL_WIDTH, 240));

	scrolly.setze_pos(koord(1,14+BUTTON_HEIGHT+2));
	scrolly.set_show_scroll_x(false);
	add_komponente(&scrolly);

	display_list();

	// a min-size for the window
	set_min_windowsize(koord(TOTAL_WIDTH, 240));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool curiositylist_frame_t::action_triggered(gui_komponente_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		setze_sortierung((curiositylist::sort_mode_t)((gib_sortierung() + 1) % curiositylist::SORT_MODES));
		display_list();
	}
	else if(komp == &sorteddir) {
		setze_reverse(!gib_reverse());
		display_list();
	}
	return true;
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void curiositylist_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	// fensterhoehe - 16(title) -offset (header)
	koord groesse = gib_fenstergroesse()-koord(0,14+BUTTON_HEIGHT+2+16);
	scrolly.setze_groesse(groesse);
}



/**
* This function refreshs the station-list
* @author Markus Weber/Volker Meyer
*/
void curiositylist_frame_t::display_list(void)
{
	sortedby.setze_text(sort_text[gib_sortierung()]);
	sorteddir.setze_text(gib_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	stats.get_unique_attractions(sortby,sortreverse);
}
