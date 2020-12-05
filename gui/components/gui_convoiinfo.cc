/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Convoi info stats, like loading status bar
 */

#include "gui_convoiinfo.h"

#include "../../simworld.h"
#include "../../vehicle/simvehicle.h"
#include "../../simconvoi.h"
#include "../../simcolor.h"
#include "../../display/simgraph.h"
#include "../../display/viewport.h"
#include "../../player/simplay.h"
#include "../../simline.h"

#include "../../dataobj/translator.h"

#include "../../utils/simstring.h"
#include "../../utils/cbuffer_t.h"
#include "../schedule_gui.h"



gui_convoiinfo_t::gui_convoiinfo_t(convoihandle_t cnv, bool show_line_name):
	formation(cnv),
	loading_bar(cnv)
{
	this->cnv = cnv;
	this->show_line_name = show_line_name;

	set_table_layout(1,0);
	add_table(2, 2)->set_alignment(ALIGN_TOP);

	add_table(1,4)->set_spacing(scr_size(D_H_SPACE, 0));
	{
		add_table(3, 1);
		{
			img_alert.set_image(IMG_EMPTY);
			img_alert.set_rigid(false);
			add_component(&img_alert);
			img_operation.set_image(IMG_EMPTY);
			img_operation.set_rigid(false);
			add_component(&img_operation);
			add_component(&label_name);
		}
		end_table();

		add_component(&label_line);

		add_table(2,1)->set_spacing(scr_size(0, 0));
		{
			new_component<gui_margin_t>(LINESPACE/2, 0);
			loading_bar.set_size_fixed(true);
			add_component(&loading_bar);
		}
		end_table();

		add_table(4,1);
		{
			new_component<gui_margin_t>(LINESPACE/3, LINEASCENT);
			add_component(&switchable_label_title);
			add_component(&switchable_label_value);
			new_component<gui_empty_t>();
		}
		end_table();
	}
	end_table();

	add_table(2,1);
	{
		add_component(&formation);
		new_component<gui_fill_t>();
	}
	end_table();

	new_component<gui_margin_t>(100, D_V_SPACE);
	new_component<gui_margin_t>(100, D_V_SPACE);
	end_table();

	update_label();
}



/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 */
bool gui_convoiinfo_t::infowin_event(const event_t *ev)
{
	if(cnv.is_bound()) {
		if(IS_LEFTRELEASE(ev)) {
			cnv->show_info();
			return true;
		}
		else if(IS_RIGHTRELEASE(ev)) {
			karte_ptr_t welt;
			welt->get_viewport()->change_world_position(cnv->get_pos());
			return true;
		}
	}
	return false;
}

const char* gui_convoiinfo_t::get_text() const
{
	return cnv->get_name();
}

void gui_convoiinfo_t::update_label()
{
	if (!cnv.is_bound()) {
		return;
	}
	img_alert.set_visible(false);
	img_operation.set_visible(false);
	if (skinverwaltung_t::alerts) {
		switch (cnv->get_state()) {
		case convoi_t::EMERGENCY_STOP:
			img_alert.set_image(skinverwaltung_t::alerts->get_image_id(2), true);
			img_alert.set_tooltip("Waiting for clearance!");
			img_alert.set_visible(true);
			break;
		case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
		case convoi_t::CAN_START_ONE_MONTH:
		case convoi_t::WAITING_FOR_LOADING_THREE_MONTHS:
			img_alert.set_image(skinverwaltung_t::alerts->get_image_id(3), true);
			img_alert.set_tooltip("Waiting for clearance!");
			img_alert.set_visible(true);
			break;
		case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
		case convoi_t::CAN_START_TWO_MONTHS:
		case convoi_t::WAITING_FOR_LOADING_FOUR_MONTHS:
			img_alert.set_image(skinverwaltung_t::alerts->get_image_id(4), true);
			img_alert.set_tooltip("clf_chk_stucked");
			img_alert.set_visible(true);
			break;
		case convoi_t::OUT_OF_RANGE:
		case convoi_t::NO_ROUTE:
		case convoi_t::NO_ROUTE_TOO_COMPLEX:
			if (skinverwaltung_t::pax_evaluation_icons) {
				img_alert.set_image(skinverwaltung_t::pax_evaluation_icons->get_image_id(4), true);
			}
			else {
				img_alert.set_image(skinverwaltung_t::alerts->get_image_id(4), true);
			}
			img_alert.set_tooltip("clf_chk_noroute");
			img_alert.set_visible(true);
			break;
		default:
			break;
		}
		if (cnv->get_withdraw()) {
			img_operation.set_image(skinverwaltung_t::alerts->get_image_id(2), true);
			img_operation.set_tooltip("withdraw");
			img_operation.set_visible(true);
		}
		else if (cnv->get_replace()) {
			img_operation.set_image(skinverwaltung_t::alerts->get_image_id(1), true);
			img_operation.set_tooltip("Replacing");
			img_operation.set_visible(true);
		}
		else if (cnv->get_no_load()) {
			img_operation.set_image(skinverwaltung_t::alerts->get_image_id(2), true);
			img_operation.set_tooltip("No load setting");
			img_operation.set_visible(true);
		}
	}

	switch (switch_label)
	{
		case 1: // Next stop
		{
			if (!cnv->in_depot()) {
				switchable_label_title.buf().printf("%s: ", translator::translate("Fahrtziel"));
				const schedule_t *schedule = cnv->get_schedule();
				schedule_t::gimme_short_stop_name(switchable_label_value.buf(), world(), cnv->get_owner(), schedule, schedule->get_current_stop(), 50 - strlen(translator::translate("Fahrtziel")));
				switchable_label_title.set_visible(true);
				switchable_label_value.set_visible(true);
			}
			break;
		}
		case 2: // Profit
			switchable_label_title.buf().printf("%s: ", translator::translate("Gewinn"));
			switchable_label_value.buf().append_money(cnv->get_jahresgewinn() / 100.0);
			switchable_label_value.set_color(cnv->get_jahresgewinn() > 0 ? MONEY_PLUS : MONEY_MINUS);
			switchable_label_title.set_visible(true);
			switchable_label_value.set_visible(true);
			break;
		case 4: // Max speed
			switchable_label_title.buf().printf("%s ", translator::translate("Max. speed:"));
			switchable_label_value.buf().printf("%3d km/h", speed_to_kmh(cnv->get_min_top_speed()));
			switchable_label_title.set_visible(true);
			switchable_label_value.set_visible(true);
			break;
		case 5: // Power
			switchable_label_title.buf().printf("%s: ", translator::translate("Leistung"));
			switchable_label_value.buf().printf("%4d kW, %d kN", cnv->get_sum_power()/1000, cnv->get_starting_force().to_sint32()/1000);
			switchable_label_title.set_visible(true);
			switchable_label_value.set_visible(true);
			break;
		case 6: // Value
			switchable_label_title.buf().printf("%s: ", translator::translate("cl_btn_sort_value"));
			switchable_label_value.buf().append_money(cnv->get_purchase_cost()/100.0);
			switchable_label_title.set_visible(true);
			switchable_label_value.set_visible(true);
			break;
		case 7: // average age(month)
			switchable_label_title.buf().printf("%s: ", translator::translate("cl_btn_sort_age"));
			switchable_label_value.buf().printf(cnv->get_average_age() == 1 ? translator::translate("%i month") : translator::translate("%i months"), cnv->get_average_age());
			switchable_label_title.set_visible(true);
			switchable_label_value.set_visible(true);
			break;
		case 0:
		default:
			switchable_label_title.set_visible(false);
			switchable_label_value.set_visible(false);
			break;
	}

	switchable_label_title.update();
	switchable_label_value.update();
	label_line.set_visible(true);

	if (cnv->in_depot()) {
		label_line.buf().append(translator::translate("(in depot)"));
		label_line.set_color(SYSCOL_TEXT_HIGHLIGHT);
		loading_bar.set_visible(false);
	}
	else if (cnv->get_line().is_bound() && show_line_name) {
		label_line.buf().printf("  %s %s", translator::translate("Line"), cnv->get_line()->get_name());
		label_line.set_color(SYSCOL_TEXT);
		loading_bar.set_visible(true);
	}
	else {
		label_line.buf();
		label_line.set_visible(false);
		loading_bar.set_visible(true);
	}
	label_line.update();

	label_name.buf().append(cnv->get_name());
	label_name.update();
	label_name.set_color(cnv->get_status_color());

	set_size(get_size());
}

/**
 * Draw the component
 */
void gui_convoiinfo_t::draw(scr_coord offset)
{
	update_label();
	gui_aligned_container_t::draw(offset);
}

void gui_convoiinfo_t::set_mode(uint8 mode)
{
	if (mode < gui_convoy_formation_t::CONVOY_OVERVIEW_MODES) {
		formation.set_mode(mode);
	}
}
