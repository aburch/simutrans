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
	scrolly(gui_scrolled_list_t::windowskin, factorylist_stats_t::compare)
{
	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(2,0);
	sortedby.init(button_t::roundbox, sort_text[factorylist_stats_t::sort_mode]);
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, factorylist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	sorteddir.add_listener(this);
	add_component(&sorteddir);
	end_table();

	add_component(&scrolly);
	fill_list();

	reset_min_windowsize();
	set_resizemode(diagonal_resize);
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
		scrolly.sort(0);
	}
	else if(komp == &sorteddir) {
		factorylist_stats_t::reverse = !factorylist_stats_t::reverse;
		sorteddir.set_text( factorylist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		scrolly.sort(0);
	}
	return true;
}


void factorylist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(const slist_tpl<fabrik_t *>,fab,world()->get_fab_list()) {
		scrolly.new_component<factorylist_stats_t>(fab) ;
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


void factorylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  world()->get_fab_list().get_count() != (uint32)scrolly.get_count()  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}
