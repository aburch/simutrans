/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include "../simhalt.h"
#include "../simmenu.h"
#include "../simplan.h"
#include "../simworld.h"

#include "../descriptor/building_desc.h"
#include "../display/viewport.h"
#include "../obj/gebaeude.h"
#include "../player/simplay.h"

#include "building_info.h"
#include "simwin.h"
#include "components/gui_image.h"
#include "components/gui_colorbox.h"

building_info_t::building_info_t(gebaeude_t* gb, player_t* owner) : base_infowin_t(translator::translate(gb->get_name()), owner),
building_view(koord3d::invalid, scr_size(max(64, get_base_tile_raster_width()), max(56, (get_base_tile_raster_width() * 7) / 8))),
scrolly_stats(&cont_stats, true),
scrolly_near_by_halt(&cont_near_by_halt, true),
scrolly_signalbox(&cont_signalbox_info, true)
{
	this->owner = owner;
	tile = gb->get_first_tile();

	building_view.set_location(tile->get_pos());
	set_embedded(&building_view);

	add_table(1,0)->set_margin(scr_size(0,0), scr_size(0, 0));
	tabs.add_tab(&scrolly_stats, translator::translate("Statistics"));
	tabs.add_tab(&scrolly_near_by_halt, translator::translate("Stops potentially within walking distance:"));
	add_component(&tabs);
	end_table();

	cont_stats.set_table_layout(1, 0);
	cont_stats.set_margin(scr_size(D_H_SPACE, D_V_SPACE), scr_size(D_MARGIN_RIGHT, 0));
	cont_near_by_halt.set_table_layout(7,0);
	cont_near_by_halt.set_margin(scr_size(D_H_SPACE, D_V_SPACE), scr_size(D_MARGIN_RIGHT, 0));

	if (tile->is_signalbox()) {
		tabs.add_tab(&scrolly_signalbox, translator::translate("Signalbox info."));
	}
	init_class_table();
	init_stats_table();
	update_near_by_halt();
	tile->info(buf);
	recalc_size();
	set_resizemode(vertical_resize);
	set_windowsize(scr_size(get_min_windowsize().w, textarea.get_size().h + cont_stats.get_size().h + D_MARGINS_Y*2 + D_V_SPACE*2 + D_TITLEBAR_HEIGHT + D_TAB_HEADER_HEIGHT));
}


bool building_info_t::is_weltpos()
{
	return (welt->get_viewport()->is_on_center(tile->get_pos()));
}

void building_info_t::init_class_table()
{
	scr_coord_val value_cell_width = max(proportional_string_width(" 888.8%"), 60);
	const bool show_population = tile->get_tile()->get_desc()->get_type() == building_desc_t::city_res;
	const bool show_job_info   = (tile->get_adjusted_jobs() && !show_population);
	const bool show_visitor_demands = (tile->get_adjusted_visitor_demand() && !show_population);
	if (show_population) {
		cont_stats.new_component<gui_heading_t>("residents_wealth", SYSCOL_TEXT, SYSCOL_STATUSBAR_BACKGROUND, 1);
	}
	else if (show_visitor_demands && show_job_info) {
		cont_stats.new_component<gui_heading_t>("wealth_of_visitors_/_commuters", SYSCOL_TEXT, SYSCOL_STATUSBAR_BACKGROUND, 1);
	}
	else if (show_job_info){
		cont_stats.new_component<gui_heading_t>("wealth_of_commuters", SYSCOL_TEXT, SYSCOL_STATUSBAR_BACKGROUND, 1);
	}
	else if (show_visitor_demands) {
		cont_stats.new_component<gui_heading_t>("wealth_of_visitors",  SYSCOL_TEXT, SYSCOL_STATUSBAR_BACKGROUND, 1);
	}
	else {
		return; // no demand
	}

	// passenger class table
	const uint8 cols = 3 + show_population + show_job_info + show_visitor_demands;
	cont_stats.add_table(cols,0);

	for (uint8 c = 0; c < goods_manager_t::passengers->get_number_of_classes(); c++) {
		cont_stats.new_component<gui_margin_t>(D_MARGIN_LEFT);
		p_class_names[c].buf().append(goods_manager_t::get_translated_wealth_name(goods_manager_t::INDEX_PAS, c));
		p_class_names[c].update();
		cont_stats.new_component<gui_label_t>(p_class_names[c]);

		if (show_population) {
			cont_stats.new_component<gui_data_bar_t>()->init(tile->get_adjusted_population_by_class(c), tile->get_adjusted_population(), value_cell_width, color_idx_to_rgb(COL_DARK_GREEN+1), false, true);
		}
		if (show_visitor_demands) {
			cont_stats.new_component<gui_data_bar_t>()->init(tile->get_adjusted_visitor_demand_by_class(c), tile->get_adjusted_visitor_demand(), value_cell_width, goods_manager_t::passengers->get_color(), false, true);
		}
		if (show_job_info) {
			cont_stats.new_component<gui_data_bar_t>()->init(tile->get_adjusted_jobs_by_class(c), tile->get_adjusted_jobs(), value_cell_width, color_idx_to_rgb(COL_COMMUTER-1), false, true);
		}
		cont_stats.new_component<gui_fill_t>();
	}
	cont_stats.end_table();
	cont_stats.new_component<gui_margin_t>(0, D_V_SPACE);
}

void building_info_t::init_stats_table()
{
	scr_coord_val value_cell_width = max(proportional_string_width(translator::translate("This Year")), proportional_string_width(translator::translate("Last Year")));

	if (tile->get_tile()->get_desc()->get_type() != building_desc_t::city_res || tile->get_adjusted_mail_demand()) {
		cont_stats.new_component<gui_heading_t>("Trip data", SYSCOL_TEXT, SYSCOL_STATUSBAR_BACKGROUND, 1);
		cont_stats.add_table(5,0);
		{
			// header
			cont_stats.new_component<gui_margin_t>(8);
			cont_stats.new_component<gui_margin_t>(D_BUTTON_WIDTH);
			cont_stats.new_component<gui_label_t>("This Year");
			cont_stats.new_component<gui_label_t>("Last Year");
			cont_stats.new_component<gui_fill_t>();

			if (tile->get_tile()->get_desc()->get_type() != building_desc_t::city_res) {
				// Buildings other than houses -> display arrival data
				if (tile->get_adjusted_visitor_demand()) {
					cont_stats.new_component<gui_colorbox_t>(goods_manager_t::passengers->get_color())->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
					cont_stats.new_component<gui_label_t>("Visitor arrivals");
					lb_visitor_arrivals[0].set_fixed_width(value_cell_width);
					lb_visitor_arrivals[1].set_fixed_width(value_cell_width);
					lb_visitor_arrivals[0].set_align(gui_label_t::right);
					lb_visitor_arrivals[1].set_align(gui_label_t::right);
					cont_stats.add_component(&lb_visitor_arrivals[0]);
					cont_stats.add_component(&lb_visitor_arrivals[1]);
					cont_stats.new_component<gui_fill_t>();
				}

				cont_stats.new_component<gui_colorbox_t>(color_idx_to_rgb(COL_COMMUTER))->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
				cont_stats.new_component<gui_label_t>("Commuter arrivals");
				lb_commuter_arrivals[0].set_fixed_width(value_cell_width);
				lb_commuter_arrivals[1].set_fixed_width(value_cell_width);
				lb_commuter_arrivals[0].set_align(gui_label_t::right);
				lb_commuter_arrivals[1].set_align(gui_label_t::right);
				cont_stats.add_component(&lb_commuter_arrivals[0]);
				cont_stats.add_component(&lb_commuter_arrivals[1]);
				cont_stats.new_component<gui_fill_t>();
			}
			if (tile->get_adjusted_mail_demand()) {
				cont_stats.new_component<gui_colorbox_t>(goods_manager_t::mail->get_color())->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
				cont_stats.new_component<gui_label_t>("hd_mailing");
				lb_mail_sent[0].set_fixed_width(value_cell_width);
				lb_mail_sent[1].set_fixed_width(value_cell_width);
				lb_mail_sent[0].set_align(gui_label_t::right);
				lb_mail_sent[1].set_align(gui_label_t::right);
				cont_stats.add_component(&lb_mail_sent[0]);
				cont_stats.add_component(&lb_mail_sent[1]);
				cont_stats.new_component<gui_fill_t>();
			}
		}
		cont_stats.end_table();
		cont_stats.new_component<gui_margin_t>(0, D_V_SPACE);
	}

	if (tile->get_tile()->get_desc()->get_type() == building_desc_t::city_res) {
		cont_stats.new_component<gui_heading_t>("Success rate", SYSCOL_TEXT, SYSCOL_STATUSBAR_BACKGROUND, 1);
		cont_stats.add_table(5, 0);
		{
			// header
			cont_stats.new_component<gui_margin_t>(8);
			cont_stats.new_component<gui_margin_t>(D_BUTTON_WIDTH);
			cont_stats.new_component<gui_label_t>("This Year");
			cont_stats.new_component<gui_label_t>("Last Year");
			cont_stats.new_component<gui_fill_t>();

			cont_stats.new_component<gui_colorbox_t>(goods_manager_t::passengers->get_color())->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
			cont_stats.new_component<gui_label_t>("Visiting trip");
			lb_visiting_success_rate[0].set_fixed_width(value_cell_width);
			lb_visiting_success_rate[1].set_fixed_width(value_cell_width);
			lb_visiting_success_rate[0].set_align(gui_label_t::right);
			lb_visiting_success_rate[1].set_align(gui_label_t::right);
			cont_stats.add_component(&lb_visiting_success_rate[0]);
			cont_stats.add_component(&lb_visiting_success_rate[1]);
			cont_stats.new_component<gui_fill_t>();

			cont_stats.new_component<gui_colorbox_t>(color_idx_to_rgb(COL_COMMUTER))->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
			cont_stats.new_component<gui_label_t>("Commuting trip");
			lb_commuting_success_rate[0].set_fixed_width(value_cell_width);
			lb_commuting_success_rate[1].set_fixed_width(value_cell_width);
			lb_commuting_success_rate[0].set_align(gui_label_t::right);
			lb_commuting_success_rate[1].set_align(gui_label_t::right);
			cont_stats.add_component(&lb_commuting_success_rate[0]);
			cont_stats.add_component(&lb_commuting_success_rate[1]);
			cont_stats.new_component<gui_fill_t>();

			// show only if this building has mail demands
			if(tile->get_adjusted_mail_demand()) {
				cont_stats.new_component<gui_colorbox_t>(goods_manager_t::mail->get_color())->set_size(scr_size(LINESPACE/2 + 2, LINESPACE/2 + 2));
				cont_stats.new_component<gui_label_t>("hd_mailing");
				lb_mail_success_rate[0].set_fixed_width(value_cell_width);
				lb_mail_success_rate[1].set_fixed_width(value_cell_width);
				lb_mail_success_rate[0].set_align(gui_label_t::right);
				lb_mail_success_rate[1].set_align(gui_label_t::right);
				cont_stats.add_component(&lb_mail_success_rate[0]);
				cont_stats.add_component(&lb_mail_success_rate[1]);
				cont_stats.new_component<gui_fill_t>();
			}
		}
		cont_stats.end_table();
	}

	update_stats();
}

void building_info_t::update_stats()
{
	if (tile->get_tile()->get_desc()->get_type() == building_desc_t::city_res) {
		lb_visiting_success_rate[0].buf().printf( tile->get_passenger_success_percent_this_year_visiting()<65535 ? "%u%%" : "-",  tile->get_passenger_success_percent_this_year_visiting());
		lb_visiting_success_rate[1].buf().printf( tile->get_passenger_success_percent_last_year_visiting()<65535 ? "%u%%" : "-",  tile->get_passenger_success_percent_last_year_visiting());
		lb_commuting_success_rate[0].buf().printf(tile->get_passenger_success_percent_this_year_commuting()<65535 ? "%u%%" : "-", tile->get_passenger_success_percent_this_year_commuting());
		lb_commuting_success_rate[1].buf().printf(tile->get_passenger_success_percent_last_year_commuting()<65535 ? "%u%%" : "-", tile->get_passenger_success_percent_last_year_commuting());
		lb_visiting_success_rate[0].update();
		lb_visiting_success_rate[1].update();
		lb_commuting_success_rate[0].update();
		lb_commuting_success_rate[1].update();
	}
	else {
		if (tile->get_adjusted_visitor_demand()) {
			lb_visitor_arrivals[0].buf().printf(tile->get_passengers_succeeded_visiting() < 65535 ? "%u" : "-", tile->get_passengers_succeeded_visiting());
			lb_visitor_arrivals[1].buf().printf(tile->get_passenger_success_percent_last_year_visiting() < 65535 ? "%u" : "-", tile->get_passenger_success_percent_last_year_visiting());
			lb_visitor_arrivals[0].update();
			lb_visitor_arrivals[1].update();
		}
		lb_commuter_arrivals[0].buf().printf(tile->get_passengers_succeeded_commuting() < 65535 ? "%u" : "-", tile->get_passengers_succeeded_commuting());
		lb_commuter_arrivals[1].buf().printf(tile->get_passenger_success_percent_last_year_commuting() < 65535 ? "%u" : "-", tile->get_passenger_success_percent_last_year_commuting());
		lb_commuter_arrivals[0].update();
		lb_commuter_arrivals[1].update();
	}

	if (tile->get_adjusted_mail_demand()) {
		lb_mail_success_rate[0].buf().printf(tile->get_mail_delivery_success_percent_this_year()<65535 ? "%u%%" : "-", tile->get_mail_delivery_success_percent_this_year());
		lb_mail_success_rate[1].buf().printf(tile->get_mail_delivery_success_percent_last_year()<65535 ? "%u%%" : "-", tile->get_mail_delivery_success_percent_last_year());
		lb_mail_success_rate[0].update();
		lb_mail_success_rate[1].update();
		lb_mail_sent[0].buf().printf(tile->get_mail_delivery_succeeded() < 65535 ? "%u" : "-", tile->get_mail_delivery_succeeded());
		lb_mail_sent[1].buf().printf(tile->get_mail_delivery_succeeded_last_year() < 65535 ? "%u" : "-", tile->get_mail_delivery_succeeded_last_year());
		lb_mail_sent[0].update();
		lb_mail_sent[1].update();
	}
}

void building_info_t::update_near_by_halt()
{
	cont_near_by_halt.remove_all();
	// List of stops potentially within walking distance.
	const planquadrat_t* plan = welt->access(tile->get_pos().get_2d());
	const nearby_halt_t *const halt_list = plan->get_haltlist();
	bool any_operating_stops_passengers = false;
	bool any_operating_stops_mail = false;
	uint16 max_walking_time;

	// header
	if (plan->get_haltlist_count()>0) {
		cont_near_by_halt.new_component<gui_margin_t>(8);
		cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::passengers->get_image_id(0), true);
		cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::mail->get_image_id(0), true);
		cont_near_by_halt.new_component_span<gui_empty_t>(3);
		cont_near_by_halt.new_component<gui_empty_t>();
	}

	for (int h = 0; h < plan->get_haltlist_count(); h++) {
		const halthandle_t halt = halt_list[h].halt;
		if (!halt->is_enabled(goods_manager_t::passengers) && !halt->is_enabled(goods_manager_t::mail)) {
			continue; // Exclude freight stations as this is not for a factory window
		}
		// distance
		const uint32 distance = halt_list[h].distance * world()->get_settings().get_meters_per_tile();
		gui_label_buf_t *l = cont_near_by_halt.new_component<gui_label_buf_t>();
		if (distance < 1000) {
			l->buf().printf("%um", distance);
		}
		else {
			l->buf().append((float)(distance/1000.0), 2);
			l->buf().append("km");
		}
		l->set_align(gui_label_t::right);
		l->update();

		// Service operation indicator
		if (halt->is_enabled(goods_manager_t::passengers)) {
			if (halt->gibt_ab(goods_manager_t::get_info(goods_manager_t::INDEX_PAS))) {
				any_operating_stops_passengers = true;
				// If it is crowded, display the overcrowding color
				cont_near_by_halt.new_component<gui_colorbox_t>()->init(halt->is_overcrowded(goods_manager_t::INDEX_PAS) ? color_idx_to_rgb(COL_OVERCROWD) : color_idx_to_rgb(COL_GREEN), scr_size(10,D_INDICATOR_HEIGHT), true, false);
			}
			else {
				// there is an attribute but the service does not provide it
				cont_near_by_halt.new_component<gui_colorbox_t>()->init(COL_INACTIVE, scr_size(10, D_INDICATOR_HEIGHT), true, false);
			}
		}
		else {
			cont_near_by_halt.new_component<gui_margin_t>(8);
		}
		if (halt->is_enabled(goods_manager_t::mail)) {
			if (halt->gibt_ab(goods_manager_t::get_info(goods_manager_t::INDEX_MAIL))) {
				any_operating_stops_mail = true;
				cont_near_by_halt.new_component<gui_colorbox_t>()->init(halt->is_overcrowded(goods_manager_t::INDEX_MAIL) ? color_idx_to_rgb(COL_OVERCROWD) : color_idx_to_rgb(COL_GREEN), scr_size(10, D_INDICATOR_HEIGHT), true, false);
			}
			else {
				// there is an attribute but the service does not provide it
				cont_near_by_halt.new_component<gui_colorbox_t>()->init(COL_INACTIVE, scr_size(10, D_INDICATOR_HEIGHT), true, false);
			}
		}
		else {
			cont_near_by_halt.new_component<gui_margin_t>(8);
		}

		// station name with owner color
		cont_near_by_halt.new_component<gui_label_t>(halt->get_name(), color_idx_to_rgb(halt->get_owner()->get_player_color1()+2), gui_label_t::left);

		if (skinverwaltung_t::on_foot) {
			cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::on_foot->get_image_id(0), true);
		}
		else {
			cont_near_by_halt.new_component<gui_label_t>("Walking time");
		}

		const uint16 walking_time = welt->walking_time_tenths_from_distance(halt_list[h].distance);
		char walking_time_as_clock[32];
		welt->sprintf_time_tenths(walking_time_as_clock, sizeof(walking_time_as_clock), walking_time);
		gui_label_buf_t *lb_wt = cont_near_by_halt.new_component<gui_label_buf_t>();
		lb_wt->buf().printf("%s %s", skinverwaltung_t::on_foot ? "" : ":", walking_time_as_clock);
		lb_wt->update();
		cont_near_by_halt.new_component<gui_fill_t>();
	}
	// footer
	if (plan->get_haltlist_count() > 0) {
		cont_near_by_halt.new_component<gui_margin_t>(8);
		if (skinverwaltung_t::alerts && !any_operating_stops_passengers) {
			cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::alerts->get_image_id(2), true);
		}
		else {
			cont_near_by_halt.new_component<gui_empty_t>();
		}
		if (skinverwaltung_t::alerts && !any_operating_stops_mail) {
			cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::alerts->get_image_id(2), true);
		}
		else {
			cont_near_by_halt.new_component<gui_empty_t>();
		}
		cont_near_by_halt.new_component_span<gui_empty_t>(3);
		cont_near_by_halt.new_component<gui_empty_t>();
	}

	if (!any_operating_stops_passengers) {
		cont_near_by_halt.new_component_span<gui_margin_t>((D_H_SPACE,D_V_SPACE), 7);
		cont_near_by_halt.new_component<gui_empty_t>();
		if (skinverwaltung_t::alerts) {
			cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::alerts->get_image_id(2), true);
		}
		else {
			cont_near_by_halt.new_component<gui_empty_t>();
		}
		cont_near_by_halt.new_component_span<gui_label_t>("No passenger service", 4);
		cont_near_by_halt.new_component<gui_empty_t>();
	}
	if (!any_operating_stops_mail) {
		cont_near_by_halt.new_component_span<gui_margin_t>((D_H_SPACE, D_V_SPACE), 7);
		cont_near_by_halt.new_component<gui_empty_t>();
		if (skinverwaltung_t::alerts) {
			cont_near_by_halt.new_component<gui_image_t>()->set_image(skinverwaltung_t::alerts->get_image_id(2), true);
		}
		else {
			cont_near_by_halt.new_component<gui_empty_t>();
		}
		cont_near_by_halt.new_component_span<gui_label_t>("No mail service", 4);
		cont_near_by_halt.new_component<gui_empty_t>();
	}

	reset_min_windowsize();
}


void building_info_t::draw(scr_coord pos, scr_size size)
{
	buf.clear();
	tile->info(buf);

	switch (tabs.get_active_tab_index())
	{
		case 0:
			update_stats(); break;
		case 1:
			update_near_by_halt(); break;
		default:
			break;
	}
	if (get_embedded() == NULL) {
		// no building anymore, destroy window
		destroy_win(this);
		return;
	}
	base_infowin_t::draw(pos, size);
}
