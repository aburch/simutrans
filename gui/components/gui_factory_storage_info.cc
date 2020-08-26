/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "gui_factory_storage_info.h"

#include "../../simcolor.h"
#include "../../simworld.h"

#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"

#include "../../display/viewport.h"
#include "../../utils/simstring.h"

#include "../../player/simplay.h"

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
					display_cylinderbar_wh_clip(pos.x + offset.x + left + 1, pos.y + offset.y + yoff + 1, colored_width, 6, goods_color, true);
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
				left += 14;

				// [storage capacity]
				buf.clear();
				buf.printf("%u/%u,", stock_quantity, storage_capacity);
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				left += D_H_SPACE;

				buf.clear();
				// [in transit]
				if (fab->get_status() != fabrik_t::inactive) {
					//const bool in_transit_over_storage = (stock_quantity + (uint32)goods.get_in_transit() > storage_capacity);
					const sint32 actual_max_transit = max(goods.get_in_transit(), max_transit);
					if (skinverwaltung_t::in_transit) {
						display_color_img_with_tooltip(skinverwaltung_t::in_transit->get_image_id(0), pos.x + offset.x + left, pos.y + offset.y + yoff, 0, false, false, translator::translate("symbol_help_txt_in_transit"));
						left += 14;
					}
					buf.printf("%i/%i", goods.get_in_transit(), actual_max_transit);
					COLOR_VAL col = actual_max_transit == 0 ? COL_RED : max_transit == 0 ? COL_DARK_ORANGE: SYSCOL_TEXT;
					left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, col, true);
					buf.clear();
					buf.append(", ");
				}


				// [monthly production]
				const uint32 monthly_prod = (uint32)(fab->get_current_production()*pfactor * 10 >> DEFAULT_PRODUCTION_FACTOR_BITS);
				if (monthly_prod < 100) {
					buf.printf(translator::translate("consumption %.1f%s/month"), (float)monthly_prod / 10.0, translator::translate(goods.get_typ()->get_mass()));
				}
				else {
					buf.printf(translator::translate("consumption %u%s/month"), monthly_prod/10, translator::translate(goods.get_typ()->get_mass()));
				}
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
					display_cylinderbar_wh_clip(pos.x + offset.x + left + 1, pos.y + offset.y + yoff + 1, colored_width, 6, goods_color, true);
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
				buf.printf("%u/%u,", stock_quantity, storage_capacity);
				left += display_proportional_clip(pos.x + offset.x + left, pos.y + offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				left += D_H_SPACE;

				// [monthly production]
				buf.clear();
				const uint32 monthly_prod = (uint32)(fab->get_current_production()*pfactor * 10 >> DEFAULT_PRODUCTION_FACTOR_BITS);
				if (monthly_prod < 100) {
					buf.printf(translator::translate("production %.1f%s/month"), (float)monthly_prod / 10.0, translator::translate(goods.get_typ()->get_mass()));
				}
				else {
					buf.printf(translator::translate("production %u%s/month"), monthly_prod / 10, translator::translate(goods.get_typ()->get_mass()));
				}
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

void gui_factory_storage_info_t::recalc_size()
{
	if (fab) {
		uint lines = fab->get_input().get_count() ? fab->get_input().get_count() + 1 : 0;
		if (fab->get_output().get_count()) {
			if (lines) { lines++; }
			lines += fab->get_output().get_count()+1;
		}
		set_size(scr_size(400, lines * (LINESPACE + 1)));
	}
	else {
		set_size(scr_size(400, LINESPACE + 1));
	}
}


gui_factory_connection_stat_t::gui_factory_connection_stat_t(fabrik_t *factory, bool is_input_display)
{
	this->fab = factory;
	this->is_input_display = is_input_display;
	recalc_size();
	line_selected = 0xFFFFFFFFu;
}

bool gui_factory_connection_stat_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE + 1);
	line_selected = 0xFFFFFFFFu;
	if (line >= fab_list.get_count()) {
		return false;
	}

	fabrik_t* target_fab = fabrik_t::get_fab(fab_list[line]);
	const koord3d target_pos = target_fab->get_pos();
	if (!target_fab) {
		welt->get_viewport()->change_world_position(target_pos);
		return false;
	}

	// un-press goto button
	if (ev->button_state > 0 && ev->cx > 0 && ev->cx < 15) {
		line_selected = line;
	}

	if (IS_LEFTRELEASE(ev)) {
		if (ev->cx > 0 && ev->cx < 15) {
			welt->get_viewport()->change_world_position(target_pos);
		}
		else {
			target_fab->show_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->get_viewport()->change_world_position(target_pos);
	}
	return false;
}


void gui_factory_connection_stat_t::recalc_size()
{
	// show_scroll_x==false ->> size.w not important ->> no need to calc text pixel length
	if (fab) {
		uint lines = is_input_display ? fab->get_suppliers().get_count() : fab->get_lieferziele().get_count();
		set_size(scr_size(400, lines * (LINESPACE + 1)));
	}
	else {
		set_size(scr_size(0, 0));
	}
}

void gui_factory_connection_stat_t::draw(scr_coord offset)
{
	if (!fab) {
		return;
	}

	static cbuffer_t buf;
	int xoff = pos.x;
	int yoff = pos.y;

	double distance;
	char distance_display[10];

	fab_list = is_input_display ? fab->get_suppliers() : fab->get_lieferziele();


	uint32 sel = line_selected;
	FORX(const vector_tpl<koord>, k, fab_list, yoff += LINESPACE + 1) {
		fabrik_t *target_fab = fabrik_t::get_fab(k);

		if (target_fab) {
			const bool is_active = is_input_display ?
				target_fab->is_active_lieferziel(fab->get_pos().get_2d()) :
				fab->is_active_lieferziel(k);
			const bool is_connected_to_own_network = fab->is_connected_to_network(welt->get_active_player()) && target_fab->is_connected_to_network(welt->get_active_player());
			const bool is_within_own_network = target_fab->is_connected_to_network(welt->get_active_player());
			xoff = D_POS_BUTTON_WIDTH + D_H_SPACE;

			const goods_desc_t *transport_goods = NULL;
			if (!is_input_display) {
				FOR(array_tpl<ware_production_t>, const& product, fab->get_output()) {
					const goods_desc_t *inquiry_goods = product.get_typ();
					if (target_fab->get_desc()->get_accepts_these_goods(inquiry_goods)) {
						transport_goods = inquiry_goods;
						break;
					}
				}
			}
			else {
				FOR(array_tpl<ware_production_t>, const& product, target_fab->get_output()) {
					const goods_desc_t *inquiry_goods = product.get_typ();
					if (fab->get_desc()->get_accepts_these_goods(inquiry_goods)) {
						transport_goods = inquiry_goods;
						break;
					}
				}
			}

			assert(transport_goods != NULL);

			// [status color bar]
			if (fab->get_status() >= fabrik_t::staff_shortage) {
				display_ddd_box_clip(offset.x + xoff, offset.y + yoff + 2, D_INDICATOR_WIDTH / 2, D_INDICATOR_HEIGHT + 2, COL_STAFF_SHORTAGE, COL_STAFF_SHORTAGE);
			}
			COLOR_VAL col = fabrik_t::status_to_color[target_fab->get_status() % fabrik_t::staff_shortage];
			display_fillbox_wh_clip(offset.x + xoff + 1, offset.y + yoff + 3, D_INDICATOR_WIDTH / 2 - 1, D_INDICATOR_HEIGHT, col, true);
			xoff += D_INDICATOR_WIDTH / 2 + 3;

			// [name]
			buf.clear();
			buf.printf("%s (%d,%d) - ", target_fab->get_name(), k.x, k.y);
			xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			if (is_active) {
				display_color_img_with_tooltip(transport_goods->get_catg_symbol(), offset.x + xoff - 2, offset.y + yoff, 0, false, false, translator::translate("hlptxt_factory_connected"));
				xoff += 11;
			}
			// [goods color box] This design is the same as the goods list
			display_ddd_box_clip(offset.x + xoff, offset.y + yoff + 2, 8, 8, MN_GREY0, MN_GREY4);
			display_fillbox_wh_clip(offset.x + xoff + 1, offset.y + yoff + 3, 6, 6, transport_goods->get_color(), true);
			xoff += 12;
			// [distance]
			col = is_within_own_network ? SYSCOL_TEXT : COL_GREY3;
			distance = (double)(shortest_distance(k, fab->get_pos().get_2d()) * welt->get_settings().get_meters_per_tile()) / 1000.0;
			if (distance < 1)
			{
				sprintf(distance_display, "%.0fm", distance * 1000);
			}
			else
			{
				uint n_actual = distance < 5 ? 1 : 0;
				char tmp[10];
				number_to_string(tmp, distance, n_actual);
				sprintf(distance_display, "%skm", tmp);
			}
			buf.clear();
			buf.append(distance_display);
			xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, col, true);

			xoff += D_H_SPACE;

			if (target_fab->get_status() != fabrik_t::inactive && fab->get_status() != fabrik_t::inactive) {
				// [lead time]
				const uint32 lead_time = is_input_display ? fab->get_lead_time(transport_goods) : target_fab->get_lead_time(transport_goods);
				if (skinverwaltung_t::travel_time){
					display_color_img_with_tooltip(skinverwaltung_t::travel_time->get_image_id(0), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate("symbol_help_txt_lead_time"));
					xoff += 12;
				}
				buf.clear();
				if (lead_time == UINT32_MAX_VALUE) {
					buf.append("--:--:--");
					col = COL_GREY4;
				}
				else {
					char lead_time_as_clock[32];
					welt->sprintf_time_tenths(lead_time_as_clock, 32, lead_time);
					buf.append(lead_time_as_clock);
					col = is_connected_to_own_network ? SYSCOL_TEXT : COL_GREY4;
				}
				xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, col, true);
				xoff += D_H_SPACE * 2;

				buf.clear();
				// [max transit] for consumer
				sint32 max_transit = -1;
				sint32 in_transit = 0;
				sint32 goods_needed = 0;
				int index = 0;
				if (!is_input_display) {
					// NOTE: this is not the only shipping situation from THIS factory.
					// may have been shipped from another factory.
					// but we can't tell it apart, and that affects shipping from this factory.
					FORX(array_tpl<ware_production_t>, const& buyingup_goods, target_fab->get_input(), index++) {
						if (transport_goods == buyingup_goods.get_typ()) {
							const sint64 pfactor = target_fab->get_desc()->get_supplier(index) ? (sint64)target_fab->get_desc()->get_supplier(index)->get_consumption() : 1ll;
							goods_needed = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)(target_fab->goods_needed(buyingup_goods.get_typ())) * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
							max_transit = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)buyingup_goods.max_transit * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
							in_transit = buyingup_goods.get_in_transit();
							break;
						}
					}
					if (max(in_transit, min(max_transit, goods_needed))) {
						if (skinverwaltung_t::in_transit) {
							display_color_img_with_tooltip(skinverwaltung_t::in_transit->get_image_id(0), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate("Incoming shippment status of this item for this factory"));
							xoff += 14;
						}
						//buf.printf("%i/%i (%i)", in_transit, max_transit, goods_needed);
						buf.printf("%i/%i", in_transit, max(in_transit, min(max_transit, goods_needed)));
					}
					else {
						if (skinverwaltung_t::pax_evaluation_icons) {
							display_color_img_with_tooltip(skinverwaltung_t::pax_evaluation_icons->get_image_id(4), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate("Shipment has been suspended due to consumption demand"));
							xoff += 14;
						}
						buf.append(translator::translate("Shipment is suspended"));
					}
				}
				else {
					// - supplier
					// We do not know which supplier the goods are coming from.
					// So it doesn't show the number in transit, just informs player that this factory has stopped the order.
					FORX(array_tpl<ware_production_t>, const& buyingup_goods, fab->get_input(), index++) {
						if (transport_goods == buyingup_goods.get_typ()) {
							const sint64 pfactor = fab->get_desc()->get_supplier(index) ? (sint64)fab->get_desc()->get_supplier(index)->get_consumption() : 1ll;
							goods_needed = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)(fab->goods_needed(buyingup_goods.get_typ())) * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
							//max_transit = (uint32)((FAB_DISPLAY_UNIT_HALF + (sint64)buyingup_goods.max_transit * pfactor) >> (fabrik_t::precision_bits + DEFAULT_PRODUCTION_FACTOR_BITS));
							in_transit = buyingup_goods.get_in_transit();
							break;
						}
					}
					if (!in_transit && goods_needed <= 0) {
						if (skinverwaltung_t::pax_evaluation_icons) {
							display_color_img_with_tooltip(skinverwaltung_t::pax_evaluation_icons->get_image_id(4), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate("Shipment has been suspended due to consumption demand"));
							xoff += 14;
						}
						buf.append(translator::translate("Shipment is suspended"));
					}
					else if (goods_needed <= 0) {
						if (skinverwaltung_t::alerts) {
							display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(3), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate("Suspension of new orders due to sufficient supply"));
							xoff += 14;
						}
					}
					//buf.printf("%i/%i (%i)", in_transit, max_transit, goods_needed);
				}

				xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, col, true);
			}

			// goto button
			bool selected = sel == 0 || welt->get_viewport()->is_on_center(target_fab->get_pos());
			display_img_aligned(gui_theme_t::pos_button_img[selected], scr_rect(offset.x + D_H_SPACE, offset.y + yoff, D_POS_BUTTON_WIDTH, LINESPACE), ALIGN_CENTER_V | ALIGN_CENTER_H, true);
			sel--;

			if (win_get_magic((ptrdiff_t)target_fab)) {
				display_blend_wh(offset.x, offset.y + yoff, size.w, LINESPACE, SYSCOL_TEXT, 20);
			}
		}
	}
	set_size(scr_size(400, yoff));
}


gui_factory_nearby_halt_info_t::gui_factory_nearby_halt_info_t(fabrik_t *factory)
{
	this->fab = factory;
	if (fab) {
		halt_list = factory->get_nearby_freight_halts();
		recalc_size();
	}
	line_selected = 0xFFFFFFFFu;
}

bool gui_factory_nearby_halt_info_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE + 1);
	line_selected = 0xFFFFFFFFu;
	if (line >= halt_list.get_count()) {
		return false;
	}

	halthandle_t halt = halt_list[line].halt;
	const koord3d halt_pos = halt->get_basis_pos3d();
	if (!halt.is_bound()) {
		welt->get_viewport()->change_world_position(halt_pos);
		return false;
	}

	if (IS_LEFTRELEASE(ev)) {
		if (ev->cx > 0 && ev->cx < 15) {
			welt->get_viewport()->change_world_position(halt_pos);
		}
		else {
			halt->show_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		welt->get_viewport()->change_world_position(halt_pos);
	}
	return false;
}

void gui_factory_nearby_halt_info_t::recalc_size()
{
	if (halt_list.get_count()) {
		set_size(scr_size(400, halt_list.get_count() * (LINESPACE + 1)));
	}
	else {
		set_size(scr_size(0, 0));
	}
}


void gui_factory_nearby_halt_info_t::update()
{
	if (fab) {
		halt_list = fab->get_nearby_freight_halts();
	}
	recalc_size();
}


void gui_factory_nearby_halt_info_t::draw(scr_coord offset)
{
	if (!halt_list.get_count()) {
		return;
	}

	static cbuffer_t buf;
	int xoff = pos.x;
	int yoff = pos.y;

	FORX(const vector_tpl<nearby_halt_t>, freight_halt, halt_list, yoff += LINESPACE + 1) {
		COLOR_VAL col = SYSCOL_TEXT;

		halthandle_t halt = freight_halt.halt;

		if (halt.is_bound()) {
			xoff = D_V_SPACE;
			col = SYSCOL_TEXT;
			xoff += D_INDICATOR_WIDTH * 2 / 3 +D_H_SPACE;

			// [name]
			buf.clear();
			buf.append(halt->get_name());
			xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, halt->get_owner()->get_player_color1(), true);
			xoff += D_H_SPACE * 2;

			bool has_active_freight_connection = false;

			// [capacity]
			uint32 wainting_sum = 0;
			uint32 transship_sum = 0;
			for (uint i = 0; i < goods_manager_t::get_max_catg_index(); i++) {
				if (i == goods_manager_t::INDEX_PAS || i == goods_manager_t::INDEX_MAIL)
				{
					continue;
				}
				const goods_desc_t *wtyp = goods_manager_t::get_info(i);
				switch (i) {
					case 0:
						wainting_sum += halt->get_ware_summe(wtyp);
						break;
					default:
						const uint8  count = goods_manager_t::get_count();
						for (uint32 j = 3; j < count; j++) {
							goods_desc_t const* const wtyp2 = goods_manager_t::get_info(j);
							if (wtyp2->get_catg_index() != i) {
								continue;
							}
							wainting_sum += halt->get_ware_summe(wtyp2);
							transship_sum += halt->get_transferring_goods_sum(wtyp2, 0);
						}
						break;
				}
			}
			display_color_img_with_tooltip(skinverwaltung_t::goods->get_image_id(0), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate("station_capacity_freight"));
			xoff += 14;

			if (wainting_sum || transship_sum) {
				has_active_freight_connection = true;
			}

			buf.clear();
			buf.printf("%u", wainting_sum);
			if (transship_sum) {
				buf.printf("(%u)", transship_sum);
			}
			buf.printf("/%u", halt->get_capacity(2));
			xoff += display_proportional_clip(offset.x + xoff, offset.y + yoff, buf, ALIGN_LEFT, col, true);
			xoff += D_H_SPACE;

			// [station handled goods category] (symbol)
			for (uint i = 0; i < goods_manager_t::get_max_catg_index(); i++) {
				if (i == goods_manager_t::INDEX_PAS || i == goods_manager_t::INDEX_MAIL)
				{
					continue;
				}
				typedef quickstone_hashtable_tpl<haltestelle_t, haltestelle_t::connexion*> connexions_map_single_remote;
				uint8 g_class = goods_manager_t::get_classes_catg_index(i) - 1;
				connexions_map_single_remote *connexions = halt->get_connexions(i, g_class);

				if (!connexions->empty())
				{
					display_color_img_with_tooltip(goods_manager_t::get_info_catg_index(i)->get_catg_symbol(), offset.x + xoff, offset.y + yoff, 0, false, false, translator::translate(goods_manager_t::get_info_catg_index(i)->get_catg_name()));
					xoff += 14;
					has_active_freight_connection = true;
				}
			}

			// [status bar]
			// get_status_farbe() may not fit for freight station. so a dedicated alert display
			// This can be separated as a function of simhalt if used elsewhere.
			//if (halt->is_overcrowded(2)) { col = COL_RED; } // This may be extremely lagging
			if (!has_active_freight_connection) {
				col = COL_GREY3; // seems that the freight convoy is not running here
			}
			else if (wainting_sum + transship_sum > halt->get_capacity(2)) {
				col = COL_RED;
			}
			else if (wainting_sum + transship_sum > halt->get_capacity(2)*0.9) {
				col = COL_ORANGE;
			}
			else if (!wainting_sum && !transship_sum) {
				// Still have to consider
				col = COL_YELLOW;
			}
			else {
				col = COL_GREEN;
			}
			display_fillbox_wh_clip(offset.x + D_V_SPACE + 1, offset.y + yoff + 3, D_INDICATOR_WIDTH * 2 / 3, D_INDICATOR_HEIGHT + 1, col, true);

			if (win_get_magic(magic_halt_info + halt.get_id())) {
				display_blend_wh(offset.x, offset.y + yoff, size.w, LINESPACE, SYSCOL_TEXT, 20);
			}
		}
	}
	scr_size size(400, yoff);
	if (size != get_size()) {
		set_size(size);
	}
}
