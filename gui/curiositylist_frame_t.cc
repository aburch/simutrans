/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "curiositylist_frame_t.h"
#include "curiositylist_stats_t.h"

#include "components/gui_label.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../player/simplay.h"
#include "../obj/gebaeude.h"


const char* sort_text[curiositylist::SORT_MODES] = {
	"hl_btn_sort_name",
	"Passagierrate"
};


class playername_const_scroll_item_t : public gui_scrolled_list_t::const_text_scrollitem_t {
public:
	const uint8 player_nr;
	playername_const_scroll_item_t( player_t *pl ) : gui_scrolled_list_t::const_text_scrollitem_t( pl->get_name(), color_idx_to_rgb(pl->get_player_color1()+env_t::gui_player_color_dark) ), player_nr(pl->get_player_nr()) { }
};

curiositylist_frame_t::curiositylist_frame_t() :
	gui_frame_t( translator::translate("curlist_title") ),
	scrolly(gui_scrolled_list_t::windowskin, curiositylist_stats_t::compare)
{
	attraction_count = 0;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(4,0);
	sortedby.init(button_t::roundbox, sort_text[curiositylist_stats_t::sortby]);
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, curiositylist_stats_t::sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	filter_by_owner.init( button_t::square_automatic, "Served by" );
	filter_by_owner.add_listener(this);
	filter_by_owner.set_tooltip( "At least one tile is connected to one stop" );
	add_component(&filter_by_owner);

	for( int i = 0; i < MAX_PLAYER_COUNT; i++ ) {
		if( player_t *pl=welt->get_player(i) ) {
			filterowner.new_component<playername_const_scroll_item_t>(pl);
			if( pl == welt->get_active_player() ) {
				filterowner.set_selection( filterowner.count_elements()-1 );
			}
		}
	}
	filterowner.add_listener(this);
	add_component(&filterowner);
	end_table();

	set_alignment(ALIGN_STRETCH_V | ALIGN_STRETCH_H);
	add_component(&scrolly);
	fill_list();

	set_resizemode(diagonal_resize);
	scrolly.set_maximize(true);
	reset_min_windowsize();
}


void curiositylist_frame_t::fill_list()
{
	scrolly.clear_elements();
	const weighted_vector_tpl<gebaeude_t*>& world_attractions = welt->get_attractions();
	attraction_count = world_attractions.get_count();
	player_t *pl = (filter_by_owner.pressed  &&  filterowner.get_selection()>0) ? world()->get_player(filterowner.get_selection()) : NULL;

	FOR(const weighted_vector_tpl<gebaeude_t*>, const geb, world_attractions) {
		if (geb != NULL &&
			geb->get_first_tile() == geb &&
			geb->get_passagier_level() != 0) {
			if( pl == NULL || geb->is_within_players_network( pl ) ) {
				scrolly.new_component<curiositylist_stats_t>(geb) ;
			}
		}
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


/**
 * This method is called if an action is triggered
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
	else if(comp == &filterowner) {
		if(  filter_by_owner.pressed ) {
			fill_list();
		}
	}
	else if( comp == &filter_by_owner ) {
		fill_list();
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
