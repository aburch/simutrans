/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "labellist_frame.h"
#include "labellist_stats.h"

#include "../dataobj/translator.h"
#include "../obj/label.h"
#include "../world/simworld.h"

char labellist_frame_t::name_filter[256];

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

	add_table(3, 3);
	{
		new_component<gui_label_t>("Filter:");
		name_filter_input.set_text(name_filter, lengthof(name_filter));
		add_component(&name_filter_input);
		new_component<gui_fill_t>();

		filter.init( button_t::square_automatic, "Active player only");
		filter.pressed = labellist_stats_t::filter;
		add_component(&filter, 2);
		filter.add_listener( this );
		new_component<gui_fill_t>();

		new_component<gui_label_t>("hl_txt_sort");
		sortedby.set_unsorted(); // do not sort
		for (size_t i = 0; i < lengthof(sort_text); i++) {
			sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
		}
		sortedby.set_selection(labellist_stats_t::sortby);
		sortedby.add_listener(this);
		add_component(&sortedby);

		sorteddir.init(button_t::sortarrow_state, NULL);
		sorteddir.add_listener(this);
		sorteddir.pressed = labellist_stats_t::sortreverse;
		add_component(&sorteddir);

		new_component<gui_fill_t>();
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
	strcpy(last_name_filter, name_filter);
	label_count = welt->get_label_list().get_count();

	scrolly.clear_elements();
	for(koord const& pos : welt->get_label_list()) {
		label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		const char* name = welt->lookup_kartenboden(pos)->get_text();
		// some old version games don't have label nor name.
		// Check them to avoid crashes.
		if(label  &&  name  &&  (!labellist_stats_t::filter  ||  (label  &&  (label->get_owner() == welt->get_active_player())))) {
			if(  name_filter[0] == 0  ||  utf8caseutf8(name, name_filter)  ) {
				scrolly.new_component<labellist_stats_t>(pos);
			}
		}
	}
	scrolly.sort(0);
	reset_min_windowsize();
}


uint32 labellist_frame_t::count_label()
{
	uint32 labelcount = 0;
	for(koord const& pos : welt->get_label_list()) {
		label_t* label = welt->lookup_kartenboden(pos)->find<label_t>();
		const char* name = welt->lookup_kartenboden(pos)->get_text();
		// some old version games don't have label nor name.
		if(label  &&  name  &&  (!labellist_stats_t::filter  ||  (label  &&  (label->get_owner() == welt->get_active_player())))) {
			labelcount++;
		}
	}
	return labelcount;
}


bool labellist_frame_t::action_triggered( gui_action_creator_t *comp,value_t v)
{
	if(comp == &sortedby) {
		labellist_stats_t::sortby = (labellist::sort_mode_t)v.i;
		scrolly.sort(0);
	}
	else if(comp == &sorteddir) {
		labellist_stats_t::sortreverse = !labellist_stats_t::sortreverse;
		sorteddir.pressed = labellist_stats_t::sortreverse;
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
	if(  label_count != welt->get_label_list().get_count()  ||  strcmp(last_name_filter, name_filter)  ) {
		fill_list();
	}
	gui_frame_t::draw(pos, size);
}


void labellist_frame_t::rdwr(loadsave_t* file)
{
	scr_size size = get_windowsize();
	sint16 sort_mode = labellist_stats_t::sortby;

	size.rdwr(file);
	scrolly.rdwr(file);
	file->rdwr_str(name_filter, lengthof(name_filter));
	file->rdwr_short(sort_mode);
	file->rdwr_bool(labellist_stats_t::sortreverse);
	file->rdwr_bool(labellist_stats_t::filter);
	if (file->is_loading()) {
		labellist_stats_t::sortby = (labellist::sort_mode_t)sort_mode;
		fill_list();
		set_windowsize(size);
	}
}
