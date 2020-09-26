/*
 * This file is part of the Simutrans project under the Artistic License.
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
	"player"
};

labellist_frame_t::labellist_frame_t() :
	gui_frame_t( translator::translate("labellist_title") ),
	scrolly(gui_scrolled_list_t::windowskin, labellist_stats_t::compare)
{
	set_table_layout(1,0);
	new_component<gui_label_t>("hl_txt_sort");

	add_table(3,1);
	{
		sortedby.init(button_t::roundbox, sort_text[labellist_stats_t::sortby]);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::roundbox, labellist_stats_t::sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		sorteddir.add_listener(this);
		add_component(&sorteddir);

		filter.init( button_t::square_automatic, "Active player only");
		filter.pressed = labellist_stats_t::filter;
		add_component(&filter);
		filter.add_listener( this );
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
bool labellist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		labellist_stats_t::sortby = (labellist::sort_mode_t)( (labellist_stats_t::sortby + 1) % labellist::SORT_MODES);
		sortedby.set_text(sort_text[labellist_stats_t::sortby]);
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		labellist_stats_t::sortreverse = !labellist_stats_t::sortreverse;
		sorteddir.set_text(labellist_stats_t::sortreverse ? "hl_btn_sort_desc" : "hl_btn_sort_asc");
		scrolly.sort(0);
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
