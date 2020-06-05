/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

//#include <stdio.h>

#include "gui_factory_storage_info.h"

#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../simworld.h"

#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"

#define STORAGE_INDICATOR_WIDTH (50)
//#define STORAGE_INDICATOR_HEIGHT (6)

// Half a display unit (0.5).
static const sint64 FAB_DISPLAY_UNIT_HALF = ((sint64)1 << (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS - 1));

// component for factory storage display
gui_factory_storage_info_t::gui_factory_storage_info_t(fabrik_t* factory)
{
	this->fab = factory;
}

void gui_factory_storage_info_t::draw(scr_coord offset)
{
	//// keep previous maximum width
	//int x_size = get_size().w - 51 - pos.x;
	int left = 0;
	int yoff = 2;
	if (fab) {
		offset.x += D_MARGIN_LEFT;

		cbuffer_t buf;

		const uint32 input_count = fab->get_input().get_count();
		const uint32 output_count = fab->get_output().get_count();

		// input storage info (Consumption)
		if (input_count) {
			// if pakset has symbol, display it
			if (skinverwaltung_t::input_output)
			{
				display_color_img(skinverwaltung_t::input_output->get_image_id(0), pos.x + offset.x, pos.y + offset.y + yoff, 0, false, false);
				left += 12;
			}
			display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, translator::translate("Verbrauch"), ALIGN_LEFT, SYSCOL_TEXT, true);
			yoff += LINESPACE;

			int i = 0;
			FORX(array_tpl<ware_production_t>, const& goods, fab->get_input(), i++) {
				if (!fab->get_desc()->get_supplier(i))
				{
					continue;
				}
				const sint64 pfactor = fab->get_desc()->get_supplier(i) ? (sint64)fab->get_desc()->get_supplier(i)->get_consumption() : 1ll;
				const uint16 max_intransit_percentage = fab->get_max_intransit_percentage(i);
				const sint64 max_transit = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.max_transit * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const uint32 stock_quantity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.menge * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const uint32 storage_capacity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.max * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const COLOR_VAL goods_color = goods.get_typ()->get_color();

				left = 2;
				yoff += 2; // box position adjistment
				// [storage indicator]
				display_ddd_box_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, STORAGE_INDICATOR_WIDTH + 2, 8, MN_GREY0, MN_GREY4);
				display_fillbox_wh_clip(pos.x + offset.x + left + 1, pos.y + offset.y + yoff + 1, STORAGE_INDICATOR_WIDTH, 6, MN_GREY2, true);
				if (storage_capacity) {
					const uint16 colored_width = min(STORAGE_INDICATOR_WIDTH, (uint16)(STORAGE_INDICATOR_WIDTH * stock_quantity / storage_capacity));
					display_fillbox_wh_clip(pos.x + offset.x + left + 1, pos.y + offset.y + yoff + 1, colored_width, 6, goods_color, true);
					if (goods.get_in_transit()) {
						const uint16 intransint_width = min(STORAGE_INDICATOR_WIDTH - colored_width, STORAGE_INDICATOR_WIDTH * (uint16)goods.get_in_transit() / storage_capacity);
						display_fillbox_wh_clip(pos.x + offset.x + left + 1 + colored_width, pos.y + offset.y + yoff + 1, intransint_width, 6, COL_YELLOW, true);
					}
				}
				left += STORAGE_INDICATOR_WIDTH + 2 + D_H_SPACE;

				// [goods color box] This design is the same as the goods list
				display_ddd_box_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, 8, 8, MN_GREY0, MN_GREY4);
				display_fillbox_wh_clip(pos.x + offset.x + left + 1, pos.y + offset.y + yoff + 1, 6, 6, goods_color, true);
				left += 12;
				yoff -= 2; // box position adjistment

				// [goods name]
				buf.clear();
				buf.printf("%s", translator::translate(goods.get_typ()->get_name()));
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

				left += 10;

				// [goods category]
				display_color_img_with_tooltip(goods.get_typ()->get_catg_symbol(), pos.x + offset.x + left, pos.y + offset.y + yoff, 0, false, false, translator::translate(goods.get_typ()->get_catg_name()));
				goods.get_typ()->get_catg_name();
				//->get_catg_name()
				left += 14;

				// [storage capacity]
				buf.clear();
				buf.printf("%u/%u,", stock_quantity, storage_capacity);
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				left += D_H_SPACE;

				// [in transit]
				if (fab->get_status() != fabrik_t::inactive) {
				buf.clear();
					const bool in_transit_over_storage = (stock_quantity + (uint32)goods.get_in_transit() > storage_capacity);
					if (skinverwaltung_t::in_transit) {
						display_color_img_with_tooltip(skinverwaltung_t::in_transit->get_image_id(0), pos.x + offset.x + left, pos.y + offset.y + yoff, 0, false, false, translator::translate("symbol_help_txt_in_transit"));
						left += 14;
					}
					buf.append(goods.get_in_transit(), 0);
					left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					buf.clear();
					if (max_intransit_percentage) {
						buf.printf("/%i", max_transit);
					}
					buf.append(", ");
					left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					left += D_H_SPACE;
				}


				// [monthly production]
				buf.clear();
				const uint32 monthly_prod = (uint32)(fab->get_current_production()*pfactor * 10 >> DEFAULT_PRODUCTION_FACTOR_BITS);
				buf.append((float)monthly_prod / 10.0, monthly_prod < 100 ? 1 : 0);
				buf.printf("%s%s", translator::translate(goods.get_typ()->get_mass()), translator::translate("/month"));
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

				yoff += LINESPACE;
			}
			yoff += LINESPACE;
		}


		// output storage info (Production)
		if (output_count) {
			left = 0;
			// if pakset has symbol, display it
			if(skinverwaltung_t::input_output)
			{
				display_color_img(skinverwaltung_t::input_output->get_image_id(1), pos.x + offset.x, pos.y + offset.y + yoff, 0, false, false);
				left += 12;
			}
			display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, translator::translate("Produktion"), ALIGN_LEFT, SYSCOL_TEXT, true);
			yoff += LINESPACE;

			int i = 0;
			FORX(array_tpl<ware_production_t>, const& goods, fab->get_output(), i++) {
				const goods_desc_t * type = goods.get_typ();
				const sint64 pfactor = (sint64)fab->get_desc()->get_product(i)->get_factor();
				const uint32 stock_quantity   = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.menge * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const uint32 storage_capacity = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)goods.max * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
				const COLOR_VAL goods_color  = goods.get_typ()->get_color();

				left = 2;
				yoff+=2; // box position adjistment
				// [storage indicator]
				display_ddd_box_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, STORAGE_INDICATOR_WIDTH+2, 8, MN_GREY0, MN_GREY4);
				display_fillbox_wh_clip(pos.x + offset.x + left+1, pos.y + offset.y + yoff+1, STORAGE_INDICATOR_WIDTH, 6, MN_GREY2, true);
				if (storage_capacity) {
					const uint16 colored_width = min(STORAGE_INDICATOR_WIDTH, (uint16)(STORAGE_INDICATOR_WIDTH * stock_quantity / storage_capacity));
					display_fillbox_wh_clip(pos.x + offset.x + left + 1, pos.y + offset.y + yoff + 1, colored_width, 6, goods_color, true);
				}
				left += STORAGE_INDICATOR_WIDTH + 2 + D_H_SPACE;

				// [goods color box] This design is the same as the goods list
				display_ddd_box_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, 8, 8, MN_GREY0, MN_GREY4);
				display_fillbox_wh_clip(pos.x + offset.x + left+1, pos.y + offset.y + yoff + 1, 6, 6, goods_color, true);
				left += 12;
				yoff-=2; // box position adjistment

				// [goods name]
				buf.clear();
				buf.printf("%s", translator::translate(goods.get_typ()->get_name()));
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

				left += 10;

				// [goods category]
				display_color_img_with_tooltip(goods.get_typ()->get_catg_symbol(), pos.x + offset.x + left, pos.y + offset.y + yoff, 0, false, false, translator::translate(goods.get_typ()->get_catg_name()));
				left += 14;

				// [storage capacity]
				buf.clear();
				buf.printf("%u/%u,", stock_quantity,	storage_capacity);
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				left += D_H_SPACE;

				// [monthly production]
				buf.clear();
				const uint32 monthly_prod = (uint32)(fab->get_current_production()*pfactor * 10 >> DEFAULT_PRODUCTION_FACTOR_BITS);
				buf.append((float)monthly_prod / 10.0, monthly_prod < 100 ? 1 : 0);
				buf.printf("%s%s", translator::translate(goods.get_typ()->get_mass()), translator::translate("/month"));
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

				yoff += LINESPACE;
			}
			yoff += LINESPACE;
		}

	}
	scr_size size(400, yoff);
	if (size != get_size()) {
		set_size(size);
	}
}
