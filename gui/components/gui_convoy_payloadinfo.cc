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
		const bool overcrowded = cnv->get_overcrowded() ? true : false;

		for (uint8 catg_index = 0; catg_index < goods_manager_t::get_max_catg_index(); catg_index++)
		{
			if (!convoy.get_goods_catg_index().is_contained(catg_index)) {
				continue;
			}
			buf.clear();
			uint32 capacity = 0;
			uint32 cargo_sum = 0;
			if (catg_index == goods_manager_t::INDEX_PAS || catg_index == goods_manager_t::INDEX_MAIL)
			{
				const uint8 classes = catg_index == goods_manager_t::INDEX_PAS ? goods_manager_t::passengers->get_number_of_classes() : goods_manager_t::mail->get_number_of_classes();
				for (uint8 i = 0; i < classes; i++) {
					capacity = cnv->get_unique_fare_capacity(catg_index, i);
					cargo_sum = cnv->get_total_cargo_by_fare_class(catg_index, i);

					if (capacity) {
						buf.clear();
						display_color_img(goods_manager_t::get_info_catg_index(catg_index)->get_catg_symbol(), offset.x + left, offset.y, 0, false, false);
						left += 12;
						// [class name]
						buf.append(goods_manager_t::get_translated_wealth_name(catg_index, i));

						buf.printf(" %i/%i", cargo_sum, capacity);
						left += display_proportional_clip(offset.x + left, offset.y, buf, ALIGN_LEFT, overcrowded ? COL_OVERCROWD - 1 : SYSCOL_TEXT, true);
						left += D_H_SPACE * 2;
					}
				}

				if (cnv->get_overcrowded_capacity() && catg_index == goods_manager_t::INDEX_PAS) {
					buf.clear();
					if (skinverwaltung_t::pax_evaluation_icons) {
						if (cnv->get_overcrowded() > 0) {
							display_color_img_with_tooltip(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), offset.x + left, offset.y, 0, false, false, translator::translate("Standing passengers"));
						}
						else {
							display_img_blend(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), offset.x + left, offset.y, TRANSPARENT50_FLAG | OUTLINE_FLAG | COL_GREY2, false, false);
						}
						left += 14;
					}
					buf.printf("%i/%i", cnv->get_overcrowded(), cnv->get_overcrowded_capacity());
					left += display_proportional_clip(offset.x + left, offset.y, buf, ALIGN_LEFT, overcrowded ? COL_OVERCROWD - 1 : SYSCOL_TEXT, true);
					left += D_H_SPACE * 2;
				}
			}
			else {
				display_color_img(goods_manager_t::get_info_catg_index(catg_index)->get_catg_symbol(), offset.x + left, offset.y, 0, false, false);
				left += 14;
				capacity = cnv->get_unique_fare_capacity(catg_index, 0);
				cargo_sum = cnv->get_total_cargo_by_fare_class(catg_index, 0);
				buf.printf("%i/%i", cargo_sum, capacity);
				left += display_proportional_clip(offset.x + left, offset.y, buf, ALIGN_LEFT, overcrowded ? COL_OVERCROWD - 1 : SYSCOL_TEXT, true);
				left += D_H_SPACE * 2;
			}
		}

		scr_size size(left + D_MARGIN_RIGHT, LINESPACE + D_H_SPACE + 2);
		if (get_size().w+1==0) {
			set_size(size);
		}
	}
}
