/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

#include "labellist_frame_t.h"
#include "labellist_stats_t.h"
#include "components/list_button.h"
#include "../dataobj/translator.h"
#include "../simcolor.h"


/**
 * This variable defines the sort order (ascending or descending)
 * Values: 1 = ascending, 2 = descending)
 * @author Markus Weber
 */
bool labellist_frame_t::sortreverse = false;
bool labellist_frame_t::filter_state = true;

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = label name
 *         1 = label koord
 *         2 = label owner
 * @author Markus Weber
 */
labellist::sort_mode_t labellist_frame_t::sortby = labellist::by_name;

const char *labellist_frame_t::sort_text[labellist::SORT_MODES] = {
    "hl_btn_sort_name",
    "koord",
    "player"
};

labellist_frame_t::labellist_frame_t(karte_t * welt) :
    gui_frame_t(translator::translate("labellist_title")),
    sort_label(translator::translate("hl_txt_sort")),
	stats(welt,sortby,sortreverse,filter_state),
	scrolly(&stats)
{
	sort_label.set_pos(koord(BUTTON1_X, 4));
	add_komponente(&sort_label);

	sortedby.init(button_t::roundbox, "", koord(BUTTON1_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sortedby.add_listener(this);
	add_komponente(&sortedby);

	sorteddir.init(button_t::roundbox, "", koord(BUTTON2_X, 14), koord(BUTTON_WIDTH,BUTTON_HEIGHT));
	sorteddir.add_listener(this);
	add_komponente(&sorteddir);

	filter.init( button_t::square_state, "Active player only", koord(BUTTON3_X+10,14) );
	filter.pressed = filter_state;
	add_komponente(&filter);
	filter.add_listener( this );

	set_fenstergroesse(koord(TOTAL_WIDTH, 240));

	scrolly.set_pos(koord(1,14+BUTTON_HEIGHT+2));
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
bool labellist_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		set_sortierung((labellist::sort_mode_t)((get_sortierung() + 1) % labellist::SORT_MODES));
		display_list();
	}
	else if(komp == &sorteddir) {
		set_reverse(!get_reverse());
		display_list();
	}
	else if (komp == &filter) {
		filter_state ^= 1;
		filter.pressed = filter_state;
		display_list();
	}
	return true;
}



/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void labellist_frame_t::resize(const koord delta)
{
	gui_frame_t::resize(delta);
	// fensterhoehe - 16(title) -offset (header)
	koord groesse = get_fenstergroesse()-koord(0,14+BUTTON_HEIGHT+2+16);
	scrolly.set_groesse(groesse);
}



/**
* This function refreshs the station-list
* @author Markus Weber/Volker Meyer
*/
void labellist_frame_t::display_list(void)
{
	sortedby.set_text(sort_text[get_sortierung()]);
	sorteddir.set_text(get_reverse() ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	stats.get_unique_labels(sortby, sortreverse, filter_state);
}
