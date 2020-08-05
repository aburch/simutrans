/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "factorylist_stats_t.h"

#include "../display/simgraph.h"
#include "../display/viewport.h"
#include "../simskin.h"
#include "../simcolor.h"
#include "../simfab.h"
#include "../simworld.h"
#include "../simskin.h"
#include "simwin.h"

#include "gui_frame.h"
#include "components/gui_button.h"

#include "../bauer/goods_manager.h"
#include "../descriptor/skin_desc.h"
#include "../utils/cbuffer_t.h"
#include "../utils/simstring.h"

#define STORAGE_BAR_WIDTH 50

factorylist_stats_t::factorylist_stats_t(factorylist::sort_mode_t sortby, bool sortreverse, bool own_network, uint8 goods_catg)
{
	sort(sortby,sortreverse,own_network, goods_catg);
	recalc_size();
	line_selected = 0xFFFFFFFFu;
}


class compare_factories
{
	public:
		compare_factories(factorylist::sort_mode_t sortby_, bool reverse_) :
			sortby(sortby_),
			reverse(reverse_)
		{}

		bool operator ()(const fabrik_t* a, const fabrik_t* b)
		{
			int cmp;
			switch (sortby) {
				default:
				case factorylist::by_name:
					cmp = 0;
					break;

				case factorylist::by_input:
				{
					int a_in = a->get_input().empty() ? -1 : (int)a->get_total_in();
					int b_in = b->get_input().empty() ? -1 : (int)b->get_total_in();
					cmp = a_in - b_in;
					break;
				}

				case factorylist::by_transit:
				{
					int a_transit = a->get_input().empty() ? -1 : (int)a->get_total_transit();
					int b_transit = b->get_input().empty() ? -1 : (int)b->get_total_transit();
					cmp = a_transit - b_transit;
					break;
				}

				case factorylist::by_available:
				{
					int a_in = a->get_input().empty() ? -1 : (int)(a->get_total_in()+a->get_total_transit());
					int b_in = b->get_input().empty() ? -1 : (int)(b->get_total_in()+b->get_total_transit());
					cmp = a_in - b_in;
					break;
				}

				case factorylist::by_output:
				{
					int a_out = a->get_output().empty() ? -1 : (int)a->get_total_out();
					int b_out = b->get_output().empty() ? -1 : (int)b->get_total_out();
					cmp = a_out - b_out;
					break;
				}

				case factorylist::by_maxprod:
					cmp = a->get_base_production()*a->get_prodfactor() - b->get_base_production()*b->get_prodfactor();
					break;

				case factorylist::by_status:
					cmp = a->get_status() - b->get_status();
					break;

				case factorylist::by_power:
					cmp = a->get_prodfactor_electric() - b->get_prodfactor_electric();
					break;
				case factorylist::by_sector:
					cmp = a->get_sector() - b->get_sector();
					break;
				case factorylist::by_staffing:
					cmp = a->get_staffing_level_percentage() - b->get_staffing_level_percentage();
					break;
				case factorylist::by_operation_rate:
					cmp = a->get_stat(1, FAB_PRODUCTION) - b->get_stat(1, FAB_PRODUCTION);
					break;
			}
			if (cmp == 0) {
				cmp = STRICMP(a->get_name(), b->get_name());
			}
			return reverse ? cmp > 0 : cmp < 0;
		}

	private:
		const factorylist::sort_mode_t sortby;
		const bool reverse;
};

/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 * @author Hj. Malthaner
 */
bool factorylist_stats_t::infowin_event(const event_t * ev)
{
	const unsigned int line = (ev->cy) / (LINESPACE+1);
	line_selected = 0xFFFFFFFFu;
	if (line >= fab_list.get_count()) {
		return false;
	}

	fabrik_t* fab = fab_list[line];
	if (!fab) {
		return false;
	}

	// un-press goto button
	if(  ev->button_state>0  &&  ev->cx>0  &&  ev->cx<15  ) {
		line_selected = line;
	}

	if (IS_LEFTRELEASE(ev)) {
		if(ev->cx>0  &&  ev->cx<15) {
			const koord3d pos = fab->get_pos();
			welt->get_viewport()->change_world_position(pos);
		}
		else {
			fab->show_info();
		}
	}
	else if (IS_RIGHTRELEASE(ev)) {
		const koord3d pos = fab->get_pos();
		welt->get_viewport()->change_world_position(pos);
	}
	return false;
} // end of function factorylist_stats_t::infowin_event(const event_t * ev)


void factorylist_stats_t::recalc_size()
{
	// show_scroll_x==false ->> size.w not important ->> no need to calc text pixel length
	set_size( scr_size(390, welt->get_fab_list().get_count() * (LINESPACE+1) ) );
}


/**
 * Draw the component
 * @author Hj. Malthaner
 */
void factorylist_stats_t::draw(scr_coord offset)
{
	clip_dimension const cd = display_get_clip_wh();
	const int start = cd.y-LINESPACE-1;
	const int end = cd.yy+LINESPACE+1;

	static cbuffer_t buf;
	int xoff = offset.x+D_POS_BUTTON_WIDTH+D_H_SPACE;
	int yoff = offset.y;

	if(  fab_list.get_count()!=welt->get_fab_list().get_count()  ) {
		// some deleted/ added => resort
		sort( sortby, sortreverse, filter_own_network, filter_goods_catg);
	}

	uint32 sel = line_selected;
	FORX(vector_tpl<fabrik_t*>, const fab, fab_list, yoff += LINESPACE + 1) {
		if (yoff >= end) break;

		// skip invisible lines
		if (yoff < start) continue;

		if(fab) {
			unsigned indikatorfarbe = fabrik_t::status_to_color[fab->get_status()%fabrik_t::staff_shortage];
			int offset_left = 1;

			// goto button
			bool selected = sel == 0 || welt->get_viewport()->is_on_center(fab->get_pos());
			display_img_aligned(gui_theme_t::pos_button_img[selected], scr_rect(offset.x + D_H_SPACE, yoff, D_POS_BUTTON_WIDTH, LINESPACE), ALIGN_CENTER_V | ALIGN_CENTER_H, true);
			sel--;

			// status bar
			if (fab->get_status() >= fabrik_t::staff_shortage) {
				display_ddd_box_clip(xoff + offset_left, yoff + 1, D_INDICATOR_WIDTH, D_INDICATOR_HEIGHT + 2, COL_STAFF_SHORTAGE, COL_STAFF_SHORTAGE);
			}
			offset_left++;
			display_fillbox_wh_clip(xoff+ offset_left, yoff+2, D_INDICATOR_WIDTH-2, D_INDICATOR_HEIGHT, indikatorfarbe, true);
			offset_left += D_INDICATOR_WIDTH;

			buf.clear();
			buf.append(fab->get_name());
			// show name
			offset_left += max(150, display_proportional_clip(xoff + offset_left, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true));
			buf.clear();

			if (display_operation_stats) {
				// this month and last month operation rate
				buf.printf("%3.1f - ", fab->get_stat_converted(0,FAB_PRODUCTION) / 100.0);
				offset_left += proportional_string_width("0000.0 -");
				display_proportional_clip(xoff + offset_left, yoff, buf, ALIGN_RIGHT, SYSCOL_TEXT, true);
				buf.clear();
				buf.printf("%3.1f", fab->get_stat_converted(1, FAB_PRODUCTION) / 100.0);
				offset_left += proportional_string_width("0000.0");
				display_proportional_clip(xoff + offset_left, yoff, buf, ALIGN_RIGHT, SYSCOL_TEXT, true);
				buf.clear();
				const uint32 max_productivity = (100 * (fab->get_desc()->get_electric_boost() + fab->get_desc()->get_pax_boost() + fab->get_desc()->get_mail_boost())) >> DEFAULT_PRODUCTION_FACTOR_BITS;
				buf.printf("/%d%%", max_productivity + 100);
				offset_left += display_proportional_clip(xoff + offset_left, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				buf.clear();

				// electricity
				offset_left += D_V_SPACE;
				if (fab->get_desc()->get_electric_boost()) {
					if (fab->get_prodfactor_electric() > 0) {
						display_color_img(skinverwaltung_t::electricity->get_image_id(0), xoff + offset_left, yoff, 0, false, true);
					}
					else {
						display_img_blend(skinverwaltung_t::electricity->get_image_id(0), xoff + offset_left, yoff, TRANSPARENT50_FLAG | OUTLINE_FLAG | COL_GREY2, false, false);
					}
				}
				offset_left += 10;

				/* This boost display does not match the current extended system
				if(  fab->get_prodfactor_pax()>0  ) {
					display_color_img(skinverwaltung_t::passengers->get_image_id(0), xoff+offset_left, yoff, 0, false, true);
				}
				if(  fab->get_prodfactor_mail()>0  ) {
					display_color_img(skinverwaltung_t::mail->get_image_id(0), xoff+offset_left, yoff, 0, false, true);
				} */

				// staffing
				if (fab->get_desc()->get_building()->get_class_proportions_sum_jobs()) {
					display_ddd_box_clip(xoff + offset_left, yoff+2, 52, D_INDICATOR_HEIGHT+2, MN_GREY0, MN_GREY4);
					uint32 staff_shortage_factor=0;
					if (fab->get_sector() == fabrik_t::end_consumer) {
						staff_shortage_factor = welt->get_settings().get_minimum_staffing_percentage_consumer_industry();
					}
					else if (!(welt->get_settings().get_rural_industries_no_staff_shortage() && fab->get_sector() == fabrik_t::resource)) {
						staff_shortage_factor = welt->get_settings().get_minimum_staffing_percentage_full_production_producer_industry();
					}
					if (staff_shortage_factor) {
						display_fillbox_wh_clip(xoff + offset_left + 1, yoff + 3, staff_shortage_factor/2, D_INDICATOR_HEIGHT, COL_YELLOW, true);
					}
					display_cylinderbar_wh_clip(xoff + offset_left + 1, yoff + 3,
						(fab->get_staffing_level_percentage() > 100) ? STORAGE_BAR_WIDTH : fab->get_staffing_level_percentage()/2, D_INDICATOR_HEIGHT,
						(staff_shortage_factor > fab->get_staffing_level_percentage()) ? COL_STAFF_SHORTAGE : goods_manager_t::passengers->get_color(), true);
				}
			}
			else {
				// storage info
				// input storage
				if (!fab->get_input().empty())
				{
					// in transit
					if (fab->get_total_transit())
					{
						if (skinverwaltung_t::in_transit) {
							display_color_img(skinverwaltung_t::in_transit->get_image_id(0), xoff + offset_left, yoff, 0, false, false);
						}
						buf.printf("%i", fab->get_total_transit());
						display_proportional_clip(xoff + offset_left + 14, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						buf.clear();
					}

					// input
					if (skinverwaltung_t::input_output) {
						display_color_img(skinverwaltung_t::input_output->get_image_id(0), xoff + offset_left + 14 + 32, yoff, 0, false, false);
					}
					uint colored_with = fab->get_total_input_occupancy() * STORAGE_BAR_WIDTH / 100;
					display_vlinear_gradiwnt_wh(xoff + offset_left + 14 + 32 + 12, yoff + 2, colored_with, LINESPACE - 3, colored_with == STORAGE_BAR_WIDTH ? COL_DARK_GREEN : COL_DARK_GREEN + 1, 10, 75);

					buf.printf("%4i", fab->get_total_in());
					display_proportional_clip(xoff + offset_left + 14 + 40 + 14, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					buf.clear();
				}
				offset_left += 14 * 2 + 32 + STORAGE_BAR_WIDTH;

				// output storage
				if (!fab->get_output().empty()) {
					if (skinverwaltung_t::input_output) {
						display_color_img(skinverwaltung_t::input_output->get_image_id(1), xoff + offset_left, yoff, 0, false, false);
					}

					if (fab->get_total_output_capacity()) {
						uint colored_with = fab->get_total_out() * STORAGE_BAR_WIDTH / fab->get_total_output_capacity();
						display_vlinear_gradiwnt_wh(xoff + offset_left + 12, yoff + 2, colored_with, LINESPACE - 3, colored_with == STORAGE_BAR_WIDTH ? COL_DARK_GREEN : COL_DARK_GREEN + 1, 10, 75);
					}

					buf.printf("%4i", fab->get_total_out());
					display_proportional_clip(xoff + offset_left + 14, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					buf.clear();
				}
				else if (fab->get_sector() == fabrik_t::power_plant) {
					if (fab->is_currently_producing()) {
						display_color_img(skinverwaltung_t::electricity->get_image_id(0), xoff + offset_left+2, yoff, 0, false, true);
					}
					else {
						display_img_blend(skinverwaltung_t::electricity->get_image_id(0), xoff + offset_left+2, yoff, TRANSPARENT50_FLAG | OUTLINE_FLAG | COL_GREY2, false, false);
					}
				}
				offset_left += STORAGE_BAR_WIDTH + 14;

				/*
					NOTE: This number is ambiguous. This is an internal basic parameter and may differ from the actual production value.
				*/
				buf.append(fab->get_current_production(), 0);
				display_proportional_clip(xoff + offset_left, yoff, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			}

			if(  win_get_magic( (ptrdiff_t)fab )  ) {
				display_blend_wh( xoff, yoff, size.w+D_INDICATOR_WIDTH, LINESPACE, SYSCOL_TEXT, 25 );
			}
		}
	}
}

void factorylist_stats_t::sort(factorylist::sort_mode_t sb, bool sr, bool own_network, uint8 goods_catg_index)
{
	sortby = sb;
	sortreverse = sr;
	filter_own_network = own_network;
	filter_goods_catg = goods_catg_index;

	fab_list.clear();
	int lines = 0;
	for(sint16 i = welt->get_fab_list().get_count() - 1; i >= 0; i --)
	{
		// own network filter
		if(filter_own_network && !welt->get_fab_list()[i]->is_connected_to_network(welt->get_active_player())){
			continue;
		}
		// goods category filter
		if (filter_goods_catg != goods_manager_t::INDEX_NONE && !welt->get_fab_list()[i]->has_goods_catg_demand(filter_goods_catg)) {
			continue;
		}
		fab_list.insert_ordered( welt->get_fab_list()[i], compare_factories(sortby, sortreverse) );
		lines++;
	}
	fab_list.resize(welt->get_fab_list().get_count());
	set_size(scr_size(390, lines*(LINESPACE+1)));
}
