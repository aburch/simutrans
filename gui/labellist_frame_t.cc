/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "labellist_frame_t.h"
#include "labellist_stats_t.h"

#include "../dataobj/translator.h"
#include "../obj/label.h"
#include "../simworld.h"


static const char *sort_text[labellist::SORT_MODES] = {
	"hl_btn_sort_name",
	"koord",
	"player",
	"by_region"
};

class label_sort_item_t : public gui_scrolled_list_t::const_text_scrollitem_t {
public:
	label_sort_item_t(uint8 i) : gui_scrolled_list_t::const_text_scrollitem_t(translator::translate(sort_text[i]), SYSCOL_TEXT) { }
};

labellist_frame_t::labellist_frame_t() :
	gui_frame_t( translator::translate("labellist_title") ),
	scrolly(gui_scrolled_list_t::windowskin, labellist_stats_t::compare)
{
	set_table_layout(1,0);
	add_table(3, 2);
	{
		// 1st row
		new_component<gui_label_t>("hl_txt_sort");
		new_component<gui_label_t>("Filter:");

		filter.init( button_t::square_automatic, "Active player only");
		filter.pressed = labellist_stats_t::filter;
		filter.add_listener( this );
		add_component(&filter);

		// 2nd row
		add_table(3, 1);
		{
			for (int i = 0; i < labellist::SORT_MODES; i++) {
				sortedby.new_component<label_sort_item_t>(i);
			}
			sortedby.set_selection(labellist_stats_t::sort_mode);
			sortedby.set_width_fixed(true);
			sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
			sortedby.add_listener(this);
			add_component(&sortedby);

			// sort ascend/descend button
			sort_asc.init(button_t::arrowup_state, "");
			sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
			sort_asc.add_listener(this);
			sort_asc.pressed = labellist_stats_t::sortreverse;
			add_component(&sort_asc);

			sort_desc.init(button_t::arrowdown_state, "");
			sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
			sort_desc.add_listener(this);
			sort_desc.pressed = !labellist_stats_t::sortreverse;
			add_component(&sort_desc);
		}
		end_table();

		new_component<gui_empty_t>();

		if (!welt->get_settings().regions.empty()) {
			//region_selector
			region_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All region"), SYSCOL_TEXT);

			for (uint8 r = 0; r < welt->get_settings().regions.get_count(); r++) {
				region_selector.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(welt->get_settings().regions[r].name.c_str()), SYSCOL_TEXT);
			}
			region_selector.set_selection(labellist_stats_t::region_filter);
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

	add_component(&scrolly);
	scrolly.set_maximize(true);

	fill_list();

	reset_min_windowsize();
	set_resizemode(diagonal_resize);
}


void labellist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(slist_tpl<koord>, const& pos, welt->get_label_list()) {
		if (labellist_stats_t::region_filter && (labellist_stats_t::region_filter - 1) != welt->get_region(pos)) {
			continue;
		}
		label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		const char* name = welt->lookup_kartenboden(pos)->get_text();
		// some old version games don't have label nor name.
		// Check them to avoid crashes.
		if(label  &&  name  &&  (!labellist_stats_t::filter  ||  (label  &&  (label->get_owner() == welt->get_active_player())))) {
			scrolly.new_component<labellist_stats_t>(pos);
		}
	}
	scrolly.sort(0);
	reset_min_windowsize();
}


uint32 labellist_frame_t::count_label()
{
	uint32 labelcount = 0;
	FOR(slist_tpl<koord>, const& pos, welt->get_label_list()) {
		label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		const char* name = welt->lookup_kartenboden(pos)->get_text();
		// some old version games don't have label nor name.
		if(label  &&  name  &&  (!labellist_stats_t::filter  ||  (label  &&  (label->get_owner() == welt->get_active_player())))) {
			labelcount++;
		}
	}
	return labelcount;
}


/**
 * This method is called if an action is triggered
 */
bool labellist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		labellist_stats_t::sort_mode = max(0, v.i);
		scrolly.sort(0);
	}
	else if (comp == &region_selector) {
		labellist_stats_t::region_filter = max(0, v.i);
		fill_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		labellist_stats_t::sortreverse = !labellist_stats_t::sortreverse;
		scrolly.sort(0);
		sort_asc.pressed = labellist_stats_t::sortreverse;
		sort_desc.pressed = !labellist_stats_t::sortreverse;
	}
	else if (comp == &filter) {
		labellist_stats_t::filter = !labellist_stats_t::filter;
		fill_list();
	}
	return true;
}


void labellist_frame_t::draw(scr_coord pos, scr_size size)
{
	if(  count_label() != (uint32)scrolly.get_count()  ) {
		fill_list();
	}

	gui_frame_t::draw(pos, size);
}
