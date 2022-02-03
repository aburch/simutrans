/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "curiositylist_frame.h"
#include "curiositylist_stats.h"

#include "components/gui_label.h"
#include "../dataobj/translator.h"
#include "../dataobj/environment.h"
#include "../simcolor.h"
#include "../world/simworld.h"
#include "../player/simplay.h"
#include "../obj/gebaeude.h"
#include "../descriptor/building_desc.h"


char curiositylist_frame_t::name_filter[256];

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

		new_component<gui_label_t>("hl_txt_sort");
		sortedby.set_unsorted(); // do not sort
		for (size_t i = 0; i < lengthof(sort_text); i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(curiositylist_stats_t::sortby);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::sortarrow_state, NULL);
		sorteddir.add_listener(this);
		sorteddir.pressed = curiositylist_stats_t::sortby;
		add_component(&sorteddir);

		new_component<gui_fill_t>();
	}
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
	player_t* pl = (filter_by_owner.pressed && filterowner.get_selection() >= 0) ? welt->get_player(((const playername_const_scroll_item_t*)(filterowner.get_selected_item()))->player_nr) : NULL;

	if (filter_by_owner.pressed && filterowner.get_selection() == 0) {
		for(gebaeude_t* const geb : world_attractions) {
			if (geb != NULL &&
				geb->get_first_tile() == geb &&
				geb->get_passagier_level() != 0) {
				bool add = (name_filter[0] == 0 || utf8caseutf8(translator::translate(geb->get_tile()->get_desc()->get_name()), name_filter));
				for (int i = 0; add && i < MAX_PLAYER_COUNT; i++) {
					if (player_t* pl = welt->get_player(i)) {
						if (geb->is_within_players_network(pl)) {
							// already connected
							add = false;
						}
					}
				}
				if (add) {
					scrolly.new_component<curiositylist_stats_t>(geb);
				}
			}
		}
	}
	else {
		for(gebaeude_t* const geb : world_attractions) {
			if (geb != NULL &&
				geb->get_first_tile() == geb &&
				geb->get_passagier_level() != 0) {

				if(  pl == NULL  ||  geb->is_within_players_network( pl )   ) {
					curiositylist_stats_t* cs = new curiositylist_stats_t(geb);
					if(  name_filter[0] == 0 ||  utf8caseutf8(cs->get_text(), name_filter)  ) {
						scrolly.take_component(cs);
					}
					else {
						delete cs;
					}
				}
			}
		}
	}
	scrolly.sort(0);
	scrolly.set_size( scrolly.get_size());
}


bool curiositylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		curiositylist_stats_t::sortby = (curiositylist::sort_mode_t)v.i;
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		curiositylist_stats_t::sortreverse = !curiositylist_stats_t::sortreverse;
		sorteddir.pressed = curiositylist_stats_t::sortreverse;
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
	if(  world()->get_attractions().get_count() != attraction_count  ||  strcmp(last_name_filter, name_filter)  ) {
		strcpy(last_name_filter, name_filter);
		fill_list();
	}

	gui_frame_t::draw(pos,size);
}


void curiositylist_frame_t::rdwr(loadsave_t* file)
{
	scr_size size = get_windowsize();
	sint16 sort_mode = curiositylist_stats_t::sortby;

	size.rdwr(file);
	scrolly.rdwr(file);
	file->rdwr_str(name_filter, lengthof(name_filter));
	file->rdwr_short(sort_mode);
	file->rdwr_bool(curiositylist_stats_t::sortreverse);
	if (file->is_loading()) {
		curiositylist_stats_t::sortby = (curiositylist::sort_mode_t)sort_mode;
		fill_list();
		set_windowsize(size);
	}
}
