/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
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
	"Passagierrate",
	"sort_pas_arrived",
	/*"by_city",*/
	"by_region"
};

class attraction_item_t : public gui_scrolled_list_t::const_text_scrollitem_t {
public:
	attraction_item_t(uint8 i) : gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(sort_text[i]), SYSCOL_TEXT) { }
};

curiositylist_frame_t::curiositylist_frame_t() :
	gui_frame_t(translator::translate("curlist_title")),
	scrolly(gui_scrolled_list_t::windowskin, curiositylist_stats_t::compare)
{
	attraction_count = 0;

	set_table_layout(1, 0);
	add_table(3, 2);
	{
		// 1st row
		new_component<gui_label_t>("hl_txt_sort");
		new_component<gui_label_t>("Filter:");

		filter_within_network.init(button_t::square_state, "Within own network");
		filter_within_network.set_tooltip("Show only connected to own passenger transportation network");
		filter_within_network.add_listener(this);
		filter_within_network.pressed = curiositylist_stats_t::filter_own_network;
		add_component(&filter_within_network);

		// 2nd row
		add_table(3, 1);
		{
			for (int i = 0; i < curiositylist::SORT_MODES; i++) {
				sortedby.new_component<attraction_item_t>(i);
			}
			sortedby.set_selection(curiositylist_stats_t::sort_mode);
			sortedby.set_width_fixed(true);
			sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
			sortedby.add_listener(this);
			add_component(&sortedby);

			// sort ascend/descend button
			sort_asc.init(button_t::arrowup_state, "");
			sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
			sort_asc.add_listener(this);
			sort_asc.pressed = curiositylist_stats_t::sortreverse;
			add_component(&sort_asc);

			sort_desc.init(button_t::arrowdown_state, "");
			sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
			sort_desc.add_listener(this);
			sort_desc.pressed = !curiositylist_stats_t::sortreverse;
			add_component(&sort_desc);
		}
		end_table();

		new_component<gui_empty_t>();

		if (!welt->get_settings().regions.empty()) {
			//region_selector
			region_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All regions"), SYSCOL_TEXT);

			for (uint8 r = 0; r < welt->get_settings().regions.get_count(); r++) {
				region_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(welt->get_settings().regions[r].name.c_str()), SYSCOL_TEXT);
			}
			region_selector.set_selection(curiositylist_stats_t::region_filter);
			region_selector.set_width_fixed(true);
			region_selector.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
			region_selector.add_listener(this);
			add_component(&region_selector);
		}
		else {
			new_component<gui_empty_t>();
		}
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

	FOR(const weighted_vector_tpl<gebaeude_t*>, const geb, world_attractions) {
		if (curiositylist_stats_t::region_filter && (curiositylist_stats_t::region_filter - 1) != welt->get_region(geb->get_pos().get_2d())) {
			continue;
		}
		if (geb != NULL &&
			geb->get_first_tile() == geb &&
			geb->get_adjusted_visitor_demand() != 0) {

			if (!curiositylist_stats_t::filter_own_network ||
				(curiositylist_stats_t::filter_own_network && geb->is_within_players_network(welt->get_active_player(), goods_manager_t::INDEX_PAS))) {
				scrolly.new_component<curiositylist_stats_t>(geb);
			}
		}
	}
	scrolly.sort(0);
	scrolly.set_size(scrolly.get_size());
}


/**
 * This method is called if an action is triggered
 */
bool curiositylist_frame_t::action_triggered(gui_action_creator_t *comp, value_t v)
{
	if (comp == &sortedby) {
		curiositylist_stats_t::sort_mode = max(0, v.i);
		scrolly.sort(0);
	}
	else if (comp == &region_selector) {
		curiositylist_stats_t::region_filter = max(0, v.i);
		fill_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		curiositylist_stats_t::sortreverse = !curiositylist_stats_t::sortreverse;
		scrolly.sort(0);
		sort_asc.pressed = curiositylist_stats_t::sortreverse;
		sort_desc.pressed = !curiositylist_stats_t::sortreverse;
	}
	else if (comp == &filter_within_network) {
		curiositylist_stats_t::filter_own_network = !curiositylist_stats_t::filter_own_network;
		filter_within_network.pressed = curiositylist_stats_t::filter_own_network;
		fill_list();
	}
	return true;
}



void curiositylist_frame_t::draw(scr_coord pos, scr_size size)
{
	if (world()->get_attractions().get_count() != attraction_count) {
		fill_list();
	}

	gui_frame_t::draw(pos, size);
}
