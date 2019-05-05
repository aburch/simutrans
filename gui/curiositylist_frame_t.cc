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

#include "components/gui_label.h"
#include "../dataobj/translator.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../obj/gebaeude.h"


const char* sort_text[curiositylist::SORT_MODES] = {
	"hl_btn_sort_name",
	"Passagierrate"
};


curiositylist_frame_t::curiositylist_frame_t() :
	gui_frame_t( translator::translate("curlist_title") ),
	scrolly(gui_scrolled_list_t::windowskin, curiositylist_stats_t::compare)
{
	attraction_count = 0;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(2,0);
	sortedby.init(button_t::roundbox, sort_text[curiositylist_stats_t::sortby]);
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, curiositylist_stats_t::sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	sorteddir.add_listener(this);
	add_component(&sorteddir);
	end_table();

	add_component(&scrolly);
	fill_list();

	reset_min_windowsize();
	set_resizemode(diagonal_resize);
}


void curiositylist_frame_t::fill_list()
{
	scrolly.clear_elements();
	const weighted_vector_tpl<gebaeude_t*>& world_attractions = welt->get_attractions();
	attraction_count = world_attractions.get_count();

	FOR(const weighted_vector_tpl<gebaeude_t*>, const geb, world_attractions) {
		if (geb != NULL &&
			geb->get_first_tile() == geb &&
			geb->get_passagier_level() != 0) {

			scrolly.new_component<curiositylist_stats_t>(geb) ;
		}
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


/**
 * This method is called if an action is triggered
 * @author Markus Weber/Volker Meyer
 */
bool curiositylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		curiositylist_stats_t::sortby = (curiositylist::sort_mode_t) ((curiositylist_stats_t::sortby + 1) % curiositylist::SORT_MODES);
		sortedby.set_text(sort_text[curiositylist_stats_t::sortby]);
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		curiositylist_stats_t::sortreverse = !curiositylist_stats_t::sortreverse;
		sorteddir.set_text( curiositylist_stats_t::sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		scrolly.sort(0);
	}
	return true;
}



void curiositylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  world()->get_attractions().get_count() != attraction_count  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}
