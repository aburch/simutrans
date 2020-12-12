#include <stdio.h>

#include "gui_convoy_formation.h"

#include "../../simconvoi.h"
#include "../../vehicle/simvehicle.h"

#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../simworld.h"

#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"

#include "../../player/simplay.h"


const char *gui_convoy_formation_t::cnvlist_mode_button_texts[CONVOY_OVERVIEW_MODES] = {
	"cl_btn_general",
	"cl_btn_payload",
	"cl_btn_formation"
};


// component for vehicle display
gui_convoy_formation_t::gui_convoy_formation_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}

void gui_convoy_formation_t::draw(scr_coord offset)
{
	if (cnv.is_bound()) {
		switch (mode)
		{
			case appearance:
				size = draw_vehicles(offset, true);
				break;
			case capacities:
				size = draw_capacities(offset);
				break;
			case formation:
			default:
				size = draw_formation(offset);
				break;
		}

		if (size != get_size()) {
			set_size(size);
		}
	}
}


scr_size gui_convoy_formation_t::draw_formation(scr_coord offset) const
{
	offset.y += 2; // margin top
	offset.x += D_MARGIN_LEFT;
	karte_t *welt = world();
	const uint16 month_now = welt->get_timeline_year_month();
	const uint16 grid_width = D_BUTTON_WIDTH / 3;

	uint8 color;
	cbuffer_t buf;

	for (unsigned veh = 0; veh < cnv->get_vehicle_count(); ++veh) {
		buf.clear();
		vehicle_t *v = cnv->get_vehicle(veh);

		// set the loading indicator color
		if (v->get_number_of_accommodation_classes()) {
			int bar_offset_left = 0;
			int bar_width = (grid_width - 3) / v->get_number_of_accommodation_classes() - 1;

			// drawing the color bar
			int found = 0;
			for (int i = 0; i < v->get_desc()->get_number_of_classes(); i++) {
				if (v->get_accommodation_capacity(i) > 0) {
					color = COL_GREEN;
					if (!v->get_total_cargo_by_class(i)) {
						color = COL_YELLOW; // empty
					}
					else if (v->get_fare_capacity(v->get_reassigned_class(i)) == v->get_total_cargo_by_class(i)) {
						color = COL_ORANGE;
					}
					else if (v->get_fare_capacity(v->get_reassigned_class(i)) < v->get_total_cargo_by_class(i)) {
						color = COL_OVERCROWD;
					}
					// [loading indicator]
					display_fillbox_wh_clip_rgb(offset.x + 2 + bar_offset_left, offset.y + LINESPACE + VEHICLE_BAR_HEIGHT + 3, bar_width, 3, color_idx_to_rgb(color), true);
					bar_offset_left += bar_width + 1;
					found++;
					if (found == v->get_number_of_accommodation_classes()) {
						break;
					}
				}
			}

			// [goods symbol]
			// assume that the width of the goods symbol is 10px
			display_color_img(v->get_cargo_type()->get_catg_symbol(), offset.x + grid_width / 2 - 5, offset.y + LINESPACE + VEHICLE_BAR_HEIGHT + 8, 0, false, false);
		}
		else {
			// [loading indicator]
			display_fillbox_wh_clip_rgb(offset.x + 2, offset.y + LINESPACE + VEHICLE_BAR_HEIGHT + 3, grid_width - 4, 3, color_idx_to_rgb(MN_GREY0), true);
		}

		// [cars no.]
		// cars number in this convoy
		sint8 car_number = cnv->get_car_numbering(veh);
		if (car_number < 0) {
			buf.printf("%.2s%d", translator::translate("LOCO_SYM"), abs(car_number)); // This also applies to horses and tractors and push locomotives.
		}
		else {
			buf.append(car_number);
		}

		int left = display_proportional_clip_rgb(offset.x + 2, offset.y, buf, ALIGN_LEFT, v->get_desc()->has_available_upgrade(month_now) ? COL_UPGRADEABLE : color_idx_to_rgb(COL_GREY2), true);
#ifdef DEBUG
		if (v->is_reversed()) {
			display_proportional_clip_rgb(offset.x + 2 + left, offset.y - 2, "*", ALIGN_LEFT, COL_CAUTION, true);
		}
		if (!v->get_desc()->is_bidirectional()) {
			display_proportional_clip_rgb(offset.x + 2 + left, offset.y - 2, "<", ALIGN_LEFT, color_idx_to_rgb(COL_LIGHT_TURQUOISE), true);
		}
#else
		(void)left;
#endif

		PIXVAL col_val = v->get_desc()->is_future(month_now) || v->get_desc()->is_retired(month_now) ? COL_OUT_OF_PRODUCTION : COL_SAFETY;
		if (v->get_desc()->is_obsolete(month_now, welt)) {
			col_val = COL_OBSOLETE;
		}
		display_veh_form_wh_clip_rgb(offset.x + 2, offset.y + LINESPACE, (grid_width-6)/2, col_val, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_next() : v->get_desc()->get_basic_constraint_prev(), v->get_desc()->get_interactivity(), false);
		display_veh_form_wh_clip_rgb(offset.x + (grid_width-6)/2 + 2, offset.y + LINESPACE, (grid_width-6)/2, col_val, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_prev() : v->get_desc()->get_basic_constraint_next(), v->get_desc()->get_interactivity(), true);

		offset.x += grid_width;
	}

	scr_size size(grid_width*cnv->get_vehicle_count() + D_MARGIN_LEFT * 2, LINESPACE + VEHICLE_BAR_HEIGHT + 10 + D_SCROLLBAR_HEIGHT);
	return size;
}

scr_size gui_convoy_formation_t::draw_vehicles(scr_coord offset, bool display_images) const
{
	scr_coord p = offset + get_pos();
	p.y += get_size().h/2;
	// we will use their images offsets and width to shift them to their correct position
	// this should work with any vehicle size ...
	scr_size s(D_H_SPACE*2, D_V_SPACE*2);
	for(unsigned i=0; i<cnv->get_vehicle_count();i++) {
		scr_coord_val x, y, w, h;
		const image_id image = cnv->get_vehicle(i)->get_loaded_image();
		display_get_base_image_offset(image, &x, &y, &w, &h );
		if (display_images) {
			display_base_img(image, p.x + s.w - x, p.y - y - h/2, cnv->get_owner()->get_player_nr(), false, true);
		}
		s.w += (w*2)/3;
		s.h = max(s.h, h);
	}
	return s;
}

scr_size gui_convoy_formation_t::draw_capacities(scr_coord offset) const
{
	uint16 top = 0;
	uint16 left = 0;
	if (cnv.is_bound()) {
		convoi_t &convoy = *cnv.get_rep();

		offset.y += 2; // margin top
		offset.x += D_MARGIN_LEFT;
		uint16 size_x = 0;
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
						display_color_img(goods_manager_t::get_info_catg_index(catg_index)->get_catg_symbol(), offset.x + left, offset.y + top + FIXED_SYMBOL_YOFF, 0, false, false);
						left += 12;
						// [class name]
						buf.append(goods_manager_t::get_translated_wealth_name(catg_index, i));

						buf.printf(" %i/%i", cargo_sum, capacity);
						left += display_proportional_clip_rgb(offset.x + left, offset.y + top, buf, ALIGN_LEFT, text_col, true);
						left += D_H_SPACE * 2;
					}
				}

				if (cnv->get_overcrowded_capacity() && catg_index == goods_manager_t::INDEX_PAS) {
					buf.clear();
					if (skinverwaltung_t::pax_evaluation_icons) {
						if (cnv->get_overcrowded() > 0) {
							display_color_img_with_tooltip(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), offset.x + left, offset.y + top + FIXED_SYMBOL_YOFF, 0, false, false, translator::translate("Standing passengers"));
						}
						else {
							display_img_blend(skinverwaltung_t::pax_evaluation_icons->get_image_id(1), offset.x + left + top, offset.y + top + FIXED_SYMBOL_YOFF, TRANSPARENT50_FLAG | OUTLINE_FLAG | color_idx_to_rgb(COL_GREY2), false, false);
						}
						left += 14;
					}
					buf.printf("%i/%i", cnv->get_overcrowded(), cnv->get_overcrowded_capacity());
					left += display_proportional_clip_rgb(offset.x + left, offset.y + top, buf, ALIGN_LEFT, text_col, true);
					left += D_H_SPACE * 2;
				}
				top += LINESPACE;
				size_x = max(size_x, left);
				left = 0;
			}
			else {
				display_color_img(goods_manager_t::get_info_catg_index(catg_index)->get_catg_symbol(), offset.x + left, offset.y + top + FIXED_SYMBOL_YOFF, 0, false, false);
				left += 14;
				capacity = cnv->get_unique_fare_capacity(catg_index, 0);
				cargo_sum = cnv->get_total_cargo_by_fare_class(catg_index, 0);
				buf.printf("%i/%i", cargo_sum, capacity);
				left += display_proportional_clip_rgb(offset.x + left, offset.y + top, buf, ALIGN_LEFT, text_col, true);
				left += D_H_SPACE * 2;
			}
		}
	}
	scr_size size(left, top+LINESPACE);
	return size;
}
