/*
 * This file is part of the Simutrans-Extended project under the Artistic License.
 * (see LICENSE.txt)
 */

/*
 * Roads for Simutrans
 *
 * Revised January 2001
 * Hj. Malthaner
 */

#include <stdio.h>

#include "strasse.h"
#include "../../simworld.h"
#include "../../obj/bruecke.h"
#include "../../obj/tunnel.h"
#include "../../dataobj/loadsave.h"
#include "../../descriptor/way_desc.h"
#include "../../descriptor/tunnel_desc.h"
#include "../../bauer/wegbauer.h"
#include "../../dataobj/translator.h"
#include "../../dataobj/ribi.h"
#include "../../utils/cbuffer_t.h"
#include "../../vehicle/simvehicle.h" /* for calc_direction */
#include "../../obj/wayobj.h"
#include "../../player/simplay.h"

const way_desc_t *strasse_t::default_strasse=NULL;

bool strasse_t::show_masked_ribi = false;


void strasse_t::set_gehweg(bool janein)
{
	grund_t *gr = welt->lookup(get_pos());
	wayobj_t *wo = gr ? gr->get_wayobj(road_wt) : NULL;

	if (wo && wo->get_desc()->is_noise_barrier()) {
		janein = false;
	}

	weg_t::set_gehweg(janein);
	if(janein && get_desc())
	{
		if(welt->get_settings().get_town_road_speed_limit())
		{
			set_max_speed(min(welt->get_settings().get_town_road_speed_limit(), get_desc()->get_topspeed()));
		}
		else
		{
			set_max_speed(get_desc()->get_topspeed());
		}
	}
	if(!janein && get_desc()) {
		set_max_speed(get_desc()->get_topspeed());
	}
	if(gr) {
		gr->calc_image();
	}
}



strasse_t::strasse_t(loadsave_t *file) : weg_t(road_wt)
{
	rdwr(file);
}


strasse_t::strasse_t() : weg_t(road_wt)
{
	set_gehweg(false);
	set_desc(default_strasse);
	ribi_mask_oneway =ribi_t::none;
	overtaking_mode = twoway_mode;
}


void strasse_t::rdwr(loadsave_t *file)
{
	xml_tag_t s( file, "strasse_t" );

	weg_t::rdwr(file);

	if(  file->get_extended_version() >= 14  ) {
		uint8 mask_oneway = get_ribi_mask_oneway();
		file->rdwr_byte(mask_oneway);
		set_ribi_mask_oneway(mask_oneway);
		sint8 ov = get_overtaking_mode();
		if (file->is_loading() && file->get_extended_version() == 14 && (ov == 2 || ov == 4)) {
			ov = twoway_mode; // loading_only_mode and inverted_mode has been removed
		}
		file->rdwr_byte(ov);
		overtaking_mode = (overtaking_mode_t)ov;
	} else {
		set_ribi_mask_oneway(ribi_t::none);
		overtaking_mode = twoway_mode;
	}

	if(file->get_version()<89000) {
		bool gehweg;
		file->rdwr_bool(gehweg);
		set_gehweg(gehweg);
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
		const way_desc_t *desc = way_builder_t::get_desc(bname);

#ifndef SPECIAL_RESCUE_12_3
		const way_desc_t* loaded_replacement_way = NULL;
		if(file->get_extended_version() >= 12)
		{
			char rbname[128];
			file->rdwr_str(rbname, lengthof(rbname));
			loaded_replacement_way = way_builder_t::get_desc(rbname);
		}
#endif

		const sint32 old_max_speed = get_max_speed();
		const uint32 old_max_axle_load = get_max_axle_load();
		const uint32 old_bridge_weight_limit = get_bridge_weight_limit();
		if(desc==NULL) {
			desc = way_builder_t::get_desc(translator::compatibility_name(bname));
			if(desc==NULL) {
				desc = default_strasse;
				welt->add_missing_paks( bname, karte_t::MISSING_WAY );
			}
			dbg->warning("strasse_t::rdwr()", "Unknown street %s replaced by %s (old_max_speed %i)", bname, desc->get_name(), old_max_speed );
		}

		set_desc(desc, file->get_extended_version() >= 12);
#ifndef SPECIAL_RESCUE_12_3
		if(file->get_extended_version() >= 12)
		{
			replacement_way = loaded_replacement_way;
		}
#endif

		if(old_max_speed > 0)
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
		if(old_max_axle_load > 0)
		{
			set_max_axle_load(old_max_axle_load);
		}
		if(old_bridge_weight_limit > 0)
		{
			set_bridge_weight_limit(old_bridge_weight_limit);
		}
		const grund_t* gr = welt->lookup(get_pos());
		const bruecke_t *bridge = gr ? gr->find<bruecke_t>() : NULL;
		const tunnel_t *tunnel = gr ? gr->find<tunnel_t>() : NULL;
		const slope_t::type hang = gr ? gr->get_weg_hang() : slope_t::flat;

		if(hang != slope_t::flat)
		{
			const uint slope_height = (hang & 7) ? 1 : 2;
			if(slope_height == 1)
			{
				uint32 gradient_speed = desc->get_topspeed_gradient_1();
				if (is_degraded())
				{
					if (get_remaining_wear_capacity() > 0)
					{
						gradient_speed /= 2;
					}
					else
					{
						gradient_speed = 0;
					}
				}
				if(bridge)
				{
					set_max_speed(min(gradient_speed, bridge->get_desc()->get_topspeed_gradient_1()));
				}
				else if(tunnel)
				{
					set_max_speed(min(gradient_speed, tunnel->get_desc()->get_topspeed_gradient_1()));
				}
				else
				{
					set_max_speed(gradient_speed);
				}
			}
			else
			{
				uint32 gradient_speed = desc->get_topspeed_gradient_2();
				if (is_degraded())
				{
					if (get_remaining_wear_capacity() > 0)
					{
						gradient_speed /= 2;
					}
					else
					{
						gradient_speed = 0;
					}
				}
				if(bridge)
				{
					set_max_speed( min(gradient_speed, bridge->get_desc()->get_topspeed_gradient_2()));
				}
				else if(tunnel)
				{
					set_max_speed(min(gradient_speed, tunnel->get_desc()->get_topspeed_gradient_2()));
				}
				else
				{
					set_max_speed(desc->get_topspeed_gradient_2());
				}
			}
		}
		else
		{
			if(bridge)
				{
					set_max_speed(min(desc->get_topspeed(), bridge->get_desc()->get_topspeed()));
				}
			else if(tunnel)
				{
					set_max_speed(min(desc->get_topspeed(), tunnel->get_desc()->get_topspeed()));
				}
			else if (old_max_speed == 0)
			{
				if (is_degraded())
				{
					// The maximum speed has to be reduced on account of the degridation.
					if (get_remaining_wear_capacity() > 0)
					{
						set_max_speed(desc->get_topspeed() / 2);
					}
					else
					{
						set_max_speed(0);
					}
				}
				else
				{
					set_max_speed(desc->get_topspeed());
				}
			}
		}

		if(hat_gehweg() && desc->get_wtyp() == road_wt)
		{
			set_max_speed(min(get_max_speed(), 50));
		}
	}
}

void strasse_t::set_overtaking_mode(overtaking_mode_t o, player_t* calling_player)
{
	if (o == invalid_mode) { return; }
	grund_t* gr = welt->lookup(get_pos());
	if ((!calling_player || !calling_player->is_public_service()) && is_public_right_of_way() && gr && gr->removing_way_would_disrupt_public_right_of_way(road_wt))
	{
		return;
	}
	if (is_deletable(calling_player) == NULL)
	{
		overtaking_mode = o;
	}
}

void strasse_t::update_ribi_mask_oneway(ribi_t::ribi mask, ribi_t::ribi allow, player_t* calling_player)
{
	if (is_deletable(calling_player) != NULL) {
		return;
	}

	// assertion. @mask and @allow must be single or none.
	if(!(ribi_t::is_single(mask)||(mask==ribi_t::none))) dbg->error( "weg_t::update_ribi_mask_oneway()", "mask is not single or none.");
	if(!(ribi_t::is_single(allow)||(allow==ribi_t::none))) dbg->error( "weg_t::update_ribi_mask_oneway()", "allow is not single or none.");

	if(  mask==ribi_t::none  ) {
		if(  ribi_t::is_twoway(get_ribi_unmasked())  ) {
			// auto complete
			ribi_mask_oneway |= (get_ribi_unmasked()-allow);
		}
	} else {
		ribi_mask_oneway |= mask;
	}
	// remove backward ribi
	if(  allow==ribi_t::none  ) {
		if(  ribi_t::is_twoway(get_ribi_unmasked())  ) {
			// auto complete
			ribi_mask_oneway &= ~(get_ribi_unmasked()-mask);
		}
	} else {
		ribi_mask_oneway &= ~allow;
	}
}

ribi_t::ribi strasse_t::get_ribi() const {
	ribi_t::ribi ribi = get_ribi_unmasked();
	ribi_t::ribi ribi_maske = get_ribi_maske();
	if(  get_waytype()==road_wt  &&  overtaking_mode<=oneway_mode  ) {
		return (ribi_t::ribi)((ribi & ~ribi_maske) & ~ribi_mask_oneway);
	} else {
		return (ribi_t::ribi)(ribi & ~ribi_maske);
	}
}

void strasse_t::rotate90() {
	weg_t::rotate90();
	ribi_mask_oneway = ribi_t::rotate90( ribi_mask_oneway );
}
