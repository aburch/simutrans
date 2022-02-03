/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factorylist_frame.h"
#include "gui_theme.h"
#include "../dataobj/translator.h"
#include "../player/simplay.h"
#include "../world/simworld.h"
#include "../dataobj/environment.h"


char factorylist_frame_t::name_filter[256];

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
	playername_const_scroll_item_t( player_t *pl ) : gui_scrolled_list_t::const_text_scrollitem_t( pl->get_name(), color_idx_to_rgb(pl->get_player_color1()+env_t::gui_player_color_dark) ), player_nr(pl->get_player_nr()) { }
};

factorylist_frame_t::factorylist_frame_t() :
	gui_frame_t( translator::translate("fl_title") ),
	scrolly(gui_scrolled_list_t::windowskin, factorylist_stats_t::compare)
{
	old_factories_count = 0;

	set_table_layout(1,0);
	set_table_layout(1, 0);
	add_table(3, 3);
	{
		new_component<gui_label_t>("Filter:");
		name_filter_input.set_text(name_filter, lengthof(name_filter));
		add_component(&name_filter_input);
		new_component<gui_fill_t>();

		filter_by_owner.init(button_t::square_automatic, "Served by");
		filter_by_owner.add_listener(this);
		filter_by_owner.set_tooltip("At least one tile is connected to one stop.");
		add_component(&filter_by_owner);

		filterowner.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("No player"), SYSCOL_TEXT);
		for (int i = 0; i < MAX_PLAYER_COUNT; i++) {
			if (player_t* pl = welt->get_player(i)) {
				filterowner.new_component<playername_const_scroll_item_t>(pl);
				if (pl == welt->get_active_player()) {
					filterowner.set_selection(filterowner.count_elements() - 1);
				}
			}
		}
		filterowner.add_listener(this);
		add_component(&filterowner);
		new_component<gui_fill_t>();

		new_component_span<gui_label_t>("hl_txt_sort", 1);
		sortedby.set_unsorted(); // do not sort
		for (size_t i = 0; i < lengthof(sort_text); i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(factorylist_stats_t::sort_mode);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::sortarrow_state, NULL);
		sorteddir.add_listener(this);
		sorteddir.pressed = factorylist_stats_t::reverse;
		add_component(&sorteddir);
		new_component<gui_fill_t>();
	}
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
bool factorylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if (comp == &sortedby) {
		factorylist_stats_t::sort_mode = v.i;
		scrolly.sort(0);
	}
	else if (comp == &sorteddir) {
		factorylist_stats_t::reverse = !factorylist_stats_t::reverse;
		sorteddir.pressed = factorylist_stats_t::reverse;
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
	if (filter_by_owner.pressed && filterowner.get_selection() == 0) {
		for(fabrik_t* fab : world()->get_fab_list()) {
			bool add = (name_filter[0] == 0 || utf8caseutf8(fab->get_name(), name_filter));
			for(  int i = 0;  add  &&  i < MAX_PLAYER_COUNT;  i++  ) {
				if(  player_t* pl = welt->get_player(i)  ) {
					if (fab->is_within_players_network(pl)) {
						// already connected
						add = false;
					}
				}
			}
			if (add) {
				scrolly.new_component<factorylist_stats_t>(fab);
			}
		}
	}
	else {
		player_t* pl = (filter_by_owner.pressed && filterowner.get_selection() >= 1) ? welt->get_player(((const playername_const_scroll_item_t*)(filterowner.get_selected_item()))->player_nr) : NULL;
		for(fabrik_t * fab : world()->get_fab_list()) {
			if( pl == NULL  ||  fab->is_within_players_network( pl ) ) {
				if(  name_filter[0] == 0  ||  utf8caseutf8(fab->get_name(), name_filter)) {
					scrolly.new_component<factorylist_stats_t>( fab );
				}
			}
		}
	}
	scrolly.sort(0);
	scrolly.set_size(scrolly.get_size());
}


void factorylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  world()->get_fab_list().get_count() != old_factories_count  ||  strcmp(last_name_filter, name_filter)  ) {
		strcpy(last_name_filter, name_filter);
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}


void factorylist_frame_t::rdwr(loadsave_t* file)
{
	scr_size size = get_windowsize();

	size.rdwr(file);
	scrolly.rdwr(file);
	file->rdwr_str(name_filter, lengthof(name_filter));
	file->rdwr_short(factorylist_stats_t::sort_mode);
	file->rdwr_bool(factorylist_stats_t::reverse);
	if (file->is_loading()) {
		fill_list();
		set_windowsize(size);
	}
}
