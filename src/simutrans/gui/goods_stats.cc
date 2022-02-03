/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "goods_stats.h"

#include "../simcolor.h"
#include "../world/simworld.h"

#include "../builder/goods_manager.h"
#include "../descriptor/goods_desc.h"

#include "../dataobj/translator.h"

#include "components/gui_button.h"
#include "components/gui_colorbox.h"
#include "components/gui_label.h"

karte_ptr_t goods_stats_t::welt;

void goods_stats_t::update_goodslist(vector_tpl<const goods_desc_t*>goods, int bonus)
{
	scr_size size = get_size();
	remove_all();
	set_table_layout(6,0);

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
	}

	scr_size min_size = get_min_size();
	set_size(scr_size(max(size.w, min_size.w), min_size.h) );
}
