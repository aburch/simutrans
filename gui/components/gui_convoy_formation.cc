#include <stdio.h>

#include "gui_convoy_formation.h"

#include "../../simconvoi.h"
#include "../../vehicle/simvehicle.h"

#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../simworld.h"

#include "../../dataobj/translator.h"
#include "../../utils/cbuffer_t.h"


// component for vehicle display
gui_convoy_formation_t::gui_convoy_formation_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}

void gui_convoy_formation_t::draw(scr_coord offset)
{
	if (cnv.is_bound()) {
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
						display_fillbox_wh_clip(offset.x + 2 + bar_offset_left, offset.y + LINESPACE + VEHICLE_BAR_HEIGHT + 3, bar_width, 3, color, true);
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
				display_fillbox_wh_clip(offset.x + 2, offset.y + LINESPACE + VEHICLE_BAR_HEIGHT + 3, grid_width - 4, 3, MN_GREY0, true);
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

			int left = display_proportional_clip(offset.x + 2, offset.y, buf, ALIGN_LEFT, v->get_desc()->has_available_upgrade(month_now) ? COL_UPGRADEABLE : COL_GREY2, true);
#ifdef DEBUG
			if (v->is_reversed()) {
				display_proportional_clip(offset.x + 2 + left, offset.y - 2, "*", ALIGN_LEFT, COL_YELLOW, true);
			}
			if (!v->get_desc()->is_bidirectional()) {
				display_proportional_clip(offset.x + 2 + left, offset.y - 2, "<", ALIGN_LEFT, COL_LIGHT_TURQUOISE, true);
			}
#else
			(void)left;
#endif

			color = v->get_desc()->is_future(month_now) || v->get_desc()->is_retired(month_now) ? COL_OUT_OF_PRODUCTION : COL_DARK_GREEN;
			if (v->get_desc()->is_obsolete(month_now, welt)) {
				color = COL_OBSOLETE;
			}
			display_veh_form(offset.x + 1, offset.y + LINESPACE, VEHICLE_BAR_HEIGHT * 2, color, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_next() : v->get_desc()->get_basic_constraint_prev(), v->get_desc()->get_interactivity(), false);
			display_veh_form(offset.x + grid_width / 2, offset.y + LINESPACE, VEHICLE_BAR_HEIGHT * 2, color, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_prev() : v->get_desc()->get_basic_constraint_next(), v->get_desc()->get_interactivity(), true);

			offset.x += grid_width;
		}

		scr_size size(grid_width*cnv->get_vehicle_count() + D_MARGIN_LEFT * 2, LINESPACE + VEHICLE_BAR_HEIGHT + 10 + D_SCROLLBAR_HEIGHT);
		if (size != get_size()) {
			set_size(size);
		}
	}
}
