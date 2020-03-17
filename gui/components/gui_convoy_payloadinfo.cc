#include "gui_convoy_payloadinfo.h"
#include <algorithm>

#include "../../vehicle/simvehicle.h"
#include "../../simconvoi.h"
#include "../../simcolor.h"
#include "../../utils/cbuffer_t.h"

#include "../gui_frame.h"


gui_convoy_payloadinfo_t::gui_convoy_payloadinfo_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}
void gui_convoy_payloadinfo_t::draw(scr_coord offset)
{
	if (cnv.is_bound()) {
		convoi_t &convoy = *cnv.get_rep();
		offset.y += 2;
		offset.x += D_MARGIN_LEFT;
		int left = 0;
		cbuffer_t buf;
		for (uint8 catg_index = 0; catg_index < goods_manager_t::get_max_catg_index(); catg_index++)
		{
			if (!convoy.get_goods_catg_index().is_contained(catg_index)) {
				continue;
			}
			buf.clear();
			display_color_img(goods_manager_t::get_info_catg_index(catg_index)->get_catg_symbol(), offset.x + left, offset.y, 0, false, false);
			left += 14;
			uint32 overcrowded_capacity = 0;
			uint32 capacity = 0;
			uint32 cargo_sum = 0;
			bool overcrowded = false;
			for (uint veh = 0; veh < cnv->get_vehicle_count(); ++veh) {
				uint8 classes = 1;
				vehicle_t* v = cnv->get_vehicle(veh);
				if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS || v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
				{
					classes = goods_manager_t::INDEX_PAS ? goods_manager_t::passengers->get_number_of_classes() : goods_manager_t::mail->get_number_of_classes();
				}

				if (catg_index == v->get_cargo_type()->get_catg_index()) {
					for (uint8 i = 0; i < classes; i++) {
						cargo_sum += v->get_total_cargo_by_class(i);
						capacity += v->get_fare_capacity(v->get_reassigned_class(i));
						if (v->get_fare_capacity(v->get_reassigned_class(i)) < v->get_total_cargo_by_class(i)) {
							overcrowded = true;
						}
					}
					overcrowded_capacity += v->get_desc()->get_overcrowded_capacity();
				}
			}
			buf.printf("%i/%i", cargo_sum, capacity);
			if (overcrowded_capacity && catg_index == goods_manager_t::INDEX_PAS) {
				buf.printf("(%i)", overcrowded_capacity);
			}
			left += display_proportional_clip(offset.x + left, offset.y, buf, ALIGN_LEFT, overcrowded ? COL_OVERCROWD-1 : SYSCOL_TEXT, true);
			left += D_H_SPACE*2;
		}

		scr_size size(left + D_MARGIN_RIGHT, LINESPACE + D_H_SPACE + 2);
		if (get_size().w+1==0) {
			set_size(size);
		}
	}
}
