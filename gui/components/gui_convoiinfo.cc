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
#include "../gui_frame.h"
#include "../schedule_gui.h"



gui_convoiinfo_t::gui_convoiinfo_t(convoihandle_t cnv, bool show_line_name):
	formation(cnv),
	loading_bar(cnv)
{
	this->cnv = cnv;
	this->show_line_name = show_line_name;

	set_table_layout(2,1);
	set_alignment(ALIGN_LEFT);

	add_table(1,4)->set_spacing(scr_size(D_H_SPACE, 0));
	{
		set_alignment(ALIGN_LEFT);
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
			new_component<gui_margin_t>(LINESPACE/3);
			add_component(&switchable_label_title);
			add_component(&switchable_label_value);
			new_component<gui_fill_t>();
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


	update_label();
}



/**
 * Events werden hiermit an die GUI-components
 * gemeldet
 * @author Hj. Malthaner
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
		case 1:
			switchable_label_title.buf().printf("%s: ", translator::translate("Gewinn")); // Profit
			switchable_label_value.buf().append_money(cnv->get_jahresgewinn() / 100.0);
			switchable_label_value.set_color(cnv->get_jahresgewinn() > 0 ? MONEY_PLUS : MONEY_MINUS);
			switchable_label_title.set_visible(true);
			switchable_label_value.set_visible(true);
			break;
		default:
			switchable_label_title.set_visible(false);
			switchable_label_value.set_visible(false);
			break;
	}
	//switchable_label_title.buf().printf("%s: ", translator::translate("Fahrtziel")); // "Destination"

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
 * @author Hj. Malthaner
 */
void gui_convoiinfo_t::draw(scr_coord offset)
{
/*
//<<<<<<< HEAD
	clip_dimension clip = display_get_clip_wh();
	if(! ((pos.y+offset.y) > clip.yy ||  (pos.y+offset.y) < clip.y-32) &&  cnv.is_bound()) {
		// 2nd row
		if (display_mode == cnvlist_normal) {
		}
		else if (display_mode == cnvlist_payload) {
			payload.set_cnv(cnv);
			payload.draw(pos + offset + scr_coord(0, LINESPACE + 4));
		}

		// 3rd row
		w = D_MARGIN_LEFT;
		if (display_mode == cnvlist_normal || display_mode == cnvlist_payload)
		{
			// only show assigned line, if there is one!
			if (cnv->in_depot()) {
			}
			else if(!show_line_name && display_mode == cnvlist_payload){
				// next stop
				cbuffer_t info_buf;
				info_buf.printf("%s: ", translator::translate("Fahrtziel")); // "Destination"
				const schedule_t *schedule = cnv->get_schedule();
				schedule_t::gimme_short_stop_name(info_buf, welt, cnv->get_owner(), schedule, schedule->get_current_stop(), 255);
				display_proportional_clip_rgb(pos.x + offset.x + w, pos.y + offset.y + 6 + 2 * LINESPACE, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			}

		}
	//}
//======= */
	update_label();
	gui_aligned_container_t::draw(offset);
}

void gui_convoiinfo_t::set_mode(uint8 mode)
{
	if (mode < gui_convoy_formation_t::CONVOY_OVERVIEW_MODES) {
		formation.set_mode(mode);
	}
}
