/*
 * This file is part of the Simutrans project under the Artistic License.
 * (see LICENSE.txt)
 */

#include <stdio.h>

#include "convoi_detail.h"
#include "components/gui_divider.h"
#include "components/gui_image.h"
#include "components/gui_textarea.h"

#include "../simconvoi.h"
#include "../vehicle/vehicle.h"
#include "../simcolor.h"
#include "../world/simworld.h"
#include "../simware.h"

#include "../dataobj/translator.h"
#include "../dataobj/loadsave.h"

#include "../player/simplay.h"

#include "../utils/simstring.h"
#include "../utils/cbuffer.h"

karte_ptr_t convoi_detail_t::welt;

class gui_vehicleinfo_t : public gui_aligned_container_t
{
	vehicle_t *v;
	cbuffer_t freight_info;
	gui_label_buf_t label_resale, label_friction, label_freight_summary;
	gui_textarea_t freight;

public:

	gui_vehicleinfo_t(vehicle_t *v, sint32 cnv_kmh)
	: freight(&freight_info)
	{
		this->v = v;
		set_table_layout(2,0);
		set_alignment(ALIGN_TOP | ALIGN_LEFT);

		const int month_now = world()->get_timeline_year_month();
		// image
		new_component<gui_image_t>(v->get_loaded_image(), v->get_owner()->get_player_nr())->enable_offset_removal(true);
		add_table(1,0);
		{
			// name
			new_component<gui_label_t>( v->get_desc()->get_name(), world()->use_timeline()  &&  v->get_desc()->is_retired(month_now) ? SYSCOL_OBSOLETE : SYSCOL_TEXT);
			// age
			gui_label_buf_t* l = new_component<gui_label_buf_t>();
			const sint32 month = v->get_purchase_time();
			l->buf().printf("%s %s %i", translator::translate("Manufactured:"), translator::get_month_name(month%12), month/12 );
			l->update();
			// value
			add_component(&label_resale);
			// max income
			sint64 max_income = - v->get_operating_cost();

			// cnv_kmh == SPEED_UNLIMITED means that meaningful revenue
			// cannot be calculated yet (e.g. vehicle in depot or stopped at station)
			if(v->get_cargo_max() > 0 && cnv_kmh != SPEED_UNLIMITED) {
				max_income += (v->get_cargo_max() * ware_t::calc_revenue(v->get_cargo_type(), v->get_waytype(), cnv_kmh))/3000;
			}
			add_table(2,1);
			{
				new_component<gui_label_t>("Max income:");
				l = new_component<gui_label_buf_t>();
				l->buf().append_money(max_income/100.0);
				l->update();
			}
			end_table();
			// power
			if(v->get_desc()->get_power()>0) {
				l = new_component<gui_label_buf_t>();
				l->buf().printf("%s %i kW, %s %.2f", translator::translate("Power:"), v->get_desc()->get_power(), translator::translate("Gear:"), v->get_desc()->get_gear()/64.0 );
				l->update();
			}
			// friction
			add_component(&label_friction);
			if(v->get_cargo_max() > 0) {
				add_component( &label_freight_summary );
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
		if(  sint64 fix_cost = world()->scale_with_month_length((sint64)v->get_desc()->get_maintenance())  ) {
			cbuffer_t temp_buf;
			temp_buf.printf( translator::translate("(%.2f$/km %.2f$/m)"), (double)v->get_desc()->get_running_cost()/100.0, (double)fix_cost/100.0 );
			label_resale.buf().append( temp_buf );
		}
		else {
			cbuffer_t temp_buf;
			temp_buf.printf( translator::translate("(%.2f$/km)"), (double)v->get_desc()->get_running_cost()/100.0 );
			label_resale.buf().append( temp_buf );
		}
		label_resale.update();
		label_friction.buf().printf( "%s %i", translator::translate("Friction:"), v->get_frictionfactor() );
		label_friction.update();
		if(v->get_cargo_max() > 0) {
			// freight type
			goods_desc_t const& g = *v->get_cargo_type();
			char const* const  name = translator::translate( g.get_catg() == 0?g.get_name():g.get_catg_name() );
			label_freight_summary.buf().printf( "%u/%u%s %s", v->get_total_cargo(), v->get_cargo_max(), translator::translate( v->get_cargo_mass() ), name );
			label_freight_summary.update();

			freight_info.clear();
			v->get_cargo_info(freight_info);
		}
	}


	void draw(scr_coord offset) OVERRIDE
	{
		update_labels();
		gui_aligned_container_t::draw(offset);
	}

};





convoi_detail_t::convoi_detail_t(convoihandle_t cnv)
: scrolly(&container_veh)
{
	if (cnv.is_bound()) {
		init(cnv);
	}
}

void convoi_detail_t::init(convoihandle_t cnv)
{
	this->cnv = cnv;

	set_table_layout(1,0);

	add_component( &container_txt );
	container_txt.set_margin( scr_size(D_MARGIN_LEFT,0), scr_size(D_MARGIN_RIGHT,0) );
	container_txt.set_table_layout(1,0);

	container_txt.add_component(&label_power);
	container_txt.add_component(&label_odometer);
	container_txt.add_component(&label_length);
	container_txt.add_component(&label_resale);
	container_txt.add_component(&label_speed);
	add_component(&scrolly);

	const sint32 cnv_kmh = (cnv->front()->get_waytype() == air_wt) ? speed_to_kmh(cnv->get_min_top_speed()) : cnv->get_speedbonus_kmh();

	container_veh.set_table_layout(1,0);
	container_veh.new_component<gui_fill_t>();
	for(unsigned veh=0;  veh<cnv->get_vehicle_count(); veh++ ) {
		if(veh>0) {
			container_veh.new_component<gui_divider_t>();
		}
		vehicle_t *v = cnv->get_vehicle(veh);
		container_veh.new_component<gui_vehicleinfo_t>(v, cnv_kmh);
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
	if( cnv->get_vehicle_count()>0  &&  cnv->get_vehicle( 0 )->get_desc()->get_waytype()==water_wt ) {
		label_length.buf().printf( "%s %i", translator::translate( "Vehicle count:" ), cnv->get_vehicle_count() );
	}
	else {
		label_length.buf().printf( "%s %i %s %i", translator::translate( "Vehicle count:" ), cnv->get_vehicle_count(), translator::translate( "Station tiles:" ), cnv->get_tile_length() );
	}
	label_length.update();
	label_resale.buf().printf("%s ", translator::translate("Restwert:"));
	label_resale.buf().append_money( cnv->calc_restwert() / 100.0 );
	label_resale.update();
	label_speed.buf().printf(translator::translate("Bonusspeed: %i km/h"), cnv->get_speedbonus_kmh() );
	label_speed.update();
}


void convoi_detail_t::draw(scr_coord offset)
{
	update_labels();

	scrolly.set_size(scrolly.get_size());

	gui_aligned_container_t::draw(offset);
}



void convoi_detail_t::rdwr(loadsave_t *file)
{
	scrolly.rdwr(file);
}
