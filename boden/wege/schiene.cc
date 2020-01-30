/*
 * Rails for Simutrans
 *
 * Revised January 2001
 * Hj. Malthaner
 */

#include <stdio.h>

#include "../../gui/simwin.h"
#include "../../simconvoi.h"
#include "../../simworld.h"
#include "../../utils/simstring.h"
#include "../../vehicle/simvehicle.h"

#include "../../obj/signal.h"

#include "../../dataobj/loadsave.h"
#include "../../dataobj/translator.h"

#include "../../utils/cbuffer_t.h"

#include "../../descriptor/way_desc.h"
#include "../../bauer/wegbauer.h"

#include "../../gui/schiene_info.h"

#include "schiene.h"


const way_desc_t *schiene_t::default_schiene=NULL;
bool schiene_t::show_reservations = false;


schiene_t::schiene_t(waytype_t waytype) : weg_t (waytype)
{
	reserved = convoihandle_t();
}


schiene_t::schiene_t() : weg_t(track_wt)
{
	reserved = convoihandle_t();
	type = block;
	set_desc(schiene_t::default_schiene);
}


schiene_t::schiene_t(loadsave_t *file) : weg_t(track_wt)
{
	reserved = convoihandle_t();
	type = block;
	rdwr(file);
}


void schiene_t::cleanup(player_t *)
{
	// removes reservation
	if(reserved.is_bound()) {
		set_ribi(ribi_t::none);
		reserved->suche_neue_route();
	}
}

void schiene_t::rotate90()
{
	direction = ribi_t::rotate90(direction); 
	weg_t::rotate90();
}

void schiene_t::show_info()
{
	create_win(new schiene_info_t(this), w_info, (ptrdiff_t)this);
}


void schiene_t::info(cbuffer_t & buf, bool is_bridge) const
{
	weg_t::info(buf, is_bridge);

	schiene_t* sch = (schiene_t*)this;

	uint8 textlines = 1; // to locate the clickable button
	if (reserved.is_bound())
	{
		const char* reserve_text = translator::translate("\nis reserved by:");
		// ignore linebreak
		if (reserve_text[0] == '\n') {
			reserve_text++;
		}

		buf.append(reserve_text);
		buf.append("\n   ");
		buf.append(translator::translate(reserved->get_name()));
		buf.append("\n   ");


		//		if (get_desc()->get_styp() == monorail_wt || maglev_wt || tram_wt || narrowgauge_wt)

		rail_vehicle_t* rail_vehicle = NULL;
		switch (reserved->front()->get_waytype())
		{
		case track_wt:
		case narrowgauge_wt:
		case tram_wt:
		case monorail_wt:
		case maglev_wt:
			rail_vehicle = (rail_vehicle_t*)reserved->front();
		}

		if (rail_vehicle)
		{

			buf.append(translator::translate(get_working_method_name(rail_vehicle->get_working_method())));
			textlines += 1;
			buf.append("\n   ");

			// We do not need to specify if the reservation is a "block" type. Only show the two other more interresting reservation types
			if (get_reservation_type() != block) {
				buf.append(translator::translate(get_reservation_type_name(get_reservation_type())));
				if (get_reservation_type() == directional)
				{
					buf.append(", ");
					buf.append(translator::translate("reservation_heading"));
					buf.append(": ");
					buf.append(translator::translate(get_directions_name(get_reserved_direction())));
				}
				textlines += 1;
				buf.append("\n   ");
			}
		}
		buf.append(translator::translate("distance_to_vehicle"));
		buf.append(": ");
		textlines += 1;

		koord3d vehpos = reserved->get_pos();
		koord3d schpos = sch->get_pos();
		const uint32 tiles_to_vehicle = shortest_distance(schpos.get_2d(), vehpos.get_2d());
		const double km_per_tile = welt->get_settings().get_meters_per_tile() / 1000.0;
		const double km_to_vehicle = (double)tiles_to_vehicle * km_per_tile;

		if (km_to_vehicle < 1)
		{
			float m_to_vehicle = km_to_vehicle * 1000;
			buf.append(m_to_vehicle);
			buf.append("m");
		}
		else
		{
			uint n_actual;
			if (km_to_vehicle < 20)
			{
				n_actual = 1;
			}
			else
			{
				n_actual = 0;
			}
			char number_actual[10];
			number_to_string(number_actual, km_to_vehicle, n_actual);
			buf.append(number_actual);
			buf.append("km");
		}
		buf.append(", ");
		buf.append(speed_to_kmh(reserved->get_akt_speed()));
		buf.append(translator::translate("km/h"));

		vehicle_t* vehicle = NULL;
		vehicle = (vehicle_t*)reserved->front();

		buf.append(" (");
		buf.append(translator::translate(get_directions_name(vehicle->get_direction())));
		buf.append(")");



#ifdef DEBUG_PBS
		reserved->show_info();
#endif

	}
	else
	{
		if (get_desc()->get_wtyp() == air_wt)
		{
			if (get_desc()->get_styp() == type_runway)
			{
				buf.append(translator::translate("runway_not_reserved"));
				buf.append("\n\n");
				textlines += 1;
			}
		}
		else
		{
			buf.append(translator::translate("track_not_reserved"));
			buf.append("\n\n");
			textlines += 1;
		}
	}
	sch->textlines_in_info_window = textlines;
}


/**
 * true, if this rail can be reserved
 * @author prissi
 */
bool schiene_t::reserve(convoihandle_t c, ribi_t::ribi dir, reservation_type t, bool check_directions_at_junctions)
{
	if (can_reserve(c, dir, t, check_directions_at_junctions)) 
	{
		ribi_t::ribi old_direction = direction;
		if ((type == block || type == stale_block) && t == directional && reserved.is_bound())
		{
			// Do not actually reserve here, as the directional reservation 
			// is already done, but show that this is reservable. 
			return true;
		}
		reserved = c;
		type = t;
		direction = dir;

		/* for threeway and fourway switches we may need to alter graphic, if
			* direction is a diagonal (i.e. on the switching part)
			* and there are switching graphics
			*/
		if (t == block && ribi_t::is_threeway(get_ribi_unmasked()) && ribi_t::is_bend(dir) && get_desc()->has_switch_image()) {
			mark_image_dirty(get_image(), 0);
			mark_image_dirty(get_front_image(), 0);
			set_images(image_switch, get_ribi_unmasked(), is_snow(), (dir == ribi_t::northeast || dir == ribi_t::southwest));
			set_flag(obj_t::dirty);
		}
		if (schiene_t::show_reservations) {
			set_flag(obj_t::dirty);
		}
		if (old_direction != dir)
		{
			if (signal_t* sig = welt->lookup(get_pos())->find<signal_t>())
			{
				if (sig->is_bidirectional() && sig == get_signal(dir))
				{
					// A suitable state for facing in the opposite direction
					// will not be a suitable state for facing in this new
					// direction.
					sig->set_state(roadsign_t::danger);
				}
				sig->calc_image();
			}
		}
		return true;
	}
	return false;
}


/**
* releases previous reservation
* only true, if there was something to release
* @author prissi
*/
bool schiene_t::unreserve(convoihandle_t c)
{
	// is this tile reserved by us?
	if(reserved.is_bound()  &&  reserved==c) {
		reserved = convoihandle_t();
		if(schiene_t::show_reservations) {
			set_flag( obj_t::dirty );
		}
		return true;
	}
	return false;
}




/**
* releases previous reservation
* @author prissi
*/
bool schiene_t::unreserve(vehicle_t*)
{
	// is this tile empty?
	if(!reserved.is_bound()) {
		return true;
	}
//	if(!welt->lookup(get_pos())->suche_obj(v->get_typ())) {
		reserved = convoihandle_t();
		if(schiene_t::show_reservations) {
			set_flag( obj_t::dirty );
		}
		return true;
//	}
//	return false;
}


void schiene_t::rdwr(loadsave_t *file)
{
	xml_tag_t t( file, "schiene_t" );

	weg_t::rdwr(file);

	if(file->get_version()<99008) {
		sint32 blocknr=-1;
		file->rdwr_long(blocknr);
	}

	if(file->get_version()<89000) {
		uint8 dummy;
		file->rdwr_byte(dummy);
		set_electrify(dummy);
	}

	if(file->is_saving()) 
	{
		const char *s = get_desc()->get_name();
		file->rdwr_str(s);
		if(file->get_extended_version() >= 12)
		{
			s = replacement_way ? replacement_way->get_name() : ""; 
			file->rdwr_str(s);
		}
	}
	else 
	{
		char bname[128];
		file->rdwr_str(bname, lengthof(bname));

#ifndef SPECIAL_RESCUE_12_3
		const way_desc_t* loaded_replacement_way = NULL;
		if(file->get_extended_version() >= 12)
		{
			char rbname[128];
			file->rdwr_str(rbname, lengthof(rbname));
			loaded_replacement_way = way_builder_t::get_desc(rbname);
		}
#endif

		sint32 old_max_speed=get_max_speed();
		uint32 old_max_axle_load = get_max_axle_load();
		uint32 old_bridge_weight_limit = get_bridge_weight_limit();
		const way_desc_t *desc = way_builder_t::get_desc(bname);
		if(desc==NULL) {
			sint32 old_max_speed=get_max_speed();
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_schiene;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("schiene_t::rdwr()", "Unknown rail %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}

		set_desc(desc, file->get_extended_version() >= 12);
#ifndef SPECIAL_RESCUE_12_3
		if(file->get_extended_version() >= 12)
		{
			replacement_way = loaded_replacement_way;
		}
#endif
		if (old_max_speed > 0)
		{
			if (is_degraded() && old_max_speed == desc->get_topspeed())
			{
				// The maximum speed has to be reduced on account of the degridation.
				if (get_remaining_wear_capacity() > 0)
				{
					set_max_speed(old_max_speed / 2);
				}
				else
				{
					set_max_speed(0);
				}
			}
			else
			{
				set_max_speed(old_max_speed);
			}
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,get_pos().x,get_pos().y,old_max_speed);
		if(old_max_axle_load > 0)
		{
			set_max_axle_load(old_max_axle_load);
		}
		if(old_bridge_weight_limit > 0)
		{
			set_bridge_weight_limit(old_bridge_weight_limit);
		}
		//DBG_MESSAGE("schiene_t::rdwr","track %s at (%i,%i) max_speed %i",bname,get_pos().x,get_pos().y,old_max_speed);
	}
#ifdef SPECIAL_RESCUE_12_6
	if(file->is_saving() && file->get_extended_version() >= 12)
#else
	if(file->get_extended_version() >= 12)
#endif
	{
		uint16 reserved_index = reserved.get_id();
		if (file->is_saving())
		{
			// Do not save corrupt reservations. We cannot check this on loading, as 
			// there the convoys have not been loaded yet.
			if (reserved.is_bound() && !is_type_rail_type(reserved->front()->get_waytype()))
			{
				// This is an invalid reservation - clear it.
				reserved_index = 0;
			}
		}
		file->rdwr_short(reserved_index); 
		reserved.set_id(reserved_index);

		uint8 t = (uint8)type;
		file->rdwr_byte(t);
		type = (reservation_type)t;

		uint8 d = (uint8)direction;
		file->rdwr_byte(d);
		direction = (ribi_t::ribi)d; 
	}
}
