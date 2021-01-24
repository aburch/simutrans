#include "gui_convoy_payloadinfo.h"
#include <algorithm>

#include "../../vehicle/simvehicle.h"
#include "../../simconvoi.h"
#include "../../simcolor.h"
#include "../../utils/cbuffer_t.h"

#include "../gui_frame.h"
#include "../../display/simgraph.h"


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
		uint16 left = 0;
		cbuffer_t buf;
		const bool overcrowded = cnv->get_overcrowded() ? true : false;
		PIXVAL text_col = overcrowded ? color_idx_to_rgb(COL_OVERCROWD - 1) : SYSCOL_TEXT;

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
						left += display_proportional_clip_rgb(offset.x + left, offset.y, buf, ALIGN_LEFT, text_col, true);
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
							display_img_blend(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), offset.x + left, offset.y, TRANSPARENT50_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_GREY2), false, false);
						}
						left += 14;
					}
					buf.printf("%i/%i", cnv->get_overcrowded(), cnv->get_overcrowded_capacity());
					left += display_proportional_clip_rgb(offset.x + left, offset.y, buf, ALIGN_LEFT, text_col, true);
					left += D_H_SPACE * 2;
				}
			}
			else {
				display_color_img(goods_manager_t::get_info_catg_index(catg_index)->get_catg_symbol(), offset.x + left, offset.y, 0, false, false);
				left += 14;
				capacity = cnv->get_unique_fare_capacity(catg_index, 0);
				cargo_sum = cnv->get_total_cargo_by_fare_class(catg_index, 0);
				buf.printf("%i/%i", cargo_sum, capacity);
				left += display_proportional_clip_rgb(offset.x + left, offset.y, buf, ALIGN_LEFT, text_col, true);
				left += D_H_SPACE * 2;
			}
		}

		scr_size size(left + D_MARGIN_RIGHT, LINESPACE + D_H_SPACE + 2);
		if (get_size().w+1==0) {
			set_size(size);
		}
	}
}

static karte_ptr_t welt;

gui_loadingbar_t::gui_loadingbar_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}

void gui_loadingbar_t::draw(scr_coord offset)
{
	if (!cnv.is_bound()) {
		return;
	}
	offset += pos;
	sint64 waiting_time_per_month = 0;
	uint8 waiting_bar_col = COL_APRICOT;
	switch (cnv->get_state()) {
		case convoi_t::LOADING:
			waiting_time_per_month = int(0.9 + cnv->calc_remaining_loading_time() * 200 / welt->ticks_per_world_month);
			break;
		case convoi_t::REVERSING:
			waiting_time_per_month = int(0.9 + cnv->get_wait_lock() * 200 / welt->ticks_per_world_month);
			break;
		case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
		case convoi_t::CAN_START_TWO_MONTHS:
			waiting_time_per_month = 100;
			waiting_bar_col = COL_ORANGE;
			//if (skinverwaltung_t::alerts) {
			//	display_color_img(skinverwaltung_t::alerts->get_image_id(3), xpos - 15, ypos - D_FIXED_SYMBOL_WIDTH, 0, false, true);
			//}
			break;
		case convoi_t::NO_ROUTE:
		case convoi_t::NO_ROUTE_TOO_COMPLEX:
		case convoi_t::OUT_OF_RANGE:
			waiting_time_per_month = 100;
			waiting_bar_col = COL_RED + 1;
			//if (skinverwaltung_t::pax_evaluation_icons) {
			//	display_color_img(skinverwaltung_t::pax_evaluation_icons->get_image_id(4), xpos - 15, ypos - D_FIXED_SYMBOL_WIDTH, 0, false, true);
			//}
			break;
		default:
			break;
	}

	display_ddd_box_clip_rgb(offset.x, offset.y, size.w, LOADINGBAR_HEIGHT, color_idx_to_rgb(MN_GREY2), color_idx_to_rgb(MN_GREY0));
	sint32 colored_width = cnv->get_loading_level() > 100 ? size.w-2 : cnv->get_loading_level()*(size.w-2)/100;
	const scr_coord_val bg_width = cnv->get_loading_limit()*(size.w-2)/100;

	if (bg_width>0 && cnv->get_state() == convoi_t::LOADING) {
		display_fillbox_wh_clip_rgb(offset.x+1, offset.y+1, bg_width, LOADINGBAR_HEIGHT - 2, COL_IN_TRANSIT, true);
	}
	else if (bg_width > 0) {
		display_blend_wh_rgb(offset.x+1, offset.y+1, bg_width, LOADINGBAR_HEIGHT - 2, COL_IN_TRANSIT, 60);
	}
	else {
		display_blend_wh_rgb(offset.x+1, offset.y+1, size.w-2, LOADINGBAR_HEIGHT - 2, color_idx_to_rgb(MN_GREY2), colored_width ? 65 : 40);
	}
	display_cylinderbar_wh_clip_rgb(offset.x + 1, offset.y + 1, colored_width, LOADINGBAR_HEIGHT - 2, color_idx_to_rgb(COL_GREEN - 1), true);

	// winting gauge
	if (waiting_time_per_month) {
		colored_width = waiting_time_per_month > 100 ? size.w - 2 : waiting_time_per_month * (size.w - 2) / 100;
		display_ddd_box_clip_rgb(offset.x, offset.y+LOADINGBAR_HEIGHT, colored_width + 2, WAITINGBAR_HEIGHT, color_idx_to_rgb(MN_GREY2), color_idx_to_rgb(MN_GREY0));
		display_cylinderbar_wh_clip_rgb(offset.x+1, offset.y+LOADINGBAR_HEIGHT+1, colored_width, WAITINGBAR_HEIGHT - 2, color_idx_to_rgb(waiting_bar_col), true);
		if (waiting_time_per_month > 100) {
			colored_width = waiting_time_per_month - (size.w - 2);
			display_cylinderbar_wh_clip_rgb(offset.x+1, offset.y+LOADINGBAR_HEIGHT+1, colored_width, WAITINGBAR_HEIGHT - 2, color_idx_to_rgb(waiting_bar_col-2), true);
		}
	}
}

scr_size gui_loadingbar_t::get_min_size() const
{
	return scr_size(LOADINGBAR_WIDTH + 2, LOADINGBAR_HEIGHT + WAITINGBAR_HEIGHT);
}

scr_size gui_loadingbar_t::get_max_size() const
{
	return scr_size(size_fixed ? LOADINGBAR_WIDTH+2 : scr_size::inf.w, LOADINGBAR_HEIGHT + WAITINGBAR_HEIGHT);
}
