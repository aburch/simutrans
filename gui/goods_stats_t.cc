/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "goods_stats_t.h"

#include "../simcolor.h"
#include "../simworld.h"
#include "../simconvoi.h"

#include "../bauer/goods_manager.h"
#include "../descriptor/goods_desc.h"

#include "../dataobj/translator.h"

#include "../descriptor/goods_desc.h"
#include "gui_frame.h"
#include "components/gui_button.h"
#include "components/gui_colorbox.h"
#include "components/gui_label.h"

#include "components/gui_image.h"

karte_ptr_t goods_stats_t::welt;

void goods_stats_t::update_goodslist(vector_tpl<const goods_desc_t*>goods, uint32 b, uint32 d, uint8 c, uint8 ct, uint8 gc)
{
	vehicle_speed = b;
	distance_meters = d;
	comfort = c;
	catering_level = ct;
	g_class = gc;

	scr_size size = get_size();
	remove_all();
	set_table_layout(7, 0);

	FOR(vector_tpl<const goods_desc_t*>, wtyp, goods) {
		new_component<gui_colorbox_t>(wtyp->get_color())->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
		new_component<gui_label_t>(wtyp->get_name());

		// Massively cleaned up by neroden, June 2013
		// Roundoff is deliberate here (get two-digit speed)... question this
		if (vehicle_speed <= 0) {
			// Negative and zero speeds will be due to roundoff errors
			vehicle_speed = 1;
		}

		const sint64 journey_tenths = tenths_from_meters_and_kmh(distance_meters, vehicle_speed);

		sint64 revenue = wtyp->get_total_fare(distance_meters, 0u, comfort, catering_level, min(g_class, wtyp->get_number_of_classes() - 1), journey_tenths);

		// Convert to simucents.  Should be very fast.
		sint64 price = (revenue + 2048) / 4096;

		gui_label_buf_t *lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
		lb->buf().append_money(price / 100.0);
		lb->update();

		new_component<gui_margin_t>(LINESPACE);
		new_component<gui_image_t>(wtyp->get_catg_symbol(), 0, ALIGN_NONE, true);

		new_component<gui_label_t>(wtyp->get_catg_name());

		lb = new_component<gui_label_buf_t>(SYSCOL_TEXT, gui_label_t::right);
		lb->buf().printf("%dKg", wtyp->get_weight_per_unit());
		lb->update();
	}
	scr_size min_size = get_min_size();
	set_size(scr_size(max(size.w, min_size.w), min_size.h));
}
