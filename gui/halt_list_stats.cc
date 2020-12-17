/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "halt_list_stats.h"
#include "../simhalt.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../player/simplay.h"
#include "../simworld.h"
#include "../display/simimg.h"

#include "../dataobj/translator.h"

#include "../descriptor/skin_desc.h"

#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#include "gui_frame.h"
#include "halt_info.h" // gui_halt_type_images_t

#define MINI_WAITING_BAR_HEIGHT 4
#define MINI_WAITING_BAR_WIDTH 33

 // helper class
gui_mini_halt_waiting_indicator_t::gui_mini_halt_waiting_indicator_t(halthandle_t h)
{
	halt = h;
}

void gui_mini_halt_waiting_indicator_t::draw(scr_coord offset)
{
	uint32 wainting_sum, transship_in_sum, leaving_sum;
	bool is_operating;
	bool overcrowded;
	bool served;
	PIXVAL goods_colval;
	for (uint8 i = 0; i < 3; i++) {
		is_operating = false;
		served = false;
		overcrowded = false;
		wainting_sum = 0;
		transship_in_sum = 0;
		leaving_sum = 0;
		goods_colval = color_idx_to_rgb(MN_GREY2);

		switch (i) {
		case 0:
			if (halt->get_pax_enabled()) {
				goods_colval = goods_manager_t::passengers->get_color();
				served = true;
			}
			wainting_sum = halt->get_ware_summe(goods_manager_t::get_info(goods_manager_t::INDEX_PAS));
			is_operating = halt->gibt_ab(goods_manager_t::get_info(goods_manager_t::INDEX_PAS));
			overcrowded = halt->is_overcrowded(goods_manager_t::INDEX_PAS);
			transship_in_sum = halt->get_transferring_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_PAS)) - halt->get_leaving_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_PAS));
			break;
		case 1:
			if (halt->get_mail_enabled()) {
				goods_colval = goods_manager_t::mail->get_color();
				served = true;
			}
			wainting_sum = halt->get_ware_summe(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL));
			is_operating = halt->gibt_ab(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL));
			overcrowded = halt->is_overcrowded(goods_manager_t::INDEX_MAIL);
			transship_in_sum = halt->get_transferring_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL)) - halt->get_leaving_goods_sum(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL));
			break;
		case 2:
			if (halt->get_ware_enabled()) {
				goods_colval = color_idx_to_rgb(115);
				served = true;
			}
			for (uint8 g1 = 0; g1 < goods_manager_t::get_max_catg_index(); g1++) {
				if (g1 == goods_manager_t::INDEX_PAS || g1 == goods_manager_t::INDEX_MAIL)
				{
					continue;
				}
				const goods_desc_t *wtyp = goods_manager_t::get_info(g1);
				if (!is_operating)
				{
					is_operating = halt->gibt_ab(wtyp);
				}
				switch (g1) {
				case 0:
					wainting_sum += halt->get_ware_summe(wtyp);
					break;
				default:
					const uint8 count = goods_manager_t::get_count();
					for (uint32 g2 = 3; g2 < count; g2++) {
						goods_desc_t const* const wtyp2 = goods_manager_t::get_info(g2);
						if (wtyp2->get_catg_index() != g1) {
							continue;
						}
						wainting_sum += halt->get_ware_summe(wtyp2);
						transship_in_sum += halt->get_transferring_goods_sum(wtyp2, 0);
						leaving_sum += halt->get_leaving_goods_sum(wtyp2, 0);
					}
					break;
				}
			}
			overcrowded = ((wainting_sum + transship_in_sum) > halt->get_capacity(i));
			transship_in_sum -= leaving_sum;
			break;
		default:
			continue;
		}

		// [colorbox]
		display_ddd_box_clip_rgb(pos.x+offset.x, pos.y+offset.y + i*(MINI_WAITING_BAR_HEIGHT+1), MINI_WAITING_BAR_HEIGHT, MINI_WAITING_BAR_HEIGHT, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));
		if (served) {
			const KOORD_VAL base_x = pos.x + offset.x;
			const KOORD_VAL base_y = pos.y + offset.y + i * (MINI_WAITING_BAR_HEIGHT + 1);
			const uint8 color_box_size = is_operating ? MINI_WAITING_BAR_HEIGHT : MINI_WAITING_BAR_HEIGHT-2;
			display_fillbox_wh_clip_rgb(is_operating ? base_x : base_x+1, is_operating ? base_y : base_y+1,	color_box_size, color_box_size, goods_colval, false);
		}

		// [waiting indicator]
		// If the capacity is 0 (but hundled this freught type), do not display the bar
		if (halt->get_capacity(i) > 0) {
			display_ddd_box_clip_rgb(pos.x+offset.x + MINI_WAITING_BAR_HEIGHT+1, pos.y+offset.y + i*(MINI_WAITING_BAR_HEIGHT+1), MINI_WAITING_BAR_WIDTH + 2, MINI_WAITING_BAR_HEIGHT, color_idx_to_rgb(MN_GREY0), color_idx_to_rgb(MN_GREY4));
			const PIXVAL bg_colval = overcrowded ? color_idx_to_rgb(COL_OVERCROWD) : color_idx_to_rgb(MN_GREY2);
			display_fillbox_wh_clip_rgb(pos.x+offset.x + MINI_WAITING_BAR_HEIGHT+2, pos.y+offset.y + i*(MINI_WAITING_BAR_HEIGHT+1) + 1, MINI_WAITING_BAR_WIDTH, MINI_WAITING_BAR_HEIGHT-2, bg_colval, true);

			uint8 waiting_factor = min(100, wainting_sum * 100 / halt->get_capacity(i));
			display_fillbox_wh_clip_rgb(pos.x+offset.x + MINI_WAITING_BAR_HEIGHT+2, pos.y+offset.y + i*(MINI_WAITING_BAR_HEIGHT+1) + 1, MINI_WAITING_BAR_WIDTH*waiting_factor/100, MINI_WAITING_BAR_HEIGHT-2, COL_CLEAR, true);
		}

	}
	scr_size size(MINI_WAITING_BAR_WIDTH+MINI_WAITING_BAR_HEIGHT+3, max(MINI_WAITING_BAR_HEIGHT*3+2, LINEASCENT));
	if (size != get_size()) {
		set_size(size);
	}
	gui_container_t::draw(offset);
}

// main class
static karte_ptr_t welt;
/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool halt_list_stats_t::infowin_event(const event_t *ev)
{
	bool swallowed = gui_aligned_container_t::infowin_event(ev);
	if(  !swallowed  &&  halt.is_bound()  ) {

		if(IS_LEFTRELEASE(ev)) {
			if(  event_get_last_control_shift() != 2  ) {
				halt->show_info();
			}
			return true;
		}
		if(  IS_RIGHTRELEASE(ev)  ) {
			welt->get_viewport()->change_world_position(halt->get_basis_pos3d());
			return true;
		}
	}
	return swallowed;
}


halt_list_stats_t::halt_list_stats_t(halthandle_t h)
{
	halt = h;
	set_table_layout(2,3);
	set_spacing(scr_size(D_H_SPACE, 0));


	add_component(&indicator);
	indicator.set_max_size(scr_size(D_INDICATOR_WIDTH,D_INDICATOR_HEIGHT));

	add_table(2,1);
	{
		add_component(&label_name);
		label_name.buf().append(halt->get_name());
		label_name.update();

		img_types = new_component<gui_halt_type_images_t>(halt);
	}
	end_table();

	gui_aligned_container_t *table = add_table(3,1);
	{
		add_component(&img_enabled[0]);
		img_enabled[0].set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		add_component(&img_enabled[1]);
		img_enabled[1].set_image(skinverwaltung_t::mail->get_image_id(0), true);
		add_component(&img_enabled[2]);
		img_enabled[2].set_image(skinverwaltung_t::goods->get_image_id(0), true);

		img_enabled[0].set_rigid(true);
		img_enabled[1].set_rigid(true);
		img_enabled[2].set_rigid(true);
	}
	end_table();
	indicator.set_max_size(scr_size(table->get_min_size().w,D_INDICATOR_HEIGHT));

	add_component(&label_cargo);
	halt->get_short_freight_info( label_cargo.buf() );
	label_cargo.update();

	new_component<gui_margin_t>(0,D_V_SPACE);
}


const char* halt_list_stats_t::get_text() const
{
	return halt->get_name();
}

/**
 * Draw the component
 */
void halt_list_stats_t::draw(scr_coord offset)
{
	indicator.set_color(halt->get_status_farbe());
	img_enabled[0].set_visible(halt->get_pax_enabled());
	img_enabled[1].set_visible(halt->get_mail_enabled());
	img_enabled[2].set_visible(halt->get_ware_enabled());

	label_name.buf().append(halt->get_name());
	label_name.update();

	halt->get_short_freight_info( label_cargo.buf() );
	label_cargo.update();

	set_size(get_size());
	gui_aligned_container_t::draw(offset);
}
