/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factorylist_frame_t.h"
#include "gui_theme.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"


const char *factorylist_frame_t::sort_text[factorylist::SORT_MODES] = {
	"Fabrikname",
	"Input",
	"Output",
	"Produktion",
	"Rating",
	"Power"
};

class playername_const_scroll_item_t : public gui_scrolled_list_t::const_text_scrollitem_t {
public:
	const uint8 player_nr;

	playername_const_scroll_item_t( player_t *pl ) : gui_scrolled_list_t::const_text_scrollitem_t( pl->get_name(), color_idx_to_rgb(pl->get_player_color1()+3) ), player_nr(pl->get_player_nr()) { }
};

factorylist_frame_t::factorylist_frame_t() :
	gui_frame_t( translator::translate("fl_title") ),
	scrolly(gui_scrolled_list_t::windowskin, factorylist_stats_t::compare)
{
	old_factories_count = 0;

	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(4,0);
	sortedby.init(button_t::roundbox, sort_text[factorylist_stats_t::sort_mode]);
	sortedby.add_listener(this);
	add_component(&sortedby);

	sorteddir.init(button_t::roundbox, factorylist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
	sorteddir.add_listener(this);
	add_component(&sorteddir);

	filter_by_owner.init( button_t::square_automatic, "Served by" );
	filter_by_owner.add_listener(this);
	filter_by_owner.set_tooltip( "At least one stop is connected to the factory" );
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

	add_component(&scrolly);
	fill_list();

	set_resizemode(diagonal_resize);
	scrolly.set_maximize(true);
	reset_min_windowsize();
}


/**
 * This method is called if an action is triggered
 */
bool factorylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		factorylist_stats_t::sort_mode = (factorylist_stats_t::sort_mode + 1) % factorylist::SORT_MODES;
		sortedby.set_text(sort_text[factorylist_stats_t::sort_mode]);
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		factorylist_stats_t::reverse = !factorylist_stats_t::reverse;
		sorteddir.set_text( factorylist_stats_t::reverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
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


void factorylist_frame_t::fill_list()
{
	old_factories_count = world()->get_fab_list().get_count(); // to avoid too many redraws ...
	scrolly.clear_elements();
	player_t *pl = (filter_by_owner.pressed  &&  filterowner.get_selection()>0) ? world()->get_player(filterowner.get_selection()) : NULL;
	FOR(const slist_tpl<fabrik_t *>,fab,world()->get_fab_list()) {
		if( pl == NULL || fab->is_within_players_network( pl ) ) {
			scrolly.new_component<factorylist_stats_t>( fab );
		}
	}
	scrolly.sort(0);
	scrolly.set_size(scrolly.get_size());
}


void factorylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  world()->get_fab_list().get_count() != old_factories_count  ) {
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}
