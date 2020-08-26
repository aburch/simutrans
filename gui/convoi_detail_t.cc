/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "convoi_detail_t.h"

#include "../simunits.h"
#include "../simconvoi.h"
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../display/simgraph.h"
#include "../simworld.h"
#include "../gui/simwin.h"

#include "../dataobj/schedule.h"
#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"
// @author hsiegeln
#include "../simline.h"
#include "messagebox.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"

#include "components/gui_chart.h"

#include "../obj/roadsign.h"
#include "vehicle_class_manager.h"



#define LOADING_BAR_WIDTH 150
#define LOADING_BAR_HEIGHT 5

convoi_detail_t::convoi_detail_t(convoihandle_t cnv) :
	gui_frame_t( cnv->get_name(), cnv->get_owner() ),
	scrolly(&veh_info),
	scrolly_formation(&formation),
	scrolly_payload_info(&cont_payload),
	scrolly_maintenance(&maintenance),
	veh_info(cnv),
	formation(cnv),
	payload_info(cnv),
	maintenance(cnv)
{
	this->cnv = cnv;

	sale_button.init(button_t::roundbox, "Verkauf", scr_coord(BUTTON4_X, 0), D_BUTTON_SIZE);
	sale_button.set_tooltip("Remove vehicle from map. Use with care!");
	sale_button.add_listener(this);
	add_component(&sale_button);

	withdraw_button.init(button_t::roundbox, "withdraw", scr_coord(BUTTON3_X, 0), D_BUTTON_SIZE);
	withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
	withdraw_button.add_listener(this);
	add_component(&withdraw_button);

	retire_button.init(button_t::roundbox, "Retire", scr_coord(BUTTON3_X, 16), D_BUTTON_SIZE);
	retire_button.set_tooltip("Convoi is sent to depot when all wagons are empty.");
	add_component(&retire_button);
	retire_button.add_listener(this);

	class_management_button.init(button_t::roundbox, "class_manager", scr_coord(BUTTON4_X, 16), D_BUTTON_SIZE);
	class_management_button.set_tooltip("see_and_change_the_class_assignments");
	add_component(&class_management_button);
	class_management_button.add_listener(this);

	scrolly_formation.set_pos(scr_coord(0, LINESPACE*4));
	scrolly_formation.set_show_scroll_x(true);
	scrolly_formation.set_show_scroll_y(false);
	scrolly_formation.set_scroll_discrete_x(false);
	scrolly_formation.set_size_corner(false);
	add_component(&scrolly_formation);

	int header_height = D_TITLEBAR_HEIGHT + LINESPACE * 7 + 4;

	scrolly.set_show_scroll_x(true);

	if (cnv->get_vehicle_count() > 1) {
		display_detail_button.init(button_t::square_state, "Display loaded detail", scr_coord(BUTTON3_X + D_BUTTON_SIZE.w / 2, LINESPACE / 2));
		display_detail_button.set_tooltip("Displays detailed information of the vehicle's load.");
		display_detail_button.add_listener(this);
		display_detail_button.pressed = true;
		cont_payload.add_component(&display_detail_button);
	}
	payload_info.set_show_detail(true);
	cont_payload.add_component(&payload_info);

	tabs.add_tab(&scrolly, translator::translate("cd_spec_tab"));
	tabs.add_tab(&scrolly_payload_info, translator::translate("cd_payload_tab"));
	tabs.add_tab(&scrolly_maintenance, translator::translate("cd_maintenance_tab"));
	tabs.set_pos(scr_coord(0, header_height));

	add_component(&tabs);
	tabs.add_listener(this);

	set_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+17*(LINESPACE+1)+D_SCROLLBAR_HEIGHT-6));
	set_min_windowsize(scr_size(D_DEFAULT_WIDTH, D_TITLEBAR_HEIGHT+50+10*(LINESPACE+1)+D_SCROLLBAR_HEIGHT-3));

	set_resizemode(diagonal_resize);
	resize(scr_coord(0,0));
}


void convoi_detail_t::draw(scr_coord pos, scr_size size)
{
	if(!cnv.is_bound()) {
		destroy_win(this);
	}
	else {
		bool any_class = false;
		for (unsigned veh = 0; veh < cnv->get_vehicle_count(); ++veh)
		{
			vehicle_t* v = cnv->get_vehicle(veh);
			if (v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS || v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL)
			{
				if (v->get_desc()->get_total_capacity() > 0)
				{
					any_class = true;
				}
			}
		}

		if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {
			withdraw_button.enable();
			sale_button.enable();
			retire_button.enable();
			if (any_class)
			{
				class_management_button.enable();
			}
			else
			{
				class_management_button.disable();
			}
		}
		else {
			withdraw_button.disable();
			sale_button.disable();
			retire_button.disable();
			class_management_button.disable();
		}
		withdraw_button.pressed = cnv->get_withdraw();
		retire_button.pressed = cnv->get_depot_when_empty();
		class_management_button.pressed = win_get_magic(magic_class_manager);

		// all gui stuff set => display it
		gui_frame_t::draw(pos, size);
		cont_payload.set_size(payload_info.get_size());

		int offset_y = pos.y+2+16;

		// current value
		char number[64];
		cbuffer_t buf;

		number_to_string( number, (double)cnv->get_total_distance_traveled(), 0 );
		buf.clear();
		buf.printf( translator::translate("Odometer: %s km"), number );
		display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		offset_y += LINESPACE;

		// current resale value
		money_to_string( number, cnv->calc_sale_value() / 100.0 );
		buf.clear();
		buf.printf("%s %s", translator::translate("Restwert:"), number );
		display_proportional_clip( pos.x + 10, offset_y, buf, ALIGN_LEFT, MONEY_PLUS, true );
		offset_y += LINESPACE;

		buf.clear();
		buf.printf("%s %i (%s %i)", translator::translate("Fahrzeuge:"), cnv->get_vehicle_count(), translator::translate("Station tiles:"), cnv->get_tile_length() );
		display_proportional_clip( pos.x + 10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
		offset_y += LINESPACE;

		vehicle_t* v1 = cnv->get_vehicle(0);

		if(v1->get_waytype() == track_wt || v1->get_waytype() == maglev_wt || v1->get_waytype() == tram_wt || v1->get_waytype() == narrowgauge_wt || v1->get_waytype() == monorail_wt)
		{
			// Current working method
			rail_vehicle_t* rv1 = (rail_vehicle_t*)v1;
			rail_vehicle_t* rv2 = (rail_vehicle_t*)cnv->get_vehicle(cnv->get_vehicle_count() - 1);
			buf.clear();
			buf.printf("%s: %s", translator::translate("Current working method"), translator::translate(rv1->is_leading() ? roadsign_t::get_working_method_name(rv1->get_working_method()) : roadsign_t::get_working_method_name(rv2->get_working_method())));
			display_proportional_clip( pos.x+10, offset_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true );
			offset_y += LINESPACE;
		}
	}
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool convoi_detail_t::action_triggered(gui_action_creator_t *comp, value_t)           // 28-Dec-01    Markus Weber    Added
{
	if(cnv.is_bound()) {
		if(comp==&sale_button) {
			cnv->call_convoi_tool( 'x', NULL );
			return true;
		}
		else if(comp==&withdraw_button) {
			cnv->call_convoi_tool( 'w', NULL );
			return true;
		}
		else if(comp==&retire_button) {
			cnv->call_convoi_tool( 'T', NULL );
			return true;
		}
		else if (comp == &class_management_button) {
			create_win(20, 40, new vehicle_class_manager_t(cnv), w_info, magic_class_manager + cnv.get_id());
			return true;
		}
		else if (comp == &display_detail_button) {
			display_detail_button.pressed = !display_detail_button.pressed;
			payload_info.set_show_detail(display_detail_button.pressed);
			return true;
		}
	}
	return false;
}



/**
 * Set window size and adjust component sizes and/or positions accordingly
 * @author Markus Weber
 */
void convoi_detail_t::set_windowsize(scr_size size)
{
	gui_frame_t::set_windowsize(size);
	tabs.set_size(get_client_windowsize() - tabs.get_pos());
	scrolly_formation.set_size(scr_size(size.w-1, LINESPACE + VEHICLE_BAR_HEIGHT + 12 + 10 + D_SCROLLBAR_HEIGHT)); // (margin + indicator bar height) + goods symbol height
}


// dummy for loading
convoi_detail_t::convoi_detail_t() :
	gui_frame_t("", NULL ),
	cnv(convoihandle_t()),
	scrolly(&veh_info),
	scrolly_formation(&formation),
	scrolly_payload_info(&cont_payload),
	scrolly_maintenance(&maintenance),
	veh_info(convoihandle_t()),
	formation(cnv),
	payload_info(cnv),
	maintenance(cnv)
{
}


void convoi_detail_t::rdwr(loadsave_t *file)
{
	// convoy data
	if (file->get_version() <=112002) {
		// dummy data
		koord3d cnv_pos( koord3d::invalid);
		char name[128];
		name[0] = 0;
		cnv_pos.rdwr( file );
		file->rdwr_str( name, lengthof(name) );
	}
	else {
		// handle
		convoi_t::rdwr_convoihandle_t(file, cnv);
	}
	// window size, scroll position
	scr_size size = get_windowsize();
	sint32 xoff = scrolly.get_scroll_x();
	sint32 yoff = scrolly.get_scroll_y();
	sint32 formation_xoff = scrolly_formation.get_scroll_x();
	sint32 formation_yoff = scrolly_formation.get_scroll_y();

	size.rdwr( file );
	file->rdwr_long( xoff );
	file->rdwr_long( yoff );

	if(  file->is_loading()  ) {
		// convoy vanished
		if(  !cnv.is_bound()  ) {
			dbg->error( "convoi_detail_t::rdwr()", "Could not restore convoi detail window of (%d)", cnv.get_id() );
			destroy_win( this );
			return;
		}

		// now we can open the window ...
		scr_coord const& pos = win_get_pos(this);
		convoi_detail_t *w = new convoi_detail_t(cnv);
		create_win(pos.x, pos.y, w, w_info, magic_convoi_detail + cnv.get_id());
		w->set_windowsize( size );
		w->scrolly.set_scroll_position( xoff, yoff );
		w->scrolly_formation.set_scroll_position(formation_xoff, formation_yoff);
		// we must invalidate halthandle
		cnv = convoihandle_t();
		destroy_win( this );
	}
}


// component for vehicle display
gui_vehicleinfo_t::gui_vehicleinfo_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}


/*
 * Draw the component
 * @author Hj. Malthaner
 */
void gui_vehicleinfo_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w - 51 - pos.x;
	karte_t *welt = world();
	offset.y += LINESPACE/2;

	int total_height = 0;
	if (cnv.is_bound()) {
		cbuffer_t buf;
		const uint16 month_now = welt->get_timeline_year_month();
		uint8 vehicle_count = cnv->get_vehicle_count();

		// for air vehicle
		if (uint16 minimum_runway_length = cnv->get_vehicle(0)->get_desc()->get_minimum_runway_length()){
			buf.clear();
			buf.printf("%s: %i m \n", translator::translate("Minimum runway length"), minimum_runway_length);
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE*2;
		}

		// display total values
		if (vehicle_count > 1) {
			// vehicle min max. speed (not consider weight)
			buf.clear();
			buf.printf("%s %3d km/h\n", translator::translate("Max. speed:"), speed_to_kmh(cnv->get_min_top_speed()));
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;

			// convoy power
			buf.clear();
			// NOTE: These value needs to be modified because these are multiplied by "gear"
			buf.printf(translator::translate("%s %4d kW, %d kN"), translator::translate("Power:"), cnv->get_sum_power() / 1000, cnv->get_starting_force().to_sint32() / 1000);
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;

			// current brake force
			buf.clear();
			buf.printf("%s %.2f kN", translator::translate("Max. brake force:"), cnv->get_braking_force().to_double() / 1000.0);
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;

			// starting acceleration
			lazy_convoy_t &convoy = *cnv.get_rep();
			const sint32 friction = convoy.get_current_friction();
			const uint32 starting_acceleration = convoy.calc_acceleration(weight_summary_t(cnv->get_weight_summary().weight, friction), 0);
			const uint32 starting_acceleration_min = convoy.calc_acceleration(weight_summary_t(cnv->get_sum_weight(), friction), 0);
			buf.clear();
			buf.printf("%s %.2f km/h/s (%.2f km/h/s)", translator::translate("Starting acceleration:"), (double)starting_acceleration / 100.0, (double)starting_acceleration_min / 100.0);
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;

			// convoy weight
			buf.clear();
			buf.printf("%s %.1ft (%.1ft)", translator::translate("Weight:"), cnv->get_weight_summary().weight / 1000.0, cnv->get_sum_weight() / 1000.0);
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;

			// max axles load
			buf.clear();
			buf.printf("%s %dt", translator::translate("Max. axle load:"), cnv->get_highest_axle_load());
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;

			// convoy applied livery scheme
			if (cnv->get_livery_scheme_index()) {
				buf.clear();
				buf.printf(translator::translate("Applied livery scheme: %s"), translator::translate(welt->get_settings().get_livery_scheme(cnv->get_livery_scheme_index())->get_name()));
				display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, buf, ALIGN_LEFT, welt->get_settings().get_livery_scheme(cnv->get_livery_scheme_index())->is_available(month_now) ? SYSCOL_TEXT : COL_OBSOLETE, true);
				total_height += LINESPACE;
			}
			total_height += LINESPACE;
		}

		for (unsigned veh = 0; veh < vehicle_count; veh++) {
			vehicle_t *v = cnv->get_vehicle(veh);
			vehicle_as_potential_convoy_t convoy(*v->get_desc());
			const uint8 upgradable_state = v->get_desc()->has_available_upgrade(month_now, welt->get_settings().get_show_future_vehicle_info());

			// first image
			scr_coord_val x, y, w, h;
			const image_id image = v->get_loaded_image();
			display_get_base_image_offset(image, x, y, w, h);
			display_base_img(image, 11 - x + pos.x + offset.x, pos.y + offset.y + total_height - y + 2 + LINESPACE, cnv->get_owner()->get_player_nr(), false, true);
			w = max(40, w + 4) + 11;

			// now add the other info
			int extra_y = 0;

			// cars number in this convoy
			sint8 car_number = cnv->get_car_numbering(veh);
			buf.clear();
			if (car_number < 0) {
				buf.printf("%.2s%d", translator::translate("LOCO_SYM"), abs(car_number)); // This also applies to horses and tractors and push locomotives.
			}
			else {
				buf.append(car_number);
			}
			display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, upgradable_state == 2 ? COL_UPGRADEABLE : COL_GREY2, true);
			buf.clear();

			// upgradable symbol
			if (upgradable_state && skinverwaltung_t::upgradable) {
				if (welt->get_settings().get_show_future_vehicle_info() || (!welt->get_settings().get_show_future_vehicle_info() && v->get_desc()->is_future(month_now) != 2)) {
					display_color_img(skinverwaltung_t::upgradable->get_image_id(upgradable_state - 1), pos.x + w + offset.x - LINESPACE, pos.y + offset.y + total_height + extra_y + h + LINESPACE, 0, false, false);
				}
			}

			// name of this
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE + D_V_SPACE;
			w += D_H_SPACE;

			// power, tractive force, gear
			if (v->get_desc()->get_power() > 0) {
				buf.clear();
				buf.printf(translator::translate("Power/tractive force (%s): %4d kW / %d kN\n"),
					translator::translate(vehicle_desc_t::get_engine_type((vehicle_desc_t::engine_t)v->get_desc()->get_engine_type())),
					v->get_desc()->get_power(), v->get_desc()->get_tractive_effort());
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;
				buf.clear();
				buf.printf("%s %0.2f : 1", translator::translate("Gear:"), v->get_desc()->get_gear() / 64.0);
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;
			}

			// max speed
			buf.clear();
			buf.printf("%s %3d km/h\n", translator::translate("Max. speed:"), v->get_desc()->get_topspeed());
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			// weight
			buf.clear();
			buf.printf("%s %.1ft (%.1ft)", translator::translate("Weight:"), v->get_sum_weight()/1000.0, v->get_desc()->get_weight() / 1000.0);
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			// Axle load
			buf.clear();
			buf.printf("%s %dt", translator::translate("Axle load:"), v->get_desc()->get_axle_load());
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			// Brake force
			buf.clear();
			buf.printf("%s %4.1f kN", translator::translate("Max. brake force:"), convoy.get_braking_force().to_double() / 1000.0);
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			// Range
			buf.clear();
			buf.printf("%s: ", translator::translate("Range"));
			if (v->get_desc()->get_range() == 0)
			{
				buf.append(translator::translate("unlimited"));
			}
			else
			{
				buf.printf("%i km", v->get_desc()->get_range());
			}
			display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			//Catering - A vehicle can be a catering vehicle without carrying passengers.
			if (v->get_desc()->get_catering_level() > 0)
			{
				buf.clear();
				if (v->get_desc()->get_freight_type()->get_catg_index() == 1)
				{
					//Catering vehicles that carry mail are treated as TPOs.
					buf.printf("%s", translator::translate("This is a travelling post office"));
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
				}
				else
				{
					buf.printf(translator::translate("Catering level: %i"), v->get_desc()->get_catering_level());
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
				}
			}


			const way_constraints_t &way_constraints = v->get_desc()->get_way_constraints();
			// Permissive way constraints
			// (If vehicle has, way must have)
			// @author: jamespetts
			//for(uint8 i = 0; i < 8; i++)
			for (uint8 i = 0; i < way_constraints.get_count(); i++)
			{
				//if(v->get_desc()->permissive_way_constraint_set(i))
				if (way_constraints.get_permissive(i))
				{
					buf.clear();

					char tmpbuf[17];
					sprintf(tmpbuf, "Permissive %i-%hu", v->get_desc()->get_waytype(), (uint16)i);
					buf.printf("%s %s", translator::translate("\nMUST USE: "), translator::translate(tmpbuf));
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
				}
			}
			if (v->get_desc()->get_is_tall())
			{
				buf.clear();
				char tmpbuf1[13];
				sprintf(tmpbuf1, "\nMUST USE: ");
				char tmpbuf[46];
				sprintf(tmpbuf, "high_clearance_under_bridges_(no_low_bridges)");
				buf.printf("%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;
			}

			// Prohibitive way constraints
			// (If way has, vehicle must have)
			// @author: jamespetts
			for (uint8 i = 0; i < way_constraints.get_count(); i++)
			{
				if (way_constraints.get_prohibitive(i))
				{
					buf.clear();
					char tmpbuf[18];
					sprintf(tmpbuf, "Prohibitive %i-%hu", v->get_desc()->get_waytype(), (uint16)i);
					buf.printf("%s %s", translator::translate("\nMAY USE: "), translator::translate(tmpbuf));
					display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
					extra_y += LINESPACE;
				}
			}
			if (v->get_desc()->get_tilting())
			{
				buf.clear();
				char tmpbuf1[14];
				sprintf(tmpbuf1, "equipped_with");
				char tmpbuf[26];
				sprintf(tmpbuf, "tilting_vehicle_equipment");
				buf.printf("%s %s", translator::translate(tmpbuf1), translator::translate(tmpbuf));
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;
			}

			// Friction
			if (v->get_frictionfactor() != 1)
			{
				buf.clear();
				buf.printf("%s %i", translator::translate("Friction:"), v->get_frictionfactor());
				display_proportional_clip(pos.x + w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true);
			}
			extra_y += LINESPACE;

			//skip at least five lines
			total_height += max(extra_y + LINESPACE*2, 5 * LINESPACE);
		}
	}

	scr_size size(max(x_size + pos.x, get_size().w), total_height);
	if (size != get_size()) {
		set_size(size);
	}
}


// component for payload display
gui_convoy_payload_info_t::gui_convoy_payload_info_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}

void gui_convoy_payload_info_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w - 51 - pos.x;
	karte_t *welt = world();
	offset.y += LINESPACE / 2;
	offset.x += D_H_SPACE;



	int total_height = 0;
	if (cnv.is_bound()) {
		//char number[64];
		cbuffer_t buf;
		int extra_w = D_H_SPACE;
		const uint16 month_now = welt->get_timeline_year_month();
		const uint8 catering_level = cnv->get_catering_level(goods_manager_t::INDEX_PAS);

		char min_loading_time_as_clock[32];
		char max_loading_time_as_clock[32];

		if (cnv->get_no_load()) {
			if (skinverwaltung_t::alerts) {
				display_color_img(skinverwaltung_t::alerts->get_image_id(2), pos.x + offset.x + extra_w, pos.y + offset.y + total_height, 0, false, false);
			}
			buf.clear();
			buf.append(translator::translate("No load setting"));
			display_proportional_clip(pos.x + offset.x + extra_w + 14, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, false);
			total_height += LINESPACE*1.5;
		}

		// display total values
		if (cnv->get_vehicle_count() > 1) {
			// convoy min - max loading time
			welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), cnv->calc_longest_min_loading_time());
			welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), cnv->calc_longest_max_loading_time());

			buf.clear();
			buf.printf("%s %s - %s", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
			display_proportional_clip(pos.x + offset.x + extra_w, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, false);
			total_height += LINESPACE;

			// convoy (max) catering level
			if (catering_level) {
				buf.clear();
				buf.printf(translator::translate("Catering level: %i"), catering_level);
				display_proportional_clip(pos.x + offset.x + extra_w, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, false);
				total_height += LINESPACE;
			}
			if (cnv->get_catering_level(goods_manager_t::INDEX_MAIL)) {
				buf.clear();
				buf.printf("%s", translator::translate("traveling_post_office"));
				display_proportional_clip(pos.x + offset.x + extra_w, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, false);
				total_height += LINESPACE;
			}
			total_height += LINESPACE;
		}

		static cbuffer_t freight_info;
		for (unsigned veh = 0; veh < cnv->get_vehicle_count(); veh++) {
			vehicle_t *v = cnv->get_vehicle(veh);
			freight_info.clear();

			// now add the other info
			int extra_y = 0;
			const uint16 grid_width = D_BUTTON_WIDTH / 3;
			extra_w = grid_width;

			int boarding_rate = 0;
			int returns = 0;

			// cars number in this convoy
			COLOR_VAL veh_bar_color;
			sint8 car_number = cnv->get_car_numbering(veh);
			buf.clear();

			if (car_number < 0) {
				buf.printf("%.2s%d", translator::translate("LOCO_SYM"), abs(car_number)); // This also applies to horses and tractors and push locomotives.
			}
			else {
				buf.append(car_number);
			}
			display_proportional_clip(pos.x + offset.x + 1, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, v->get_desc()->has_available_upgrade(month_now) ? COL_UPGRADEABLE : COL_GREY2, true);
			buf.clear();

			// vehicle color bar
			veh_bar_color = v->get_desc()->is_future(month_now) || v->get_desc()->is_retired(month_now) ? COL_OUT_OF_PRODUCTION : COL_DARK_GREEN;
			if (v->get_desc()->is_obsolete(month_now, welt)) {
				veh_bar_color = COL_OBSOLETE;
			}
			display_veh_form(pos.x + offset.x, pos.y + offset.y + total_height + extra_y + LINESPACE, VEHICLE_BAR_HEIGHT * 2, veh_bar_color, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_next() : v->get_desc()->get_basic_constraint_prev(), v->get_desc()->get_interactivity(), false);
			display_veh_form(pos.x + offset.x + grid_width / 2 - 1, pos.y + offset.y + total_height + extra_y + LINESPACE, VEHICLE_BAR_HEIGHT * 2, veh_bar_color, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_prev() : v->get_desc()->get_basic_constraint_next(), v->get_desc()->get_interactivity(), true);

			// goods category symbol
			if (v->get_desc()->get_total_capacity() || v->get_desc()->get_overcrowded_capacity()) {
				display_color_img(v->get_cargo_type()->get_catg_symbol(), pos.x + offset.x + grid_width / 2 - 5, pos.y + offset.y + total_height + extra_y + LINESPACE * 2, 0, false, false);
			}
			// name of this
			//display_multiline_text(pos.x + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), SYSCOL_TEXT, true);
			display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE + D_V_SPACE;

			if (v->get_desc()->get_total_capacity() > 0)
			{
				boarding_rate = v->get_total_cargo() * 100 / v->get_cargo_max();

				//Loading time
				welt->sprintf_ticks(min_loading_time_as_clock, sizeof(min_loading_time_as_clock), v->get_desc()->get_min_loading_time());
				welt->sprintf_ticks(max_loading_time_as_clock, sizeof(max_loading_time_as_clock), v->get_desc()->get_max_loading_time());
				buf.clear();
				buf.printf("%s %s - %s", translator::translate("Loading time:"), min_loading_time_as_clock, max_loading_time_as_clock);
				display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
				extra_y += LINESPACE;

				// Class entries, if passenger or mail vehicle
				bool pass_veh = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_PAS;
				bool mail_veh = v->get_cargo_type()->get_catg_index() == goods_manager_t::INDEX_MAIL;

				uint8 pass_classes = goods_manager_t::passengers->get_number_of_classes();
				uint8 mail_classes = goods_manager_t::mail->get_number_of_classes();

				goods_desc_t const& g = *v->get_cargo_type();

				if (pass_veh || mail_veh) {

					int classes_to_check = pass_veh ? pass_classes : mail_classes;
					int left;
					for (uint8 i = 0; i < classes_to_check; i++)
					{
						buf.clear();
						if (v->get_accommodation_capacity(i) > 0 || v->get_overcrowded_capacity(i))
						{
							left = pos.x + extra_w + offset.x;

							// [class name]
							if (v->get_reassigned_class(i) != i)
							{
								left += display_proportional_clip(left, pos.y + offset.y + total_height + extra_y, "(*)", ALIGN_LEFT, SYSCOL_EDIT_TEXT_DISABLED, true);
								buf.clear();
							}

							buf.append(goods_manager_t::get_translated_wealth_name(v->get_cargo_type()->get_catg_index(), v->get_reassigned_class(i)));
							buf.append(":");

							COLOR_VAL text_color = boarding_rate > 100 ? COL_DARK_PURPLE : SYSCOL_TEXT;
							// [comfort]
							if (pass_veh)
							{
								buf.printf(" %s ", translator::translate("Comfort:"));
								left += display_proportional_clip(left, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
								buf.clear();

								buf.printf("%3i", v->get_comfort(catering_level, v->get_reassigned_class(i)));
								left += display_proportional_clip(left, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, text_color, true);
								buf.clear();

								buf.append(", ");
							}
							left += display_proportional_clip(left, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);

							// [capacity]
							buf.clear();

							buf.printf("%4d/%3d", v->get_total_cargo_by_class(i), v->get_fare_capacity(v->get_reassigned_class(i)));
							if (v->get_overcrowded_capacity(i)) {
								buf.printf("(%d)", v->get_overcrowded_capacity(i));
							}
							left += display_proportional_clip(left, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);


							// [color bar(pas/mail)]
							left += D_H_SPACE;
							// only for pas and mail
							display_loading_bar(left, pos.y + offset.y + total_height + extra_y, LOADING_BAR_WIDTH, LOADING_BAR_HEIGHT, g.get_color(), v->get_total_cargo_by_class(i), v->get_fare_capacity(v->get_reassigned_class(i)), v->get_overcrowded_capacity(i));

							extra_y += LINESPACE + 2;
						}
					}

					buf.clear();
				}
				else {
					int left = 0;
					char const* const name = translator::translate(g.get_catg_name());
					left += display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, name, ALIGN_LEFT, SYSCOL_TEXT, true);
					left += D_H_SPACE;
					// [color bar(goods)]
					// draw the "empty" loading bar
					display_loading_bar(pos.x + extra_w + offset.x + left, pos.y + offset.y + total_height + extra_y, LOADING_BAR_WIDTH, LOADING_BAR_HEIGHT, COL_GREY4, 0, 1, 0);

					int bar_start_offset = 0;
					int cargo_sum= 0 ;
					extra_y += (LINESPACE - LOADING_BAR_HEIGHT) / 2;
					FOR(slist_tpl<ware_t>, const ware, v->get_cargo(0))
					{
						goods_desc_t const* const wtyp = ware.get_desc();
						cargo_sum += ware.menge;

						// draw the goods loading bar
						int bar_end_offset = cargo_sum * LOADING_BAR_WIDTH / v->get_desc()->get_total_capacity();
						COLOR_VAL goods_color = wtyp->get_color();
						if (bar_end_offset - bar_start_offset) {
							display_cylinderbar_wh_clip(pos.x + extra_w + offset.x + left + bar_start_offset, pos.y + offset.y + total_height + extra_y, bar_end_offset - bar_start_offset, LOADING_BAR_HEIGHT, goods_color, true);
						}
						bar_start_offset += bar_end_offset - bar_start_offset;
					}
					if (v->get_desc()->get_mixed_load_prohibition()) {
						extra_y += LINESPACE;
						buf.clear();
						buf.printf("%s", translator::translate("(mixed_load_prohibition)"));
						display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y - (LINESPACE - LOADING_BAR_HEIGHT) / 2, buf, ALIGN_LEFT, COL_BRONZE, true);
					}
					extra_y += LINESPACE + 2;
				}

				if (show_detail) {
					// We get the freight info via the freight_list_sorter now, so no need to do anything but fetch it
					v->get_cargo_info(freight_info);
					// show it
					const int px_len = display_multiline_text(pos.x + offset.x + extra_w, pos.y + offset.y + total_height + extra_y, freight_info, SYSCOL_TEXT);
					if (px_len + extra_w > x_size)
					{
						x_size = px_len + extra_w;
					}
					// count returns
					returns = 0;
					const char *p = freight_info;
					for (int i = 0; i < freight_info.len(); i++)
					{
						if (p[i] == '\n')
						{
							returns++;
						}
					}
				}
				extra_y += (returns*LINESPACE) + LINESPACE;
			}

			// skip at least 3.5 lines. (This is for vehicles without capacity such as locomotives)
			total_height += max(extra_y + LINESPACE, 3.5 * LINESPACE);
		}
	}

	scr_size size(max(x_size + pos.x, get_size().w), total_height);
	if (size != get_size()) {
		set_size(size);
	}
}


void gui_convoy_payload_info_t::display_loading_bar(KOORD_VAL xp, KOORD_VAL yp, KOORD_VAL w, KOORD_VAL h, PIXVAL color, uint16 loading, uint16 capacity, uint16 overcrowd_capacity)
{
	if (capacity > 0 || overcrowd_capacity > 0) {
		// base
		display_fillbox_wh_clip(xp, yp + (LINESPACE - h) / 2, w * capacity / (capacity + overcrowd_capacity), h, COL_GREY4, false);
		// dsiplay loading barSta
		display_fillbox_wh_clip(xp, yp + (LINESPACE - h) / 2, min(w * loading / (capacity + overcrowd_capacity), w * capacity / (capacity + overcrowd_capacity)), h, color, true);
		display_blend_wh(xp, yp + (LINESPACE - h) / 2, w * loading / (capacity + overcrowd_capacity), 3, COL_WHITE, 15);
		display_blend_wh(xp, yp + (LINESPACE - h) / 2 + 1, w * loading / (capacity + overcrowd_capacity), 1, COL_WHITE, 15);
		display_blend_wh(xp, yp + (LINESPACE - h) / 2 + h - 1, w * loading / (capacity + overcrowd_capacity), 1, COL_BLACK, 10);
		if (overcrowd_capacity && (loading > (capacity- overcrowd_capacity))) {
			display_fillbox_wh_clip(xp + w * capacity / (capacity + overcrowd_capacity) + 1, yp + (LINESPACE - h) / 2 - 1, min(w * loading / (capacity + overcrowd_capacity), w * (loading - capacity) / (capacity + overcrowd_capacity)), h+2, COL_OVERCROWD, true);
			display_blend_wh(xp + w * capacity / (capacity + overcrowd_capacity) + 1, yp + (LINESPACE - h) / 2, min(w * loading / (capacity + overcrowd_capacity), w * (loading - capacity) / (capacity + overcrowd_capacity)), 3, COL_WHITE, 15);
			display_blend_wh(xp + w * capacity / (capacity + overcrowd_capacity) + 1, yp + (LINESPACE - h) / 2 + 1, min(w * loading / (capacity + overcrowd_capacity), w * (loading - capacity) / (capacity + overcrowd_capacity)), 1, COL_WHITE, 15);
			display_blend_wh(xp + w * capacity / (capacity + overcrowd_capacity) + 1, yp + (LINESPACE - h) / 2 + h - 1, min(w * loading / (capacity + overcrowd_capacity), w * (loading - capacity) / (capacity + overcrowd_capacity)), 2, COL_BLACK, 10);
		}

		// frame
		display_ddd_box_clip(xp - 1, yp + (LINESPACE - h) / 2 - 1, w * capacity / (capacity + overcrowd_capacity) + 2, h + 2, MN_GREY0, MN_GREY0);
		// overcrowding frame
		if (overcrowd_capacity) {
			display_direct_line_dotted(xp + w, yp + (LINESPACE - h) / 2 - 1, xp + w * capacity / (capacity + overcrowd_capacity) + 1, yp + (LINESPACE - h) / 2 - 1, 1, 1, MN_GREY0);  // top
			display_direct_line_dotted(xp + w, yp + (LINESPACE - h) / 2 - 2, xp + w, yp + (LINESPACE - h) / 2 + h, 1, 1, MN_GREY0);  // right. start from dot
			display_direct_line_dotted(xp + w, yp + (LINESPACE - h) / 2 + h, xp + w * capacity / (capacity + overcrowd_capacity) + 1, yp + (LINESPACE - h) / 2 + h, 1, 1, MN_GREY0); // bottom
		}
	}
}


// component for cargo display
gui_convoy_maintenance_info_t::gui_convoy_maintenance_info_t(convoihandle_t cnv)
{
	this->cnv = cnv;
}


void gui_convoy_maintenance_info_t::draw(scr_coord offset)
{
	// keep previous maximum width
	int x_size = get_size().w - 51 - pos.x;
	karte_t *welt = world();
	offset.y += LINESPACE / 2;
	offset.x += D_H_SPACE;

	int total_height = 0;
	if (cnv.is_bound()) {
		uint8 vehicle_count = cnv->get_vehicle_count();
		const uint16 month_now = welt->get_timeline_year_month();
		cbuffer_t buf;
		int extra_w = D_H_SPACE;

		if (cnv->get_replace()) {
			if (skinverwaltung_t::alerts) {
				display_color_img(skinverwaltung_t::alerts->get_image_id(1), pos.x + offset.x + extra_w, pos.y + offset.y + total_height, 0, false, false);
			}
			buf.clear();
			buf.append(translator::translate("Replacing"));
			display_proportional_clip(pos.x + offset.x + extra_w + 14, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, false);
			total_height += LINESPACE * 1.5;
		}

		// display total values
		if (vehicle_count > 1) {
			// convoy maintenance
			buf.clear();
			buf.printf(translator::translate("Maintenance: %1.2f$/km, %1.2f$/month\n"), cnv->get_per_kilometre_running_cost() / 100.0, welt->calc_adjusted_monthly_figure(cnv->get_fixed_cost()) / 100.0);
			//txt_convoi_cost.printf(translator::translate("Cost: %8s (%.2f$/km, %.2f$/month)\n"), buf, (double)maint_per_km / 100.0, (double)maint_per_month / 100.0);
			display_proportional_clip(pos.x + offset.x + extra_w, pos.y + offset.y + total_height, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			total_height += LINESPACE;
		}

		// Bernd Gabriel, 16.06.2009: current average obsolescence increase percentage
		if (vehicle_count > 0)
		{
			any_obsoletes = false;
			/* Bernd Gabriel, 17.06.2009:
			The average percentage tells nothing about the real cost increase: If a cost-intensive
			loco is very old and at max increase (1 * 400% * 1000 cr/month, but 15 low-cost cars are
			brand new (15 * 100% * 100 cr/month), an average percentage of
			(1 * 400% + 15 * 100%) / 16 = 118.75% does not tell the truth. Actually we pay
			(1 * 400% * 1000 + 15 * 100% * 100) / (1 * 1000 + 15 * 100) = 220% twice as much as in the
			early years of the loco.

				uint32 percentage = 0;
				for (uint16 i = 0; i < vehicle_count; i++) {
					percentage += cnv->get_vehicle(i)->get_desc()->calc_running_cost(welt, 10000);
				}
				percentage = percentage / (vehicle_count * 100) - 100;
				if (percentage > 0)
				{
					sprintf( tmp, "%s: %d%%", translator::translate("Obsolescence increase"), percentage);
					display_proportional_clip( pos.x+10, offset_y, tmp, ALIGN_LEFT, COL_OBSOLETE, true );
					offset_y += LINESPACE;
				}
			On the other hand: a single effective percentage does not tell the truth as well. Supposed we
			calculate the percentage from the costs per km, the relations for the month costs can widely differ.
			Therefore I show different values for running and monthly costs:
			*/
			uint32 run_actual = 0, run_nominal = 0, run_percent = 0;
			uint32 mon_actual = 0, mon_nominal = 0, mon_percent = 0;
			for (uint8 i = 0; i < vehicle_count; i++) {
				const vehicle_desc_t *desc = cnv->get_vehicle(i)->get_desc();
				run_nominal += desc->get_running_cost();
				run_actual += desc->get_running_cost(welt);
				mon_nominal += welt->calc_adjusted_monthly_figure(desc->get_fixed_cost());
				mon_actual += welt->calc_adjusted_monthly_figure(desc->get_fixed_cost(welt));
			}
			buf.clear();
			if (run_nominal) run_percent = ((run_actual - run_nominal) * 100) / run_nominal;
			if (mon_nominal) mon_percent = ((mon_actual - mon_nominal) * 100) / mon_nominal;
			if (run_percent)
			{
				if (mon_percent)
				{
					buf.printf("%s: %d%%/km %d%%/mon", translator::translate("Obsolescence increase"), run_percent, mon_percent);
				}
				else
				{
					buf.printf("%s: %d%%/km", translator::translate("Obsolescence increase"), run_percent);
				}
			}
			else
			{
				if (mon_percent)
				{
					buf.printf("%s: %d%%/mon", translator::translate("Obsolescence increase"), mon_percent);
				}
			}
			if (buf.len() > 0)
			{
				if (skinverwaltung_t::alerts) {
					display_color_img(skinverwaltung_t::alerts->get_image_id(2), pos.x + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height, 0, false, false);
				}
				display_proportional_clip(pos.x + offset.x + D_MARGIN_LEFT + 13, pos.y + offset.y + total_height, buf, ALIGN_LEFT, COL_OBSOLETE, true);
				total_height += LINESPACE * 1.5;
				any_obsoletes = true;
			}
		}

		// convoy applied livery scheme
		if (cnv->get_livery_scheme_index()) {
			buf.clear();
			buf.printf(translator::translate("Applied livery scheme: %s"), translator::translate(welt->get_settings().get_livery_scheme(cnv->get_livery_scheme_index())->get_name()));
			display_proportional_clip(pos.x + offset.x + extra_w, pos.y + offset.y + total_height, buf, ALIGN_LEFT, welt->get_settings().get_livery_scheme(cnv->get_livery_scheme_index())->is_available(month_now) ? SYSCOL_TEXT : COL_OBSOLETE, true);
			total_height += LINESPACE;
		}

		total_height += LINESPACE;

		char number[64];

		for (unsigned veh = 0; veh < vehicle_count; veh++) {
			vehicle_t *v = cnv->get_vehicle(veh);
			const uint8 upgradable_state = v->get_desc()->has_available_upgrade(month_now, welt->get_settings().get_show_future_vehicle_info());

			int extra_y = 0;
			const uint8 grid_width = D_BUTTON_WIDTH / 3;
			extra_w = grid_width;

			// cars number in this convoy
			COLOR_VAL veh_bar_color;
			sint8 car_number = cnv->get_car_numbering(veh);
			buf.clear();
			if (car_number < 0) {
				buf.printf("%.2s%d", translator::translate("LOCO_SYM"), abs(car_number)); // This also applies to horses and tractors and push locomotives.
			}
			else {
				buf.append(car_number);
			}
			display_proportional_clip(pos.x + offset.x + 1, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, upgradable_state == 2 ? COL_UPGRADEABLE : COL_GREY2, true);
			buf.clear();

			// vehicle color bar
			veh_bar_color = v->get_desc()->is_future(month_now) || v->get_desc()->is_retired(month_now) ? COL_OUT_OF_PRODUCTION : COL_DARK_GREEN;
			if (v->get_desc()->is_obsolete(month_now, welt)) {
				veh_bar_color = COL_OBSOLETE;
			}
			display_veh_form(pos.x + offset.x, pos.y + offset.y + total_height + extra_y + LINESPACE, VEHICLE_BAR_HEIGHT * 2, veh_bar_color, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_next() : v->get_desc()->get_basic_constraint_prev(), v->get_desc()->get_interactivity(), false);
			display_veh_form(pos.x + offset.x + grid_width / 2 - 1, pos.y + offset.y + total_height + extra_y + LINESPACE, VEHICLE_BAR_HEIGHT * 2, veh_bar_color, true, v->is_reversed() ? v->get_desc()->get_basic_constraint_prev() : v->get_desc()->get_basic_constraint_next(), v->get_desc()->get_interactivity(), true);

			// goods category symbol
			if (v->get_desc()->get_total_capacity() || v->get_desc()->get_overcrowded_capacity()) {
				display_color_img(v->get_cargo_type()->get_catg_symbol(), pos.x + offset.x + grid_width / 2 - 5, pos.y + offset.y + total_height + extra_y + LINESPACE * 2, 0, false, false);
			}

			// name of this
			//display_multiline_text(pos.x + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), SYSCOL_TEXT, true);
			display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate(v->get_desc()->get_name()), ALIGN_LEFT, SYSCOL_TEXT, true);
			// livery scheme info
			if ( strcmp( v->get_current_livery(), "default") ) {
				buf.clear();
				vector_tpl<livery_scheme_t*>* schemes = welt->get_settings().get_livery_schemes();
				livery_scheme_t* convoy_scheme = schemes->get_element(cnv->get_livery_scheme_index());
				COLOR_VAL livery_state_col = SYSCOL_TEXT;
				if (convoy_scheme->is_contained(v->get_current_livery(), month_now)) {
					// current livery belongs to convoy applied livery scheme and active
					buf.printf("(%s)", translator::translate(convoy_scheme->get_name()));
					// is current livery latest one?
					if(strcmp(convoy_scheme->get_latest_available_livery(month_now, v->get_desc()), v->get_current_livery()))
					{
						livery_state_col = COL_UPGRADEABLE;
					}
				}
				else if (convoy_scheme->is_contained(v->get_current_livery())) {
					buf.printf("(%s)", translator::translate(convoy_scheme->get_name()));
					livery_state_col = COL_OBSOLETE;
				}
				else {
					// current livery does not belong to convoy applied livery scheme
					// note: livery may belong to more than one livery scheme
					bool found_active_scheme = false;
					livery_state_col = COL_BROWN;
					cbuffer_t temp_buf;
					int cnt = 0;
					ITERATE_PTR(schemes, i)
					{
						livery_scheme_t* scheme = schemes->get_element(i);
						if (scheme->is_contained(v->get_current_livery())) {
							if (scheme->is_available(month_now)) {
								found_active_scheme = true;
								if (cnt) { buf.append(", "); }
								buf.append(translator::translate(scheme->get_name()));
								cnt++;
							}
							else if(!found_active_scheme){
								if (cnt) { buf.append(", "); }
								temp_buf.append(translator::translate(scheme->get_name()));
								cnt++;
							}
						}
					}
					if (!found_active_scheme) {
						buf = temp_buf;
						livery_state_col = COL_DARK_BROWN;
					}
				}
				extra_y += LINESPACE;
				display_proportional_clip(pos.x + extra_w + offset.x + D_H_SPACE, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, livery_state_col, true);
			}
			extra_y += LINESPACE + D_V_SPACE;
			extra_w += D_H_SPACE;

			// age
			buf.clear();
			{
				const sint32 month = v->get_purchase_time();
				uint32 age_in_months = welt->get_current_month() - month;
				buf.printf("%s %s  (", translator::translate("Manufactured:"), translator::get_year_month(month));
				buf.printf(age_in_months < 2 ? translator::translate("%i month") : translator::translate("%i months"), age_in_months);
				buf.append(")");
			}
			display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			// Bernd Gabriel, 16.06.2009: current average obsolescence increase percentage
			uint32 percentage = v->get_desc()->calc_running_cost(welt, 100) - 100;
			if (percentage > 0)
			{
				buf.clear();
				buf.printf("%s: %d%%", translator::translate("Obsolescence increase"), percentage);
				display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, COL_OBSOLETE, true);
				extra_y += LINESPACE;
			}

			// value
			money_to_string(number, v->calc_sale_value() / 100.0);
			buf.clear();
			buf.printf("%s %s", translator::translate("Restwert:"), number);
			display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true);
			extra_y += LINESPACE;

			// maintenance
			buf.clear();
			buf.printf(translator::translate("Maintenance: %1.2f$/km, %1.2f$/month\n"), v->get_desc()->get_running_cost() / 100.0, v->get_desc()->get_adjusted_monthly_fixed_cost(welt) / 100.0);
			display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, MONEY_PLUS, true);
			extra_y += LINESPACE;

			// Revenue
			int len = 5 + display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate("Base profit per km (when full):"), ALIGN_LEFT, SYSCOL_TEXT, true);
			// Revenue for moving 1 unit 1000 meters -- comes in 1/4096 of simcent, convert to simcents
			// Excludes TPO/catering revenue, class and comfort effects.  FIXME --neroden
			sint64 fare = v->get_cargo_type()->get_total_fare(1000); // Class needs to be added here (Ves?)
																	 // Multiply by capacity, convert to simcents, subtract running costs
			sint64 profit = (v->get_cargo_max()*fare + 2048ll) / 4096ll/* - v->get_running_cost(welt)*/;
			money_to_string(number, profit / 100.0);
			display_proportional_clip(pos.x + extra_w + offset.x + len, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, SYSCOL_TEXT, true);
			extra_y += LINESPACE;

			if (sint64 cost = welt->calc_adjusted_monthly_figure(v->get_desc()->get_maintenance())) {
				KOORD_VAL len = display_proportional_clip(pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, translator::translate("Maintenance"), ALIGN_LEFT, SYSCOL_TEXT, true);
				len += display_proportional_clip(pos.x + extra_w + offset.x + len, pos.y + offset.y + total_height + extra_y, ": ", ALIGN_LEFT, SYSCOL_TEXT, true);
				money_to_string(number, cost / (100.0));
				display_proportional_clip(pos.x + extra_w + offset.x + len, pos.y + offset.y + total_height + extra_y, number, ALIGN_LEFT, MONEY_MINUS, true);
				extra_y += LINESPACE;
			}

			// upgrade info
			if (upgradable_state)
			{
				int found = 0;
				COLOR_VAL upgrade_state_color = COL_UPGRADEABLE;
				for (int i = 0; i < v->get_desc()->get_upgrades_count(); i++)
				{
					if (const vehicle_desc_t* desc = v->get_desc()->get_upgrades(i)) {
						if (!welt->get_settings().get_show_future_vehicle_info() && desc->is_future(month_now)==1) {
							continue; // skip future information
						}
						found++;
						if (found == 1) {
							if (skinverwaltung_t::upgradable) {
								display_color_img(skinverwaltung_t::upgradable->get_image_id(upgradable_state-1), pos.x + extra_w + offset.x, pos.y + offset.y + total_height + extra_y, 0, false, false);
							}
							buf.clear();
							buf.printf("%s:", translator::translate("this_vehicle_can_upgrade_to"));
							display_proportional_clip(pos.x + extra_w + offset.x + 14, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
							extra_y += LINESPACE;
						}

						const uint16 intro_date = desc->is_future(month_now) ? desc->get_intro_year_month() : 0;

						if (intro_date) {
							upgrade_state_color = MN_GREY0;
						}
						else if (desc->is_retired(month_now)) {
							upgrade_state_color = COL_OUT_OF_PRODUCTION;
						}
						else if (desc->is_obsolete(month_now, welt)) {
							upgrade_state_color = COL_OBSOLETE;
						}
						display_veh_form(pos.x + extra_w + offset.x + D_MARGIN_LEFT, pos.y + offset.y + total_height + extra_y + 1, VEHICLE_BAR_HEIGHT * 2, upgrade_state_color, true, desc->get_basic_constraint_prev(), desc->get_interactivity(), false);
						display_veh_form(pos.x + extra_w + offset.x + D_MARGIN_LEFT + VEHICLE_BAR_HEIGHT * 2 - 1, pos.y + offset.y + total_height + extra_y + 1, VEHICLE_BAR_HEIGHT * 2, upgrade_state_color, true, desc->get_basic_constraint_next(), desc->get_interactivity(), true);

						buf.clear();
						buf.append(translator::translate(v->get_desc()->get_upgrades(i)->get_name()));
						if (intro_date) {
							buf.printf(", %s %s", translator::translate("Intro. date:"), translator::get_year_month(intro_date));
						}
						display_proportional_clip(pos.x + extra_w + offset.x + D_MARGIN_LEFT + VEHICLE_BAR_HEIGHT*4, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, upgrade_state_color, true);
						extra_y += LINESPACE;
						// 2nd row
						buf.clear();
						money_to_string(number, desc->get_upgrade_price() / 100);
						buf.printf("%s %s,  ", translator::translate("Upgrade price:"), number);
						buf.printf(translator::translate("Maintenance: %1.2f$/km, %1.2f$/month\n"), desc->get_running_cost() / 100.0, desc->get_adjusted_monthly_fixed_cost(welt) / 100.0);
						display_proportional_clip(pos.x + extra_w + offset.x + D_MARGIN_LEFT + grid_width, pos.y + offset.y + total_height + extra_y, buf, ALIGN_LEFT, SYSCOL_TEXT, true);
						extra_y += LINESPACE + 2;
					}
				}
			}

			extra_y += LINESPACE;

			//skip at least five lines
			total_height += max(extra_y + LINESPACE, 5 * LINESPACE);
		}
	}

	scr_size size(max(x_size + pos.x, get_size().w), total_height);
	if (size != get_size()) {
		set_size(size);
	}
}
