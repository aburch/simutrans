/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factorylist_frame_t.h"
#include "gui_theme.h"
#include "../dataobj/translator.h"

bool factorylist_frame_t::sortreverse = false;

factorylist::sort_mode_t factorylist_frame_t::sortby = factorylist::by_name;
static uint8 default_sortmode = 0;

bool factorylist_frame_t::display_operation_stats = false;
// filter by within current player's network
bool factorylist_frame_t::filter_own_network = false;
uint8 factorylist_frame_t::filter_goods_catg = goods_manager_t::INDEX_NONE;

const char *factorylist_frame_t::sort_text[factorylist::SORT_MODES] = {
	"Fabrikname",
	"Input",
	"Output",
	"Produktion",
	"Rating",
	"Power",
	"Sector",
	"Staffing",
	"Operation rate",
	"by_region"
};

factorylist_frame_t::factorylist_frame_t() :
	gui_frame_t( translator::translate("fl_title") ),
	stats(sortby,sortreverse, filter_own_network, filter_goods_catg),
	scrolly(&stats)
{
	set_table_layout(1, 0);
	add_table(2, 2);
	{
		new_component<gui_label_t>("hl_txt_sort"); // (1,1)

		filter_within_network.init(button_t::square_state, "Within own network");
		filter_within_network.set_tooltip("Show only connected to own transport network");
		filter_within_network.add_listener(this);
		filter_within_network.pressed = filter_own_network;
		add_component(&filter_within_network); // (1,2)

		add_table(4, 1);
		{
			for (int i = 0; i < factorylist::SORT_MODES; i++) {
				sortedby.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(sort_text[i]), SYSCOL_TEXT);
			}
			sortedby.set_selection(default_sortmode);
			sortedby.set_width_fixed(true);
			sortedby.set_size(scr_size(D_BUTTON_WIDTH*1.5, D_EDIT_HEIGHT));
			sortedby.add_listener(this);
			add_component(&sortedby); // (2,1,1)

			// sort ascend/descend button
			sort_asc.init(button_t::arrowup_state, "");
			sort_asc.set_tooltip(translator::translate("hl_btn_sort_asc"));
			sort_asc.add_listener(this);
			sort_asc.pressed = sortreverse;
			add_component(&sort_asc); // (2,1,2)

			sort_desc.init(button_t::arrowdown_state, "");
			sort_desc.set_tooltip(translator::translate("hl_btn_sort_desc"));
			sort_desc.add_listener(this);
			sort_desc.pressed = !sortreverse;
			add_component(&sort_desc); // (2,1,3)
			new_component<gui_margin_t>(LINESPACE); // (2,1,4)
		}
		end_table(); // (2,1)

		add_table(2, 1);
		{
			viewable_freight_types.append(NULL);
			freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate("All"), SYSCOL_TEXT);
			for (int i = 0; i < goods_manager_t::get_max_catg_index(); i++) {
				const goods_desc_t *freight_type = goods_manager_t::get_info_catg(i);
				const int index = freight_type->get_catg_index();
				if (index == goods_manager_t::INDEX_NONE || freight_type->get_catg() == 0) {
					continue;
				}
				freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(freight_type->get_catg_name()), SYSCOL_TEXT);
				viewable_freight_types.append(freight_type);
			}
			for (int i = 0; i < goods_manager_t::get_count(); i++) {
				const goods_desc_t *ware = goods_manager_t::get_info(i);
				if (ware->get_catg() == 0 && ware->get_index() > 2) {
					// Special freight: Each good is special
					viewable_freight_types.append(ware);
					freight_type_c.new_component<gui_scrolled_list_t::const_text_scrollitem_t>(translator::translate(ware->get_name()), SYSCOL_TEXT);
				}
			}
			freight_type_c.set_selection((filter_goods_catg == goods_manager_t::INDEX_NONE) ? 0 : filter_goods_catg);
			set_filter_goods_catg(filter_goods_catg);

			freight_type_c.add_listener(this);
			add_component(&freight_type_c); // (2,2,1)

			btn_display_mode.init(button_t::roundbox, translator::translate(display_operation_stats ? "fl_btn_operation" : "fl_btn_storage"), scr_coord(BUTTON4_X, 14), D_BUTTON_SIZE);
			btn_display_mode.add_listener(this);
			add_component(&btn_display_mode); // (2,2,2)
		}
		end_table(); // (2,2)

	}
	end_table(); // (2,0)

	scrolly.set_scroll_amount_y(LINESPACE+1);
	add_component(&scrolly); // (3,0)

	display_list();

	scrolly.set_maximize(true);
	reset_min_windowsize();
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, get_min_windowsize().h));
	set_resizemode(diagonal_resize);
}



/**
 * This method is called if an action is triggered
 */
bool factorylist_frame_t::action_triggered( gui_action_creator_t *comp,value_t /* */)
{
	if(comp == &sortedby) {
		int tmp = sortedby.get_selection();
		if (tmp >= 0 && tmp < sortedby.count_elements())
		{
			sortedby.set_selection(tmp);
			set_sortierung((factorylist::sort_mode_t)tmp);
		}
		else {
			sortedby.set_selection(0);
			set_sortierung(factorylist::by_name);
		}
		default_sortmode = (uint8)tmp;
		display_list();
	}
	else if (comp == &sort_asc || comp == &sort_desc) {
		set_reverse(!get_reverse());
		display_list();
		sort_asc.pressed = sortreverse;
		sort_desc.pressed = !sortreverse;
	}
	else if (comp == &filter_within_network) {
		filter_own_network = !filter_own_network;
		filter_within_network.pressed = filter_own_network;
		display_list();
	}
	else if (comp == &btn_display_mode) {
		display_operation_stats = !display_operation_stats;
		btn_display_mode.pressed = display_operation_stats;
		btn_display_mode.set_text(translator::translate(display_operation_stats ? "fl_btn_operation" : "fl_btn_storage"));
		stats.display_operation_stats = display_operation_stats;
	}
	else if (comp == &freight_type_c) {
		if (freight_type_c.get_selection() > 0) {
			filter_goods_catg = viewable_freight_types[freight_type_c.get_selection()]->get_catg_index();
		}
		else if (freight_type_c.get_selection() == 0) {
			filter_goods_catg = goods_manager_t::INDEX_NONE;
		}
		display_list();
	}
	return true;
}


/*
void factorylist_frame_t::fill_list()
{
	scrolly.clear_elements();
	FOR(const slist_tpl<fabrik_t *>,fab,world()->get_fab_list()) {
		scrolly.new_component<factorylist_stats_t>(fab) ;
	}
	scrolly.sort(0);
	scrolly.set_size(scrolly.get_size());
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, min(scrolly.get_pos().y + scrolly.get_size().h + D_MARGIN_BOTTOM, D_DEFAULT_HEIGHT)));
	set_dirty();
	resize(scr_size(0, 0));
}
*/

void factorylist_frame_t::display_list()
{
	stats.sort(sortby, get_reverse(), get_filter_own_network(), filter_goods_catg);
	stats.recalc_size();
/*
	if(  world()->get_fab_list().get_count() != (uint32)scrolly.get_count()  ) {
		fill_list();
	}

	set_dirty();
	resize(scr_size(0, 0));

	gui_frame_t::draw(pos,size);
*/
}
