/*
 * Copyright (c) 1997 - 2003 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Factory list window
 * @author Hj. Malthaner
 */

#include "factorylist_frame_t.h"

#include "../dataobj/translator.h"
#include "../simcolor.h"
#include "../dataobj/environment.h"
#include "../utils/cbuffer_t.h"

/**
 * This variable defines by which column the table is sorted
 * Values: 0 = Station number
 *         1 = Station name
 *         2 = Waiting goods
 *         3 = Station type
 * @author Markus Weber
 */

const char *factorylist_frame_t::sort_text[factorylist::SORT_MODES] = {
	"Fabrikname",
	"Input",
	"Output",
	"Produktion",
	"Rating",
	"Power"
};

factorylist_frame_t::factorylist_frame_t() :
	gui_frame_t( translator::translate("fl_title") ),
	sort_label(translator::translate("hl_txt_sort")),
	scrolly(gui_scrolled_list_t::windowskin)
{
	sort_label.set_pos(scr_coord(BUTTON1_X, D_MARGIN_TOP));
	add_component(&sort_label);

	sortedby.init(button_t::roundbox, "", scr_coord(BUTTON1_X, D_MARGIN_TOP+LINESPACE));
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, "", scr_coord(BUTTON2_X, D_MARGIN_TOP+LINESPACE));
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	// name buttons
	sortedby.set_text(sort_text[factorylist_stats_t::sort_mode]);
	sorteddir.set_text(factorylist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");

	scrolly.set_pos(scr_coord(0, D_MARGIN_TOP+LINESPACE+D_BUTTON_HEIGHT+D_V_SPACE));
	add_component(&scrolly);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_DEFAULT_HEIGHT));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+D_MARGIN_TOP+LINESPACE+D_V_SPACE+30));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool factorylist_frame_t::action_triggered( gui_action_creator_t *komp,value_t /* */)
{
	if(komp == &sortedby) {
		factorylist_stats_t::sort_mode = (factorylist_stats_t::sort_mode + 1) % factorylist::SORT_MODES;
		sortedby.set_text(sort_text[factorylist_stats_t::sort_mode]);
		scrolly.sort(0,NULL);
	}
	else if(komp == &sorteddir) {
		factorylist_stats_t::reverse = !factorylist_stats_t::reverse;
		sorteddir.set_text( factorylist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		scrolly.sort(0,NULL);
	}
	return true;
}


/**
 * resize window in response to a resize event
 * @author Hj. Malthaner
 * @date   16-Oct-2003
 */
void factorylist_frame_t::resize(const scr_coord delta)
{
	gui_frame_t::resize(delta);
	// window size - titlebar - offset (header)
	scr_size size = get_windowsize()-scr_size(0,D_TITLEBAR_HEIGHT+D_MARGIN_TOP+LINESPACE+D_BUTTON_HEIGHT+D_V_SPACE);
	scrolly.set_size(size);
}



void factorylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  world()->get_fab_list().get_count() != scrolly.get_count()  ) {
		scrolly.clear_elements();
		FOR(const slist_tpl<fabrik_t *>,fab,world()->get_fab_list()) {
			scrolly.append_element( new factorylist_stats_t(fab) );
		}
		scrolly.sort(0,NULL);
	}

	gui_frame_t::draw(pos,size);
}
