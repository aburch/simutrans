/*
 * Copyright (c) 1997 - 2001 Hansjörg Malthaner
 *
 * This file is part of the Simutrans project under the artistic licence.
 * (see licence.txt)
 */

/*
 * Convoi details window
 */

#include <stdio.h>

#include "convoi_detail_t.h"
#include "components/gui_divider.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"

#include "../simconvoi.h"
#include "../vehicle/simvehicle.h"
#include "../simcolor.h"
#include "../simworld.h"
#include "../simware.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer_t.h"


karte_ptr_t convoi_detail_t:: welt;

class gui_vehicleinfo_t : public gui_aligned_container_t
{
	vehicle_t *v;
	cbuffer_t freight_info;
	gui_label_buf_t label_resale, label_friction;
	gui_textarea_t freight;

public:

	gui_vehicleinfo_t(vehicle_t *v, sint32 cnv_kmh, karte_t *welt)
	: freight(&freight_info)
	{
		this->v = v;
		set_table_layout(2,0);
		set_alignment(ALIGN_TOP | ALIGN_LEFT);

		// image
		new_component<gui_image_t>(v->get_loaded_image())->enable_offset_removal(true);
		add_table(1,0);
		{
			// name
			new_component<gui_label_t>( v->get_desc()->get_name() );
			// age
			gui_label_buf_t* l = new_component<gui_label_buf_t>();
			const sint32 month = v->get_purchase_time();
			l->buf().printf("%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12 );
			l->update();
			// value
			add_component(&label_resale);
			// power
			if(v->get_desc()->get_power()>0) {
				l = new_component<gui_label_buf_t>();
				l->buf().printf("%s %i kW, %s %.2f", translator::translate("Power:"), v->get_desc()->get_power(), translator::translate("Gear:"), v->get_desc()->get_gear()/64.0 );
				l->update();
			}
			// friction
			add_component(&label_friction);
			// max income
			sint64 max_income = - v->get_operating_cost();
			if(v->get_cargo_max() > 0) {
				max_income += (v->get_cargo_max() * ware_t::calc_revenue(v->get_cargo_type(), v->get_waytype(), cnv_kmh) )/3000;
			}
			add_table(2,1);
			{
				new_component<gui_label_t>("Max income:");
				l = new_component<gui_label_buf_t>();
				l->buf().append_money(max_income/100.0);
				l->update();
			}
			end_table();
			// maintenance
			if(  sint64 cost = welt->scale_with_month_length(v->get_desc()->get_maintenance())  ) {
				add_table(2,1);
				{
					new_component<gui_label_t>("Maintenance");
					l = new_component<gui_label_buf_t>();
					l->buf().append_money(cost/100.0);
					l->set_color(MONEY_MINUS);
					l->update();
				}
			}

			if(v->get_cargo_max() > 0) {
				// freight type
				goods_desc_t const& g    = *v->get_cargo_type();
				char const*  const  name = translator::translate(g.get_catg() == 0 ? g.get_name() : g.get_catg_name());
				gui_label_buf_t* l = new_component<gui_label_buf_t>();
				l->buf().printf("%u/%u%s %s", v->get_total_cargo(), v->get_cargo_max(), translator::translate(v->get_cargo_mass()), name);
				l->update();
				// freight
				add_component(&freight);
			}
		}
		end_table();
		update_labels();
	}

	void update_labels()
	{
		label_resale.buf().printf("%s ", translator::translate("Restwert:"));
		label_resale.buf().append_money(v->calc_sale_value() / 100.0);
		label_resale.update();
		label_friction.buf().printf( "%s %i", translator::translate("Friction:"), v->get_frictionfactor() );
		label_friction.update();
		if(v->get_cargo_max() > 0) {
			freight_info.clear();
			v->get_cargo_info(freight_info);
		}
	}


	void draw(scr_coord offset)
	{
		update_labels();
		gui_aligned_container_t::draw(offset);
	}

};





convoi_detail_t::convoi_detail_t(convoihandle_t cnv)
: scrolly(&container)
{
	if (cnv.is_bound()) {
		init(cnv);
	}
}

void convoi_detail_t::init(convoihandle_t cnv)
{
	this->cnv = cnv;

	set_table_layout(1,0);


	add_table(3,1);
	{
		add_component(&label_power);

		new_component<gui_fill_t>();

		add_table(2,1)->set_force_equal_columns(true);
		{
			sale_button.init(button_t::roundbox| button_t::flexible, "Verkauf");
			sale_button.set_tooltip("Remove vehicle from map. Use with care!");
			sale_button.add_listener(this);
			add_component(&sale_button);

			withdraw_button.init(button_t::roundbox| button_t::flexible, "withdraw");
			withdraw_button.set_tooltip("Convoi is sold when all wagons are empty.");
			withdraw_button.add_listener(this);
			add_component(&withdraw_button);
		}
		end_table();
	}
	end_table();

	add_component(&label_odometer);
	add_component(&label_length);
	add_component(&label_resale);
	add_component(&label_speed);
	add_component(&scrolly);

	const sint32 cnv_kmh = (cnv->front()->get_waytype() == air_wt) ? speed_to_kmh(cnv->get_min_top_speed()) : cnv->get_speedbonus_kmh();

	container.set_table_layout(1,0);
	for(unsigned veh=0;  veh<cnv->get_vehicle_count(); veh++ ) {
		vehicle_t *v = cnv->get_vehikel(veh);
		container.new_component<gui_vehicleinfo_t>(v, cnv_kmh, welt);
		container.new_component<gui_divider_t>();
	}
	update_labels();
}


void convoi_detail_t::update_labels()
{
	char number[128];
	number_to_string( number, (double)cnv->get_total_distance_traveled(), 0 );
	label_odometer.buf().printf(translator::translate("Odometer: %s km"), number );
	label_odometer.update();
	label_power.buf().printf( translator::translate("Leistung: %d kW"), cnv->get_sum_power() );
	label_power.update();
	label_length.buf().printf("%s %i %s %i", translator::translate("Vehicle count:"), cnv->get_vehicle_count(), translator::translate("Station tiles:"), cnv->get_tile_length());
	label_length.update();
	label_resale.buf().printf("%s ", translator::translate("Restwert:"));
	label_resale.buf().append_money( cnv->calc_restwert() / 100.0 );
	label_resale.update();
	label_speed.buf().printf(translator::translate("Bonusspeed: %i km/h"), cnv->get_speedbonus_kmh() );
	label_speed.update();
}


void convoi_detail_t::draw(scr_coord offset)
{
	if(cnv->get_owner()==welt->get_active_player()  &&  !welt->get_active_player()->is_locked()) {
		withdraw_button.enable();
		sale_button.enable();
	}
	else {
		sale_button.disable();
		withdraw_button.disable();
	}
	withdraw_button.pressed = cnv->get_withdraw();
	update_labels();

	scrolly.set_size(scrolly.get_size());

	gui_aligned_container_t::draw(offset);
}



/**
 * This method is called if an action is triggered
 * @author Markus Weber
 */
bool convoi_detail_t::action_triggered(gui_action_creator_t *komp,value_t /* */)           // 28-Dec-01    Markus Weber    Added
{
	if(cnv.is_bound()) {
		if(komp==&sale_button) {
			cnv->call_convoi_tool( 'x', NULL );
			return true;
		}
		else if(komp==&withdraw_button) {
			cnv->call_convoi_tool( 'w', NULL );
			return true;
		}
	}
	return false;
}


void convoi_detail_t::rdwr(loadsave_t *file)
{
	scrolly.rdwr(file);
}
