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



class gui_convoi_images_t : public gui_component_t
{
	convoihandle_t cnv;
public:
	gui_convoi_images_t(convoihandle_t cnv) { this->cnv = cnv; }

	scr_size get_min_size() const OVERRIDE
	{
		return draw_vehicles( scr_coord(0,0), false);
	}

	scr_size draw_vehicles(scr_coord offset, bool display_images) const
	{
		scr_coord p = offset + get_pos();
		p.y += get_size().h/2;
		// we will use their images offsets and width to shift them to their correct position
		// this should work with any vehicle size ...
		scr_size s(0,0);
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

	scr_size get_max_size() const OVERRIDE { return get_min_size(); }

	void draw( scr_coord offset ) OVERRIDE
	{
		draw_vehicles( offset, true);
	}
};

const char *gui_convoiinfo_t::cnvlist_mode_button_texts[gui_convoiinfo_t::DISPLAY_MODES] = {
	"cl_btn_general",
	"cl_btn_payload",
	"cl_btn_formation"
};


gui_convoiinfo_t::gui_convoiinfo_t(convoihandle_t cnv, bool show_line_name):
	formation(cnv),
	payload(cnv)
{
	this->cnv = cnv;
	this->show_line_name = show_line_name;

	set_table_layout(2,1);
	set_alignment(ALIGN_LEFT | ALIGN_TOP);

	add_table(1,3);
	{
		add_component(&label_name);
		add_component(&label_line);
		add_table(2,1);
		{
			new_component<gui_label_t>("Gewinn");
			add_component(&label_profit);
		}
		end_table();
	}
	end_table();

	add_table(1,2);
	{
		new_component<gui_convoi_images_t>(cnv);
		add_component(&filled_bar);
	}
	end_table();

	filled_bar.add_color_value(&cnv->get_loading_limit(), color_idx_to_rgb(COL_YELLOW));
	filled_bar.add_color_value(&cnv->get_loading_level(), color_idx_to_rgb(COL_GREEN));

	// FIXME
	//add_table(1,2);
	//{
	//	formation.set_pos(scr_coord(0, D_MARGIN_TOP));
	//	payload.set_pos(scr_coord(0, D_MARGIN_TOP));
	//}
	//end_table();

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
	label_profit.buf().append_money(cnv->get_jahresgewinn() / 100.0);
	label_profit.update();
	label_line.set_visible(true);

	if (cnv->in_depot()) {
		label_line.buf().append(translator::translate("(in depot)"));
	}
	else if (cnv->get_line().is_bound()) {
		label_line.buf().printf("%s %s", translator::translate("Line"), cnv->get_line()->get_name());
	}
	else {
		label_line.buf();
		label_line.set_visible(false);
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
		// 1st row: convoy name
		int w = 0;
		if (skinverwaltung_t::alerts) {
			switch (cnv->get_state()) {
				case convoi_t::EMERGENCY_STOP:
					display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("Waiting for clearance!"));
					w += 14;
					break;
				case convoi_t::WAITING_FOR_CLEARANCE_ONE_MONTH:
				case convoi_t::CAN_START_ONE_MONTH:
					display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(3), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("Waiting for clearance!"));
					w += 14;
					break;
				case convoi_t::WAITING_FOR_CLEARANCE_TWO_MONTHS:
				case convoi_t::CAN_START_TWO_MONTHS:
					display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(4), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("clf_chk_stucked"));
					w += 14;
					break;
				case convoi_t::OUT_OF_RANGE:
				case convoi_t::NO_ROUTE:
				case convoi_t::NO_ROUTE_TOO_COMPLEX:
					display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(4), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("clf_chk_noroute"));
					w += 14;
					break;
				default:
					break;
			}
			if (cnv->get_withdraw()){
				display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("withdraw"));
				w += 14;
			}
			else if (cnv->get_no_load() && !cnv->in_depot()) {
				display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(2), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("No load setting"));
				w += 14;
			}
			if (cnv->get_replace()) {
				display_color_img_with_tooltip(skinverwaltung_t::alerts->get_image_id(1), pos.x + offset.x + 2 + w, pos.y + offset.y + 6, 0, false, false, translator::translate("Replacing"));
				w += 14;
			}
		}
		// name, use the convoi status color for redraw: Possible colors are YELLOW (not moving) BLUE: obsolete in convoi, RED: minus income, BLACK: ok
		int max_x = display_proportional_clip_rgb(pos.x+offset.x+2, pos.y+offset.y+6, cnv->get_name(), ALIGN_LEFT, cnv->get_status_color(), true)+2;

		// 2nd row
		w = D_MARGIN_LEFT;
		if (display_mode == cnvlist_normal) {
			w += display_proportional_clip_rgb(pos.x + offset.x + w, pos.y + offset.y + 6 + LINESPACE, translator::translate("Gewinn"), ALIGN_LEFT, SYSCOL_TEXT, true) + 2;
			char buf[256];
			money_to_string(buf, cnv->get_jahresgewinn() / 100.0);
			w += display_proportional_clip_rgb(pos.x + offset.x + w + 5, pos.y + offset.y + 6 + LINESPACE, buf, ALIGN_LEFT, cnv->get_jahresgewinn() > 0 ? MONEY_PLUS : MONEY_MINUS, true);
			max_x = max(max_x, w + 5);
		}
		else if (display_mode == cnvlist_payload) {
			payload.set_cnv(cnv);
			payload.draw(pos + offset + scr_coord(0, LINESPACE + 4));
		}
		else if (display_mode == cnvlist_formation) {
			formation.set_cnv(cnv);
			formation.draw(pos + offset + scr_coord(0, LINESPACE + 6));
		}

		// 3rd row
		w = D_MARGIN_LEFT;
		if (display_mode == cnvlist_normal || display_mode == cnvlist_payload)
		{
			// only show assigned line, if there is one!
			if (cnv->in_depot()) {
				const char *txt = translator::translate("(in depot)");
				w += display_proportional_clip_rgb(pos.x + offset.x + w, pos.y + offset.y + 6 + 2 * LINESPACE, txt, ALIGN_LEFT, SYSCOL_TEXT, true) + 2;
				max_x = max(max_x, w);
			}
			else if (cnv->get_line().is_bound() && show_line_name) {
				w += display_proportional_clip_rgb(pos.x + offset.x + w, pos.y + offset.y + 6 + 2 * LINESPACE, translator::translate("Line"), ALIGN_LEFT, SYSCOL_TEXT, true) + 2;
				w += display_proportional_clip_rgb(pos.x + offset.x + w + 5, pos.y + offset.y + 6 + 2 * LINESPACE, cnv->get_line()->get_name(), ALIGN_LEFT, cnv->get_line()->get_state_color(), true);
				max_x = max(max_x, w + 5);
			}
			else if(!show_line_name && display_mode == cnvlist_payload){
				// next stop
				cbuffer_t info_buf;
				info_buf.printf("%s: ", translator::translate("Fahrtziel")); // "Destination"
				const schedule_t *schedule = cnv->get_schedule();
				schedule_t::gimme_short_stop_name(info_buf, welt, cnv->get_owner(), schedule, schedule->get_current_stop(), 255);
				display_proportional_clip_rgb(pos.x + offset.x + w, pos.y + offset.y + 6 + 2 * LINESPACE, info_buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			}

			if (display_mode == cnvlist_normal) {
				// we will use their images offsets and width to shift them to their correct position
				// this should work with any vehicle size ...
				const int xoff = max(190, max_x);
				int left = pos.x + offset.x + xoff + 4;
				for (unsigned i = 0; i < cnv->get_vehicle_count();i++) {
					scr_coord_val x, y, w, h;
					const image_id image = cnv->get_vehicle(i)->get_loaded_image();
					display_get_base_image_offset(image, x, y, w, h);
					display_base_img(image, left - x, pos.y + offset.y + 13 - y - h / 2, cnv->get_owner()->get_player_nr(), false, true);
					left += (w * 2) / 3;
				}

				// since the only remaining object is the loading bar, we can alter its position this way ...
				filled_bar.draw(pos + offset + scr_coord(xoff, 0));
			}
		}
	//}
//======= */
	update_label();
	gui_aligned_container_t::draw(offset);
}

void gui_convoiinfo_t::set_mode(uint8 mode)
{
	if (mode < DISPLAY_MODES) {
		display_mode = mode;
	}
}
