/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "goods_stats_t.h"

#include "../simcolor.h"
#include "../simworld.h"

#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"
#include "../player/simplay.h"

#include "../dataobj/environment.h"
#include "../dataobj/translator.h"

#include "components/gui_button.h"
#include "components/gui_colorbox.h"
#include "components/gui_combobox.h"
#include "components/gui_label.h"

karte_ptr_t goods_stats_t::welt;


void goods_stats_t::update_goodslist(vector_tpl<const goods_desc_t*>goods, int bonus)
{
	goods_catg_indexes.clear();
	scr_size size = get_size();
	remove_all();
	set_table_layout(7,0);

	FOR(vector_tpl<const goods_desc_t*>, wtyp, goods) {

		new_component<gui_colorbox_t>(wtyp->get_color())->set_max_size(scr_size(D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT));
		new_component<gui_label_t>(wtyp->get_name());

		const sint32 grundwert128 = (sint32)wtyp->get_value() * welt->get_settings().get_bonus_basefactor(); // bonus price will be always at least this
		const sint32 grundwert_bonus = (sint32)wtyp->get_value()*(1000l+(bonus-100l)*wtyp->get_speed_bonus());
		const sint32 price = (grundwert128>grundwert_bonus ? grundwert128 : grundwert_bonus);

		gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
		lb->buf().append_money(price/300000.0);
		lb->update();

		lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
		lb->buf().printf("%d%%", wtyp->get_speed_bonus());
		lb->update();

		new_component<gui_label_t>(wtyp->get_catg_name());

		lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
		lb->buf().printf("%dKg", wtyp->get_weight_per_unit());
		lb->update();

		const uint8 goods_catg_index = wtyp->get_catg_index();
		gui_combobox_t* routing_box = new_component<gui_combobox_t>();
		goods_catg_indexes.set(routing_box, goods_catg_index);
		routing_box->add_listener(this);
		routing_box->new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "route by route cost" ), SYSCOL_TEXT );
		routing_box->new_component<gui_scrolled_list_t::const_text_scrollitem_t>( translator::translate( "route by journey time" ), SYSCOL_TEXT );
		routing_box->set_selection(welt->get_settings().get_time_based_routing_enabled(goods_catg_index));
	}

	scr_size min_size = get_min_size();
	set_size(scr_size(max(size.w, min_size.w), min_size.h) );
}


bool goods_stats_t::action_triggered(gui_action_creator_t* cmp, value_t value) {
	uint8* goods_catg_index = goods_catg_indexes.access(cmp);
	if(  !goods_catg_index  ) {
		return true;
	}
	const bool is_tbgr_enabled = (bool)value.i;
	welt->get_settings().set_time_based_routing_enabled(*goods_catg_index, is_tbgr_enabled);
	return true;
}


void goods_stats_t::draw(scr_coord offset) {
	const bool is_routing_selection_enabled = (welt->get_active_player()->is_public_service() && !env_t::networkmode);
	for(auto iter = goods_catg_indexes.begin(); iter != goods_catg_indexes.end(); ++iter) {
		gui_combobox_t* box = static_cast<gui_combobox_t*>(iter->key);
		is_routing_selection_enabled ? box->enable() : box->disable();
		const uint8 goods_catg_index = iter->value;
		box->set_selection(welt->get_settings().get_time_based_routing_enabled(goods_catg_index));
	}
	gui_aligned_container_t::draw(offset);
}
