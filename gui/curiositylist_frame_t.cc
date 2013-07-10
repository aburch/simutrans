/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/**
 * Curiosity list window
 * @author Hj. Malthaner
 */

#include "curiositylist_frame_t.h"
#include "curiositylist_stats_t.h"

#include "../dataobj/translator.h"
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
	gui_frame_t( translator::translate("curlist_title") ),
	sort_label(translator::translate("hl_txt_sort")),
	stats(welt,sortby,sortreverse),
	scrolly(&stats)
{
	sort_label.set_pos(koord(BUTTON1_X, 2));
	add_komponente(&sort_label);

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 14), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 14), koord(D_BUTTON_WIDTH,D_BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	scrolly.set_pos(koord(0,14+D_BUTTON_HEIGHT+2));
	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_komponente(&scrolly);

	display_list();

	set_fenstergroesse(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+18*(LINESPACE+1)+14+D_BUTTON_HEIGHT+2+1));
	set_min_windowsize(koord(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+4*(LINESPACE+1)+14+D_BUTTON_HEIGHT+2+1));

	set_resizemode(diagonal_resize);
	resize(koord(0,0));
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool curiositylist_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		set_sortierung((curiositylist::sort_mode_t)((get_sortierung() + 1) % curiositylist::SORT_MODES));
		display_list();
	}
	else if(komp == &sorteddir) {
		set_reverse(!get_reverse());
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
	// window size -titlebar -offset (header)
	koord groesse = get_fenstergroesse()-koord(0,D_TITLEBAR_HEIGHT+14+D_BUTTON_HEIGHT+2+1);
	scrolly.set_groesse(groesse);
}



/**
* This function refreshes the station-list
* @author Markus Weber/Volker Meyer
*/
void curiositylist_frame_t::display_list(void)
{
	sortedby.set_text(sort_text[get_sortierung()]);
	sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	stats.get_unique_attractions(sortby,sortreverse);
	stats.recalc_size();
}
